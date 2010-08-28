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

#ifndef _GF_P2P_NATCHECK_H
#define _GF_P2P_NATCHECK_H

typedef struct _gfire_p2p_natcheck gfire_p2p_natcheck;

#include "gf_base.h"

typedef void (*gfire_p2p_natcheck_callback)(int p_nat_type, guint32 p_ip, guint16 p_port, gpointer p_data);

typedef enum _gfire_p2p_natcheck_state
{
	GF_NATCHECK_NOSTATE = 0,
	GF_NATCHECK_DNS1,
	GF_NATCHECK_DNS2,
	GF_NATCHECK_DNS3,
	GF_NATCHECK_RUNNING,
	GF_NATCHECK_DONE
} gfire_p2p_natcheck_state;

struct _gfire_p2p_natcheck
{
	int socket;
	guint prpl_inpa;

	int stage, server;
	guint timeout, retries;
	gboolean multiple_ports;
	guint32 ips[3];
	guint16 ports[3];

	PurpleDnsQueryData *dnsdata;
	struct sockaddr_in nat_servers[3];

	gfire_p2p_natcheck_state state;

	int type;
	gpointer callback_data;
	gfire_p2p_natcheck_callback callback;
};

// Creation/Freeing
gfire_p2p_natcheck *gfire_p2p_natcheck_create();
void gfire_p2p_natcheck_destroy(gfire_p2p_natcheck *p_nat);

// Starting
gboolean gfire_p2p_natcheck_start(gfire_p2p_natcheck *p_nat, int p_socket, gfire_p2p_natcheck_callback p_callback, gpointer p_data);

// Status
gboolean gfire_p2p_natcheck_isdone(const gfire_p2p_natcheck *p_nat);

#endif // _GF_P2P_NATCHECK_H
