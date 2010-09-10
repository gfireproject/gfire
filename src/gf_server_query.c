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

#include "gf_server_query.h"

#ifdef HAVE_GTK

// Available server query drivers
extern gfire_server_query_driver gf_sq_quake_driver;
extern gfire_server_query_driver gf_sq_source_driver;

static const struct { const gchar *proto; const gfire_server_query_driver *driver; } registeredDrivers[] =
{
	{ "WOLFET", &gf_sq_quake_driver },
	{ "SOURCE", &gf_sq_source_driver },
	{ NULL, NULL }
};

gfire_server_query *gfire_server_query_create()
{
	gfire_server_query *ret = g_malloc0(sizeof(gfire_server_query));
	ret->servers = g_queue_new();
	ret->socket = -1;
	return ret;
}

void gfire_server_query_free(gfire_server_query *p_query)
{
	if(!p_query)
		return;

	if(p_query->prpl_data)
		purple_network_listen_cancel(p_query->prpl_data);

	if(p_query->prpl_inpa)
		purple_input_remove(p_query->prpl_inpa);

	if(p_query->socket >= 0)
		close(p_query->socket);

	if(p_query->timeout)
		g_source_remove(p_query->timeout);

	while(p_query->cur_servers)
	{
		gfire_game_query_server *server = p_query->cur_servers->data;
		p_query->cur_servers = g_list_delete_link(p_query->cur_servers, p_query->cur_servers);
		gfire_game_server_free(server->server);
		g_free(server);
	}

	while(!g_queue_is_empty(p_query->servers))
	{
		gfire_game_query_server *server = g_queue_pop_head(p_query->servers);
		gfire_game_server_free(server->server);
		g_free(server);
	}
	g_queue_free(p_query->servers);

	g_free(p_query);
}

gchar *gfire_game_server_details(gfire_game_server *p_server)
{
	if(!p_server || !p_server->data || !p_server->data->driver)
		return NULL;

	return p_server->data->driver->details(p_server);
}

void gfire_game_server_free(gfire_game_server *p_server)
{
	if(!p_server)
		return;

	if(p_server->data)
	{
		if(p_server->data->driver)
			p_server->data->driver->free_server(p_server);

		g_free(p_server->data->name);
		g_free(p_server->data->map);
		g_free(p_server->data);
	}

	g_free(p_server);
}

void gfire_server_query_add_server(gfire_server_query *p_query, guint32 p_ip, guint16 p_port, gpointer p_data)
{
	if(!p_query || p_ip == 0 || p_port == 0)
		return;

	gfire_game_query_server *server = g_malloc0(sizeof(gfire_game_query_server));
	server->server = g_malloc0(sizeof(gfire_game_server));
	server->p_data = p_data;
	server->server->ip = p_ip;
	server->server->port = p_port;

	// Query the server immediately if there is space and we're running
	if(p_query->socket >= 0 && g_list_length(p_query->cur_servers) < 10)
	{
		p_query->cur_servers = g_list_append(p_query->cur_servers, server);
		p_query->driver->query(server->server, p_query->full_query, p_query->socket);

		// Timeout value
		GTimeVal gtv;
		g_get_current_time(&gtv);
		server->timeout = (gtv.tv_sec * 1000) + (gtv.tv_usec / 1000);
	}
	// Otherwise just add it to the queue
	else
		g_queue_push_tail(p_query->servers, server);
}

