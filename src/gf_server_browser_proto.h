/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009  Laurent De Marez <laurentdemarez@gmail.com>
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

#ifndef _GF_SERVER_BROWSER_PROTO_H
#define _GF_SERVER_BROWSER_PROTO_H

typedef struct _gfire_server_browser_server gfire_server_browser_server;
typedef struct _gfire_server_browser_server_info gfire_server_browser_server_info;

#include "gf_base.h"
#include "gf_server_browser.h"

#ifdef HAVE_GTK
#include <sys/time.h>
#include <fcntl.h>

#include "gf_network.h"
#include "gf_protocol.h"
#include "gfire.h"

typedef struct _gfire_server_browser
{
	// General
	guint32 gameid;
	guint32 max_favs;
	GList *fav_servers;

	// Socket
	int fd;
	guint fd_handler;
	guint timeout_src;

	// Queues
	GQueue *queue;
	GList *queried_list;
} gfire_server_browser;

struct _gfire_server_browser_server
{
	const gchar *protocol;
	guint32 gameid;

	guint32 ip;
	guint16 port;

	gint parent;
	GTimeVal time;
};

struct _gfire_server_browser_server_info
{
	const gchar *protocol;
	guint32 gameid;

	gchar *raw_info;
	gchar *ip_full;

	gchar *name;

	guint32 ip;
	guint16 port;
	guint16 ping;

	guint32 players;
	guint32 max_players;

	gchar *map;
	gchar *game_type;
};

// Creation & freeing
gfire_server_browser_server *gfire_server_browser_server_new();
gfire_server_browser_server_info *gfire_server_browser_server_info_new();

// Server query functions
void gfire_server_browser_proto_free();

// Serverlist requests
void gfire_server_browser_proto_fav_serverlist_request(guint32 p_gameid);
guint16 gfire_server_browser_proto_create_friends_fav_serverlist_request(guint32 p_gameid);
guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid);

// Serverlist handlers
void gfire_server_browser_proto_fav_serverlist(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_server_browser_proto_friends_fav_serverlist(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len);

// Remote fav serverlist functions (requests)
guint16 gfire_server_browser_proto_request_add_fav_server(guint32 p_gameid, guint32 p_ip, guint32 p_port);
guint16 gfire_server_browser_proto_request_remove_fav_server(guint32 p_gameid, guint32 p_ip, guint32 p_port);

// Local fav serverlist functions
void gfire_server_browser_add_fav_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port);
void gfire_server_browser_remove_fav_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port);
gboolean gfire_server_browser_proto_can_add_fav_server();
void gfire_server_browser_proto_add_fav_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port);
void gfire_server_browser_proto_remove_fav_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port);
gboolean gfire_server_browser_proto_is_fav_server(guint32 p_ip, guint16 p_port);

// Misc.
void gfire_server_browser_proto_set_curr_gameid(guint32 p_gameid);
gint gfire_server_brower_proto_get_parent(gfire_server_browser_server_info *p_server);
void gfire_server_browser_proto_push_server(gfire_server_browser_server *p_server);
#endif // HAVE_GTK

#endif // _GF_SERVER_BROWSER_PROTO_H
