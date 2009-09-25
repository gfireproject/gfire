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

#include "gf_p2p_dl_handler.h"
#include "gf_protocol.h"
#include "gf_network.h"

static guint32 gfire_transfer_count = 0;

static void gfire_filetransfer_send_completion_msg(gfire_filetransfer *p_transfer)
{
	if(!p_transfer)
		return;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_transfer->fileid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8D, 4, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending file completion message\n");
	gfire_p2p_session_send_data32_packet(p_transfer->session, tmp_buf, offset, "DL");
	g_free(tmp_buf);
}

static void gfire_filetransfer_send_chunk(gfire_filetransfer *p_transfer, guint64 p_offset, guint8 *p_data, guint16 p_len)
{
	if(!p_transfer)
		return;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_transfer->fileid, 4, offset);

	p_offset = GUINT64_TO_LE(p_offset);
	offset = gfire_proto_write_attr_ss("offset", 0x07, &p_offset, 8, offset);

	guint32 size = GUINT32_TO_LE((guint32)p_len);
	offset = gfire_proto_write_attr_ss("size", 0x02, &size, 4, offset);

	GList *data = NULL;
	guint16 i;
	for(i = 0; i < p_len; i++)
		data = g_list_append(data, p_data + i);

	offset = gfire_proto_write_attr_list_ss("data", data, 0x08, 1, offset);

	g_list_free(data);

	guint32 msgid = GUINT32_TO_LE(p_transfer->msgid);
	p_transfer->msgid++;
	offset = gfire_proto_write_attr_ss("msgid", 0x02, &msgid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8C, 5, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending chunk\n");
	gfire_p2p_session_send_data32_packet(p_transfer->session, tmp_buf, offset, "DL");
	g_free(tmp_buf);
}

static void gfire_filetransfer_send_transfer_info(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_chunk_size, const gchar *p_hash)
{
	if(!p_transfer)
		return;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_transfer->fileid, 4, offset);

	p_offset = GUINT64_TO_LE(p_offset);
	offset = gfire_proto_write_attr_ss("offset", 0x07, &p_offset, 8, offset);

	p_chunk_size = GUINT32_TO_LE(p_chunk_size);
	offset = gfire_proto_write_attr_ss("size", 0x02, &p_chunk_size, 4, offset);

	offset = gfire_proto_write_attr_ss("checksum", 0x01, p_hash, strlen(p_hash), offset);

	guint32 msgid = GUINT32_TO_LE(p_transfer->msgid);
	p_transfer->msgid++;
	offset = gfire_proto_write_attr_ss("msgid", 0x02, &msgid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8A, 5, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending transfer information\n");
	gfire_p2p_session_send_data32_packet(p_transfer->session, tmp_buf, offset, "DL");
	g_free(tmp_buf);
}

static void gfire_filetransfer_send_chunk_request(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_chunk_size)
{
	if(!p_transfer)
		return;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_transfer->fileid, 4, offset);

	p_offset = GUINT64_TO_LE(p_offset);
	offset = gfire_proto_write_attr_ss("offset", 0x07, &p_offset, 8, offset);

	p_chunk_size = GUINT32_TO_LE(p_chunk_size);
	offset = gfire_proto_write_attr_ss("size", 0x02, &p_chunk_size, 4, offset);

	guint32 msgid = GUINT32_TO_LE(p_transfer->msgid);
	p_transfer->msgid++;
	offset = gfire_proto_write_attr_ss("msgid", 0x02, &msgid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8B, 4, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending request for chunk at offset=%u\n", GUINT32_FROM_LE(p_offset));
	gfire_p2p_session_send_data32_packet(p_transfer->session, tmp_buf, offset, "DL");
	g_free(tmp_buf);
}

