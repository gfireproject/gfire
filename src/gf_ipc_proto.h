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

#ifndef _GF_IPC_PROTO_H
#define _GF_IPC_PROTO_H

#include "gf_ipc_server.h"

// Packet creation
guint16 gfire_ipc_proto_write_server_handshake(guint32 p_pid, gboolean p_ok, guint8 *p_data);
guint16 gfire_ipc_proto_write_shutdown(guint32 p_pid, guint8 *p_data);
guint16 gfire_ipc_proto_write_keep_alive(guint32 p_pid, guint8 *p_data);

// Packet parsing
void gfire_ipc_proto_client_handshake(gfire_ipc_server *p_server, guint16 p_len, const struct sockaddr_in *p_addr);
void gfire_ipc_proto_sdk(gfire_ipc_server *p_server, guint16 p_len);

#endif // _GF_IPC_PROTO_H
