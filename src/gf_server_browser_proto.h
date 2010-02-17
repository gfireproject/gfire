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

typedef struct _gfire_server_info gfire_server_info;

#include "gf_base.h"
#include "gf_server_browser.h"

// #ifdef HAVE_GTK
#include "gf_network.h"
#include "gf_protocol.h"
#include "gfire.h"

#define GFIRE_SERVER_BROWSER_BUF 2048
#define GFIRE_SERVER_BROWSER_THREADS_LIMIT 15

static GThreadPool *servers_list_thread_pool;
guint32 servers_list_queried_game_id;

struct _gfire_server_info
{
	GtkTreeIter server_list_iter;
	gchar *query_type;

	gchar *raw_info;
	gchar *ip_full;

	gchar *name;

	guint32 ip;
	guint16 port;
	guint32 ping;

	guint32 players;
	guint32 max_players;

	gchar *map;
	gchar *game_type;
};

typedef struct _gfire_server_browser_server
{
	guint32 gameid;
	guint32 ip;
	guint16 port;
} gfire_server_browser_server;

typedef struct _gfire_server_browser
{
	guint32 max_favorites;
	GList *favorite_servers;
} gfire_server_browser;

guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid);
void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len);
GList *server_browser_proto_get_favorite_servers(const guint32 p_gameid);
guint16 gfire_server_browser_proto_request_add_favorite_server(guint32 p_gameid, guint32 p_ip, guint32 p_port);
gboolean gfire_server_browser_can_add_favorite_server();
gfire_server_info *gfire_server_browser_proto_query_server(gfire_server_info *p_server);
gfire_server_info *gfire_server_info_new();
void gfire_server_browser_add_favorite_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port);
gboolean gfire_server_browser_proto_is_favorite_server(guint32 p_ip, guint16 p_port);
guint16 gfire_server_browser_proto_request_remove_favorite_server(guint32 p_gameid, guint32 p_ip, guint32 p_port);
void gfire_server_browser_add_favorite_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port);
void gfire_server_browser_remove_favorite_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port);
// #endif // HAVE_GTK

#endif // _GF_SERVER_BROWSER_PROTO_H
