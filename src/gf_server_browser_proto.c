/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009, Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009       Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009       Oliver Ney <oliver@dryder.de>
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

#include "gf_server_browser_proto.h"
#include "gf_server_browser.h"

#ifdef HAVE_GTK
#include <sys/time.h>

static GMutex *mutex;
static GQueue servers_list_queue = G_QUEUE_INIT;
static gfire_server_browser *server_browser;

// Protocol queries
const char cod4_query[] = {0xFF, 0xFF, 0xFF, 0XFF, 0x67, 0x65, 0x74, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73};
const char quake3_query[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x67, 0x65, 0x74, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0x0A};
const char ut2004_query[] = {0x5C, 0x69, 0x6E, 0x66, 0x6F, 0x5C};

void gfire_server_browser_proto_init_mutex()
{
	if(!mutex)
		mutex = g_mutex_new();
}

static gfire_server_browser *gfire_server_browser_new()
{
	gfire_server_browser *ret;
	ret = (gfire_server_browser *)g_malloc0(sizeof(gfire_server_browser));

	return ret;
}

gfire_server_info *gfire_server_info_new()
{
	gfire_server_info *ret;
	ret = (gfire_server_info *)g_malloc0(sizeof(gfire_server_info));

	return ret;
}

static gchar *gfire_server_browser_send_packet(const guint32 server_ip, const gint server_port, const gchar server_query[])
{
	if(server_ip && server_port && server_query)
	{
		// Create socket and set options
		int query_socket;

		struct sockaddr_in query_address;
		unsigned short int query_port = server_port;

		// Open socket using UDP
		if((query_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			return NULL;

		query_address.sin_family = AF_INET;
		query_address.sin_addr.s_addr = server_ip;
		query_address.sin_port = htons(query_port);

		// Send query
		if(sendto(query_socket, server_query, strlen(server_query), 0, (struct sockaddr *)&query_address, sizeof(query_address)) < 0)
			return NULL;

		// Wait for response
		char response[GFIRE_SERVER_BROWSER_BUF];
		int n_read;

		// Define query timeout
		struct timeval timeout;

		timeout.tv_sec = 10;
		timeout.tv_usec = 500 * 1000;
		setsockopt(query_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));

		// Read query response
		n_read = recvfrom(query_socket, response, GFIRE_SERVER_BROWSER_BUF, 0, NULL, NULL);
		if(n_read <= 0)
		{
			close(query_socket);
			return NULL;
		}

		// Close socket
		close(query_socket);

		// Return response
		return g_strdup_printf("%s", response);
	}
	else
		return NULL;
}

static gfire_server_info *gfire_server_browser_quake3(const guint32 server_ip, const gint server_port)
{
	gfire_server_info *server_info = gfire_server_info_new();
	gchar *server_response = gfire_server_browser_send_packet(server_ip, server_port, quake3_query);

	if(!server_response)
		return server_info;

	// Fill raw info
	g_strchomp(server_response);
	server_info->raw_info = g_strdup(server_response);

	// Build full IP
	struct sockaddr_in antelope;
	antelope.sin_addr.s_addr = server_ip;

	server_info->ip_full = g_strdup_printf("%s:%u", inet_ntoa(antelope.sin_addr), server_port);

	// Split response in 3 parts (message, server information, players)
	gchar **server_response_parts = g_strsplit(server_response, "\n", 0);
	g_free(server_response);

	if(g_strv_length(server_response_parts) < 3)
		return server_info;

	gchar **server_response_split = g_strsplit(server_response_parts[1], "\\", 0);
	if(server_response_split)
	{
		int i;
		for (i = 0; i < g_strv_length(server_response_split); i++)
		{
			// IP & port
			server_info->ip = server_ip;
			server_info->port = server_port;

			// Server name
			if(!g_strcmp0("sv_hostname", server_response_split[i]))
				server_info->name = server_response_split[i+1];

			// Server map
			if(!g_strcmp0("mapname", server_response_split[i]))
				server_info->map = server_response_split[i+1];

			// Max. players
			if(!g_strcmp0("sv_maxclients", server_response_split[i]))
				server_info->max_players = atoi(server_response_split[i+1]);
		}

		// Count players
		guint32 players = g_strv_length(server_response_parts) - 2;
		server_info->players = players;

		// g_strfreev(server_response_parts);
		// g_strfreev(server_response_split);
	}

	return server_info;
}

static gfire_server_info *gfire_server_browser_wolfet(const guint32 server_ip, const gint server_port)
{
	gfire_server_info *server_info = gfire_server_info_new();
	server_info = gfire_server_browser_quake3(server_ip, server_port);

	return server_info;
}

gfire_server_info *gfire_server_browser_proto_query_server(gfire_server_info *p_server)
{
	gfire_server_info *ret;
	ret = gfire_server_info_new();

	struct timeval query_time_start, query_time_end, query_time_elapsed;
	gettimeofday(&query_time_start, NULL);

	gchar **server_ip_full_parts = g_strsplit(p_server->ip_full, ":", -1);

	// Determine query type
	if(!g_strcmp0(p_server->query_type, "WOLFET"))
		ret = gfire_server_browser_wolfet(inet_addr(server_ip_full_parts[0]), atoi(server_ip_full_parts[1]));
	else if(!g_strcmp0(p_server->query_type, "COD2"))
		ret = gfire_server_browser_wolfet(inet_addr(server_ip_full_parts[0]), atoi(server_ip_full_parts[1]));
	else if(!g_strcmp0(p_server->query_type, "COD4MW"))
		ret = gfire_server_browser_wolfet(inet_addr(server_ip_full_parts[0]), atoi(server_ip_full_parts[1]));
	else
		return ret;

	gettimeofday(&query_time_end, NULL);

	if(query_time_start.tv_usec > query_time_end.tv_usec)
	{
		query_time_end.tv_usec += 1000000;
		query_time_end.tv_sec--;
	}

	query_time_elapsed.tv_usec = query_time_end.tv_usec - query_time_start.tv_usec;
	query_time_elapsed.tv_sec = query_time_end.tv_sec - query_time_start.tv_sec;

	ret->server_list_iter = p_server->server_list_iter;
	ret->ping = query_time_elapsed.tv_usec / 1000;

	return ret;
}

void gfire_server_browser_update_server_list_thread(gfire_server_info *server_info)
{
	if(!server_info)
		return;

	gfire_server_info *server_info_queried;
	server_info_queried = gfire_server_info_new();
	server_info_queried = gfire_server_browser_proto_query_server(server_info);

	g_mutex_lock(mutex);
	g_queue_push_head(&servers_list_queue, server_info_queried);
	g_mutex_unlock(mutex);

	return;
}

gboolean gfire_server_browser_display_servers_cb(GtkTreeStore *p_tree_store)
{
	int i = 0;
	while(i != -1)
	{
		g_mutex_lock(mutex);
		if(g_queue_is_empty(&servers_list_queue) == TRUE)
		{
			g_mutex_unlock(mutex);
			return TRUE;
		}

		gfire_server_info *server = g_queue_pop_head(&servers_list_queue);

		if(server)
			gfire_server_browser_set_server(server);

		i++;
		g_mutex_unlock(mutex);
	}

	return TRUE;
}

void gfire_server_browser_show_fav_serverlist(guint32 p_gameid)
{
	// Initialize fav serverlist thread pool
	fav_serverlist_thread_pool = g_thread_pool_new((GFunc )gfire_server_browser_update_server_list_thread,
												   NULL, GFIRE_SERVER_BROWSER_THREADS_LIMIT + 1, FALSE, NULL);

	// Add favorite servers to list store & thread pool
	GList *favorite_servers = server_browser_proto_get_favorite_servers(p_gameid);

	for(; favorite_servers; favorite_servers = g_list_next(favorite_servers))
	{
		GtkTreeIter iter;
		gfire_game_data ip_data;

		gfire_server_browser_server *server;
		server = favorite_servers->data;

		ip_data.ip.value = server->ip;
		ip_data.port = server->port;

		gchar *addr = gfire_game_data_addr_str(&ip_data);

		// Add row for server, will be filled in later
		iter = gfire_server_browser_add_favorite_server_row(addr);

		// Get query type
		const gchar *server_query_type = gfire_game_server_query_type(server->gameid);

		// Add server to list
		gfire_server_info *server_info = gfire_server_info_new();
		server_info->ip_full = g_strdup(addr);
		server_info->query_type = g_strdup(server_query_type);

		// Insert tree iter
		server_info->server_list_iter = iter;

		// Push server to pool
		g_thread_pool_push(fav_serverlist_thread_pool, server_info, NULL);

		// Free data and go to next server
		g_free(addr);
		// g_free(i->data);
	}
}

void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;
	guint32 gameid;
	GList *ips = NULL;
	GList *ports = NULL;
	GList *i, *p = NULL;

	if(p_packet_len < 16)
	{
		purple_debug_error("gfire", "Packet 150 received, but too short (%d bytes)\n", p_packet_len);
		return;
	}

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &gameid, 0x21, offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &ips, 0x22, offset);
	if(offset == -1 || !ips)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &ports, 0x23, offset);
	if(offset == -1 || !ports)
	{
		if(ips) g_list_free(ips);
		return;
	}

	i = ips; p = ports;

	purple_debug_misc("gfire", "(serverlist): got the server list for %u\n", gameid);

	// Initialize serverlist thread pool (all)
	serverlist_thread_pool = g_thread_pool_new((GFunc )gfire_server_browser_update_server_list_thread,
											   NULL, GFIRE_SERVER_BROWSER_THREADS_LIMIT + 1, FALSE, NULL);

	// Add servers to list store & thread pool
	for(; i; i = g_list_next(i))
	{
		GtkTreeIter iter;
		gfire_game_data ip_data;

		ip_data.ip.value = *((guint32*)i->data);
		ip_data.port = *((guint32*)p->data);

		gchar *addr = gfire_game_data_addr_str(&ip_data);

		// Add row for server, will be filled in later
		iter = gfire_server_browser_add_server_row(addr);

		// Get query type
		const gchar *server_query_type = gfire_game_server_query_type(gameid);

		// Add server to list
		gfire_server_info *server_info = gfire_server_info_new();
		server_info->ip_full = g_strdup(addr);
		server_info->query_type = g_strdup(server_query_type);

		// Insert tree iter
		server_info->server_list_iter = iter;

		// Push server to pool
		g_thread_pool_push(serverlist_thread_pool, server_info, NULL);

		// Free data and go to next server
		g_free(addr);
		g_free(i->data);
		g_free(p->data);

		p = g_list_next(p);
	}
}

