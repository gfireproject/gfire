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

#include "gf_base.h"

// GFIRE GAME DATA //////////////////////////////////////////////////
typedef struct _gfire_game_data
{
	guint32 id;
	guint16 port;
	union _ip_data
	{
		guint32 value;
		guint8 octets[4];
	} ip;
} gfire_game_data;

// Resetting
void gfire_game_data_reset(gfire_game_data *p_game);

// Validity checks
gboolean gfire_game_data_is_valid(const gfire_game_data *p_game);
gboolean gfire_game_data_has_addr(const gfire_game_data *p_game);

// String <-> Address conversions
void gfire_game_data_ip_from_str(gfire_game_data *p_game, const gchar *p_ipstr);
gchar *gfire_game_data_ip_str(const gfire_game_data *p_game);
gchar *gfire_game_data_port_str(const gfire_game_data *p_game);
gchar *gfire_game_data_addr_str(const gfire_game_data *p_game);

// GFIRE GAME LAUNCH INFO ///////////////////////////////////////////
typedef struct _gfire_game_launch_info
{
	guint32 game_id;
	gchar *game_name;
	gchar *game_prefix;
	gchar *game_path;
	gchar *game_launch;
	gchar *game_connect;
} gfire_game_launch_info;

// Creation and freeing
gfire_game_launch_info *gfire_game_launch_info_new();
void gfire_game_launch_info_free(gfire_game_launch_info *p_launch_info);

// Parsing
gfire_game_launch_info *gfire_game_launch_info_get(guint32 p_gameid);
gchar *gfire_game_launch_info_get_command(gfire_game_launch_info *game_launch_info, const gfire_game_data *p_game_data);

// GFIRE GAME DETECTION INFO ////////////////////////////////////////
typedef struct _gfire_game_detection_info
{
	guint32 id;
	gchar *executable;
	gchar *arguments;
	gchar **exclude_ports;
	gboolean detect;
} gfire_game_detection_info;

// Creation and freeing
gfire_game_detection_info *gfire_game_detection_info_create();
void gfire_game_detection_info_free(gfire_game_detection_info *p_info);

// Parsing
gfire_game_detection_info *gfire_game_detection_info_get(guint32 p_gameid);

// GFIRE GAMES XML //////////////////////////////////////////////////
gboolean gfire_game_load_games_xml();
guint32 gfire_game_id(const gchar *p_name);
gchar *gfire_game_name(guint32 p_gameid);
xmlnode *gfire_game_node_first();
xmlnode *gfire_game_node_next(xmlnode *p_node);
void gfire_xml_download_cb(PurpleUtilFetchUrlData *p_url_data, gpointer p_data, const gchar *p_buf, gsize p_len, const gchar *p_error_message);

// GFIRE GAME LAUNCH XML ////////////////////////////////////////////
gboolean gfire_game_load_launch_xml();
gboolean gfire_game_playable(guint32 p_gameid);
xmlnode *gfire_game_launch_node_first();
xmlnode *gfire_game_launch_node_next(xmlnode *p_node);

// GFIRE GAME MANAGER ///////////////////////////////////////////////
void gfire_game_manager_show(PurplePluginAction *p_action);

#endif // _GF_GAMES_H
