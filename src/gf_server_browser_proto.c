/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
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

#include "gf_server_browser_proto.h"
#include "gf_server_browser.h"

#ifdef HAVE_GTK
#include <sys/time.h>

// Protocol queries
#define GS_QUERY "\x5Cinfo\x5C"
#define Q3A_QUERY "\xFF\xFF\xFF\xFFgetstatus"
#define SOURCE_QUERY "\xFF\xFF\xFF\xFF\x54Source Engine Query"
#define UT2K3_QUERY "\x5Cstatus\x5C\x00"

static void gfire_server_browser_proto_parse_packet_gs(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info);
static void gfire_server_browser_proto_parse_packet_q3a(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info);
static void gfire_server_browser_proto_parse_packet_source(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info);

// Creation & freeing
static gfire_server_browser *gfire_server_browser_new()
{
	gfire_server_browser *ret;
	ret = (gfire_server_browser *)g_malloc0(sizeof(gfire_server_browser));

	ret->fav_servers = NULL;

	ret->fd = -1;
	ret->fd_handler = 0;
	ret->timeout_src = 0;

	ret->queue = NULL;
	ret->queried_list = NULL;

	return ret;
}

static void gfire_server_browser_free(gfire_server_browser *p_server_browser)
{
    if(!p_server_browser)
        return;

    gfire_list_clear(p_server_browser->fav_servers);
    g_free(p_server_browser);
}

gfire_server_browser_server *gfire_server_browser_server_new()
{
	gfire_server_browser_server *ret;
	ret = (gfire_server_browser_server *)g_malloc0(sizeof(gfire_server_browser_server));

	return ret;
}

void gfire_server_browser_server_free(gfire_server_browser_server *p_server)
{
    if(!p_server)
        return;

    g_free(p_server);
}

gfire_server_browser_server_info *gfire_server_browser_server_info_new()
{
	gfire_server_browser_server_info *ret;
	ret = (gfire_server_browser_server_info *)g_malloc0(sizeof(gfire_server_browser_server_info));

	ret->players = -1;
	ret->max_players = -1;

	ret->ping = -1;

	return ret;
}

void gfire_server_browser_server_info_free(gfire_server_browser_server_info *p_server_info)
{
    if(!p_server_info)
        return;

    if(p_server_info->raw_info)
        g_free(p_server_info->raw_info);

    if(p_server_info->ip_full)
        g_free(p_server_info->ip_full);

    g_free(p_server_info);
}

// Server query functions
static void gfire_server_browser_send_packet(gfire_server_browser *p_server_browser, guint32 p_ip, guint16 p_port, const gchar p_query[])
{
	if(!p_ip || !p_port || !p_query)
		return;

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(p_ip);
	address.sin_port = htons(p_port);

	// Send query
    sendto(p_server_browser->fd, p_query, strlen(p_query), 0, (struct sockaddr *)&address, sizeof(address));
}

static void gfire_server_browser_proto_send_query(gfire_server_browser *p_server_browser, gfire_server_browser_server *p_server)
{
	// Set start time for calculating ping later
	GTimeVal time;
	g_get_current_time(&time);

	p_server->time = time;

	// Determine query type & perform query
    if(!g_strcmp0(p_server->protocol, "GS"))
        gfire_server_browser_send_packet(p_server_browser, p_server->ip, p_server->port, GS_QUERY);
    else if(!g_strcmp0(p_server->protocol, "SOURCE"))
        gfire_server_browser_send_packet(p_server_browser, p_server->ip, p_server->port, SOURCE_QUERY);
	else if(!g_strcmp0(p_server->protocol, "UT2K3"))
        gfire_server_browser_send_packet(p_server_browser, p_server->ip, p_server->port + 10, UT2K3_QUERY);
    else if(!g_strcmp0(p_server->protocol, "WOLFET"))
        gfire_server_browser_send_packet(p_server_browser, p_server->ip, p_server->port, Q3A_QUERY);
}

