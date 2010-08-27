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

#include "gf_server_browser.h"

#ifdef HAVE_GTK
// Protocol queries
#define GS_QUERY "\x5Cinfo\x5C"
#define Q3A_QUERY "\xFF\xFF\xFF\xFFgetstatus"
#define SOURCE_QUERY "\xFF\xFF\xFF\xFF\x54Source Engine Query"
#define UT2K3_QUERY "\x5Cstatus\x5C\x00"

static void gfire_server_browser_parse_gs(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info);
static void gfire_server_browser_parse_q3a(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info);
static void gfire_server_browser_parse_source(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info);

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

static void gfire_server_browser_send_query(gfire_server_browser *p_server_browser, gfire_server_browser_server *p_server)
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

static void gfire_server_browser_calculate_ping(gfire_server_browser *p_server_browser, gfire_server_browser_server_info *p_server)
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

static void gfire_server_brower_input_cb(gfire_server_browser *p_server_browser, PurpleInputCondition p_condition)
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
	gfire_server_browser_calculate_ping(p_server_browser, server_info);

	// Determine protocol parser
	const gchar *protocol = gfire_game_server_query_type(p_server_browser->gameid);

	if(!g_strcmp0(protocol, "WOLFET"))
		gfire_server_browser_parse_q3a(n_read, buffer, server_info);
	else if(!g_strcmp0(protocol, "SOURCE"))
		gfire_server_browser_parse_source(n_read, buffer, server_info);
	else if(!g_strcmp0(protocol, "GS"))
		gfire_server_browser_parse_gs(n_read, buffer, server_info);
	else if(!g_strcmp0(protocol, "UT2K3"))
		 gfire_server_browser_parse_gs(n_read, buffer, server_info);

	// Add server to tree store
	gfire_server_browser_add_server(p_server_browser, server_info);
}

static gboolean gfire_server_browser_timeout_cb(gfire_server_browser *p_server_browser)
{
	gint i;
	for(i = 0 ; i < 20; i++)
	{
		gfire_server_browser_server *server = g_queue_pop_head(p_server_browser->queue);
		if(server)
		{
			// Send server query & push to queried queue
			gfire_server_browser_send_query(p_server_browser, server);
			p_server_browser->queried_list = g_list_append(p_server_browser->queried_list, server);
		}
	}

	return TRUE;
}

void gfire_server_browser_proto_init(gfire_data *p_gfire)
{
	// Init server browser struct
	p_gfire->server_browser = gfire_server_browser_new();
	p_gfire->server_browser->gfire = p_gfire;

	// Init socket
	if(p_gfire->server_browser->fd < 0)
		p_gfire->server_browser->fd = socket(AF_INET, SOCK_DGRAM, 0);

	// Query timeout
	if(!(p_gfire->server_browser->timeout_src))
		p_gfire->server_browser->timeout_src = g_timeout_add(500, (GSourceFunc )gfire_server_browser_timeout_cb, p_gfire->server_browser);

	// Queue
	if(!(p_gfire->server_browser->queue))
		p_gfire->server_browser->queue = g_queue_new();

	// Input handler
	if(!(p_gfire->server_browser->fd_handler))
		p_gfire->server_browser->fd_handler = purple_input_add(p_gfire->server_browser->fd, PURPLE_INPUT_READ, (PurpleInputFunction )gfire_server_brower_input_cb, p_gfire->server_browser);
}

void gfire_server_browser_proto_free(gfire_server_browser *p_server_browser)
{
	if(!p_server_browser)
		return;

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

// Local fav serverlist functions
static void gfire_server_browser_add_fav_server_local(gfire_data *p_gfire, guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
	gfire_server_browser_server *server = gfire_server_browser_server_new();

	server->gameid = p_gameid;
	server->ip = p_ip;
	server->port = p_port;

	// Add favorite server to local list
	p_gfire->server_browser->fav_servers = g_list_append(p_gfire->server_browser->fav_servers, server);
}

static void gfire_server_browser_remove_fav_server_local(gfire_data *p_gfire, guint32 p_gameid, guint32 p_ip, guint16 p_port)
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

static gboolean gfire_server_browser_can_add_fav_server(gfire_server_browser *p_server_browser)
{
	if(g_list_length(p_server_browser->fav_servers) < p_server_browser->max_favs)
		return TRUE;
	else
		return FALSE;
}

static void gfire_server_browser_add_fav_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port)
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

