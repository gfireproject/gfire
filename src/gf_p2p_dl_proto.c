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

#include "gf_p2p_dl_proto.h"
#include "gf_filetransfer.h"
#include "gf_protocol.h"
#include "gf_network.h"

// For now this function always tells the peer to send the complete file again
guint32 gfire_p2p_dl_proto_send_file_request(gfire_p2p_session *p_session, guint32 p_fileid, guint64 p_size,
											 const gchar *p_name, const gchar *p_desc, guint32 p_mtime)
{
	if(!p_session || !p_name || !p_desc)
		return 0;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	p_fileid = GUINT32_TO_LE(p_fileid);
	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_fileid, 4, offset);
	offset = gfire_proto_write_attr_ss("filename", 0x01, p_name, strlen(p_name), offset);
	offset = gfire_proto_write_attr_ss("desc", 0x01, p_desc, strlen(p_desc), offset);

	guint64 size = GUINT64_TO_LE(p_size);
	offset = gfire_proto_write_attr_ss("size", 0x07, &size, 8, offset);

	p_mtime = GUINT32_TO_LE(p_mtime);
	offset = gfire_proto_write_attr_ss("mtime", 0x02, &p_mtime, 4, offset);

	gfire_proto_write_header32(offset, 0x3E87, 5, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending file transfer request\n");
	gfire_p2p_session_send_data32_packet(p_session, tmp_buf, offset, "DL");
	g_free(tmp_buf);

	return offset;
}

guint32 gfire_p2p_dl_proto_send_file_request_reply(gfire_p2p_session *p_session, guint32 p_fileid, gboolean p_reply)
{
	if(!p_session)
		return 0;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	p_fileid = GUINT32_TO_LE(p_fileid);
	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_fileid, 4, offset);

	guint8 reply = p_reply ? 1 : 0;
	offset = gfire_proto_write_attr_ss("reply", 0x08, &reply, 1, offset);

	gfire_proto_write_header32(offset, 0x3E88, 2, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending file transfer request reply\n");
	gfire_p2p_session_send_data32_packet(p_session, tmp_buf, offset, "DL");
	g_free(tmp_buf);

	return offset;
}

guint32 gfire_p2p_dl_proto_send_file_event(gfire_p2p_session *p_session, guint32 p_fileid,
										   guint32 p_event, guint32 p_type)
{
	if(!p_session)
		return 0;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	p_fileid = GUINT32_TO_LE(p_fileid);
	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_fileid, 4, offset);
	p_event = GUINT32_TO_LE(p_event);
	offset = gfire_proto_write_attr_ss("event", 0x02, &p_event, 4, offset);
	p_type = GUINT32_TO_LE(p_type);
	offset = gfire_proto_write_attr_ss("type", 0x02, &p_type, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8E, 3, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending file event\n");
	gfire_p2p_session_send_data32_packet(p_session, tmp_buf, offset, "DL");
	g_free(tmp_buf);

	return offset;
}

guint32 gfire_p2p_dl_proto_send_file_transfer_info(gfire_p2p_session *p_session, guint32 p_fileid,
												   guint64 p_offset, guint32 p_chunk_size,
												   guint32 p_chunk_count, guint32 p_msgid)
{
	if(!p_session)
		return 0;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	p_fileid = GUINT32_TO_LE(p_fileid);
	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_fileid, 4, offset);

	p_offset = GUINT64_TO_LE(p_offset);
	offset = gfire_proto_write_attr_ss("offset", 0x07, &p_offset, 8, offset);

	p_chunk_size = GUINT32_TO_LE(p_chunk_size);
	offset = gfire_proto_write_attr_ss("size", 0x02, &p_chunk_size, 4, offset);

	p_chunk_count = GUINT32_TO_LE(p_chunk_count);
	offset = gfire_proto_write_attr_ss("chunkcnt", 0x02, &p_chunk_count, 4, offset);

	p_msgid = GUINT32_TO_LE(p_msgid);
	offset = gfire_proto_write_attr_ss("msgid", 0x02, &p_msgid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E89, 5, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: Sending information on previous transfer (stub)\n");
#endif // DEBUG
	gfire_p2p_session_send_data32_packet(p_session, tmp_buf, offset, "DL");
	g_free(tmp_buf);

	return offset;
}