static void gfire_server_browser_proto_calculate_ping(gfire_server_browser *p_server_browser, gfire_server_browser_server_info *p_server)
{
	if(!p_server)
		return;

    GList *queried = p_server_browser->queried_list;
	GTimeVal start, now;

	start.tv_sec = 0;
	start.tv_usec = 0;

	for(; queried; queried = g_list_next(queried))
	{
		gfire_server_browser_server *server = (gfire_server_browser_server *)queried->data;

		if((p_server->ip == server->ip) && (p_server->port == server->port))
		{
			start.tv_usec = server->time.tv_usec;
			start.tv_sec = server->time.tv_sec;

			break;
		}
	}

	// Calculate ping & set it
	g_get_current_time(&now);
	p_server->ping = ((now.tv_usec + now.tv_sec * 1000000) - (start.tv_usec + start.tv_sec * 1000000)) / 1000;
}

static void gfire_server_brower_proto_input_cb(gfire_server_browser *p_server_browser, PurpleInputCondition p_condition)
{
	gchar buffer[1024];
	gint n_read;

	struct sockaddr_in server;
	socklen_t server_len = sizeof(server);

    n_read = recvfrom(p_server_browser->fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &server_len);
	if(n_read <= 0)
		return;

	// Get IP and port
	gchar ip[INET_ADDRSTRLEN];
	guint16 port;

	inet_ntop(AF_INET, &(server.sin_addr), ip, INET_ADDRSTRLEN);
	port = ntohs(server.sin_port);

	// Create server struct
	gfire_server_browser_server_info *server_info = gfire_server_browser_server_info_new();

	// Fill raw info
	g_strchomp(buffer);
	server_info->raw_info = g_strdup(buffer);
	server_info->ip_full = g_strdup_printf("%s:%u", ip, port);

	// IP & port
	gfire_game_data game;
	gfire_game_data_ip_from_str(&game, ip);

	server_info->ip = game.ip.value;
	server_info->port = port;

	// Calculate ping
    gfire_server_browser_proto_calculate_ping(p_server_browser, server_info);

	// Determine protocol parser
    const gchar *protocol = gfire_game_server_query_type(p_server_browser->gameid);

	if(!g_strcmp0(protocol, "WOLFET"))
		gfire_server_browser_proto_parse_packet_q3a(n_read, buffer, server_info);
	else if(!g_strcmp0(protocol, "SOURCE"))
		gfire_server_browser_proto_parse_packet_source(n_read, buffer, server_info);
	else if(!g_strcmp0(protocol, "GS"))
		gfire_server_browser_proto_parse_packet_gs(n_read, buffer, server_info);
	else if(!g_strcmp0(protocol, "UT2K3"))
		 gfire_server_browser_proto_parse_packet_gs(n_read, buffer, server_info);

    // Add server to tree store
    gfire_server_browser_add_server(p_server_browser, server_info);
}

static gboolean gfire_server_browser_proto_timeout_cb(gfire_server_browser *p_server_browser)
{
	gint i;
	for(i = 0 ; i < 20; i++)
	{
        gfire_server_browser_server *server = g_queue_pop_head(p_server_browser->queue);
		if(server)
		{
			// Send server query & push to queried queue
            gfire_server_browser_proto_send_query(p_server_browser, server);
            p_server_browser->queried_list = g_list_append(p_server_browser->queried_list, server);
		}
	}

	return TRUE;
}

