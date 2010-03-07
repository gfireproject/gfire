/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
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

#include "gf_p2p_dl_handler.h"
#include "gf_p2p_dl_proto.h"

gboolean gfire_p2p_dl_handler_handle(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

#ifdef DEBUG
	purple_debug_error("gfire", "handling DL packet\n");
#endif // DEBUG

	guint32 offset = 4;
	guint16 type;

	memcpy(&type, p_data + offset, 2);
	type = GUINT16_FROM_LE(type);
	offset += 3;

	switch(type)
	{
		case 0x3E87:
			return gfire_p2p_dl_proto_file_request(p_session, p_data + offset, p_len);
		case 0x3E88:
			return gfire_p2p_dl_proto_file_request_reply(p_session, p_data + offset, p_len);
		case 0x3E89:
			return gfire_p2p_dl_proto_file_transfer_info(p_session, p_data + offset, p_len);
		case 0x3E8A:
			return gfire_p2p_dl_proto_file_chunk_info(p_session, p_data + offset, p_len);
		case 0x3E8B:
			return gfire_p2p_dl_proto_file_data_packet_request(p_session, p_data + offset, p_len);
		case 0x3E8C:
			return gfire_p2p_dl_proto_file_data_packet(p_session, p_data + offset, p_len);
		case 0x3E8D:
			return gfire_p2p_dl_proto_file_completion_msg(p_session, p_data + offset, p_len);
		case 0x3E8E:
			return gfire_p2p_dl_proto_file_event(p_session, p_data + offset, p_len);
		default:
			purple_debug_warning("gfire", "P2P: unknown type for DL category (%u)\n", type);
			return FALSE;
	}
}
