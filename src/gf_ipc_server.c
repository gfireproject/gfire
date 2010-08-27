/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2005-2006, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009-2010  Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009-2010  Oliver Ney <oliver@dryder.de>
 *
 * This file is part of Gfire.
 *
 * Gfire is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gfire.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gf_ipc_server.h"
#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <netinet/in.h>
#endif // _WIN32

#include "gf_ipc_proto.h"

// IPC connection is shared between all Gfire instances
static gfire_ipc_server *gfire_ipc = NULL;

static gfire_ipc_client *gfire_ipc_client_create(const struct sockaddr_in *p_addr, guint32 p_pid)
{
	gfire_ipc_client *ret = (gfire_ipc_client*)g_malloc0(sizeof(gfire_ipc_client));
	if(!ret)
		return NULL;

	memcpy(&ret->addr, p_addr, sizeof(struct sockaddr_in));
	ret->pid = p_pid;

	// Initialize the keep alive timeout
	g_get_current_time(&ret->last_keep_alive);
	return ret;
}

static void gfire_ipc_client_free(gfire_ipc_client *p_client)
{
	if(!p_client)
		return;

	g_free(p_client);
}

// Forward declaration
static gboolean gfire_ipc_server_keep_alive_timeout_cb(gpointer p_data);
static void gfire_ipc_server_input_cb(gpointer p_data, gint p_fd, PurpleInputCondition p_condition);

static gfire_ipc_server *gfire_ipc_server_create()
{
	gfire_ipc_server *ret = (gfire_ipc_server*)g_malloc0(sizeof(gfire_ipc_server));
	if(!ret)
		return NULL;

	ret->buffer = (guint8*)g_malloc0(GFIRE_IPC_BUFFER_LEN);
	if(!ret->buffer)
	{
		g_free(ret);
		return NULL;
	}

	ret->socket = -1;

	return ret;
}

static void gfire_ipc_server_shutdown();

static void gfire_ipc_server_free(gfire_ipc_server *p_ipc)
{
	if(!p_ipc)
		return;

	gfire_ipc_server_shutdown();

	while(p_ipc->clients)
	{
		gfire_ipc_client_free((gfire_ipc_client*)p_ipc->clients->data);
		p_ipc->clients = g_list_delete_link(p_ipc->clients, p_ipc->clients);
	}

	if(p_ipc->prpl_inpa)
		purple_input_remove(p_ipc->prpl_inpa);

	if(p_ipc->socket >= 0)
		close(p_ipc->socket);

	if(p_ipc->instances)
		g_list_free(p_ipc->instances);

	g_free(p_ipc->buffer);

	g_free(p_ipc);

	purple_debug_info("gfire", "IPC: Server freed\n");
}