static void gfire_server_browser_remove_fav_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port)
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

static gboolean gfire_server_browser_is_fav_server(gfire_server_browser *p_server_browser, guint32 p_ip, guint16 p_port)
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
static void gfire_server_browser_set_curr_gameid(gfire_server_browser *p_server_browser, guint32 p_gameid)
{
	p_server_browser->gameid = p_gameid;
}

static gint gfire_server_brower_get_parent(gfire_server_browser *p_server_browser, gfire_server_browser_server_info *p_server)
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

static void gfire_server_browser_push_server(gfire_server_browser *p_server_browser, gfire_server_browser_server *p_server)
{
	g_queue_push_head(p_server_browser->queue, p_server);
}

// Packet parsers (per protocol)
static void gfire_server_browser_parse_q3a(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info)
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

static void gfire_server_browser_parse_source(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info)
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

static void gfire_server_browser_parse_gs(gint p_n_read, gchar p_buffer[1024], gfire_server_browser_server_info *p_server_info)
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

// GUI functions (displayal)
static void gfire_server_browser_add_parent_rows(gfire_server_browser *p_server_browser)
{
	// Clear tree store
	gtk_tree_store_clear(p_server_browser->tree_store);

	// Add main (parent) rows
	gtk_tree_store_append(p_server_browser->tree_store, &(p_server_browser->recent_serverlist_iter), NULL);
	gtk_tree_store_set(p_server_browser->tree_store, &(p_server_browser->recent_serverlist_iter), 0, _("Recent servers"), 1, "", -1);

	gtk_tree_store_append(p_server_browser->tree_store, &(p_server_browser->fav_serverlist_iter), NULL);
	gtk_tree_store_set(p_server_browser->tree_store, &(p_server_browser->fav_serverlist_iter), 0, _("Favorite servers"), -1);

	gtk_tree_store_append(p_server_browser->tree_store, &(p_server_browser->friends_fav_serverlist_iter), NULL);
	gtk_tree_store_set(p_server_browser->tree_store, &(p_server_browser->friends_fav_serverlist_iter), 0, _("Friends' favorite servers"), -1);

	gtk_tree_store_append(p_server_browser->tree_store, &(p_server_browser->serverlist_iter), NULL);
	gtk_tree_store_set(p_server_browser->tree_store, &(p_server_browser->serverlist_iter), 0, _("All servers"), -1);
}

void gfire_server_browser_add_server(gfire_server_browser *p_server_browser, gfire_server_browser_server_info *p_server)
{
	// Get parent row
	gint parent = gfire_server_brower_get_parent(p_server_browser, p_server);
	if(parent == -1)
		return;

	// Add row
	GtkTreeIter iter;

	if(parent == 0)
		gtk_tree_store_append(p_server_browser->tree_store, &iter, &(p_server_browser->recent_serverlist_iter));
	else if(parent == 1)
		gtk_tree_store_append(p_server_browser->tree_store, &iter, &(p_server_browser->fav_serverlist_iter));
	else if(parent == 2)
		gtk_tree_store_append(p_server_browser->tree_store, &iter, &(p_server_browser->friends_fav_serverlist_iter));
	else if(parent == 3)
		gtk_tree_store_append(p_server_browser->tree_store, &iter, &(p_server_browser->serverlist_iter));

	gtk_tree_store_set(p_server_browser->tree_store, &iter, 0, p_server->ip_full, 1, _("N/A"), 2, _("N/A"), 3, _("N/A"), 4, p_server->ip_full, -1);

	// Set row data
	if(p_server)
	{
		if(p_server->name)
		{
			gchar *server_name_clean = g_strstrip(gfire_remove_quake3_color_codes(p_server->name));
			gtk_tree_store_set(p_server_browser->tree_store, &iter, 0, server_name_clean, -1);
			g_free(server_name_clean);
		}
		else
			return;

		if(p_server->ping != -1)
		{
			gchar *server_ping = g_strdup_printf("%d", p_server->ping);
			gtk_tree_store_set(p_server_browser->tree_store, &iter, 1, server_ping, 6, p_server->ping, -1);
			g_free(server_ping);
		}

		if(p_server->players != -1 && p_server->max_players != -1)
		{
			gchar *server_players = g_strdup_printf("%u/%u", p_server->players, p_server->max_players);
			gtk_tree_store_set(p_server_browser->tree_store, &iter, 2, server_players, 7, p_server->players, -1);
			g_free(server_players);
		}

		if(p_server->map)
			gtk_tree_store_set(p_server_browser->tree_store, &iter, 3, p_server->map, -1);

		if(p_server->raw_info)
		{
			gchar *server_raw_info = g_strdup(p_server->raw_info);
			gtk_tree_store_set(p_server_browser->tree_store, &iter, 5, server_raw_info, -1);
			g_free(server_raw_info);
		}
	}
}

