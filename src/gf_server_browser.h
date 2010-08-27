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

#ifndef _GF_SERVER_BROWSER_H
#define _GF_SERVER_BROWSER_H

typedef struct _gfire_server_browser gfire_server_browser;
typedef struct _gfire_server_browser_server gfire_server_browser_server;
typedef struct _gfire_server_browser_server_info gfire_server_browser_server_info;

#include "gf_base.h"
#include "gf_server_browser_proto.h"

#ifdef HAVE_GTK
#include "gf_network.h"
#include "gf_protocol.h"
#include "gfire.h"

struct _gfire_server_browser
{
	// GUI-related
	GtkBuilder *builder;
	GtkTreeStore *tree_store;

	GtkTreeIter recent_serverlist_iter;
	GtkTreeIter fav_serverlist_iter;
	GtkTreeIter friends_fav_serverlist_iter;
	GtkTreeIter serverlist_iter;

	// General
	gfire_data *gfire;

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
};

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
void gfire_server_browser_proto_init(gfire_data *p_gfire);
void gfire_server_browser_proto_free(gfire_server_browser *p_server_browser);

// GUI functions (displayal)
void gfire_server_browser_show(PurplePluginAction *p_action);
void gfire_server_browser_add_server(gfire_server_browser *server_browser, gfire_server_browser_server_info *p_server);
#endif // HAVE_GTK

#endif // _GF_SERVER_BROWSER_H