void gfire_server_browser_proto_friends_favorite_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	// FIXME: What's the correct length in bytes?
	if(p_packet_len < 16)
	{
		purple_debug_error("gfire", "Packet 149 received, but too short (%d bytes)\n", p_packet_len);
		return;
	}

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 gameid;
	GList *ips = NULL;
	GList *ports = NULL;
	GList *userids = NULL;

	offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &gameid, "gameid", offset);

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ips, "gip", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ports, "gport", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &userids, "friends", offset);
	if(offset == -1)
		return;

	// Initialize friends' favorite serverlist thread pool
	friends_fav_serverlist_thread_pool = g_thread_pool_new((GFunc )gfire_server_browser_update_server_list_thread,
														   NULL, GFIRE_SERVER_BROWSER_THREADS_LIMIT + 1, FALSE, NULL);

	// Add servers to list store & thread pool
	for(; ips; ips = g_list_next(ips))
	{
		GtkTreeIter iter;
		gfire_game_data ip_data;

		ip_data.ip.value = *((guint32*)ips->data);
		ip_data.port = *((guint32*)ports->data);

		gchar *addr = gfire_game_data_addr_str(&ip_data);

		// Add row for server, will be filled in later
		iter = gfire_server_browser_add_friends_favorite_server_row(addr);

		// Get query type
		const gchar *server_query_type = gfire_game_server_query_type(gameid);

		// Add server to list
		gfire_server_info *server_info = gfire_server_info_new();
		server_info->ip_full = g_strdup(addr);
		server_info->query_type = g_strdup(server_query_type);

		// Insert tree iter
		server_info->server_list_iter = iter;

		// Push server to pool
		g_thread_pool_push(friends_fav_serverlist_thread_pool, server_info, NULL);

		// Free data and go to next server
		g_free(addr);
		// g_free(i->data);
		// g_free(p->data);

		ports = g_list_next(ports);
	}

	// Add timeout to display queried servers
	p_gfire->server_browser_pool = g_timeout_add(800, (GSourceFunc )gfire_server_browser_display_servers_cb, NULL);
}

