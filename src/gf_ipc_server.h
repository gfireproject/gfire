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

#ifndef _GF_IPC_SERVER_H
#define _GF_IPC_SERVER_H

#include "gf_base.h"
#include "gfire_ipc.h"
#include "gfire.h"

typedef struct _gfire_ipc_client
{
	struct sockaddr_in addr;
	guint32 pid;
	GTimeVal last_keep_alive;
} gfire_ipc_client;

typedef struct _gfire_ipc_server
{
	// IPC connection
	int socket;
	guint8 *buffer;
	int prpl_inpa;

	// Connected clients
	GList *clients;
	guint keep_alive_to;

	// Registered Gfire instances
	GList *instances;
} gfire_ipc_server;

void gfire_ipc_server_register(gfire_data *p_gfire);
void gfire_ipc_server_unregister(gfire_data *p_gfire);

// Internal
void gfire_ipc_server_client_handshake(guint16 p_version, guint32 p_pid, const struct sockaddr_in *p_addr);

#endif // _GF_IPC_SERVER_H