static void gfire_server_browser_add_fav_server_cb(gfire_server_browser *p_server_browser, GtkWidget *p_sender)
{
	if(!gfire_server_browser_can_add_fav_server(p_server_browser))
	{
		purple_notify_error(NULL, _("Server Browser: error"), _("Can't add favorite server"),
							_("You've reached the limit of favorite servers, you can however still remove favorite servers in order to add new ones."));

		return;
	}

	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "servers_tree_view"));
	GtkWidget *add_favorite_dialog = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "add_favorite_dialog"));
	GtkWidget *ip_address_entry = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "ip_address_entry"));
	GtkWidget *port_entry = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "port_entry"));

	// Eventually get selected server
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));
	GtkTreeIter iter;

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *selected_server;
		gtk_tree_model_get(model, &iter, 4, &selected_server, -1);

		if(selected_server)
		{
			gchar **server_split;
			server_split = g_strsplit(selected_server, ":", -1);
			g_free(selected_server);

			if(server_split)
			{
				gchar *ip_str = g_strdup(server_split[0]);
				gchar *port_str = g_strdup(server_split[1]);

				gtk_entry_set_text(GTK_ENTRY(ip_address_entry), ip_str);
				gtk_entry_set_text(GTK_ENTRY(port_entry), port_str);

				g_free(ip_str);
				g_free(port_str);

				g_strfreev(server_split);
			}
		}
	}

	gint result = gtk_dialog_run(GTK_DIALOG(add_favorite_dialog));
	if(result == 0)
	{
		// Get user input
		GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "game_combo"));

		const gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
		guint32 gameid = gfire_game_id(game_name);

		const gchar *ip_str = gtk_entry_get_text(GTK_ENTRY(ip_address_entry));
		const gchar *port_str = gtk_entry_get_text(GTK_ENTRY(port_entry));

		// Check given IP
		guint16 ip1, ip2, ip3, ip4;

		if(sscanf(ip_str, "%3hu.%3hu.%3hu.%3hu", &ip1, &ip2, &ip3, &ip4) == 4)
		{
			// Check every IP part
			if((ip1 < 256) || (ip2 < 256) || (ip3 < 256) || (ip4 < 256))
			{
				// Check port
				guint16 port;

				if(sscanf(port_str, "%hu", &port) == 1)
				{
					// Add fav server to user Xfire's profile
					gfire_server_browser_add_fav_server(p_server_browser->gfire, gameid, ip_str, port_str);

					// Get IP in correct datatype
					gfire_game_data game_tmp;
					gfire_game_data_ip_from_str(&game_tmp, ip_str);

					guint32 ip = game_tmp.ip.value;

					// Add to server to local list
					gfire_server_browser_server *server = gfire_server_browser_server_new();
					gfire_server_browser_add_fav_server_local(p_server_browser->gfire, gameid, ip, port);

					// Push fav server to fav queue
					server->protocol = (gchar *)gfire_game_server_query_type(gameid);
					server->ip = ip;
					server->port = port;
					server->parent = 1;

					// Push server struct to queue
					gfire_server_browser_push_server(p_server_browser, server);
				}
			}
		}
	}

	gtk_widget_hide(add_favorite_dialog);
}

