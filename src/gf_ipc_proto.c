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

#include "gfire_ipc.h"
#include "gf_ipc_proto.h"
#include "gfire_proto.h"
#include "gf_game_detection.h"

static void gfire_ipc_proto_header_write(guint16 p_len, guint32 p_pid, guint16 p_type, guint8 *p_data)
{
	memcpy(p_data, &p_len, 2);
	memcpy(p_data + 2, &p_pid, 4);
	memcpy(p_data + 6, &p_type, 2);
}

guint16 gfire_ipc_proto_write_server_handshake(guint32 p_pid, gboolean p_ok, guint8 *p_data)
{
	if(!p_data)
		return 0;

	guint16 offset = GFIRE_IPC_HEADER_LEN;

	// Write version validation
	p_data[offset] = p_ok ? 1 : 0;
	offset++;

	gfire_ipc_proto_header_write(offset, p_pid, GFIRE_IPC_SERVER_HS, p_data);

	return offset;
}

guint16 gfire_ipc_proto_write_shutdown(guint32 p_pid, guint8 *p_data)
{
	if(!p_data)
		return 0;

	gfire_ipc_proto_header_write(4, p_pid, GFIRE_IPC_SHUTDOWN, p_data);
	return GFIRE_IPC_HEADER_LEN;
}

guint16 gfire_ipc_proto_write_keep_alive(guint32 p_pid, guint8 *p_data)
{
	if(!p_data)
		return 0;

	gfire_ipc_proto_header_write(4, p_pid, GFIRE_IPC_KEEP_ALIVE, p_data);
	return GFIRE_IPC_HEADER_LEN;
}

void gfire_ipc_proto_client_handshake(gfire_ipc_server *p_server, guint16 p_len, const struct sockaddr_in *p_addr)
{
	if(!p_server || !p_len || !p_addr)
		return;

	// Get the PID of the header
	guint32 pid;
	memcpy(&pid, p_server->buffer + 2, 4);

	guint16 offset = GFIRE_IPC_HEADER_LEN;

	if(p_len < (GFIRE_IPC_HEADER_LEN + 2))
		return;

	guint16 version;
	memcpy(&version, p_server->buffer + offset, 2);
	offset += 2;

	gfire_ipc_server_client_handshake(version, pid, p_addr);
}

void gfire_ipc_proto_sdk(gfire_ipc_server *p_server, guint16 p_len)
{
	if(!p_server || !p_len)
		return;

	if(!gfire_game_detector_is_playing())
	{
		purple_debug_misc("gfire", "Not playing; ignoring SDK data\n");
		return;
	}

	guint16 offset = GFIRE_IPC_HEADER_LEN;

	if(p_len < (GFIRE_IPC_HEADER_LEN + 4))
		return;

	unsigned int numpairs;
	memcpy(&numpairs, p_server->buffer + offset, 4);
	offset += 4;

	GList *keys = NULL, *values = NULL;

	unsigned int i = 0;
	for(; i < numpairs; i++)
	{
		if(p_len < (offset + 2))
		{
			gfire_list_clear(keys);
			gfire_list_clear(values);
			return;
		}

		unsigned short keylen;
		memcpy(&keylen, p_server->buffer + offset, 2);
		offset += 2;

		if(p_len < (offset + 2))
		{
			gfire_list_clear(keys);
			gfire_list_clear(values);
			return;
		}

		keys = g_list_append(keys, g_strndup((gchar*)p_server->buffer + offset, keylen));
		offset += keylen;

		if(p_len < (offset + 2))
		{
			gfire_list_clear(keys);
			gfire_list_clear(values);
			return;
		}

		unsigned short valuelen;
		memcpy(&valuelen, p_server->buffer + offset, 2);
		offset += 2;

		if(p_len < (offset + valuelen))
		{
			gfire_list_clear(keys);
			gfire_list_clear(values);
			return;
		}

		values = g_list_append(values, g_strndup((gchar*)p_server->buffer + offset, valuelen));
		offset += valuelen;

		purple_debug_misc("gfire", "\t%s=%s\n", (gchar*)g_list_last(keys)->data, (gchar*)g_list_last(values)->data);
	}

	guint16 packet = gfire_proto_create_game_sdk(keys, values);
	GList *instance = p_server->instances;
	while(instance)
	{
		if(packet > 0) gfire_send(gfire_get_connection((gfire_data*)instance->data), packet);
		instance = g_list_next(instance);
	}

	gfire_list_clear(keys);
	gfire_list_clear(values);
}
