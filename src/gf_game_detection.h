/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009  Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009       Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009	    Oliver Ney <oliver@dryder.de>
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

#ifndef _GF_GAME_DETECTION_H

#include "gf_base.h"
#include "gfire.h"
#include "gf_server_detection.h"

#define GFIRE_DETECTION_INTERVAL 10 // in seconds
#define GFIRE_WEB_DETECTION_TIMEOUT 6 // in seconds


typedef struct _process_info
{
	guint32 pid;
	gchar *args;
	gchar *exe;
} process_info;

typedef struct _gfire_process_list
{
	GList *processes;
} gfire_process_list;

typedef enum
{
	GFGT_PROCESS = 0,
	GFGT_EXTERNAL,
	GFGT_WEB
} gfire_game_detection_type;

typedef struct
{
	int socket;
	guint input;
} gfire_game_detection_http_connection;

typedef struct _gfire_game_detector
{
	// Process list
	gfire_process_list *process_list;

	// Detected programs
	gfire_game_data game_data;
	gfire_game_data voip_data;
	gfire_game_detection_type game_type;

	// Server detection
	guint8 server_detection_ref;
	GMutex *server_mutex;
	//	Game
	gfire_server_detector *g_server_detector;
	gboolean g_server_changed;
	guint32 g_server_ip;
	guint16 g_server_port;
	//	VoIP
	gfire_server_detector *v_server_detector;
	gboolean v_server_changed;
	guint32 v_server_ip;
	guint16 v_server_port;

	// Webgame detection
	int socket;
	guint accept_input;
	GList *connections;
	guint timeout_check;
	glong web_timeout;

	// Detection timer
	guint det_source;

	// Registered Gfire instances
	GList *instances;
} gfire_game_detector;

// OS independent (gf_game_detection.c) //////////////////////////////////////
// Gfire Game Detector
void gfire_game_detector_register(gfire_data *p_gfire);
void gfire_game_detector_unregister(gfire_data *p_gfire);

void gfire_game_detector_set_external_game(guint32 p_gameid);

gboolean gfire_game_detector_is_playing();
gboolean gfire_game_detector_is_voiping();

guint32 gfire_game_detector_current_game();
guint32 gfire_game_detector_current_voip();

// Gfire Process List
gfire_process_list *gfire_process_list_new();
void gfire_process_list_free(gfire_process_list *p_list);
void gfire_process_list_clear(gfire_process_list *p_list);
guint32 gfire_process_list_contains(const gfire_process_list *p_list, const gchar *p_exe, const GList *p_required_args, const GList *p_invalid_args/*, const GList *p_required_libraries*/);
guint32 gfire_process_list_get_pid(const gfire_process_list *p_list, const gchar *p_exe);
//GList *gfire_game_detection_get_process_libraries(const guint32 p_pid);

// For internal use only
process_info *gfire_process_info_new(const gchar *p_exe, const guint32 p_pid, const gchar *p_args);

// OS dependent (gf_game_detection_X.c) ////////////////////////////////////
void gfire_process_list_update(gfire_process_list *p_list);
//void gfire_game_detection_process_libraries_clear(GList *p_list);

#endif // _GF_GAME_DETECTION_H
