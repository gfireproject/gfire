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

	// Set max favs
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
#endif // HAVE_GTK
