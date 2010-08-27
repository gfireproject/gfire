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

#ifndef _GF_SERVER_DETECTION_H
#define _GF_SERVER_DETECTION_H

#include "gf_base.h"

typedef struct _gfire_server_detector gfire_server_detector;
typedef struct _gfire_server gfire_server;

struct _gfire_server_detector
{
	gboolean running;
	gboolean finished;
	gboolean quit;

	guint32 gameid;
	guint32 pid;

	GList *tcp_servers;
	GList *local_udp_connections;
	GList *udp_servers;

	GList *excluded_ports;

	GCallback server_callback;

	GMutex *mutex;

	void *os_data;
};

struct _gfire_server
{
	guint32 ip;
	guint16 port;
	guint8 priority;
};

// Creation/freeing
gfire_server_detector *gfire_server_detector_create(GCallback p_server_callback);
void gfire_server_detector_free(gfire_server_detector *p_detector);

// Starting/stopping (OS dependent)
void gfire_server_detector_start(gfire_server_detector *p_detector, guint32 p_gameid, guint32 p_pid);
void gfire_server_detector_stop(gfire_server_detector *p_detector);

// Server handling
void gfire_server_detection_remove_invalid_servers(gfire_server_detector *p_detection, const GList *p_local_ips);
const gfire_server *gfire_server_detection_guess_server(gfire_server_detector *p_detection);

// Status checks
gboolean gfire_server_detector_running(const gfire_server_detector *p_detector);
gboolean gfire_server_detector_finished(const gfire_server_detector *p_detector);

#endif // _GF_SERVER_DETECTION_H