// For now this function always tells the peer to send the complete file again
static void gfire_filetransfer_send_previous_status(gfire_filetransfer *p_transfer)
{
	if(!p_transfer)
		return;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_transfer->fileid, 4, offset);

	guint64 foffset = 0;
	offset = gfire_proto_write_attr_ss("offset", 0x07, &foffset, 8, offset);

	guint32 chunk_size = GUINT32_TO_LE(XFIRE_P2P_FT_UNKNOWN_CHUNK_SIZE);
	offset = gfire_proto_write_attr_ss("size", 0x02, &chunk_size, 4, offset);

	guint32 chunk_count = 0;
	offset = gfire_proto_write_attr_ss("chunkcnt", 0x02, &chunk_count, 4, offset);

	guint32 msgid = GUINT32_TO_LE(p_transfer->msgid);
	p_transfer->msgid++;
	offset = gfire_proto_write_attr_ss("msgid", 0x02, &msgid, 4, offset);

	gfire_proto_write_header32(offset, 0x3E89, 5, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending information on previous transfer (stub)\n");
	gfire_p2p_session_send_data32_packet(p_transfer->session, tmp_buf, offset, "DL");
	g_free(tmp_buf);
}

static void gfire_filetransfer_send_request_reply(gfire_filetransfer *p_transfer, gboolean p_accepted)
{
	if(!p_transfer)
		return;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_transfer->fileid, 4, offset);

	guint8 reply = p_accepted ? 1 : 0;
	offset = gfire_proto_write_attr_ss("reply", 0x08, &reply, 1, offset);

	gfire_proto_write_header32(offset, 0x3E88, 2, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending file transfer request reply\n");
	gfire_p2p_session_send_data32_packet(p_transfer->session, tmp_buf, offset, "DL");
	g_free(tmp_buf);
}

// For now this function always tells the peer to send the complete file again
static void gfire_filetransfer_send_request(gfire_filetransfer *p_transfer)
{
	if(!p_transfer)
		return;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_transfer->fileid, 4, offset);
	offset = gfire_proto_write_attr_ss("filename", 0x01, purple_xfer_get_filename(p_transfer->xfer), strlen(purple_xfer_get_filename(p_transfer->xfer)), offset);
	offset = gfire_proto_write_attr_ss("desc", 0x01, NULL, 0, offset);

	guint64 size = GUINT64_TO_LE(purple_xfer_get_size(p_transfer->xfer));
	offset = gfire_proto_write_attr_ss("size", 0x07, &size, 8, offset);

	GTimeVal gtv;
	g_get_current_time(&gtv);

	guint32 mtime = GUINT32_TO_LE(gtv.tv_sec);
	offset = gfire_proto_write_attr_ss("mtime", 0x02, &mtime, 4, offset);

	gfire_proto_write_header32(offset, 0x3E87, 5, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending file transfer request\n");
	gfire_p2p_session_send_data32_packet(p_transfer->session, tmp_buf, offset, "DL");
	g_free(tmp_buf);
}

static void gfire_filetransfer_send_event(gfire_filetransfer *p_transfer, guint32 p_event, guint32 p_type)
{
	if(!p_transfer)
		return;

	guint32 offset = GFIRE_P2P_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("fileid", 0x02, &p_transfer->fileid, 4, offset);
	p_event = GUINT32_TO_LE(p_event);
	offset = gfire_proto_write_attr_ss("event", 0x02, &p_event, 4, offset);
	p_type = GUINT32_TO_LE(p_type);
	offset = gfire_proto_write_attr_ss("type", 0x02, &p_type, 4, offset);

	gfire_proto_write_header32(offset, 0x3E8E, 3, 0);

	guint8 *tmp_buf = g_malloc0(offset);
	gfire_network_buffout_copy(tmp_buf, offset);

	purple_debug_misc("gfire", "P2P: Sending file event\n");
	gfire_p2p_session_send_data32_packet(p_transfer->session, tmp_buf, offset, "DL");
	g_free(tmp_buf);
}