static void gfire_server_browser_proto_init(gfire_data *p_gfire)
{
    // Init server browser struct
    p_gfire->server_browser = gfire_server_browser_new();
    p_gfire->server_browser->gfire = p_gfire;

	// Init socket
    if(p_gfire->server_browser->fd < 0)
        p_gfire->server_browser->fd = socket(AF_INET, SOCK_DGRAM, 0);

	// Query timeout
    if(!(p_gfire->server_browser->timeout_src))
        p_gfire->server_browser->timeout_src = g_timeout_add(500, gfire_server_browser_proto_timeout_cb, p_gfire->server_browser);

	// Queue
    if(!(p_gfire->server_browser->queue))
        p_gfire->server_browser->queue = g_queue_new();

	// Input handler
    if(!(p_gfire->server_browser->fd_handler))
        p_gfire->server_browser->fd_handler = purple_input_add(p_gfire->server_browser->fd, PURPLE_INPUT_READ, (PurpleInputFunction )gfire_server_brower_proto_input_cb, p_gfire->server_browser);
}

void gfire_server_browser_proto_free(gfire_server_browser *p_server_browser)
{
    // Close server browser window
    GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "server_browser_window"));
    gtk_widget_destroy(server_browser_window);

	// Close socket
    if(p_server_browser->fd >= 0)
        close(p_server_browser->fd);

	// Remove timeout
    if(p_server_browser->timeout_src)
        g_source_remove(p_server_browser->timeout_src);

	// Remove queue
    if(p_server_browser->queue)
        g_queue_free(p_server_browser->queue);

	// Remove input handler
    if(p_server_browser->fd_handler)
        purple_input_remove(p_server_browser->fd_handler);

    // Free server browser struct
    gfire_server_browser_free(p_server_browser);
}

// Serverlist requests
guint16 gfire_server_browser_proto_create_friends_fav_serverlist_request(guint32 p_gameid)
{
	guint32 offset = XFIRE_HEADER_LEN;
	p_gameid = GUINT32_TO_LE(p_gameid);

	offset = gfire_proto_write_attr_ss("gameid", 0x02, &p_gameid, sizeof(p_gameid), offset);
	gfire_proto_write_header(offset, 0x15, 1, 0);

	return offset;
}

guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid)
{
	guint32 offset = XFIRE_HEADER_LEN;
	p_gameid = GUINT32_TO_LE(p_gameid);

	offset = gfire_proto_write_attr_bs(0x21, 0x02, &p_gameid, sizeof(p_gameid), offset);
	gfire_proto_write_header(offset, 0x16, 1, 0);

	return offset;
}

// Serverlist handlers
void gfire_server_browser_proto_fav_serverlist_request(gfire_server_browser *p_server_browser, guint32 p_gameid)
{
	// Get fav serverlist (local)
    GList *servers = p_server_browser->fav_servers;

	// Push servers to queue as structs
	for(; servers; servers = g_list_next(servers))
	{
		// Create struct
		gfire_server_browser_server *server;
		server = servers->data;

        if(server->gameid == p_server_browser->gameid)
		{
			server->protocol = gfire_game_server_query_type(p_gameid);
			server->parent = 1;

			// Push server struct to queue
            g_queue_push_head(p_server_browser->queue, server);
		}
	}

	// Only free the list, not the data
	g_list_free(servers);
}

void gfire_server_browser_proto_fav_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	if(p_packet_len < 42)
	{
		purple_debug_error("gfire", "Packet 148 received, but too short (%d bytes)\n", p_packet_len);
		return;
	}

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 max_favs;
	GList *gameids = NULL;
	GList *ips = NULL;
	GList *ports = NULL;

	offset = gfire_proto_read_attr_int32_ss(p_gfire->buff_in, &max_favs, "max", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &gameids, "gameid", offset);
	if(offset == -1)
	{
                if(gameids) g_list_free(gameids);

		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ips, "gip", offset);
	if(offset == -1)
	{
		if(gameids) g_list_free(gameids);
		if(ips) g_list_free(ips);

		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ports, "gport", offset);
	if(offset == -1)
	{
		if(gameids) g_list_free(gameids);
		if(ips) g_list_free(ips);
		if(ports) g_list_free(ports);

		return;
	}

    // Initialize server browser struct
    gfire_server_browser_proto_init(p_gfire);
    gfire_server_browser *server_browser = p_gfire->server_browser;

    // Set max favorites
    server_browser->max_favs = max_favs;

	// Fill in favorite server list
	for(; ips; ips = g_list_next(ips))
	{
		// Create struct
		gfire_server_browser_server *server;
		server = gfire_server_browser_server_new();

		server->gameid = *((guint32 *)gameids->data);
		server->ip = *((guint32 *)ips->data);
		server->port = *(guint32 *)ports->data & 0xFFFF;

		// Add to fav serverlist
		server_browser->fav_servers = g_list_append(server_browser->fav_servers, server);

		// Get next server
		g_free(gameids->data);
		g_free(ips->data);
		g_free(ports->data);

		gameids = g_list_next(gameids);
		ports = g_list_next(ports);
	}

	g_list_free(gameids);
	g_list_free(ips);
	g_list_free(ports);
}