gboolean gfire_server_browser_proto_is_favorite_server(guint32 p_ip, guint16 p_port)
{
	gboolean ret = FALSE;

	GList *favorite_servers_tmp = g_list_copy(server_browser->favorite_servers);

	for(; favorite_servers_tmp; favorite_servers_tmp = g_list_next(favorite_servers_tmp))
	{
		gfire_server_browser_server *server;
		server = favorite_servers_tmp->data;

		if(server->ip == p_ip && server->port == p_port)
		{
			ret = TRUE;
			break;
		}
	}

	return ret;
}

guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid)
{
	guint32 offset = XFIRE_HEADER_LEN;
	p_gameid = GUINT32_TO_LE(p_gameid);

	offset = gfire_proto_write_attr_bs(0x21, 0x02, &p_gameid, sizeof(p_gameid), offset);
	gfire_proto_write_header(offset, 0x16, 1, 0);

	return offset;
}

guint16 gfire_server_browser_proto_request_remove_favorite_server(guint32 p_gameid, guint32 p_ip, guint32 p_port)
{
	guint32 offset = XFIRE_HEADER_LEN;

	p_gameid = GUINT32_TO_LE(p_gameid);
	p_ip = GUINT32_TO_LE(p_ip);
	p_port = GUINT32_TO_LE(p_port);

	offset = gfire_proto_write_attr_ss("gameid", 0x02, &p_gameid, sizeof(p_gameid), offset);
	offset = gfire_proto_write_attr_ss("gip", 0x02, &p_ip, sizeof(p_ip), offset);
	offset = gfire_proto_write_attr_ss("gport", 0x02, &p_port, sizeof(p_port), offset);
	gfire_proto_write_header(offset, 0x14, 3, 0);

	return offset;
}

