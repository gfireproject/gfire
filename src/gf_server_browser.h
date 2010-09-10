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

#include "gf_base.h"

#ifdef HAVE_GTK

#include "gf_server_query.h"

typedef enum _gfire_server_browser_type {
	GFSBT_RECENT = 0,
	GFSBT_FAVOURITE = 1,
	GFSBT_FFAVOURITE = 2,
	GFSBT_GENERAL = 3
} gfire_server_browser_type;

struct _gfire_server_browser
{
	PurpleConnection *gc;

	// Favourite servers
	guint max_fav;
	guint num_fav;
	GData *favourites;

	// Recently visited servers
	GData *recent;

	// Query
	gfire_server_query *query;
	guint32 query_game;

	// GUI
	GtkBuilder *builder;
	GtkTreeStore *tree_store;

	GtkTreeIter recent_iter;
	GtkTreeIter fav_iter;
	GtkTreeIter ffav_iter;
	GtkTreeIter serverlist_iter;
};

// Creation & freeing
gfire_server_browser *gfire_server_browser_create(PurpleConnection *p_gc);
void gfire_server_browser_free(gfire_server_browser *p_browser);

// Recently visited servers
void gfire_server_browser_add_recent(gfire_server_browser *p_browser, guint32 p_gameid, guint32 p_ip,
									 guint16 p_port);

// Favourite servers
gboolean gfire_server_browser_add_favourite(gfire_server_browser *p_browser, guint32 p_gameid, guint32 p_ip,
											guint16 p_port, gboolean p_remote);
void gfire_server_browser_remove_favourite(gfire_server_browser *p_browser, guint32 p_gameid, guint32 p_ip,
										   guint16 p_port);

// Internal
void gfire_server_browser_max_favs(gfire_server_browser *p_browser, guint p_max);
void gfire_server_browser_add_server(gfire_server_browser *p_browser, gfire_server_browser_type p_type,
									 guint32 p_ip, guint16 p_port);

// GUI functions (displayal)
void gfire_server_browser_show(gfire_server_browser *p_browser);
gboolean gfire_server_browser_show_single(guint32 p_gameid, guint32 p_ip, guint16 p_port);

#endif // HAVE_GTK

#endif // _GF_SERVER_BROWSER_H