void gfire_server_browser_proto_friends_fav_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
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
	{
		if(ips) g_list_free(ips);

		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &ports, "gport", offset);
	if(offset == -1)
	{
		if(ips) g_list_free(ips);
		if(ports) g_list_free(ports);

		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &userids, "friends", offset);
	if(offset == -1)
	{
		if(ips) g_list_free(ips);
		if(ports) g_list_free(ports);
		if(userids) g_list_free(userids);

		return;
	}

	// FIXME: Not used yet, free...
	g_list_free(userids);

	// Push servers to queue as structs
	for(; ips; ips = g_list_next(ips))
	{
		// Create struct
		gfire_server_browser_server *server;
		server = gfire_server_browser_server_new();

		server->protocol = gfire_game_server_query_type(gameid);
		server->ip = *((guint32 *)ips->data);
		server->port = *((guint16 *)ports->data);
		server->parent = 2;

		// Push server struct to queue
        g_queue_push_head(p_gfire->server_browser->queue, server);

		// Free data and go to next server
		g_free(ips->data);
		g_free(ports->data);

		ports = g_list_next(ports);
	}

	g_list_free(ips);
	g_list_free(ports);
}

void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	if(p_packet_len < 16)
	{
		purple_debug_error("gfire", "Packet 150 received, but too short (%d bytes)\n", p_packet_len);
		return;
	}

	guint32 offset = XFIRE_HEADER_LEN;

	guint32 gameid;
	GList *ips = NULL;
	GList *ports = NULL;

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &gameid, 0x21, offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &ips, 0x22, offset);
	if(offset == -1)
	{
		if(ips) g_list_free(ips);

		return;
	}

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &ports, 0x23, offset);
	if(offset == -1)
	{
		if(ips) g_list_free(ips);
		if(ports) g_list_free(ports);

		return;
	}

	// Push servers to queue as structs
	for(; ips; ips = g_list_next(ips))
	{
		// Create struct
		gfire_server_browser_server *server;
		server = gfire_server_browser_server_new();

		server->protocol = gfire_game_server_query_type(gameid);
		server->ip = *((guint32 *)ips->data);
		server->port = *((guint16 *)ports->data);
		server->parent = 3;

		// Push server struct to queue
        g_queue_push_head(p_gfire->server_browser->queue, server);

		// Free data and go to next server
		g_free(ips->data);
		g_free(ports->data);

		ports = g_list_next(ports);
	}

	g_list_free(ips);
	g_list_free(ports);
}

// Remote fav serverlist functions (requests)
guint16 gfire_server_browser_proto_request_add_fav_server(guint32 p_gameid, guint32 p_ip, guint32 p_port)
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

guint16 gfire_server_browser_proto_request_remove_fav_server(guint32 p_gameid, guint32 p_ip, guint32 p_port)
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