guint16 gfire_server_browser_proto_request_add_favorite_server(guint32 p_gameid, guint32 p_ip, guint32 p_port)
{
	guint32 offset = XFIRE_HEADER_LEN;

	p_gameid = GUINT32_TO_LE(p_gameid);
	p_ip = GUINT32_TO_LE(p_ip);
	p_port = GUINT32_TO_LE(p_port);

	offset = gfire_proto_write_attr_ss("gameid", 0x02, &p_gameid, sizeof(p_gameid), offset);
	offset = gfire_proto_write_attr_ss("gip", 0x02, &p_ip, sizeof(p_ip), offset);
	offset = gfire_proto_write_attr_ss("gport", 0x02, &p_port, sizeof(p_port), offset);
	gfire_proto_write_header(offset, 0x13, 3, 0);

	return offset;
}

void gfire_server_browser_proto_favorite_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	if(p_packet_len < 42)
	{
		purple_debug_error("gfire", "Packet 148 received, but too short (%d bytes)\n", p_packet_len);
		return;
	}

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 max_favorites;
	GList *servers_gameids = NULL;
	GList *servers_ips = NULL;
	GList *servers_ports = NULL;

	offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &max_favorites, "max", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &servers_gameids, "gameid", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &servers_ips, "gip", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &servers_ports, "gport", offset);
	if(offset == -1)
		return;

	// Initialize server browser struct & set max favorites
	server_browser = gfire_server_browser_new();
	server_browser->max_favorites = max_favorites;

	// Fill in favorite server list
	GList *favorite_servers = NULL;

	for(; servers_ips; servers_ips = g_list_next(servers_ips))
	{
		// Server (ip & port)
		gfire_server_browser_server *server;
		server = (gfire_server_browser_server *)g_malloc0(sizeof(gfire_server_browser_server));

		server->gameid = *((guint32 *)servers_gameids->data);
		server->ip = *((guint32 *)servers_ips->data);
		server->port = *(guint32 *)servers_ports->data & 0xFFFF;

		// Add to server list
		favorite_servers = g_list_append(favorite_servers, server);

		// Get next server
		servers_gameids = g_list_next(servers_gameids);
		servers_ports = g_list_next(servers_ports);
	}

	server_browser->favorite_servers = favorite_servers;
}

