/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009       Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009	    Oliver Ney <oliver@dryder.de>
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

// Forward declaration
static void gfire_ipc_server_input_cb(gpointer p_data, gint p_fd, PurpleInputCondition p_condition);

static gfire_ipc_server *gfire_ipc_server_create()
{
	gfire_ipc_server *ret = (gfire_ipc_server*)g_malloc0(sizeof(gfire_ipc_server));
	if(!ret)
		return NULL;

	ret->buff_in = (gchar*)g_malloc0(GFIRE_IPC_BUFFER_LEN);
	if(!ret->buff_in)
	{
		g_free(ret);
		return NULL;
	}

	ret->buff_out = (gchar*)g_malloc0(GFIRE_IPC_BUFFER_LEN);
	if(!ret->buff_out)
	{
		g_free(ret->buff_in);
		g_free(ret);
		return NULL;
	}

	ret->socket = -1;

	return ret;
}

static void gfire_ipc_server_free(gfire_ipc_server *p_ipc)
{
	if(!p_ipc)
		return;

	if(p_ipc->prpl_inpa)
		purple_input_remove(p_ipc->prpl_inpa);

	if(p_ipc->socket >= 0)
		close(p_ipc->socket);

	if(p_ipc->instances)
		g_list_free(p_ipc->instances);

	g_free(p_ipc->buff_in);
	g_free(p_ipc->buff_out);

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

	purple_debug_info("gfire", "IPC: Server started\n");

	return TRUE;
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

static void gfire_ipc_server_input_cb(gpointer p_data, gint p_fd, PurpleInputCondition p_condition)
{
	if(!gfire_ipc || p_fd < 0 || p_condition != PURPLE_INPUT_READ)
		return;

	// Receive data
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int length = recvfrom(gfire_ipc->socket, gfire_ipc->buff_in, GFIRE_IPC_BUFFER_LEN, 0, (struct sockaddr*)&addr, &addrlen);

	// Parse IPC header /////////

	// Invalid packet
	if(length < 4)
		return;

	unsigned short ipc_len;
	memcpy(&ipc_len, gfire_ipc->buff_in, 2);

	// Received incomplete/invalid packet
	if(ipc_len != length)
		return;

	unsigned short ipc_id;
	memcpy(&ipc_id, gfire_ipc->buff_in + 2, 2);

	GList *current = gfire_ipc->instances;
	for(; current; current = g_list_next(current))
	{
		switch(ipc_id)
		{
			case GFIRE_IPC_ID_SDK:
				purple_debug_misc("gfire", "IPC: Received Xfire SDK data\n");
				gfire_ipc_proto_sdk((gfire_data*)current->data, gfire_ipc->buff_in + 4, ipc_len - 4);
				break;
			default:
				purple_debug_warning("gfire", "IPC: Received packet with ID %u and length %u (unknown)\n", ipc_id, ipc_len);
				break;
		}
	}
}
