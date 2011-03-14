/*
 * purple - Xfire Protocol Plugin
 *
 * This file is part of Gfire.
 *
 * See the AUTHORS file distributed with Gfire for a full list of
 * all contributors and this files copyright holders.
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

gboolean gfire_p2p_im_handler_handle(gfire_p2p_session *p_session, guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data)
		return FALSE;

	if(p_len < 60)
	{
		purple_debug_warning("gfire", "P2P: too small IM packet\n");
		return FALSE;
	}

#ifdef DEBUG_VERBOSE
	purple_debug_misc("gfire", "handling IM packet\n");
#endif // DEBUG_VERBOSE

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
		return FALSE;
	}

	offset = gfire_proto_read_attr_sid_ss(p_data, &sid, "sid", offset);
	if(!sid)
	{
		purple_debug_error("gfire", "P2P: invalid SID\n");
		return FALSE;
	}

	offset = gfire_proto_read_attr_children_count_ss(p_data, &num_attr, "peermsg", offset);
	if(offset == -1)
		return FALSE;

	offset = gfire_proto_read_attr_int32_ss(p_data, &msgtype, "msgtype", offset);
	if(offset == -1)
		return FALSE;

	switch(msgtype)
	{
		// Instant message
		case 0:
			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_data, &imindex, "imindex", offset);
			if(offset == -1)
				return FALSE;

			// the IM itself ("im")
			offset = gfire_proto_read_attr_string_ss(p_data, &im, "im", offset);
			if(offset == -1 || !im)
				return FALSE;

			gfire_buddy_got_im(gfire_p2p_session_get_buddy(p_session), imindex, im, TRUE);
			break;
		// ACK packet
		case 1:
			// got an ack packet from a previous IM sent
			purple_debug_misc("gfire", "P2P: IM ack packet received.\n");

			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_data, &imindex, "imindex", offset);
			if(offset == -1)
				return FALSE;

			gfire_buddy_got_im_ack(gfire_p2p_session_get_buddy(p_session), imindex);
		break;
		// Typing notification
		case 3:
			// IM index ("imindex")
			offset = gfire_proto_read_attr_int32_ss(p_data, &imindex, "imindex", offset);
			if(offset == -1)
				return FALSE;

			// typing ("typing")
			offset = gfire_proto_read_attr_int32_ss(p_data, &typing, "typing", offset);
			if(offset == -1)
				return FALSE;

			gfire_buddy_got_typing(gfire_p2p_session_get_buddy(p_session), typing == 1);
		break;
		// Unknown
		default:
			purple_debug_warning("gfire", "P2P: unknown IM msgtype %u.\n", msgtype);
			return FALSE;
	}

	return TRUE;
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

	guint32 msgtype = GUINT32_TO_LE(3);
	p_imindex = GUINT32_TO_LE(p_imindex);
	guint32 typing = GUINT32_TO_LE(p_typing ? 1 : 0);

	offset = gfire_proto_write_attr_ss("sid", 0x03, p_sid, XFIRE_SID_LEN, offset);
	offset = gfire_proto_write_attr_ss("peermsg", 0x05, NULL, 3, offset);
	offset = gfire_proto_write_attr_ss("msgtype", 0x02, &msgtype, sizeof(msgtype), offset);
	offset = gfire_proto_write_attr_ss("imindex", 0x02, &p_imindex, sizeof(p_imindex), offset);
	offset = gfire_proto_write_attr_ss("typing", 0x02, &typing, sizeof(typing), offset);

	gfire_proto_write_header(offset, 0x02, 2, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending typing notification\n");
	gfire_p2p_session_send_data16_packet(p_session, tmp_buf, GUINT32_FROM_LE(offset), "IM");
	g_free(tmp_buf);
}
