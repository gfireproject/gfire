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

#include "gf_ipc_proto.h"
#include "gfire_proto.h"

void gfire_ipc_proto_sdk(gfire_data *p_gfire, gchar *p_data, unsigned short p_len)
{
	if(!p_gfire || !p_data || !p_len)
		return;

	if(!gfire_is_playing(p_gfire))
	{
		purple_debug_misc("gfire", "Not playing; ignoring SDK data\n");
		return;
	}

	unsigned short offset = 0;

	if(p_len < 4)
		return;

	unsigned int numpairs;
	memcpy(&numpairs, p_data + offset, 4);
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
		memcpy(&keylen, p_data + offset, 2);
		offset += 2;

		if(p_len < (offset + 2))
		{
			gfire_list_clear(keys);
			gfire_list_clear(values);
			return;
		}

		keys = g_list_append(keys, g_strndup(p_data + offset, keylen));
		offset += keylen;

		if(p_len < (offset + 2))
		{
			gfire_list_clear(keys);
			gfire_list_clear(values);
			return;
		}

		unsigned short valuelen;
		memcpy(&valuelen, p_data + offset, 2);
		offset += 2;

		if(p_len < (offset + valuelen))
		{
			gfire_list_clear(keys);
			gfire_list_clear(values);
			return;
		}

		values = g_list_append(values, g_strndup(p_data + offset, valuelen));
		offset += valuelen;

		purple_debug_misc("gfire", "\t%s=%s\n", (gchar*)g_list_last(keys)->data, (gchar*)g_list_last(values)->data);
	}

	guint16 packet = gfire_proto_create_game_sdk(keys, values);
	if(packet > 0) gfire_send(gfire_get_connection(p_gfire), packet);

	gfire_list_clear(keys);
	gfire_list_clear(values);
}
