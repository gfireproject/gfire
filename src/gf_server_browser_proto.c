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

static gfire_server_browser *server_browser;

// Protocol queries
#define Q3A_QUERY "\xFF\xFF\xFF\xFFgetstatus"
#define SOURCE_QUERY "\xFF\xFF\xFF\xFF\x54Source Engine Query"

void gfire_server_browser_proto_parse_packet_q3a();
void gfire_server_browser_proto_parse_packet_source();

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

gfire_server_browser_server *gfire_server_browser_server_new()
{
	gfire_server_browser_server *ret;
	ret = (gfire_server_browser_server *)g_malloc0(sizeof(gfire_server_browser_server));

	return ret;
}

gfire_server_browser_server_info *gfire_server_browser_server_info_new()
{
	gfire_server_browser_server_info *ret;
	ret = (gfire_server_browser_server_info *)g_malloc0(sizeof(gfire_server_browser_server_info));

	return ret;
}

// Server query functions
static void gfire_server_browser_send_packet(guint32 p_ip, guint16 p_port, const gchar p_query[])
{
	if(!p_ip || !p_port || !p_query)
		return;

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(p_ip);
	address.sin_port = htons(p_port);

	// Send query
	sendto(server_browser->fd, p_query, strlen(p_query), 0, (struct sockaddr *)&address, sizeof(address));
}

static void gfire_server_browser_proto_send_query(gfire_server_browser_server *p_server)
{
	// Set start time for calculating ping later
	struct timeval time;
	gettimeofday(&time, NULL);

	p_server->time = time;

	// Determine query type & perform query
	if(!g_strcmp0(p_server->protocol, "WOLFET"))
		gfire_server_browser_send_packet(p_server->ip, p_server->port, Q3A_QUERY);
	else if(!g_strcmp0(p_server->protocol, "SOURCE"))
		gfire_server_browser_send_packet(p_server->ip, p_server->port, SOURCE_QUERY);
}

static void gfire_server_brower_proto_input_cb(gpointer p_unused, PurpleInputCondition p_condition)
{
	const gchar *protocol = gfire_game_server_query_type(server_browser->gameid);

	if(!g_strcmp0(protocol, "WOLFET"))
		gfire_server_browser_proto_parse_packet_q3a();
	else if(!g_strcmp0(protocol, "SOURCE"))
		gfire_server_browser_proto_parse_packet_source();
}

static gboolean gfire_server_browser_proto_timeout_cb(gpointer p_unused)
{
	gint i;
	for(i = 0 ; i < 20; i++)
	{
		gfire_server_browser_server *server = g_queue_pop_head(server_browser->queue);
		if(server)
		{
			// Send server query & push to queried queue
			gfire_server_browser_proto_send_query(server);
			server_browser->queried_list = g_list_append(server_browser->queried_list, server);
		}
	}

	return TRUE;
}

static void gfire_server_browser_proto_init()
{
	// Init socket
	if(server_browser->fd < 0)
		server_browser->fd = socket(AF_INET, SOCK_DGRAM, 0);

	// Query timeout
	if(!server_browser->timeout_src)
		server_browser->timeout_src = g_timeout_add(500, gfire_server_browser_proto_timeout_cb, NULL);

	// Queue
	if(!server_browser->queue)
		server_browser->queue = g_queue_new();

	// Input handler
	if(!server_browser->fd_handler)
		server_browser->fd_handler = purple_input_add(server_browser->fd, PURPLE_INPUT_READ, (PurpleInputFunction )gfire_server_brower_proto_input_cb, NULL);
}