guint32 gfire_p2p_dl_proto_send_file_chunk_info(gfire_p2p_session *p_session, guint32 p_fileid, guint64 p_offset,
										   guint32 p_size, const gchar *p_checksum, guint32 p_msgid)
{
	if(!p_session || !p_checksum)
		return 0;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	p_fileid = GUINT32_TO_LE(p_fileid);
	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_fileid, 4, offset);

	p_offset = GUINT64_TO_LE(p_offset);
	offset = gfire_proto_write_attr_ss("offset", 0x07, &p_offset, 8, offset);

	p_size = GUINT32_TO_LE(p_size);
	offset = gfire_proto_write_attr_ss("size", 0x02, &p_size, 4, offset);

	offset = gfire_proto_write_attr_ss("checksum", 0x01, p_checksum, strlen(p_checksum), offset);

	p_msgid = GUINT32_TO_LE(p_msgid);
	offset = gfire_proto_write_attr_ss("msgid", 0x02, &p_msgid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8A, 5, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: Sending chunk info\n");
#endif // DEBUG
	gfire_p2p_session_send_data32_packet(p_session, tmp_buf, offset, "DL");
	g_free(tmp_buf);

	return offset;
}

guint32 gfire_p2p_dl_proto_send_file_data_packet_request(gfire_p2p_session *p_session, guint32 p_fileid, guint64 p_offset,
														 guint32 p_size, guint32 p_msgid)
{
	if(!p_session)
		return 0;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	p_fileid = GUINT32_TO_LE(p_fileid);
	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_fileid, 4, offset);

	p_offset = GUINT64_TO_LE(p_offset);
	offset = gfire_proto_write_attr_ss("offset", 0x07, &p_offset, 8, offset);

	p_size = GUINT32_TO_LE(p_size);
	offset = gfire_proto_write_attr_ss("size", 0x02, &p_size, 4, offset);

	p_msgid = GUINT32_TO_LE(p_msgid);
	offset = gfire_proto_write_attr_ss("msgid", 0x02, &p_msgid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8B, 4, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: Sending request for data packet at offset=%lu size=%u\n", GUINT64_FROM_LE(p_offset), GUINT32_FROM_LE(p_size));
#endif // DEBUG
	gfire_p2p_session_send_data32_packet(p_session, tmp_buf, offset, "DL");
	g_free(tmp_buf);

	return offset;
}

guint32 gfire_p2p_dl_proto_send_file_data_packet(gfire_p2p_session *p_session, guint32 p_fileid, guint64 p_offset,
												 guint32 p_size, const guint8 *p_data, guint32 p_msgid)
{
	if(!p_session || !p_data)
		return 0;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	p_fileid = GUINT32_TO_LE(p_fileid);
	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_fileid, 4, offset);

	p_offset = GUINT64_TO_LE(p_offset);
	offset = gfire_proto_write_attr_ss("offset", 0x07, &p_offset, 8, offset);

	p_size = GUINT32_TO_LE(p_size);
	offset = gfire_proto_write_attr_ss("size", 0x02, &p_size, 4, offset);

	GList *data = NULL;
	guint32 i;
	for(i = 0; i < p_size; i++)
		data = g_list_append(data, (gpointer)(p_data + i));

	offset = gfire_proto_write_attr_list_ss("data", data, 0x08, 1, offset);

	g_list_free(data);

	p_msgid = GUINT32_TO_LE(p_msgid);
	offset = gfire_proto_write_attr_ss("msgid", 0x02, &p_msgid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8C, 5, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: Sending data packet\n");
#endif // DEBUG
	gfire_p2p_session_send_data32_packet(p_session, tmp_buf, offset, "DL");
	g_free(tmp_buf);

	return offset;
}

guint32 gfire_p2p_dl_proto_send_file_complete(gfire_p2p_session *p_session, guint32 p_fileid)
{
	if(!p_session)
		return 0;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	p_fileid = GUINT32_TO_LE(p_fileid);
	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_fileid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8D, 1, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending file completion message\n");
	gfire_p2p_session_send_data32_packet(p_session, tmp_buf, offset, "DL");
	g_free(tmp_buf);

	return offset;
}

// Receiving
gboolean gfire_p2p_dl_proto_file_request(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

	guint32 fileid, mtime;
	guint64 size;
	gchar *name = NULL, *desc = NULL;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_string_ss(p_data, &name, "filename", offset);
	offset = gfire_proto_read_attr_string_ss(p_data, &desc, "desc", offset);
	offset = gfire_proto_read_attr_int64_ss(p_data, &size, "size", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &mtime, "mtime", offset);

	gchar *size_str = purple_str_size_to_units(size);
	purple_debug_info("gfire", "File request for file \"%s\" of size %s\n", name, size_str);
	g_free(size_str);

	PurpleXfer *xfer = purple_xfer_new(purple_connection_get_account(gfire_p2p_session_get_buddy(p_session)->gc),
									   PURPLE_XFER_RECEIVE,
									   gfire_buddy_get_name(gfire_p2p_session_get_buddy(p_session)));
	if(!xfer)
	{
		purple_debug_warning("gfire", "gfire_p2p_dl_handler_file_info: xfer creation failed\n");
		// This is an internal error, so just deny the request
		gfire_p2p_dl_proto_send_file_request_reply(p_session, fileid, FALSE);
		return TRUE;
	}

	g_strstrip(desc);
	gchar *msg = g_strdup_printf(_("File Description: %s"), (strlen(desc) > 0) ? desc : _("No description entered"));
	g_free(desc);

	purple_xfer_set_filename(xfer, name);
	purple_xfer_set_message(xfer, msg);
	purple_xfer_set_size(xfer, size);

	g_free(name);
	g_free(msg);

	gfire_filetransfer *ft = gfire_filetransfer_create(p_session, xfer, fileid);
	if(!ft)
	{
		// This is an internal error, so just deny the request
		gfire_p2p_dl_proto_send_file_request_reply(p_session, fileid, FALSE);
		return TRUE;
	}
	gfire_filetransfer_start(ft);
	gfire_p2p_session_add_recv_file_transfer(p_session, ft);

	return TRUE;
}

