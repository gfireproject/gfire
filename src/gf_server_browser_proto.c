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

const char quake3_query[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x67, 0x65, 0x74, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0x0A};
const char ut2004_query[] = {0x5C, 0x69, 0x6E, 0x66, 0x6F, 0x5C};
const char cod4_query[] = {0xFF, 0xFF, 0xFF, 0XFF, 0x67, 0x65, 0x74, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73};

static GMutex *mutex;
static GQueue servers_list_queue = G_QUEUE_INIT;

gfire_server_info *gfire_server_info_new()
{
    gfire_server_info *server_info;
    server_info = (gfire_server_info *)g_malloc0(sizeof(gfire_server_info));

    return server_info;
}

guint32 gfire_server_browser_ping(guint32 server_ip)
{
    return -1;
}

gchar *gfire_server_browser_send_packet(const guint32 server_ip, const gint server_port, const gchar server_query[])
{
    if (server_ip && server_port && server_query)
    {
	// Create socket and set options
	int query_socket;

	struct sockaddr_in query_address;
	unsigned short int query_port = server_port;

	// Open socket using UDP
	if ((query_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	    return NULL;

	query_address.sin_family = AF_INET;
	query_address.sin_addr.s_addr = server_ip;
	query_address.sin_port = htons(query_port);

	// Send query
	if (sendto(query_socket, server_query, strlen(server_query), 0, (struct sockaddr *)&query_address, sizeof(query_address)) < 0)
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
	if( n_read <= 0)
	    return NULL;

	// Close socket
	close(query_socket);

	// Return response
	gchar *query_response;
	query_response = g_strdup_printf("%s", response);

	return query_response;
    }
    else
	return NULL;
}

gfire_server_info *gfire_server_browser_quake3(const guint32 server_ip, const gint server_port)
{
    gchar *server_response = gfire_server_browser_send_packet(server_ip, server_port, quake3_query);
    gfire_server_info *server_info = gfire_server_info_new();


    char *popo;

    struct sockaddr_in antelope;
    antelope.sin_addr.s_addr = server_ip;
    popo = inet_ntoa(antelope.sin_addr); // Return the IP

    server_info->ip_full = g_strdup_printf("%s:%u", popo, server_port);
    gchar **server_response_split = g_strsplit(server_response, "\\", 0);

    if (server_response_split)
    {
	int i;
	for (i = 0; i < g_strv_length(server_response_split); i++)
	{
	    // IP & port
	    server_info->ip = server_ip;
	    server_info->port = server_port;

	    // Ping
	    server_info->ping = gfire_server_browser_ping(server_ip);

	    // Server name
	    if (!g_strcmp0("sv_hostname", server_response_split[i]))
		server_info->name = server_response_split[i+1];

	    // Game mod
	    if (!g_strcmp0("gamename", server_response_split[i]))
		server_info->mod = server_response_split[i+1];

	    // Server map
	    if (!g_strcmp0("mapname", server_response_split[i]))
		server_info->map = server_response_split[i+1];

	    // Punkbuster
	    if (!g_strcmp0("sv_punkbuster", server_response_split[i]))
	    {
		if (!g_strcmp0(server_response_split[i+1], "1"))
		    server_info->punkbuster = TRUE;
		else
		    server_info->punkbuster = FALSE;
	    }

	    // Max. players
	    if (!g_strcmp0("sv_maxclients", server_response_split[i]))
		server_info->max_players = atoi(server_response_split[i+1]);
	}

	// Count players
	guint32 players;

	gchar *baba = g_strdup_printf("%s", server_response_split[g_strv_length(server_response_split) - 1]);
	gchar **players_parts = g_strsplit(baba, "\n", 0);

	players = g_strv_length(players_parts) - 2;
	server_info->players = players;
    }

    return server_info;
}

gfire_server_info *gfire_server_browser_wolfet(const guint32 server_ip, const gint server_port)
{
    gfire_server_info *server_info = gfire_server_info_new();
    server_info = gfire_server_browser_quake3(server_ip, server_port);

    return server_info;
}

void gfire_server_browser_update_server_list_thread(gfire_server_info *server_info)
{
    if (!server_info)
	return;

    gchar *server_ip_full;
    gchar **server_ip_full_parts;

    server_ip_full = g_strdup(server_info->ip_full);
    server_ip_full_parts = g_strsplit(server_ip_full, ":", -1);

    if(!server_ip_full_parts || !server_ip_full)
	return;

    gfire_server_info *server_info_queried = gfire_server_info_new();

    // Determine query type
    if (!g_strcmp0(server_info->query_type, "WOLFET"))
	server_info_queried = gfire_server_browser_wolfet(inet_addr(server_ip_full_parts[0]), atoi(server_ip_full_parts[1]));
    else if (!g_strcmp0(server_info->query_type, "COD4MW"))
	server_info_queried = gfire_server_browser_wolfet(inet_addr(server_ip_full_parts[0]), atoi(server_ip_full_parts[1]));
    else
	return;

    server_info_queried->server_list_pos = server_info->server_list_pos;

    g_mutex_lock(mutex);
    g_queue_push_head(&servers_list_queue, server_info_queried);
    g_mutex_unlock(mutex);

    return;
}

gboolean gfire_server_browser_display_servers_cb(GtkListStore *p_list_store)
{
    int i = 0;
    while(i != -1)
    {
	g_mutex_lock(mutex);
	if (g_queue_is_empty(&servers_list_queue) == TRUE)
	{
	    // g_printf("gfire: No new servers queried, ignoring.\n");
	    g_mutex_unlock(mutex);
	    return TRUE;
	}

	gfire_server_info *server = g_queue_pop_head(&servers_list_queue);
	if (server)
	{
	    GtkTreeIter iter;
	    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(p_list_store), &iter, g_strdup_printf("%d", server->server_list_pos));

	    if (server->name)
		gtk_list_store_set(p_list_store, &iter, 0, g_strstrip(gfire_escape_color_codes(server->name)), -1);

	    if (server->players && server->max_players)
	    {
		gchar *server_players = g_strdup_printf("%u/%u", server->players, server->max_players);
		gtk_list_store_set(p_list_store, &iter, 3, server_players, -1);
	    }

	    i++;
	}

	g_mutex_unlock(mutex);
    }

    return TRUE;
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

    if(!p_gfire->server_browser)
	return;

    if (p_packet_len < 16) {
	purple_debug_error("gfire", "gfire_server_browser_proto_serverlist: Packet 131 received, but too short (%d bytes).\n", p_packet_len);
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
	if (ips) g_list_free(ips);
	    return;
    }

    i = ips; p = ports;

    purple_debug(PURPLE_DEBUG_MISC, "gfire", "(serverlist): got the server list for %u\n", gameid);

    // Set up server browser builder & list store
    GtkBuilder *builder = p_gfire->server_browser;
    GtkListStore *list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "servers_list_store"));
    gtk_list_store_clear(list_store);

    // Initialize mutex & thread pool
    if (!mutex)
	mutex = g_mutex_new();

    servers_list_thread_pool = g_thread_pool_new((GFunc )gfire_server_browser_update_server_list_thread,
						 NULL, GFIRE_SERVER_BROWSER_THREADS_LIMIT + 1, FALSE, NULL);

    // Add servers to list store & thread pool
    int servers = 0;
    for(; i; i = g_list_next(i))
    {
	GtkTreeIter iter;
	gfire_game_data ip_data;

	ip_data.ip.value = *((guint32*)i->data);
	ip_data.port = *((guint32*)p->data);

	gchar *addr = gfire_game_data_addr_str(&ip_data);

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, 0, addr, 1, addr, 2, _("N/A"), 3, _("N/A"), -1);

	// Get query type
	gchar *server_query_type = gfire_game_server_query_type(servers_list_queried_game_id);

	// Add server to list
	gfire_server_info *server_info = gfire_server_info_new();
	server_info->query_type =
	server_info->ip_full = g_strdup(addr);
	server_info->server_list_pos = servers;
	server_info->query_type = g_strdup(server_query_type);

	g_thread_pool_push(servers_list_thread_pool, server_info, NULL);

	// Free data and go to next server
	g_free(addr);
	g_free(i->data);
	g_free(p->data);

	p = g_list_next(p);

	// Count servers
	servers++;
    }

    // Add timeout to display queried servers
    g_timeout_add(800, (GSourceFunc )gfire_server_browser_display_servers_cb, list_store);
}

guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid)
{
    guint32 offset = XFIRE_HEADER_LEN;
    p_gameid = GUINT32_TO_LE(p_gameid);

    offset = gfire_proto_write_attr_bs(0x21, 0x02, &p_gameid, sizeof(p_gameid), offset);
    gfire_proto_write_header(offset, 0x16, 1, 0);

    return offset;
}
