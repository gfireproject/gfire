/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
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

#ifndef _GF_SERVER_DETECTION_LINUX_H
#define _GF_SERVER_DETECTION_LINUX_H

#ifndef _WIN32

#include "gf_base.h"
#include "gfire.h"

// Used for server detection
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>

// Used for teamspeak detection
#include <sys/socket.h>
#include <sys/un.h>

typedef struct _gfire_server_detection
{
	guint32 game_pid;
	
	gchar ***netstat_servers;
	gchar ***tcpdump_servers;

	gint detected_netstat_servers;
	gint detected_tcpdump_servers;

	gboolean udp_detected;
	gfire_game_detection_info *game_information;
} gfire_server_detection;

// Creating & freeing
gfire_server_detection *gfire_server_detection_new();
void gfire_server_detection_free(gfire_server_detection *p_gfire_server_detection);
void gfire_server_detection_arrays_clear(gfire_server_detection *p_gfire_server_detection);

// Server detection core
void gfire_server_detection_detect(gfire_server_detection *p_server_detection_infos);
gchar *gfire_server_detection_get(guint32 p_pid, gfire_server_detection *p_gfire_server_detection);
int gfire_server_detection_get_ips(gchar** p_local_ip, gchar** p_remote_ip);
void gfire_server_detection_remove_invalid_ips(gfire_server_detection *p_gfire_server_detection);
gchar *gfire_server_detection_guess_server(gfire_server_detection *p_gfire_server_detection);

// Detection methods
gchar*** gfire_server_detection_netstat(guint32 p_pid, gfire_server_detection *p_gfire_server_detection);
gchar*** gfire_server_detection_tcpdump(gfire_server_detection *p_gfire_server_detection);

#endif // _WIN32

#endif // _GF_SERVER_DETECTION_LINUX_H