static gboolean gfire_ipc_server_init()
{
	if(!gfire_ipc)
		if(!(gfire_ipc = gfire_ipc_server_create()))
			return FALSE;

	gfire_ipc->socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(gfire_ipc->socket < 0)
	{
		purple_debug_error("gfire", "IPC: gfire_ipc_server_init(): Couldn't create socket (%s)\n", strerror(errno));
		return FALSE;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(GFIRE_IPC_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if(bind(gfire_ipc->socket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != 0)
	{
		purple_debug_error("gfire", "IPC: gfire_ipc_server_init(): Couldn't bind socket (%s)\n", strerror(errno));
		close(gfire_ipc->socket);
		return FALSE;
	}

	gfire_ipc->prpl_inpa = purple_input_add(gfire_ipc->socket, PURPLE_INPUT_READ, gfire_ipc_server_input_cb, NULL);

	gfire_ipc->keep_alive_to = g_timeout_add_seconds(GFIRE_IPC_KEEP_ALIVE_INTERVAL,
													 (GSourceFunc)gfire_ipc_server_keep_alive_timeout_cb, NULL);

	purple_debug_info("gfire", "IPC: Server started\n");

	return TRUE;
}

static void gfire_ipc_server_send_direct(const struct sockaddr_in *p_addr, guint16 p_len)
{
	if(!gfire_ipc || !p_addr)
		return;

	if(sendto(gfire_ipc->socket, gfire_ipc->buffer, p_len, 0, (struct sockaddr*)p_addr,
			  sizeof(struct sockaddr_in)) != p_len)
		purple_debug_warning("gfire", "IPC: sent too less bytes!\n");
}

static void gfire_ipc_server_send(gfire_ipc_client *p_client, guint16 p_len)
{
	if(!gfire_ipc || !p_client)
		return;

	if(sendto(gfire_ipc->socket, gfire_ipc->buffer, p_len, 0, (struct sockaddr*)&p_client->addr,
			  sizeof(struct sockaddr_in)) != p_len)
		purple_debug_warning("gfire", "IPC: sent too less bytes!\n");
}

static void gfire_ipc_server_shutdown()
{
	if(!gfire_ipc)
		return;

	GList *client = gfire_ipc->clients;
	while(client)
	{
		guint16 len = gfire_ipc_proto_write_shutdown(((gfire_ipc_client*)client->data)->pid, gfire_ipc->buffer);
		if(len) gfire_ipc_server_send((gfire_ipc_client*)client->data, len);
		client = g_list_next(client);
	}

	if(gfire_ipc->keep_alive_to)
		g_source_remove(gfire_ipc->keep_alive_to);
}

void gfire_ipc_server_client_handshake(guint16 p_version, guint32 p_pid, const struct sockaddr_in *p_addr)
{
	if(!gfire_ipc)
		return;

	if(p_version < GFIRE_IPC_VERSION)
	{
		// Version is incompatible, tell that the client
		purple_debug_info("gfire", "IPC: new client from PID %u with incompatible version %hu\n", p_pid, p_version);
		guint16 len = gfire_ipc_proto_write_server_handshake(p_pid, FALSE, gfire_ipc->buffer);
		gfire_ipc_server_send_direct(p_addr, len);
		return;
	}

	gfire_ipc_client *client = gfire_ipc_client_create(p_addr, p_pid);
	gfire_ipc->clients = g_list_append(gfire_ipc->clients, client);

	guint16 len = gfire_ipc_proto_write_server_handshake(p_pid, TRUE, gfire_ipc->buffer);
	if(len) gfire_ipc_server_send(client, len);

	purple_debug_info("gfire", "IPC: new client from PID %u with version %hu (%u connected)\n", p_pid, p_version,
					  g_list_length(gfire_ipc->clients));
}

static void gfire_ipc_server_client_shutdown(gfire_ipc_client *p_client)
{
	if(!gfire_ipc || !p_client)
		return;

	GList *node = g_list_find(gfire_ipc->clients, p_client);
	if(node) gfire_ipc->clients = g_list_delete_link(gfire_ipc->clients, node);
	gfire_ipc_client_free(p_client);
	purple_debug_info("gfire", "IPC: Client removed (%u left)\n", g_list_length(gfire_ipc->clients));
}

static void gfire_ipc_server_client_keep_alive(gfire_ipc_client *p_client)
{
	if(!gfire_ipc || !p_client)
		return;

	g_get_current_time(&p_client->last_keep_alive);
}

void gfire_ipc_server_register(gfire_data *p_gfire)
{
	if(!p_gfire)
		return;

	if(!gfire_ipc)
		if(!gfire_ipc_server_init())
			return;

	gfire_ipc->instances = g_list_append(gfire_ipc->instances, p_gfire);

	purple_debug_info("gfire", "IPC: Gfire instance added (new count: %u)\n", g_list_length(gfire_ipc->instances));
}

void gfire_ipc_server_unregister(gfire_data *p_gfire)
{
	if(!gfire_ipc || !p_gfire)
		return;

	gfire_ipc->instances = g_list_remove(gfire_ipc->instances, p_gfire);

	purple_debug_info("gfire", "IPC: Gfire instance removed (new count: %u)\n", g_list_length(gfire_ipc->instances));

	if(!gfire_ipc->instances)
	{
		gfire_ipc_server_free(gfire_ipc);
		gfire_ipc = NULL;
	}
}

static gboolean gfire_ipc_server_keep_alive_timeout_cb(gpointer p_data)
{
	if(!gfire_ipc)
		return FALSE;

	GTimeVal current_time;
	g_get_current_time(&current_time);

	GList *clist = gfire_ipc->clients;
	while(clist)
	{
		gfire_ipc_client *client = (gfire_ipc_client*)clist->data;

		// Check if the client timed out
		if((current_time.tv_sec - client->last_keep_alive.tv_sec) >= GFIRE_IPC_TIMEOUT)
		{
			// Send the client a shutdown message anyway
			guint16 len = gfire_ipc_proto_write_shutdown(client->pid, gfire_ipc->buffer);
			if(len) gfire_ipc_server_send(client, len);

			// Delete the client
			gfire_ipc_client_free(client);

			// Get the next list element
			GList *to_delete = clist;
			clist = g_list_next(clist);

			// Delete the element
			gfire_ipc->clients = g_list_delete_link(gfire_ipc->clients, to_delete);
			purple_debug_info("gfire", "IPC: Client timed out (%u left)\n", g_list_length(gfire_ipc->clients));
			continue;
		}

		// Send a keep alive packet
		guint16 len = gfire_ipc_proto_write_keep_alive(client->pid, gfire_ipc->buffer);
		if(len) gfire_ipc_server_send(client, len);

		clist = g_list_next(clist);
	}

	return TRUE;
}

static void gfire_ipc_server_input_cb(gpointer p_data, gint p_fd, PurpleInputCondition p_condition)
{
	if(!gfire_ipc || p_fd < 0 || p_condition != PURPLE_INPUT_READ)
		return;

	// Receive data
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int length = recvfrom(gfire_ipc->socket, (gchar*)gfire_ipc->buffer, GFIRE_IPC_BUFFER_LEN, 0,
						  (struct sockaddr*)&addr, &addrlen);

	// Parse IPC header /////////

	// Invalid packet
	if(length < GFIRE_IPC_HEADER_LEN)
		return;

	guint16 ipc_len;
	memcpy(&ipc_len, gfire_ipc->buffer, 2);

	// Received incomplete/invalid packet
	if(ipc_len != length)
		return;

	guint32 pid;
	memcpy(&pid, gfire_ipc->buffer + 2, 4);

	// Get the client
	gfire_ipc_client *client = NULL;
	GList *clist = gfire_ipc->clients;
	while(clist)
	{
		if(((gfire_ipc_client*)clist->data)->pid == pid)
		{
			client = (gfire_ipc_client*)clist->data;
			break;
		}

		clist = g_list_next(clist);
	}

	guint16 ipc_id;
	memcpy(&ipc_id, gfire_ipc->buffer + 6, 2);

	switch(ipc_id)
	{
	case GFIRE_IPC_CLIENT_HS:
		purple_debug_misc("gfire", "IPC: Received client handshake\n");
		gfire_ipc_proto_client_handshake(gfire_ipc, ipc_len, &addr);
		break;
	case GFIRE_IPC_SHUTDOWN:
		purple_debug_misc("gfire", "IPC: Received shutdown message\n");
		gfire_ipc_server_client_shutdown(client);
		break;
	case GFIRE_IPC_KEEP_ALIVE:
		purple_debug_misc("gfire", "IPC: Received keep-alive\n");
		gfire_ipc_server_client_keep_alive(client);
		break;
	case GFIRE_IPC_ID_SDK:
		purple_debug_misc("gfire", "IPC: Received Xfire SDK data\n");
		gfire_ipc_proto_sdk(gfire_ipc, ipc_len);
		break;
	default:
		purple_debug_warning("gfire", "IPC: Received packet with ID %u and length %u (unknown)\n", ipc_id, ipc_len);
		break;
	}
}