void gfire_server_browser_proto_free()
{
	// Close socket
	if(server_browser->fd >= 0)
		close(server_browser->fd);

	// Remove timeout
	if(server_browser->timeout_src)
		g_source_remove(server_browser->timeout_src);

	// Remove queue
	if(server_browser->queue)
		g_queue_free(server_browser->queue);

	// Remove input handler
	if(server_browser->fd_handler)
		purple_input_remove(server_browser->fd_handler);
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
void gfire_server_browser_proto_fav_serverlist_request(guint32 p_gameid)
{
	// Get fav serverlist (local)
	GList *servers = server_browser->fav_servers;

	// Push servers to queue as structs
	for(; servers; servers = g_list_next(servers))
	{
		// Create struct
		gfire_server_browser_server *server;
		server = servers->data;

		server->protocol = gfire_game_server_query_type(p_gameid);
		server->parent = 1;

		// Push server struct to queue
		g_queue_push_head(server_browser->queue, server);
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

	// Initialize server browser struct & set max favorites
	server_browser = gfire_server_browser_new();
	server_browser->max_favs = max_favs;

	gfire_server_browser_proto_init();

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
		g_queue_push_head(server_browser->queue, server);

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
		g_queue_push_head(server_browser->queue, server);

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
void gfire_server_browser_add_fav_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
	gfire_server_browser_server *server = gfire_server_browser_server_new();

	server->gameid = p_gameid;
	server->ip = p_ip;
	server->port = p_port;

	// Add favorite server to local list
	server_browser->fav_servers = g_list_append(server_browser->fav_servers, server);
}

void gfire_server_browser_remove_fav_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port)
{
	GList *favorite_servers_tmp = server_browser->fav_servers;

	for(; favorite_servers_tmp; favorite_servers_tmp = g_list_next(favorite_servers_tmp))
	{
		gfire_server_browser_server *server;
		server = favorite_servers_tmp->data;

		if(server->gameid == p_gameid && server->ip == p_ip && server->port == p_port)
			server_browser->fav_servers = g_list_remove(server_browser->fav_servers, server);
	}
}

gboolean gfire_server_browser_proto_can_add_fav_server()
{
	if(g_list_length(server_browser->fav_servers) < server_browser->max_favs)
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

gboolean gfire_server_browser_proto_is_fav_server(guint32 p_ip, guint16 p_port)
{
	gboolean ret = FALSE;
	GList *favorite_servers_tmp = server_browser->fav_servers;

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
void gfire_server_browser_proto_set_curr_gameid(guint32 p_gameid)
{
	server_browser->gameid = p_gameid;
}

// FIXME: WTF, when copying the list it never fails and is f*cking fast!
gint gfire_server_brower_proto_get_parent(gfire_server_browser_server_info *p_server)
{
	if(!p_server)
		return -1;

	GList *queried = server_browser->queried_list;
	gint ret = -1;

	for(; queried; queried = g_list_next(queried))
	{
		gfire_server_browser_server *server = (gfire_server_browser_server *)queried->data;

		if((p_server->ip == server->ip) && (p_server->port == server->port))
		{
			ret = server->parent;
			server_browser->queried_list = g_list_remove(server_browser->queried_list, server);

			break;
		}
	}

	// Return -1 if not found at all
	return ret;
}

void gfire_server_browser_proto_push_server(gfire_server_browser_server *p_server)
{
	g_queue_push_head(server_browser->queue, p_server);
}


// Packet parsers (per protocol)
static void gfire_server_browser_proto_calculate_ping(gfire_server_browser_server_info *p_server_info)
{
	if(!p_server_info)
		return;

	GList *queried = server_browser->queried_list;
	gfire_server_browser_server *server;

	struct timeval start, now;

	for(; queried; queried = g_list_next(queried))
	{
		gfire_server_browser_server *server_tmp = (gfire_server_browser_server *)queried->data;

		if((p_server_info->ip == server_tmp->ip) && (p_server_info->port == server_tmp->port))
		{
			start.tv_usec = server->time.tv_usec;
			start.tv_sec = server->time.tv_sec;

			break;
		}
	}

	// Calculate ping & set it
	gettimeofday(&now, NULL);
	p_server_info->ping = (now.tv_usec - start.tv_usec) / 1000;
}

void gfire_server_browser_proto_parse_packet_q3a()
{
	gchar server_response[1024];

	struct sockaddr_in server;
	socklen_t server_len = sizeof(server);

	if(recvfrom(server_browser->fd, server_response, sizeof(server_response), 0, (struct sockaddr *)&server, &server_len) <= 0)
		return;

	// Get IP & port
	gchar *ip = inet_ntoa(server.sin_addr);
	guint16 port = ntohs(server.sin_port);

	// Create server struct
	gfire_server_browser_server_info *server_info = gfire_server_browser_server_info_new();

	// Calculate ping
	gfire_server_browser_proto_calculate_ping(server_info);

	// IP & port
	gfire_game_data game;
	gfire_game_data_ip_from_str(&game, ip);

	server_info->ip = game.ip.value;
	server_info->port = port;

	// Fill raw info
	g_strchomp(server_response);
	server_info->raw_info = g_strdup(server_response);
	server_info->ip_full = g_strdup_printf("%s:%u", ip, port);

	// Split response in 3 parts (message, server information, players)
	gchar **server_response_parts = g_strsplit(server_response, "\n", 0);
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
				server_info->name = g_strdup(server_response_split[i + 1]);

			// Server map
			if(!g_strcmp0("mapname", server_response_split[i]))
				server_info->map = g_strdup(server_response_split[i + 1]);

			// Max. players
			if(!g_strcmp0("sv_maxclients", server_response_split[i]))
				server_info->max_players = atoi(server_response_split[i + 1]);
		}

		// Count players
		guint32 players = g_strv_length(server_response_parts) - 2;
		server_info->players = players;

		g_strfreev(server_response_split);
	}

	g_strfreev(server_response_parts);

	// Add server to tree strore
	gfire_server_browser_add_server(server_info);
}

void gfire_server_browser_proto_parse_packet_source()
{
	gchar buffer[1024];

	struct sockaddr_in server;
	socklen_t server_len;
	server_len = sizeof(server);

	gint n_received;
	gint index, str_len;

	if((n_received = recvfrom(server_browser->fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &server_len)) <= 0)
		return;

	gchar *ip = inet_ntoa(server.sin_addr);
	guint16 port = ntohs(server.sin_port);

	// Verify packet
	if(buffer[4] != 0x49)
		return;

	// Directly skip version for server name
	gchar hostname[256];
	index = 6;

	if((str_len = strlen(&buffer[index])) > (sizeof(hostname) - 1))
	{
		if(g_strlcpy(hostname, &buffer[index], sizeof(hostname) - 1) == 0)
			return;

		buffer[index + (sizeof(hostname) - 1)] = 0;
	}
	else if(buffer[index] != 0)
	{
		if(g_strlcpy(hostname, &buffer[index], str_len + 1) == 0)
			return;
	}
	else
		hostname[0] = 0;

	index += str_len + 1;

	// Map
	gchar map[256];
	sscanf(&buffer[index], "%s", map);

	index += strlen(map) + 1;

	// Game dir
	gchar game_dir[256];
	sscanf(&buffer[index], "%s", game_dir);

	index += strlen(game_dir) + 1;

	// Game description
	gchar game_description[256];

	if((str_len = strlen(&buffer[index])) > (sizeof(game_description) -1))
	{
		if(g_strlcpy(game_description, &buffer[index], sizeof(game_description) -1) == 0)
			return;

		buffer[index + (sizeof(game_description) - 1)] = 0;
	}
	else if(buffer[index] != 0)
	{
		if(g_strlcpy(game_description, &buffer[index], str_len + 1) == 0)
			return;
	}
	else
		game_description[0] = 0;

	index += strlen(game_description) + 1;

	// AppID
	// short appid = *(short *)&buffer[index];

	// Players
	gchar num_players = buffer[index + 2];
	gchar max_players = buffer[index + 3];
	// gchar num_bots = buffer[index + 4];

	/* Other vital information (not needed yet)
	gchar dedicated = buffer[index + 5];
	gchar os = buffer[index + 6];
	gchar password = buffer[index + 7];
	gchar secure = buffer[index + 8]; */

	// Create server info struct
	gfire_server_browser_server_info *server_info = gfire_server_browser_server_info_new();

	gfire_game_data game;
	gfire_game_data_ip_from_str(&game, ip);

	server_info->ip = game.ip.value;
	server_info->port = port;

	server_info->ip_full = g_strdup_printf("%s:%u", ip, port);
	server_info->players = num_players;
	server_info->max_players = max_players;
	server_info->map = map;
	server_info->name = hostname;

	// Add server to tree strore
	gfire_server_browser_add_server(server_info);
}
#endif // HAVE_GTK
