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

#ifdef _WIN32

#include "gf_server_detection.h"
#include <windows.h>
#include <iphlpapi.h>

// We only support TCP as of now because MinGW32 does not offer the necessary functions for UDP yet!

static void get_tcp_connections(gfire_server_detector *p_detector)
{
	PMIB_TCPTABLE_OWNER_PID tcptable = NULL;
	DWORD size = 0;

	DWORD ret = GetExtendedTcpTable(NULL, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_CONNECTIONS, 0);
	if(ret != ERROR_INSUFFICIENT_BUFFER)
		return;

	tcptable = (PMIB_TCPTABLE_OWNER_PID)g_malloc(size);
	ret = GetExtendedTcpTable(tcptable, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_CONNECTIONS, 0);
	if(ret != NO_ERROR)
	{
		g_free(tcptable);
		return;
	}

	int i = 0;
	for(; i < tcptable->dwNumEntries; i++)
	{
		// Only store active connections which belong to the requested PID
		if(tcptable->table[i].dwState == MIB_TCP_STATE_ESTAB && tcptable->table[i].dwOwningPid == p_detector->pid)
		{
			// Store the IPs
			p_detector->ip = ntohl(tcptable->table[i].dwRemoteAddr);
			p_detector->port = ntohs(tcptable->table[i].dwRemotePort);
		}
	}
}

void gfire_server_detector_start(gfire_server_detector *p_detector, guint32 p_gameid, guint32 p_pid)
{
	if(!p_detector || p_gameid == 0 || p_pid == 0)
		return;

	p_detector->gameid = p_gameid;
	p_detector->pid = p_pid;
	p_detector->quit = FALSE;
	p_detector->finished = FALSE;
	p_detector->running = TRUE;

	get_tcp_connections(p_detector);
	// TODO: Check TCP IPs/Ports

	p_detector->finished = TRUE;
	p_detector->running = FALSE;
}

void gfire_server_detector_stop(gfire_server_detector *p_detector)
{
	// Doesn't need to do anything until UDP works
}

#endif // _WIN32