GList *server_browser_proto_get_favorite_servers(const guint32 p_gameid)
{
	GList *ret = NULL;

	if(!p_gameid)
		return ret;

	GList *favorite_servers_tmp = g_list_copy(server_browser->favorite_servers);

	for(; favorite_servers_tmp; favorite_servers_tmp = g_list_next(favorite_servers_tmp))
	{
		gfire_server_browser_server *server;
		server = favorite_servers_tmp->data;

		if(server->gameid == p_gameid)
			ret = g_list_append(ret, server);
	}

	return ret;
}

gboolean gfire_server_browser_can_add_favorite_server()
{
	if(g_list_length(server_browser->favorite_servers) < server_browser->max_favorites)
		return TRUE;
	else
		return FALSE;
}

void gfire_server_browser_add_favorite_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
	gfire_server_browser_server *server;
	server = (gfire_server_browser_server *)g_malloc0(sizeof(gfire_server_browser_server));

	server->gameid = p_gameid;
	server->ip = p_ip;
	server->port = p_port;

	// Add favorite server to local list
	server_browser->favorite_servers = g_list_append(server_browser->favorite_servers, server);
}

void gfire_server_browser_remove_favorite_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
	GList *favorite_servers_tmp = g_list_copy(server_browser->favorite_servers);

	for(; favorite_servers_tmp; favorite_servers_tmp = g_list_next(favorite_servers_tmp))
	{
		gfire_server_browser_server *server;
		server = favorite_servers_tmp->data;

		if(server->gameid == p_gameid && server->ip == p_ip && server->port == p_port)
			server_browser->favorite_servers = g_list_remove(server_browser->favorite_servers, server);
	}
}

guint16 gfire_server_browser_proto_create_friends_favorite_serverlist_request(guint32 p_gameid)
{
	guint32 offset = XFIRE_HEADER_LEN;
	p_gameid = GUINT32_TO_LE(p_gameid);

	offset = gfire_proto_write_attr_ss("gameid", 0x02, &p_gameid, sizeof(p_gameid), offset);
	gfire_proto_write_header(offset, 0x15, 1, 0);

	return offset;
}

void gfire_server_browser_proto_remove_favorite_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port)
{
	gfire_game_data game;
	gfire_game_data_ip_from_str(&game, p_ip);

	guint16 port;
	sscanf(p_port, "%hu", &port);
	game.port = port;

	guint16 packet_len = 0;
	packet_len = gfire_server_browser_proto_request_remove_favorite_server(p_gameid, game.ip.value, port);

	if(packet_len > 0)
		gfire_send(gfire_get_connection(p_gfire), packet_len);
}

void gfire_server_browser_proto_add_favorite_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port)
{
	gfire_game_data game;
	gfire_game_data_ip_from_str(&game, p_ip);

	guint16 port;
	sscanf(p_port, "%hu", &port);
	game.port = port;

	guint16 packet_len = 0;
	packet_len = gfire_server_browser_proto_request_add_favorite_server(p_gameid, game.ip.value, port);

	if(packet_len > 0)
		gfire_send(gfire_get_connection(p_gfire), packet_len);
}
#endif // HAVE_GTK
