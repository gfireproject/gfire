/*
 * purple - Xfire Protocol Plugin
 *
 * This file is part of Gfire.
 *
 * See the AUTHORS file distributed with Gfire for a full list of
 * all contributors and this files copyright holders.
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

// GFIRE GAME JOINING
void gfire_join_game(const gfire_game_data *p_game_data);

// GFIRE GAMES XML //////////////////////////////////////////////////
typedef struct _gfire_game_detection_set
{
	// Detection
	GList *required_args;
	GList *invalid_args;

	gboolean external;
	gchar **detect_urls;

	// Server info
	gchar *server_game_name;
	gchar *server_status_type;
	GList *server_broadcast_ports;

	// Launching
	gchar *password_args;
	gchar *network_args;
	gchar *launch_args;
	gchar *launch_url;

	// Server detection
	gboolean detect_server;
	GList *excluded_ports;
} gfire_game_detection_set;

typedef struct _gfire_game
{
	guint32 id;
	gchar *name;
	gchar *short_name;

	gboolean is_voice;

	GList *detection_sets;
} gfire_game;

gboolean gfire_game_load_games_xml();
gboolean gfire_game_have_list();
guint32 gfire_game_get_version();
gchar *gfire_game_get_version_str();
guint32 gfire_game_id(const gchar *p_name);
gchar *gfire_game_name(guint32 p_gameid, gboolean p_html);
gchar *gfire_game_short_name(guint32 p_gameid);
void gfire_update_version_cb(PurpleUtilFetchUrlData *p_url_data, gpointer p_data, const gchar *p_buf, gsize p_len, const gchar *p_error_message);
void gfire_update_games_list_cb(PurpleUtilFetchUrlData *p_url_data, gpointer p_data, const gchar *p_buf, gsize p_len, const gchar *p_error_message);
const gfire_game *gfire_game_by_id(guint32 p_gameid);
guint32 gfire_game_id_by_url(const gchar *p_url);
const gchar *gfire_game_server_query_type(guint32 p_gameid);
GList *gfire_game_excluded_ports_copy(const gfire_game *p_game);
gboolean gfire_game_foreach_dset(const gfire_game *p_game, GCallback p_callback, gpointer p_data, gboolean p_external);
void gfire_game_cleanup();

// GFIRE GAME CONFIG XML ////////////////////////////////////////////
typedef struct _gfire_game_configuration
{
	guint32 game_id;
	gchar *launch_prefix;
	gchar *detect_file;
	gchar *launch_file;
} gfire_game_configuration;

// Loading / Cleanup
gboolean gfire_game_load_config_xml(gboolean p_force);
void gfire_game_config_cleanup();

// Usage
const gfire_game_configuration *gfire_game_config_by_id(guint32 p_gameid);
gboolean gfire_game_config_foreach(GCallback p_callback, gpointer p_data);

gboolean gfire_game_playable(guint32 p_gameid);

// GFIRE GAME MANAGER ///////////////////////////////////////////////
#ifdef HAVE_GTK
void gfire_game_manager_show(PurplePluginAction *p_action);
#endif // HAVE_GTK

#endif // _GF_GAMES_H
