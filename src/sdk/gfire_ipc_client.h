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

#ifndef _GFIRE_IPC_CLIENT_H
#define _GFIRE_IPC_CLIENT_H

#ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
#endif // _WIN32

#include "gfire_ipc.h"

typedef struct _gfire_ipc_client
{
#ifdef _WIN32
	WSADATA wsaData;
#endif // _WIN32

	int socket;
	struct sockaddr_in addr;

	char *out_buffer;
} gfire_ipc_client;

gfire_ipc_client *gfire_ipc_client_create();
int gfire_ipc_client_init(gfire_ipc_client *p_ipc);
void gfire_ipc_client_cleanup(gfire_ipc_client *p_ipc);
int gfire_ipc_client_send(gfire_ipc_client *p_ipc, unsigned short p_id, void *p_data, unsigned int p_len);

#endif // _GFIRE_IPC_CLIENT_H
