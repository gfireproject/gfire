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

#include "gf_preferences_proto.h"
#include "gf_preferences.h"
#include "gf_protocol.h"

guint16 gfire_pref_proto_create_changed_preferences(const GList *p_prefs)
{
	guint32 offset = XFIRE_HEADER_LEN;

	// "prefs"
	offset = gfire_proto_write_attr_ss("prefs", 0x09, NULL, g_list_length((GList*)p_prefs), offset);

	// Write preferences
	while(p_prefs)
	{
		const gf_pref *pref = (gf_pref*)p_prefs->data;
		const gchar *value = pref->set ? "1" : "0";
		offset = gfire_proto_write_attr_bs(pref->id, 0x01, value, 1, offset);
		p_prefs = g_list_next(p_prefs);
	}

	gfire_proto_write_header(offset, 0x0A, 1, 0);
	return offset;
}

void gfire_pref_proto_client_preferences(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire || p_packet_len < 8)
		return;

	if((*(p_gfire->buff_in + 5) != 0x4C) || (*(p_gfire->buff_in + 6) != 0x09))
		return;

	guint8 attributes = *(p_gfire->buff_in + 7);
	guint16 offset = XFIRE_HEADER_LEN + 3;
	guint8 pos = 0;
	for(; pos < attributes; pos++)
	{
		if(p_packet_len < (offset + 4))
			return;

		guint8 id = *(p_gfire->buff_in + offset);
		offset++;
		guint8 type = *(p_gfire->buff_in + offset);
		offset++;

		if(type != 0x01)
			return;

		guint16 len = *(guint16*)(p_gfire->buff_in + offset);
		offset += 2;

		if(p_packet_len < (offset + len))
			return;

		// Store the changed preference
		gfire_preferences_set(p_gfire->prefs, id, *(p_gfire->buff_in + offset) == '1');
		offset += len;
	}

	gfire_got_preferences(p_gfire);
}