static void gfire_filetransfer_generate_chunks(gfire_filetransfer *p_transfer)
{
	if(!p_transfer)
		return;

	if(p_transfer->chunks)
	{
		gfire_list_clear(p_transfer->chunks);
		p_transfer->chunks = NULL;
	}

	guint64 remaining = p_transfer->size;
	while(remaining > 0)
	{
		gfire_file_chunk *new_chunk = g_malloc0(sizeof(gfire_file_chunk));
		new_chunk->offset = p_transfer->size - remaining;

		if(remaining > p_transfer->chunk_size)
			new_chunk->size = p_transfer->chunk_size;
		else
			new_chunk->size = remaining;

		remaining -= new_chunk->size;

		p_transfer->chunks = g_list_append(p_transfer->chunks, new_chunk);
	}

	purple_debug_misc("gfire", "P2P: Generated %u chunks for file \"%s\"\n", g_list_length(p_transfer->chunks), purple_xfer_get_local_filename(p_transfer->xfer));
}

static void gfire_filetransfer_request_chunks(gfire_filetransfer *p_transfer)
{
	if(!p_transfer)
		return;

	GTimeVal gtv;
	g_get_current_time(&gtv);

	if(!p_transfer->chunks)
		gfire_filetransfer_generate_chunks(p_transfer);
	else
	{
		GList *cur = p_transfer->chunks;
		for(; cur; cur = g_list_next(cur))
		{
			gfire_file_chunk *chunk = cur->data;
			if(!chunk->received && chunk->requested && (gtv.tv_sec - chunk->request_time) >= 10)
			{
				chunk->requested = FALSE;
				p_transfer->chunks_requested--;
			}
		}
	}

	if(p_transfer->chunks_requested < GFIRE_P2P_FT_CHUNK_REQUEST_COUNT)
	{
		purple_debug_misc("gfire", "Requesting up to %u chunks\n", GFIRE_P2P_FT_CHUNK_REQUEST_COUNT - p_transfer->chunks_requested);

		GList *cur = p_transfer->chunks;
		for(; cur; cur = g_list_next(cur))
		{
			gfire_file_chunk *chunk = cur->data;
			if(!chunk->received && (!chunk->requested || (chunk->requested && (gtv.tv_sec - chunk->request_time) > 10)))
			{
				chunk->requested = TRUE;
				chunk->request_time = gtv.tv_sec;

				gfire_filetransfer_send_chunk_request(p_transfer, chunk->offset, chunk->size);

				p_transfer->chunks_requested++;
				if(p_transfer->chunks_requested == GFIRE_P2P_FT_CHUNK_REQUEST_COUNT)
					break;
			}
		}
	}
}

static void gfire_filetransfer_request_accepted(PurpleXfer *p_xfer)
{
	if(!p_xfer)
		return;

	purple_debug_info("gfire", "P2P: file transfer request accepted\n");

	gfire_filetransfer *ft = p_xfer->data;

	ft->file = fopen(purple_xfer_get_local_filename(ft->xfer), "w+b");
	if(!ft->file)
	{
		purple_debug_error("gfire", "gfire_filetransfer_request_accepted: Couldn't open file for writing\n");
		gfire_filetransfer_send_request_reply(ft, FALSE);
		purple_xfer_cancel_local(p_xfer);
		return;
	}

	purple_xfer_start(p_xfer, 0, NULL, 0);

	gfire_filetransfer_send_request_reply(ft, TRUE);
	gfire_filetransfer_send_previous_status(ft);

	// Request first chunks
	gfire_filetransfer_request_chunks(ft);
}

static void gfire_filetransfer_request_denied(PurpleXfer *p_xfer)
{
	if(!p_xfer)
		return;

	purple_debug_info("gfire", "P2P: file transfer request denied\n");

	gfire_filetransfer *ft = p_xfer->data;

	gfire_filetransfer_send_request_reply(ft, FALSE);

	gfire_p2p_session_remove_file_transfer(ft->session, ft, TRUE);
}

static void gfire_filetransfer_cancel(PurpleXfer *p_xfer)
{
	if(!p_xfer)
		return;

	if(purple_xfer_get_status(p_xfer) == PURPLE_XFER_STATUS_CANCEL_LOCAL)
	{
		gfire_filetransfer *ft = p_xfer->data;
		// Send abortion event
		gfire_filetransfer_send_event(ft, 1, 2);
		gfire_p2p_session_remove_file_transfer(ft->session, ft, TRUE);
	}
}

