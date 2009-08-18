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

gboolean check_process(const gchar *process, const gchar *process_argument);
void gfire_detect_teamspeak_server(guint8 **voip_ip, guint32 *voip_port);
void gfire_server_detection_detect(gfire_data *p_gfire);

#endif // _WIN32

#endif // _GF_SERVER_DETECTION_LINUX_H