static void gfire_server_browser_remove_fav_server_cb(gfire_server_browser *p_server_browser, GtkWidget *p_sender)
{
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "game_combo"));
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "servers_tree_view"));

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	guint32 gameid = gfire_game_id(game_name);
	g_free(game_name);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));
	GtkTreeIter iter;

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *selected_server;
		gtk_tree_model_get(model, &iter, 4, &selected_server, -1);

		if(!selected_server)
			return;

		gchar **server_split;
		server_split = g_strsplit(selected_server, ":", -1);
		g_free(selected_server);

		if(!server_split)
			return;

		gchar *ip_str = g_strdup(server_split[0]);
		gchar *port_str = g_strdup(server_split[1]);

		// Get IP & port in correct datatype
		gfire_game_data game_tmp;
		gfire_game_data_ip_from_str(&game_tmp, ip_str);

		// Put server in struct
		gfire_server_browser_server_info *server_info = gfire_server_browser_server_info_new();

		server_info->ip = game_tmp.ip.value;
		server_info->port =  atoi(port_str);

		g_strfreev(server_split);

		// Check if server is fav server
		if(!gfire_server_browser_is_fav_server(p_server_browser, server_info->ip, server_info->port))
		{
			purple_notify_error(NULL, _("Server Browser: error"), _("Can't remove favorite server"),
								_("The selected server is not a favorite server and thereby can't be removed."));

			return;
		}

		// Remove fav server from user Xfire's profile & local list
		gfire_server_browser_remove_fav_server(p_server_browser->gfire, gameid, ip_str, port_str);
		gfire_server_browser_remove_fav_server_local(p_server_browser->gfire, gameid, server_info->ip, server_info->port);

		// Remove server from tree view
		gtk_tree_store_remove(p_server_browser->tree_store, &iter);

		g_free(ip_str);
		g_free(port_str);
	}
	else
		purple_debug_error("gfire", "Couldn't get selected favorite server to remove.\n");
}

static void gfire_server_browser_request_serverlist_cb(gfire_server_browser *p_server_browser, GtkWidget *p_sender)
{
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "game_combo"));
	GtkTreeStore *tree_store = GTK_TREE_STORE(gtk_builder_get_object(p_server_browser->builder, "servers_list_store"));

	gtk_tree_store_clear(tree_store);

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	guint32 gameid = gfire_game_id(game_name);
	g_free(game_name);

	if(gameid != 0)
	{
		// Set requested gameid
		gfire_server_browser_set_curr_gameid(p_server_browser, gameid);

		// Add parent rows (also clears tree store)
		gfire_server_browser_add_parent_rows(p_server_browser);

		// Show fav serverlist (local)
		gfire_server_browser_proto_fav_serverlist_request(p_server_browser, gameid);

		// Make serverlist requests to Xfire
		guint16 packet_len = 0;

		// Request serverlist
		packet_len = gfire_server_browser_proto_create_serverlist_request(gameid);
		if(packet_len > 0)
			gfire_send(gfire_get_connection(p_server_browser->gfire), packet_len);

		// Request friends' fav serverlist
		packet_len = gfire_server_browser_proto_create_friends_fav_serverlist_request(gameid);
		if(packet_len > 0)
			gfire_send(gfire_get_connection(p_server_browser->gfire), packet_len);
	}
}

static void gfire_server_browser_connect_cb(gfire_server_browser *p_server_browser, GtkWidget *p_sender)
{
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "game_combo"));
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "servers_tree_view"));

	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));

	// Get selected server
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *selected_server;
		gtk_tree_model_get(model, &iter, 4, &selected_server, -1);

		if(!selected_server)
			return;

		gchar **server_split;
		server_split = g_strsplit(selected_server, ":", -1);
		g_free(selected_server);

		if(!server_split)
			return;

		// Put server in struct
		gfire_game_data game;

		game.id = gfire_game_id(game_name);
		gfire_game_data_ip_from_str(&game, server_split[0]);
		game.port = atoi(server_split[1]);

		g_free(game_name);
		g_strfreev(server_split);

		// Finally join server
		gfire_join_game(&game);
	}
	else
	{
		g_free(game_name);
		purple_debug_error("gfire", "Couldn't get selected server to join.\n");
	}
}