static void gfire_filetransfer_start_xfer(PurpleXfer *p_xfer)
{
	// We don't need to do anything here
}

gfire_filetransfer *gfire_filetransfer_init(gfire_p2p_session *p_session, PurpleXfer *p_xfer)
{
	if(!p_session || !p_xfer)
		return NULL;

	gfire_filetransfer *ret = g_malloc0(sizeof(gfire_filetransfer));
	if(!ret)
	{
		purple_xfer_cancel_local(p_xfer);
		return NULL;
	}

	purple_xfer_ref(p_xfer);

	ret->session = p_session;
	ret->xfer = p_xfer;
	p_xfer->data = ret;
	ret->msgid = 1;

	purple_xfer_set_start_fnc(p_xfer, gfire_filetransfer_start_xfer);
	purple_xfer_set_cancel_recv_fnc(p_xfer, gfire_filetransfer_cancel);
	purple_xfer_set_cancel_send_fnc(p_xfer, gfire_filetransfer_cancel);

	if(purple_xfer_get_type(p_xfer) == PURPLE_XFER_SEND)
	{
		ret->file = fopen(purple_xfer_get_local_filename(p_xfer), "rb");
		if(!ret->file)
		{
			purple_debug_error("gfire", "gfire_filetransfer_init: Couldn't open file for reading\n");
			purple_xfer_cancel_local(p_xfer);
			purple_xfer_unref(p_xfer);
			g_free(ret);
			return NULL;
		}

		gfire_transfer_count++;
		ret->fileid = 0x80000000 + gfire_transfer_count;

		ret->chunk_size = XFIRE_P2P_FT_UNKNOWN_CHUNK_SIZE;

		gfire_filetransfer_send_request(ret);

		ret->size = purple_xfer_get_size(p_xfer);

		gfire_filetransfer_generate_chunks(ret);
	}
	else
	{
		purple_xfer_set_init_fnc(p_xfer, gfire_filetransfer_request_accepted);
		purple_xfer_set_request_denied_fnc(p_xfer, gfire_filetransfer_request_denied);

		ret->chunk_size = XFIRE_P2P_FT_DEFAULT_CHUNK_SIZE;

		ret->size = purple_xfer_get_size(p_xfer);

		purple_xfer_request(p_xfer);
	}

	return ret;
}

void gfire_filetransfer_free(gfire_filetransfer *p_transfer, gboolean p_local_reason)
{
	if(!p_transfer)
		return;

	if(!purple_xfer_is_completed(p_transfer->xfer))
	{
		// Remove incomplete file for now
		if(purple_xfer_get_type(p_transfer->xfer) == PURPLE_XFER_RECEIVE)
			remove(purple_xfer_get_local_filename(p_transfer->xfer));

		if(!purple_xfer_is_canceled(p_transfer->xfer))
		{
			if(p_local_reason)
				purple_xfer_cancel_local(p_transfer->xfer);
			else
				purple_xfer_cancel_remote(p_transfer->xfer);
		}
	}

	purple_xfer_unref(p_transfer->xfer);

	gfire_list_clear(p_transfer->chunks);

	g_free(p_transfer->file_checksum);

	if(p_transfer->file)
		fclose(p_transfer->file);

	g_free(p_transfer);
}

static void gfire_p2p_dl_handler_file_info(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

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
		return;
	}

	gchar *msg = g_strdup_printf(_("File Description: %s"), (strlen(desc) > 0) ? desc : _("No description entered"));
	g_free(desc);

	purple_xfer_set_filename(xfer, name);
	purple_xfer_set_message(xfer, msg);
	purple_xfer_set_size(xfer, size);

	g_free(name);
	g_free(msg);

	gfire_filetransfer *ft = gfire_filetransfer_init(p_session, xfer);
	if(!ft)
		return;

	ft->fileid = fileid;

	gfire_p2p_session_add_recv_file_transfer(p_session, ft);
}

