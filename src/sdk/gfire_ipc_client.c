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

#include "gfire_ipc_client.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

gfire_ipc_client *gfire_ipc_client_create()
{
	gfire_ipc_client *ret = malloc(sizeof(gfire_ipc_client));
	if(!ret)
		return NULL;

	memset(ret, 0, sizeof(gfire_ipc_client));

	ret->out_buffer = (char*)malloc(GFIRE_IPC_BUFFER_LEN);
	if(!ret->out_buffer)
	{
		free(ret);
		return NULL;
	}

	ret->socket = -1;

	return ret;
}

int gfire_ipc_client_init(gfire_ipc_client *p_ipc)
{
	if(!p_ipc || p_ipc->socket >= 0)
		return -1;

#ifdef _WIN32
	if(WSAStartup(MAKEWORD(2, 0), &p_ipc->wsaData) != 0)
		return -1;
#endif // _WIN32

	p_ipc->socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(p_ipc->socket < 0)
		return -1;

	p_ipc->addr.sin_family = AF_INET;
	p_ipc->addr.sin_port = htons(GFIRE_IPC_PORT);
	p_ipc->addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	return 0;
}

void gfire_ipc_client_cleanup(gfire_ipc_client *p_ipc)
{
	if(!p_ipc)
		return;

	if(p_ipc->socket >= 0)
	{
#ifdef _WIN32
		closesocket(p_ipc->socket);
#else
		close(p_ipc->socket);
#endif // _WIN32
	}

	free(p_ipc->out_buffer);

#ifdef _WIN32
	WSACleanup();
#endif // _WIN32

	free(p_ipc);
}

int gfire_ipc_client_send(gfire_ipc_client *p_ipc, unsigned short p_id, void *p_data, unsigned int p_len)
{
	if(!p_ipc || p_ipc->socket < 0 || !p_data || p_len == 0)
		return -1;

	// Generate IPC packet
	short length = 4 + p_len;
	memcpy(p_ipc->out_buffer, &length, 2);
	memcpy(p_ipc->out_buffer + 2, &p_id, 2);
	memcpy(p_ipc->out_buffer + 4, p_data, p_len);

	int sent = 0;

	while(sent < p_len)
	{
		int ret = sendto(p_ipc->socket, p_ipc->out_buffer, length, 0, (struct sockaddr*)&p_ipc->addr, sizeof(p_ipc->addr));
		if(ret == -1)
		{
			printf("Failure on sending IPC data!\n");
			return -1;
		}
		else if(ret == 0)
		{
			printf("0 bytes sent!\n");
			return -1;
		}

		sent += ret;
	}

	return sent;
}
