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

struct _gfire_server_browser_server
{
	gchar *protocol;
	guint32 gameid;

	guint32 ip;
	guint16 port;

	// Used to determine parent row in tree view
	gint parent;
};

typedef struct _gfire_server_browser
{
	guint current_gameid;
	guint32 max_favorites;
	GList *favorite_servers;
} gfire_server_browser;

struct _gfire_server_browser_server_info
{
	gchar *protocol;
	guint32 gameid;

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

// Creation & freeing
gboolean gfire_server_browser_proto_init();
void gfire_server_browser_proto_close();

gfire_server_browser_server_info *gfire_server_browser_server_info_new();
gfire_server_browser_server *gfire_server_browser_server_new();

guint16 gfire_server_browser_proto_create_friends_favorite_serverlist_request(guint32 p_gameid);
guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid);

// Local fav servers handling
void gfire_server_browser_add_favorite_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port);
void gfire_server_browser_remove_favorite_server_local(guint32 p_gameid, guint32 p_ip, guint16 p_port);

// Remote fav servers handling
guint16 gfire_server_browser_proto_request_add_favorite_server(guint32 p_gameid, guint32 p_ip, guint32 p_port);
guint16 gfire_server_browser_proto_request_remove_favorite_server(guint32 p_gameid, guint32 p_ip, guint32 p_port);

void gfire_server_browser_proto_add_favorite_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port);
void gfire_server_browser_proto_remove_favorite_server(gfire_data *p_gfire, guint32 p_gameid, const gchar *p_ip, const gchar *p_port);

// Serverlist handlers
void gfire_server_browser_proto_fav_serverlist_request(guint32 p_gameid);
void gfire_server_browser_proto_favorite_serverlist(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_server_browser_proto_friends_favorite_serverlist(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len);

// Misc.
int gfire_server_browser_proto_get_parent_row();
gboolean gfire_server_browser_proto_is_favorite_server(guint32 p_ip, guint16 p_port);
void gfire_server_browser_proto_push_fav_server(gfire_server_browser_server *p_server);
gboolean gfire_server_browser_can_add_fav_server();
gint gfire_server_brower_proto_get_parent(gfire_server_browser_server_info *p_server);
#endif // HAVE_GTK

#endif // _GF_SERVER_BROWSER_PROTO_H