static void gfire_p2p_dl_handler_request_reply(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

	guint32 fileid;
	gboolean reply;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_boolean_ss(p_data, &reply, "reply", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received file request reply for unknown file!\n");
		return;
	}

	if(!reply)
	{
		purple_debug_info("gfire", "P2P: file request denied\n");
		gfire_p2p_session_remove_file_transfer(p_session, ft, FALSE);
	}
	else
	{
		purple_debug_info("gfire", "P2P: file request accepted\n");
		purple_xfer_start(ft->xfer, 0, NULL, 0);
	}
}

static void gfire_p2p_dl_handler_previous_status(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

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
		purple_debug_warning("gfire", "P2P: Received previous file status for unknown file!\n");
		return;
	}

	purple_debug_misc("gfire", "P2P: Received info on previous file status\n");

	ft->bytes_transferred = foffset;
	ft->chunk_size = size;

	gfire_filetransfer_generate_chunks(ft);

	purple_xfer_set_bytes_sent(ft->xfer, ft->bytes_transferred);
	purple_xfer_update_progress(ft->xfer);
}

static void gfire_p2p_dl_handler_chunk_request(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

	guint32 fileid, chunk_size, msgid;
	guint64 foffset;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_int64_ss(p_data, &foffset, "offset", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &chunk_size, "size", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &msgid, "msgid", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received file chunk request for unknown file!\n");
		return;
	}

	purple_debug_misc("gfire", "P2P: Received request for chunk at offset=%lu with size of %u bytes\n", foffset, chunk_size);

	GList *cur = ft->chunks;
	while(cur && ((gfire_file_chunk*)cur->data)->offset != foffset)
		cur = g_list_next(cur);

	if(!cur)
	{
		purple_debug_warning("gfire", "Request for unexpected chunk!\n");
		return;
	}

	gfire_file_chunk *chunk = cur->data;

	fseek(ft->file, foffset, SEEK_SET);

	guint8 *buf = g_malloc0(chunk->size);
	fread(buf, 1, chunk->size, ft->file);

	if(!ft->first_sent)
	{
		ft->first_sent = TRUE;

		gchar *sha1hash = g_malloc0(41);
		hashSha1_file_to_str(ft->file, sha1hash);
		gfire_filetransfer_send_transfer_info(ft, 0, XFIRE_P2P_FT_DEFAULT_CHUNK_SIZE, sha1hash);
		g_free(sha1hash);
	}

	gfire_filetransfer_send_chunk(ft, chunk->offset, buf, chunk->size);
	g_free(buf);

	if(!chunk->received)
	{
		ft->bytes_transferred += chunk->size;
		ft->chunks_received++;

		chunk->received = TRUE;

		purple_xfer_set_bytes_sent(ft->xfer, ft->bytes_transferred);
		purple_xfer_update_progress(ft->xfer);
	}
}

static void gfire_p2p_dl_handler_completion_msg(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

	guint32 fileid;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received completion message for unknown file!\n");
		return;
	}

	purple_debug_misc("gfire", "P2P: Received completion message\n");

	purple_xfer_set_completed(ft->xfer, TRUE);
	gfire_p2p_session_remove_file_transfer(p_session, ft, TRUE);
}

static void gfire_p2p_dl_handler_event(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

	guint32 fileid, event, type;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &event, "event", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &type, "type", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received event for unknown file!\n");
		return;
	}

	purple_debug_misc("gfire", "P2P: Received event for file\n");

	if(event == 1 && type == 2)
	{
		purple_debug_misc("gfire", "P2P: Buddy aborted transfer\n");
		gfire_p2p_session_remove_file_transfer(p_session, ft, FALSE);
	}
}

static void gfire_p2p_dl_handler_transfer_info(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

	guint32 fileid, chunk_size, msgid;
	guint64 foffset;
	gchar *checksum;

	guint32 offset = 0;
	offset = gfire_proto_read_attr_int32_ss(p_data, &fileid, "fileid", offset);
	offset = gfire_proto_read_attr_int64_ss(p_data, &foffset, "offset", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &chunk_size, "size", offset);
	offset = gfire_proto_read_attr_string_ss(p_data, &checksum, "checksum", offset);
	offset = gfire_proto_read_attr_int32_ss(p_data, &msgid, "msgid", offset);

	gfire_filetransfer *ft = gfire_p2p_session_find_file_transfer(p_session, fileid);
	if(!ft)
	{
		purple_debug_warning("gfire", "P2P: Received transfer info for unknown file!\n");
		return;
	}

	purple_debug_misc("gfire", "P2P: Received transfer info for file\n");

	ft->file_checksum = checksum;
	purple_debug_misc("gfire", "P2P: Files checksum: %s\n", ft->file_checksum);
}

