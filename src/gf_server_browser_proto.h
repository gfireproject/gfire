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

#include "gf_base.h"

#ifdef HAVE_GTK

#include "gf_network.h"
#include "gf_protocol.h"
#include "gfire.h"

#define GFIRE_SERVER_BROWSER_BUF 2048
#define GFIRE_SERVER_BROWSER_THREADS_LIMIT 8

static GThreadPool *servers_list_thread_pool;
guint32 servers_list_queried_game_id;

typedef struct _gfire_server_info
{
	GtkTreeIter servers_list_iter;
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
} gfire_server_info;

typedef struct _server_browser_callback_args
{
	gfire_data *gfire;
	GtkBuilder *builder;
} server_browser_callback_args;

void gfire_server_browser_update_server_list_thread(gfire_server_info *server_info);
guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid);
void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len);

#endif // HAVE_GTK

#endif // _GF_SERVER_BROWSER_PROTO_H