static void gfire_server_browser_close(gfire_server_browser *p_server_browser, GtkWidget *p_sender)
{
	if(p_server_browser->builder)
	{
		// Destroy window
		GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(p_server_browser->builder, "server_browser_window"));
		gtk_widget_destroy(server_browser_window);

		// Free builder
		g_object_unref(G_OBJECT(p_server_browser->builder));
		p_server_browser->builder = NULL;
	}
}

static gboolean gfire_server_browser_add_configured_games_cb(const gfire_game_configuration *p_gconf, GtkWidget *p_game_combo)
{
	// Check if the game has a protocol defined
	const gchar *protocol = gfire_game_server_query_type(p_gconf->game_id);
	if(protocol)
	{
		const gfire_game *game = gfire_game_by_id(p_gconf->game_id);
		if(game)
			gtk_combo_box_append_text(GTK_COMBO_BOX(p_game_combo), game->name);
	}

	return FALSE;
}

void gfire_server_browser_show(PurplePluginAction *p_action)
{
	PurpleConnection *gc = (PurpleConnection *)p_action->context;
	gfire_data *gfire = (gfire_data *)gc->proto_data;

	if(!gc || !gfire)
		return;

	// Only one server browser at a time
	if(gfire->server_browser->builder)
		return;

	if(!(gfire->server_browser->builder = gtk_builder_new()))
	{
		purple_debug_error("gfire", "Couldn't build server browser interface.\n");
		return;
	}

	gtk_builder_set_translation_domain(gfire->server_browser->builder, GETTEXT_PACKAGE);

	gchar *builder_file = g_build_filename(DATADIR, "purple", "gfire", "servers.glade", NULL);
	gtk_builder_add_from_file(gfire->server_browser->builder, builder_file, NULL);
	g_free(builder_file);

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(gfire->server_browser->builder, "server_browser_window"));
	GtkWidget *tree_view = GTK_WIDGET(gtk_builder_get_object(gfire->server_browser->builder, "servers_tree_view"));
	GtkWidget *refresh_button = GTK_WIDGET(gtk_builder_get_object(gfire->server_browser->builder, "refresh_button"));
	GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(gfire->server_browser->builder, "connect_button"));
	GtkWidget *add_fav_button = GTK_WIDGET(gtk_builder_get_object(gfire->server_browser->builder, "add_favorite_button"));
	GtkWidget *remove_fav_button = GTK_WIDGET(gtk_builder_get_object(gfire->server_browser->builder, "remove_favorite_button"));
	GtkWidget *quit_button = GTK_WIDGET(gtk_builder_get_object(gfire->server_browser->builder, "quit_button"));
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(gfire->server_browser->builder, "game_combo"));

	// Connect signals
	g_signal_connect_swapped(server_browser_window, "destroy", G_CALLBACK(gfire_server_browser_close), gfire->server_browser);
	g_signal_connect_swapped(quit_button, "clicked", G_CALLBACK(gfire_server_browser_close), gfire->server_browser);

	g_signal_connect_swapped(refresh_button, "clicked", G_CALLBACK(gfire_server_browser_request_serverlist_cb), gfire->server_browser);
	g_signal_connect_swapped(connect_button, "clicked", G_CALLBACK(gfire_server_browser_connect_cb), gfire->server_browser);
	g_signal_connect_swapped(add_fav_button, "clicked", G_CALLBACK(gfire_server_browser_add_fav_server_cb), gfire->server_browser);
	g_signal_connect_swapped(remove_fav_button, "clicked", G_CALLBACK(gfire_server_browser_remove_fav_server_cb), gfire->server_browser);

	// Connect too when user double clicks on a server
	g_signal_connect_swapped(tree_view, "row-activated", G_CALLBACK(gfire_server_browser_connect_cb), gfire->server_browser);

	// Add configured games and initialize tree store
	gfire_game_config_foreach(G_CALLBACK(gfire_server_browser_add_configured_games_cb), game_combo);
	gfire->server_browser->tree_store = GTK_TREE_STORE(gtk_builder_get_object(gfire->server_browser->builder, "servers_list_tree_store"));

	// Show window
	gtk_widget_show_all(server_browser_window);
}
#endif // HAVE_GTK