// Local fav serverlist functions
void gfire_server_browser_add_fav_server_local(gfire_data *p_gfire, guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
	gfire_server_browser_server *server = gfire_server_browser_server_new();

	server->gameid = p_gameid;
	server->ip = p_ip;
	server->port = p_port;

	// Add favorite server to local list
    p_gfire->server_browser->fav_servers = g_list_append(p_gfire->server_browser->fav_servers, server);
}

void gfire_server_browser_remove_fav_server_local(gfire_data *p_gfire, guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
    GList *favorite_servers_tmp = p_gfire->server_browser->fav_servers;

	for(; favorite_servers_tmp; favorite_servers_tmp = g_list_next(favorite_servers_tmp))
	{
		gfire_server_browser_server *server;
		server = favorite_servers_tmp->data;

		if(server->gameid == p_gameid && server->ip == p_ip && server->port == p_port)
            p_gfire->server_browser->fav_servers = g_list_remove(p_gfire->server_browser->fav_servers, server);
	}
}

gboolean gfire_server_browser_proto_can_add_fav_server(gfire_server_browser *p_server_browser)
{
    if(g_list_length(p_server_browser->fav_servers) < p_server_browser->max_favs)
		return TRUE;
	else
		return FALSE;
}

void gfire_server_browser_proto_add_fav_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port)
{
	gfire_game_data server_data;
	gfire_game_data_ip_from_str(&server_data, p_ip);

	guint16 port;
	sscanf(p_port, "%hu", &port);

	guint16 packet_len = 0;
	packet_len = gfire_server_browser_proto_request_add_fav_server(p_gameid, server_data.ip.value, port);

	if(packet_len > 0)
		gfire_send(gfire_get_connection(p_gfire), packet_len);
}

void gfire_server_browser_proto_remove_fav_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port)
{
	gfire_game_data game;
	gfire_game_data_ip_from_str(&game, p_ip);

	guint16 port;
	sscanf(p_port, "%hu", &port);
	game.port = port;

	guint16 packet_len = 0;
	packet_len = gfire_server_browser_proto_request_remove_fav_server(p_gameid, game.ip.value, port);

	if(packet_len > 0)
		gfire_send(gfire_get_connection(p_gfire), packet_len);
}

