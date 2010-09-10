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

#ifndef _GF_SERVER_QUERY_H
#define _GF_SERVER_QUERY_H

#include "gf_base.h"

#ifdef HAVE_GTK

#define GFSQ_MAX_QUERIES 20

typedef struct _gfire_server_query_driver gfire_server_query_driver;

typedef struct _gfire_game_server_data
{
	guint32 players;
	guint32 max_players;

	guint16 ping;

	gchar *name;
	gchar *map;

	// Storage for additional data like player stats, rules etc. (depending on the protocol)
	const gfire_server_query_driver *driver;
	gpointer proto_data;
} gfire_game_server_data;

typedef struct _gfire_game_server
{
	//guint32 game;

	guint32 ip;
	guint16 port;

	gfire_game_server_data *data;
} gfire_game_server;

typedef struct _gfire_game_query_server
{
	gfire_game_server *server;
	gulong timeout;
	gpointer p_data;
} gfire_game_query_server;

typedef void (*gfire_sq_driver_query)(gfire_game_server *p_server, gboolean p_full, int p_socket);
typedef gboolean (*gfire_sq_driver_parse)(gfire_game_server *p_server, guint16 p_ping, gboolean p_full, const unsigned char *p_data, guint p_len);
typedef gchar* (*gfire_sq_driver_server_details)(gfire_game_server *p_server);
typedef void (*gfire_sq_driver_free_server)(gfire_game_server *p_server);

struct _gfire_server_query_driver
{
	gfire_sq_driver_query query;
	gfire_sq_driver_parse parse;
	gfire_sq_driver_server_details details;
	gfire_sq_driver_free_server free_server;
	guint16 timeout;
};

typedef void (*gfire_server_query_callback)(gfire_game_server *p_server, gpointer p_server_data, gpointer p_data);

typedef struct _gfire_server_query
{
	// Socket
	PurpleNetworkListenData *prpl_data;
	int socket;
	guint prpl_inpa;

	// Servers
	gboolean full_query;
	GQueue *servers;
	GList *cur_servers;
	guint timeout;

	// Driver
	const gfire_server_query_driver *driver;

	// Callback
	gfire_server_query_callback callback;
	gpointer callback_data;
} gfire_server_query;

// Creation/freeing
gfire_server_query *gfire_server_query_create();
void gfire_server_query_free(gfire_server_query *p_query);

// Game server management
gchar *gfire_game_server_details(gfire_game_server *p_server);
void gfire_game_server_free(gfire_game_server *p_server);

// Queries
void gfire_server_query_add_server(gfire_server_query *p_query, guint32 p_ip, guint16 p_port, gpointer p_data);
gboolean gfire_server_query_start(gfire_server_query *p_query, const gchar *p_type, gboolean p_full,
								  gfire_server_query_callback p_callback, gpointer p_data);

#endif // HAVE_GTK

#endif // _GF_SERVER_QUERY_H