gboolean gfire_p2p_dl_proto_file_request_reply(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

	guint32 fileid;
	gboolean reply;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_boolean_ss(p_data, &reply, "reply", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received file request reply for unknown file!\n");
		return FALSE;
	}

	gfire_filetransfer_request_reply(ft, reply);

	return TRUE;
}

gboolean gfire_p2p_dl_proto_file_event(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

	guint32 fileid, event, type;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &event, "event", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &type, "type", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received event for unknown file!\n");
		return FALSE;
	}

	purple_debug_misc("gfire", "P2P: Received event for file\n");

	gfire_filetransfer_event(ft, event, type);

	return TRUE;
}

gboolean gfire_p2p_dl_proto_file_transfer_info(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

	guint32 fileid, size, chunk_count, msgid;
	guint64 foffset;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_int64_ss(p_data, &foffset, "offset", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &size, "size", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &chunk_count, "chunkcnt", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &msgid, "msgid", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received file transfer info for unknown file!\n");
		return FALSE;
	}

	purple_debug_misc("gfire", "P2P: Received file transfer info: offset=%lu size=%u chunkcnt=%u\n", foffset, size, chunk_count);

	gfire_filetransfer_transfer_info(ft, foffset, size, chunk_count, msgid);

	return TRUE;
}

gboolean gfire_p2p_dl_proto_file_chunk_info(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

	guint32 fileid, chunk_size, msgid;
	guint64 foffset;
	gchar *checksum;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_int64_ss(p_data, &foffset, "offset", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &chunk_size, "size", offset);
	offset = gfire_proto_read_attr_string_ss(p_data, &checksum, "checksum", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &msgid, "msgid", offset);

	// Invalid chunk
	if(chunk_size == 0 || !checksum || strlen(checksum) != 40)
		return FALSE;

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received chunk info for unknown file!\n");
		return FALSE;
	}

#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: Chunk info: Bytes %lu - %lu: %s\n", foffset, foffset + chunk_size, checksum);
#endif // DEBUG

	gfire_filetransfer_chunk_info(ft, foffset, chunk_size, checksum);
	g_free(checksum);

	return TRUE;
}

gboolean gfire_p2p_dl_proto_file_data_packet_request(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

	guint32 fileid, chunk_size, msgid;
	guint64 foffset;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_int64_ss(p_data, &foffset, "offset", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &chunk_size, "size", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &msgid, "msgid", offset);

	// Invalid chunk request
	if(chunk_size == 0)
		return FALSE;

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received file chunk request for unknown file!\n");
		return FALSE;
	}

#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: Received request for chunk at offset=%lu with size of %u bytes\n", foffset, chunk_size);
#endif // DEBUG

	gfire_filetransfer_data_packet_request(ft, foffset, chunk_size, msgid);

	return TRUE;
}

gboolean gfire_p2p_dl_proto_file_data_packet(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

	guint32 fileid, chunk_size, msgid;
	guint64 foffset;
	GList *data = NULL;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_int64_ss(p_data, &foffset, "offset", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &chunk_size, "size", offset);
	offset = gfire_proto_read_attr_list_ss(p_data, &data, "data", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &msgid, "msgid", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received file chunk for unknown file!\n");
		gfire_list_clear(data);
		return FALSE;
	}

#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: received file chunk of %u bytes for offset=%lu\n", chunk_size, foffset);
#endif // DEBUG

	if((foffset + chunk_size) > ft->size)
	{
		purple_debug_error("gfire", "P2P: Received invalid chunk!\n");
		gfire_list_clear(data);
		return TRUE;
	}

	gfire_filetransfer_data_packet(ft, foffset, chunk_size, data);
	gfire_list_clear(data);

	return TRUE;
}

gboolean gfire_p2p_dl_proto_file_completion_msg(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return FALSE;

	guint32 fileid;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received completion message for unknown file!\n");
		return FALSE;
	}

	purple_debug_misc("gfire", "P2P: Received completion message\n");

	gfire_filetransfer_complete(ft);

	return TRUE;
}