gboolean gfire_server_browser_proto_is_fav_server(gfire_server_browser *p_server_browser, guint32 p_ip, guint16 p_port)
{
	gboolean ret = FALSE;
    GList *favorite_servers_tmp = p_server_browser->fav_servers;

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

// Misc.
void gfire_server_browser_proto_set_curr_gameid(gfire_server_browser *p_server_browser, guint32 p_gameid)
{
    p_server_browser->gameid = p_gameid;
}

gint gfire_server_brower_proto_get_parent(gfire_server_browser *p_server_browser, gfire_server_browser_server_info *p_server)
{
	if(!p_server)
		return -1;

    GList *queried = p_server_browser->queried_list;
	gint ret = -1;

	for(; queried; queried = g_list_next(queried))
	{
		gfire_server_browser_server *server = (gfire_server_browser_server *)queried->data;

		if((p_server->ip == server->ip) && (p_server->port == server->port))
		{
			ret = server->parent;
            p_server_browser->queried_list = g_list_remove(p_server_browser->queried_list, server);

			break;
		}
	}

	// Return -1 if not found at all
	return ret;
}

void gfire_server_browser_proto_push_server(gfire_server_browser *p_server_browser, gfire_server_browser_server *p_server)
{
    g_queue_push_head(p_server_browser->queue, p_server);
}


// Packet parsers (per protocol)
static void gfire_server_browser_proto_parse_packet_q3a(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info)
{
	// Split response in 3 parts (message, server information, players)
	gchar **server_response_parts = g_strsplit(p_buffer, "\n", 0);
	if(g_strv_length(server_response_parts) < 3)
	{
		if(server_response_parts)
			g_strfreev(server_response_parts);

		return;
	}

	gchar **server_response_split = g_strsplit(server_response_parts[1], "\\", 0);
	if(server_response_split)
	{
		int i;
		for (i = 0; i < g_strv_length(server_response_split); i++)
		{
			// Server name
			if(!g_strcmp0("sv_hostname", server_response_split[i]))
				p_server_info->name = g_strdup(server_response_split[i + 1]);

			// Server map
			if(!g_strcmp0("mapname", server_response_split[i]))
				p_server_info->map = g_strdup(server_response_split[i + 1]);

			// Max. players
			if(!g_strcmp0("sv_maxclients", server_response_split[i]))
				p_server_info->max_players = atoi(server_response_split[i + 1]);
		}

		// Count players
		guint32 players = g_strv_length(server_response_parts) - 2;
		p_server_info->players = players;

		g_strfreev(server_response_split);
	}

	g_strfreev(server_response_parts);
}

static void gfire_server_browser_proto_parse_packet_source(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info)
{
	gint index, str_len;

	// Verify packet
	if(p_buffer[4] != 0x49)
		return;

	// Directly skip version for server name
	gchar hostname[256];
	index = 6;

	if((str_len = strlen(&p_buffer[index])) > (sizeof(hostname) - 1))
	{
		if(g_strlcpy(hostname, &p_buffer[index], sizeof(hostname) - 1) == 0)
			return;

		p_buffer[index + (sizeof(hostname) - 1)] = 0;
	}
	else if(p_buffer[index] != 0)
	{
		if(g_strlcpy(hostname, &p_buffer[index], str_len + 1) == 0)
			return;
	}
	else
		hostname[0] = 0;

	index += str_len + 1;

	// Map
	gchar map[256];
	sscanf(&p_buffer[index], "%s", map);

	index += strlen(map) + 1;

	// Game dir
	gchar game_dir[256];
	sscanf(&p_buffer[index], "%s", game_dir);

	index += strlen(game_dir) + 1;

	// Game description
	gchar game_description[256];

	if((str_len = strlen(&p_buffer[index])) > (sizeof(game_description) -1))
	{
		if(g_strlcpy(game_description, &p_buffer[index], sizeof(game_description) -1) == 0)
			return;

		p_buffer[index + (sizeof(game_description) - 1)] = 0;
	}
	else if(p_buffer[index] != 0)
	{
		if(g_strlcpy(game_description, &p_buffer[index], str_len + 1) == 0)
			return;
	}
	else
		game_description[0] = 0;

	index += strlen(game_description) + 1;

	// AppID
	// short appid = *(short *)&buffer[index];

	// Players
	gchar num_players = p_buffer[index + 2];
	gchar max_players = p_buffer[index + 3];

	p_server_info->players = num_players;
	p_server_info->max_players = max_players;
	p_server_info->map = map;
	p_server_info->name = hostname;
}

static void gfire_server_browser_proto_parse_packet_gs(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info)
{
	// UT2K3 support
    // if(!g_strcmp0(gfire_game_server_query_type(server_browser->gameid), "UT2K3"))
        // p_server_info->port = p_server_info->port - 10;

	// Split response & get information
	gchar **server_response_split = g_strsplit(p_buffer, "\\", 0);
	if(server_response_split)
	{
		int i;
		for (i = 0; i < g_strv_length(server_response_split); i++)
		{
			// Server name
			if(!g_strcmp0("hostname", server_response_split[i]))
				p_server_info->name = g_strdup(server_response_split[i + 1]);

			// Server map
			if(!g_strcmp0("mapname", server_response_split[i]))
				p_server_info->map = g_strdup(server_response_split[i + 1]);

			// Players
			if(!g_strcmp0("numplayers", server_response_split[i]))
				p_server_info->players = atoi(server_response_split[i + 1]);

			// Max. players
			if(!g_strcmp0("maxplayers", server_response_split[i]))
				p_server_info->max_players = atoi(server_response_split[i + 1]);
		}

		g_strfreev(server_response_split);
	}
}
#endif // HAVE_GTK
