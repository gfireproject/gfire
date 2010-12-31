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

#include "gf_server_query.h"

#ifdef HAVE_GTK

#define GFSQ_GAMESPY2_STAGE_RULES	1
#define GFSQ_GAMESPY2_STAGE_PLAYERS	2

typedef struct _gfire_sq_gamespy2_player
{
} gfire_sq_gamespy2_player;

typedef struct _gfire_sq_gamespy2_data
{
	// Query handling
	guint8 query_stage;

	// Extended data
	GData *info;
	GData *rules;
	GData *player_data;
	GSList *players;
} gfire_sq_gamespy2_data;

// Prototypes
static void gfire_sq_gamespy2_query(gfire_game_server *p_server, gboolean p_full, int p_socket);
static gboolean gfire_sq_gamespy2_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len);
static gchar *gfire_sq_gamespy2_details(gfire_game_server *p_server);
static void gfire_sq_gamespy2_free_server(gfire_game_server *p_server);

gfire_server_query_driver gf_sq_gamespy2_driver =
{
	gfire_sq_gamespy2_query,
	gfire_sq_gamespy2_parse,
	gfire_sq_gamespy2_details,
	gfire_sq_gamespy2_free_server,
	3
};

static void gfire_sq_gamespy2_query(gfire_game_server *p_server, gboolean p_full, int p_socket)
{
	static guint8 query[] = { 0xfe, 0xfd, 0x00, 'C', 'O', 'R', 'Y', 0xff, 0x00, 0x00 };
	if(p_full)
	{
		query[8] = 0xff;
		query[9] = 0xff;
	}
	else
	{
		query[8] = 0x00;
		query[9] = 0x00;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = g_htonl(p_server->ip);
	addr.sin_port = g_htons(p_server->query_port);

	sendto(p_socket, query, 10, 0, (struct sockaddr*)&addr, sizeof(addr));
}

static gboolean gfire_sq_gamespy2_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len)
{
	return FALSE;
}

static gchar *gfire_sq_gamespy2_details(gfire_game_server *p_server)
{
	return NULL;
}

static void gfire_sq_gamespy2_free_server(gfire_game_server *p_server)
{
}

#endif // HAVE_GTK