static void gfire_server_query_read(gpointer p_data, gint p_fd, PurpleInputCondition p_condition)
{
	static unsigned char buffer[65535];

	gfire_server_query *query = (gfire_server_query*)p_data;

	// Take the time now
	GTimeVal gtv;
	g_get_current_time(&gtv);

	struct sockaddr_in addr;
	guint addr_len = sizeof(addr);
	guint len = recvfrom(p_fd, buffer, 65535, 0, (struct sockaddr*)&addr, &addr_len);

	if(len <= 0)
		return;

	// Get the responding server
	GList *cur = query->cur_servers;
	gfire_game_query_server *server = NULL;
	while(cur)
	{
		gfire_game_query_server *scur = cur->data;
		if(scur->server->ip == g_ntohl(addr.sin_addr.s_addr) && scur->server->port == g_ntohs(addr.sin_port))
		{
			server = scur;
			break;
		}
		cur = g_list_next(cur);
	}

	if(!server)
	{
		purple_debug_warning("gfire", "Server Query: Got packet from unknown server\n");
		return;
	}

	// Let the driver do its work
	if(query->driver->parse(server->server, (gtv.tv_sec * 1000) + (gtv.tv_usec / 1000) - server->timeout,
							query->full_query, buffer, len))
	{
		query->driver->query(server->server, query->full_query, p_fd);
		// Reset timeout
		server->timeout = (gtv.tv_sec * 1000) + (gtv.tv_usec / 1000);
	}
	else
	{
		// Server is done
		query->callback(server->server, server->p_data, query->callback_data);
		query->cur_servers = g_list_delete_link(query->cur_servers, cur);
		g_free(server);

		// Fill the list with new ones
		while(!g_queue_is_empty(query->servers) && g_list_length(query->cur_servers) != GFSQ_MAX_QUERIES)
		{
			server = g_queue_pop_tail(query->servers);
			query->cur_servers = g_list_append(query->cur_servers, server);
			query->driver->query(server->server, query->full_query, p_fd);

			server->timeout = (gtv.tv_sec * 1000) + (gtv.tv_usec / 1000);
		}
	}
}

static gboolean gfire_server_query_check_timeout(gpointer p_data)
{
	gfire_server_query *query = (gfire_server_query*)p_data;

	GTimeVal gtv;
	g_get_current_time(&gtv);

	GList *cur = query->cur_servers;
	while(cur)
	{
		gfire_game_query_server *server = cur->data;

		if((gtv.tv_sec - (server->timeout / 1000)) > query->driver->timeout)
		{
			query->callback(server->server, server->p_data, query->callback_data);
			g_free(server);

			server = g_queue_pop_tail(query->servers);
			if(server)
			{
				cur->data = server;
				query->driver->query(server->server, query->full_query, query->socket);
				server->timeout = (gtv.tv_sec * 1000) + (gtv.tv_usec / 1000);
			}
			else
			{
				GList *next = g_list_next(cur);
				query->cur_servers = g_list_delete_link(query->cur_servers, cur);
				cur = next;
				continue;
			}
		}

		cur = g_list_next(cur);
	}

	return TRUE;
}

static void gfire_server_query_listen(int p_socket, gpointer p_data)
{
	gfire_server_query *query = (gfire_server_query*)p_data;
	query->prpl_data = NULL;
	query->socket = p_socket;

	query->timeout = g_timeout_add_seconds(1, gfire_server_query_check_timeout, query);

	// Populate the current server list and query them
	int i = 0;
	for(; (i < GFSQ_MAX_QUERIES) && !g_queue_is_empty(query->servers); i++)
	{
		gfire_game_query_server *server = g_queue_pop_tail(query->servers);
		query->cur_servers = g_list_append(query->cur_servers, server);
		query->driver->query(server->server, query->full_query, p_socket);

		// Timeout value
		GTimeVal gtv;
		g_get_current_time(&gtv);
		server->timeout = (gtv.tv_sec * 1000) + (gtv.tv_usec / 1000);
	}

	// Start reading on the socket
	query->prpl_inpa = purple_input_add(p_socket, PURPLE_INPUT_READ, gfire_server_query_read, query);
}

gboolean gfire_server_query_start(gfire_server_query *p_query, const gchar *p_type, gboolean p_full,
								  gfire_server_query_callback p_callback, gpointer p_data)
{
	if(!p_query || !p_type || !p_callback || p_query->prpl_data || p_query->prpl_inpa)
		return FALSE;

	int i = 0;
	for(; registeredDrivers[i].proto; i++)
	{
		if(g_strcmp0(registeredDrivers[i].proto, p_type) == 0)
		{
			p_query->driver = registeredDrivers[i].driver;
			break;
		}
	}
	if(!p_query->driver)
		return FALSE;

	p_query->full_query = p_full;

	p_query->callback = p_callback;
	p_query->callback_data = p_data;

	p_query->prpl_data = purple_network_listen_range(0, 0, SOCK_DGRAM, gfire_server_query_listen, p_query);
	return TRUE;
}

gboolean gfire_server_query_supports(const gchar *p_type)
{
	if(!p_type)
		return FALSE;

	int i = 0;
	for(; registeredDrivers[i].proto; i++)
	{
		if(g_strcmp0(registeredDrivers[i].proto, p_type) == 0)
			return TRUE;
	}
	return FALSE;
}

#endif // HAVE_GTK