static void gfire_p2p_dl_handler_file_chunk(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

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
		return;
	}

	purple_debug_misc("gfire", "P2P: received file chunk of %u bytes for offset=%lu\n", chunk_size, foffset);

	GList *cur = ft->chunks;
	while(cur && ((gfire_file_chunk*)cur->data)->offset != foffset)
		cur = g_list_next(cur);

	if(!cur)
	{
		purple_debug_error("gfire", "P2P: Received unrequested chunk!\n");
		return;
	}

	gfire_file_chunk *chunk = cur->data;

	if(!chunk->received)
	{
		guint8 *buf = g_malloc0(chunk_size);
		GList *byte = g_list_first(data);
		guint32 i = 0;
		for(; byte; byte = g_list_next(byte))
		{
			guint8 conv = *((guint*)byte->data);
			memcpy(buf + i, &conv, 1);
			i++;
		}
		gfire_list_clear(data);

		ft->bytes_transferred += chunk_size;
		ft->chunks_received++;
		ft->chunks_requested--;

		fseek(ft->file, foffset, SEEK_SET);
		fwrite(buf, chunk_size, 1, ft->file);
		fflush(ft->file);

		g_free(buf);

		chunk->received = TRUE;

		purple_xfer_set_bytes_sent(ft->xfer, ft->bytes_transferred);
		purple_xfer_update_progress(ft->xfer);
		if(ft->chunks_received == g_list_length(ft->chunks))
		{
			gchar *sha1hash = g_malloc0(41);
			hashSha1_file_to_str(ft->file, sha1hash);

			if(g_utf8_collate(sha1hash, ft->file_checksum) != 0)
				purple_debug_warning("gfire", "P2P: File checksums don't match!\n");
			g_free(sha1hash);

			gfire_filetransfer_send_completion_msg(ft);
			purple_xfer_set_completed(ft->xfer, TRUE);
			gfire_p2p_session_remove_file_transfer(p_session, ft, TRUE);

			return;
		}
	}
	else
		purple_debug_warning("gfire", "P2P: Received duplicate chunk!\n");

	// Request next chunk(s)
	purple_debug_error("gfire", "Going to request new chunks...\n");
	gfire_filetransfer_request_chunks(ft);
}

void gfire_p2p_dl_handler_handle(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len)
{
	if(!p_session || !p_data || !p_len)
		return;

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
		// File information (receiving request)
		case 0x3E87:
			gfire_p2p_dl_handler_file_info(p_session, p_data + offset, p_len);
			break;
		case 0x3E88:
			gfire_p2p_dl_handler_request_reply(p_session, p_data + offset, p_len);
			break;
		case 0x3E89:
			gfire_p2p_dl_handler_previous_status(p_session, p_data + offset, p_len);
			break;
		case 0x3E8A:
			gfire_p2p_dl_handler_transfer_info(p_session, p_data + offset, p_len);
			break;
		case 0x3E8B:
			gfire_p2p_dl_handler_chunk_request(p_session, p_data + offset, p_len);
			break;
		case 0x3E8C:
			gfire_p2p_dl_handler_file_chunk(p_session, p_data + offset, p_len);
			break;
		case 0x3E8D:
			gfire_p2p_dl_handler_completion_msg(p_session, p_data + offset, p_len);
			break;
		case 0x3E8E:
			gfire_p2p_dl_handler_event(p_session, p_data + offset, p_len);
			break;
		default:
			purple_debug_warning("gfire", "P2P: unknown type for DL category (%u)\n", type);
			break;
	}
}
