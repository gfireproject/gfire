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

#include "gf_p2p_im_handler.h"
#include "gf_buddies.h"
#include "gf_network.h"

void gfire_p2p_im_handler_handle(gfire_p2p_session *p_session, guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

#ifdef DEBUG
	purple_debug_error("gfire", "handling IM packet\n");
#endif // DEBUG

	guint32 offset = 2;
	guint16 type;
	guint8 *sid = NULL;
	guint8 num_attr = 0;
	guint32 msgtype = 0;
	guint32 imindex = 0;
	gchar *im = NULL;
	guint32 typing = 0;

	memcpy(&type, p_data + offset, 2);
	type = GUINT16_FROM_LE(type);
	offset += 3;

	// No IM packet
	if(type != 0x02)
	{
		purple_debug_error("gfire", "P2P: invalid IM packet (wrong type %u)\n", type);
		return;
	}

	offset = gfire_proto_read_attr_sid_ss(p_data, &sid, "sid", offset);
	if(!sid)
	{
		purple_debug_error("gfire", "P2P: invalid SID\n");
		return;
	}

	offset = gfire_proto_read_attr_children_count_ss(p_data, &num_attr, "peermsg", offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_int32_ss(p_data, &msgtype, "msgtype", offset);
	if(offset == -1)
		return;

	switch(msgtype)
	{
		// Instant message
		case 0:
			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_data, &imindex, "imindex", offset);
			if(offset == -1)
				return;

			// the IM itself ("im")
			offset = gfire_proto_read_attr_string_ss(p_data, &im, "im", offset);
			if(offset == -1 || !im)
				return;

			gfire_buddy_got_im(gfire_p2p_session_get_buddy(p_session), imindex, im);
			break;
		// ACK packet
		case 1:
			// got an ack packet from a previous IM sent
			purple_debug_misc("gfire", "P2P: IM ack packet received.\n");

			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_data, &imindex, "imindex", offset);
			if(offset == -1)
				return;

			gfire_buddy_got_im_ack(gfire_p2p_session_get_buddy(p_session), imindex);
		break;
		// Typing notification
		case 3:
			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_data, &imindex, "imindex", offset);
			if(offset == -1)
				return;

			// typing ("typing")
			offset = gfire_proto_read_attr_int32_ss(p_data, &typing, "typing", offset);
			if(offset == -1)
				return;

			gfire_buddy_got_typing(gfire_p2p_session_get_buddy(p_session), typing == 1);
		break;
		// Unknown
		default:
			purple_debug_warning("gfire", "P2P: unknown IM msgtype %u.\n", msgtype);
	}
}

void gfire_p2p_im_handler_send_im(gfire_p2p_session *p_session, const guint8 *p_sid, guint32 p_imindex, const gchar *p_msg)
{
	if(!p_session || !p_sid || !p_msg)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint32	msgtype = GUINT32_TO_LE(0);
	p_imindex = GUINT32_TO_LE(p_imindex);

	offset = gfire_proto_write_attr_ss("sid", 0x03, p_sid, XFIRE_SID_LEN, offset);
	offset = gfire_proto_write_attr_ss("peermsg", 0x05, NULL, 3, offset);
	offset = gfire_proto_write_attr_ss("msgtype", 0x02, &msgtype, sizeof(msgtype), offset);
	offset = gfire_proto_write_attr_ss("imindex", 0x02, &p_imindex, sizeof(p_imindex), offset);
	offset = gfire_proto_write_attr_ss("im", 0x01, p_msg, strlen(p_msg), offset);

	gfire_proto_write_header(offset, 0x02, 2, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending IM\n");
	gfire_p2p_session_send_data16_packet(p_session, tmp_buf, GUINT32_FROM_LE(offset), "IM");
	g_free(tmp_buf);
}

void gfire_p2p_im_handler_send_ack(gfire_p2p_session *p_session, const guint8 *p_sid, guint32 p_imindex)
{
	if(!p_session || !p_sid)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint32	msgtype = GUINT32_TO_LE(1);
	p_imindex = GUINT32_TO_LE(p_imindex);

	offset = gfire_proto_write_attr_ss("sid", 0x03, p_sid, XFIRE_SID_LEN, offset);
	offset = gfire_proto_write_attr_ss("peermsg", 0x05, NULL, 2, offset);
	offset = gfire_proto_write_attr_ss("msgtype", 0x02, &msgtype, sizeof(msgtype), offset);
	offset = gfire_proto_write_attr_ss("imindex", 0x02, &p_imindex, sizeof(p_imindex), offset);

	gfire_proto_write_header(offset, 0x02, 2, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending IM ack\n");
	gfire_p2p_session_send_data16_packet(p_session, tmp_buf, GUINT32_FROM_LE(offset), "IM");
	g_free(tmp_buf);
}

void gfire_p2p_im_handler_send_typing(gfire_p2p_session *p_session, const guint8 *p_sid, guint32 p_imindex, gboolean p_typing)
{
	if(!p_session || !p_sid)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	// "sid"
	offset = gfire_proto_write_attr_ss("sid", 0x03, p_sid, XFIRE_SID_LEN, offset);

	// "peermsg"
	offset = gfire_proto_write_attr_ss("peermsg", 0x05, NULL, 3, offset);

	// "peermsg"->"msgtype"
	guint32 msgtype = GUINT32_TO_LE(3);
	offset = gfire_proto_write_attr_ss("msgtype", 0x02, &msgtype, sizeof(msgtype), offset);

	// "peermsg"->"imindex"
	p_imindex = GUINT32_TO_LE(p_imindex);
	offset = gfire_proto_write_attr_ss("imindex", 0x02, &p_imindex, sizeof(p_imindex), offset);

	// "peermsg"->"typing"
	guint32 typing = GUINT32_TO_LE(p_typing ? 1 : 0);
	offset = gfire_proto_write_attr_ss("typing", 0x02, &typing, sizeof(typing), offset);

	gfire_proto_write_header(offset, 0x02, 2, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending typing notification\n");
	gfire_p2p_session_send_data16_packet(p_session, tmp_buf, GUINT32_FROM_LE(offset), "IM");
	g_free(tmp_buf);
}
