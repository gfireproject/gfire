/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
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
#ifndef _GF_GAMES_H
#define _GF_GAMES_H

#include "gfire.h"

typedef struct _gfire_game_launch_info gfire_game_launch_info;

struct _gfire_game_launch_info
{
	int game_id;
	char *game_name;
	char *game_prefix;
	char *game_path;
	char *game_launch;
	char *game_connect;
};

void gfire_xml_download_cb(PurpleUtilFetchUrlData *url_data, gpointer data, const char *buf, gsize len, const gchar *error_message);
gboolean gfire_parse_games_file(PurpleConnection *gc, const char *filename);
char *gfire_game_name(PurpleConnection *gc, int game);
gboolean gfire_parse_launchinfo_file(PurpleConnection *gc, const char *filename);
gboolean gfire_game_playable(PurpleConnection *gc, int game);
int gfire_get_buddy_game(PurpleConnection *gc, PurpleBuddy *b);
int gfire_get_buddy_port(PurpleConnection *gc, PurpleBuddy *b);
const gchar *gfire_get_buddy_ip(PurpleConnection *gc, PurpleBuddy *b);


/**
 * Creates a newly allocated game launch info struct.
 *
 * @return: a newly allocated game launch info struct
**/
gfire_game_launch_info *gfire_game_launch_info_new();


/**
 * Frees a game launch info struct.
 *
 * @param *game_launch_info: the game launch info struct to free
**/
void gfire_game_launch_info_free(gfire_game_launch_info *game_launch_info);


/**
 * Gets the launch info struct of a game.
 *
 * @param game_id: the id of the game
 *
 * @return: the game launch info struct
**/
gfire_game_launch_info *gfire_game_launch_info_get(PurpleConnection *gc, int game_id);


/**
 * Gets the command line to launch a game by parsing the launch info struct.
 *
 * @param game_launch_info_struct: the launch info struct
 * @param server_ip: the server ip
 * @param server_port: the server port
 *
 * @return: a string containing the full command line
**/
gchar *gfire_game_launch_info_get_command(gfire_game_launch_info *game_launch_info, const guint8 *server_ip, int server_port);


gchar *gfire_ipstr_to_bin(const gchar *ip);

#endif /* _GF_GAMES_H */