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

#ifdef _WIN32
#	include <io.h>
#	include <share.h>
#	include <sys/stat.h>
#else
#	define _LARGEFILE64_SOURCE
#	include <fcntl.h>
#	include <sys/stat.h>
#endif // _WIN32

#include "gf_filetransfer.h"
#include "gf_p2p_dl_proto.h"

static guint32 gfire_transfer_count = 0;

static void gfire_filetransfer_create_chunks(gfire_filetransfer *p_transfer, guint64 p_offset)
{
	if(!p_transfer || p_transfer->chunks)
		return;

	p_transfer->chunk_count = ((p_transfer->size - p_offset) / p_transfer->chunk_size) +
							  ((((p_transfer->size - p_offset) % p_transfer->chunk_size) > 0) ? 1 : 0);

	p_transfer->chunks = g_malloc0(p_transfer->chunk_count * sizeof(gfire_file_chunk));
	guint32 i = 0;
	for(; i < p_transfer->chunk_count; i++)
	{
		if((p_transfer->size - p_offset) < p_transfer->chunk_size)
			gfire_file_chunk_init(p_transfer->chunks + i, p_transfer, GF_FILE_CHUNK_RECV, p_offset,
								  p_transfer->size % p_transfer->chunk_size);
		else
			gfire_file_chunk_init(p_transfer->chunks + i, p_transfer, GF_FILE_CHUNK_RECV, p_offset,
								  p_transfer->chunk_size);
		p_offset += p_transfer->chunk_size;
	}

	p_transfer->current_chunk = NULL;
}

static void gfire_filetransfer_delete_chunks(gfire_filetransfer *p_transfer)
{
	if(!p_transfer)
		return;

	if(purple_xfer_get_type(p_transfer->xfer) == PURPLE_XFER_SEND && p_transfer->current_chunk)
	{
		gfire_file_chunk_clear(p_transfer->current_chunk);
		g_free(p_transfer->current_chunk);
	}
	else if(purple_xfer_get_type(p_transfer->xfer) == PURPLE_XFER_RECEIVE && p_transfer->chunks)
	{
		guint32 i = 0;
		for(; i < p_transfer->chunk_count; i++)
			gfire_file_chunk_clear(p_transfer->chunks + i);

		g_free(p_transfer->chunks);
	}

	p_transfer->chunks = NULL;
	p_transfer->current_chunk = NULL;
	p_transfer->chunk_count = 0;
	p_transfer->chunks_received = 0;
}

static void gfire_filetransfer_cancel(PurpleXfer *p_xfer)
{
	if(!p_xfer)
		return;

	if(purple_xfer_get_status(p_xfer) == PURPLE_XFER_STATUS_CANCEL_LOCAL)
	{
		gfire_filetransfer *ft = p_xfer->data;
		if(!ft->aborted)
		{
			// Send abortion event
			gfire_p2p_dl_proto_send_file_event(ft->session, ft->fileid, 1, 2);
			gfire_p2p_session_remove_file_transfer(ft->session, ft, TRUE);
		}
	}
}

static void gfire_filetransfer_request_accepted(PurpleXfer *p_xfer)
{
	if(!p_xfer)
		return;

	purple_debug_info("gfire", "P2P: file transfer request accepted\n");

	gfire_filetransfer *ft = p_xfer->data;

#ifdef _WIN32
	if((ft->file = _sopen(purple_xfer_get_local_filename(p_xfer), _O_CREAT | _O_WRONLY | _O_TRUNC | _O_BINARY,
						  _SH_DENYNO, _S_IREAD | _S_IWRITE)) == -1)
#else
	if((ft->file = open64(purple_xfer_get_local_filename(p_xfer), O_CREAT | O_WRONLY | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
#endif // _WIN32
	{
		purple_debug_error("gfire", "gfire_filetransfer_request_accepted: Couldn't open file for writing\n");
		ft->aborted = TRUE;
		gfire_p2p_dl_proto_send_file_request_reply(ft->session, ft->fileid, FALSE);
		purple_xfer_cancel_local(p_xfer);
		gfire_p2p_session_remove_file_transfer(ft->session, ft, TRUE);
		return;
	}

	// Chunk size is set by receiver, so we can precalculate everything
	ft->chunk_size = XFIRE_P2P_FT_CHUNK_SIZE;
	ft->size = purple_xfer_get_size(p_xfer);

	purple_xfer_start(p_xfer, -1, NULL, 0);

	gfire_p2p_dl_proto_send_file_request_reply(ft->session, ft->fileid, TRUE);
	gfire_p2p_dl_proto_send_file_transfer_info(ft->session, ft->fileid, 0, XFIRE_P2P_FT_CHUNK_SIZE,
											   0, gfire_filetransfer_next_msgid(ft));

	// Request first chunk
	gfire_filetransfer_create_chunks(ft, 0);
	ft->current_chunk = ft->chunks;
	gfire_file_chunk_start_transfer(ft->current_chunk);
}

static void gfire_filetransfer_request_denied(PurpleXfer *p_xfer)
{
	if(!p_xfer)
		return;

	purple_debug_info("gfire", "P2P: file transfer request denied\n");

	gfire_filetransfer *ft = p_xfer->data;

	gfire_p2p_dl_proto_send_file_request_reply(ft->session, ft->fileid, FALSE);

	gfire_p2p_session_remove_file_transfer(ft->session, ft, TRUE);
}

static void gfire_filetransfer_start_xfer(PurpleXfer *p_xfer)
{
	// We don't need to do anything here
}

gfire_filetransfer *gfire_filetransfer_create(gfire_p2p_session *p_session, PurpleXfer *p_xfer, guint32 p_fileid)
{
	if(!p_session || !p_xfer)
		return NULL;

	gfire_filetransfer *ret = g_malloc0(sizeof(gfire_filetransfer));
	if(!ret)
	{
		purple_xfer_cancel_local(p_xfer);
		return NULL;
	}

	ret->session = p_session;
	ret->xfer = p_xfer;
	p_xfer->data = ret;

	purple_xfer_set_start_fnc(p_xfer, gfire_filetransfer_start_xfer);

	// Sending
	if(purple_xfer_get_type(p_xfer) == PURPLE_XFER_SEND)
	{
#ifdef _WIN32
		if((ret->file = _sopen(purple_xfer_get_local_filename(p_xfer), _O_RDONLY | _O_BINARY,
							   _SH_DENYNO, _S_IREAD | _S_IWRITE)) == -1)
#else
		if((ret->file = open64(purple_xfer_get_local_filename(p_xfer), O_RDONLY)) == -1)
#endif // _WIN32
		{
			purple_debug_error("gfire", "gfire_filetransfer_init: Couldn't open file for reading\n");
			ret->aborted = TRUE;
			purple_xfer_cancel_local(p_xfer);
			g_free(ret);
			return NULL;
		}

		ret->fileid = XFIRE_P2P_FT_PRIVATE_FILEID_START + ++gfire_transfer_count;
		ret->size = purple_xfer_get_size(p_xfer);
		purple_xfer_set_cancel_send_fnc(p_xfer, gfire_filetransfer_cancel);
	}
	// Receiving
	else
	{
		ret->fileid = p_fileid;

		purple_xfer_set_init_fnc(p_xfer, gfire_filetransfer_request_accepted);
		purple_xfer_set_request_denied_fnc(p_xfer, gfire_filetransfer_request_denied);
		purple_xfer_set_cancel_recv_fnc(p_xfer, gfire_filetransfer_cancel);
	}

	return ret;
}

void gfire_filetransfer_start(gfire_filetransfer *p_transfer)
{
	if(!p_transfer)
		return;

	// Sending
	if(purple_xfer_get_type(p_transfer->xfer) == PURPLE_XFER_SEND)
	{
		GTimeVal gtv;
		g_get_current_time(&gtv);
		gfire_p2p_dl_proto_send_file_request(p_transfer->session, p_transfer->fileid, p_transfer->size,
											 purple_xfer_get_filename(p_transfer->xfer),
											 "", gtv.tv_sec);
	}
	// Receiving
	else
		purple_xfer_request(p_transfer->xfer);
}

void gfire_filetransfer_free(gfire_filetransfer *p_transfer, gboolean p_local_reason)
{
	if(!p_transfer)
		return;

	gfire_filetransfer_delete_chunks(p_transfer);

	if(p_transfer->file >= 0)
#ifdef _WIN32
		_close(p_transfer->file);
#else
		close(p_transfer->file);
#endif // _WIN32

	if(!purple_xfer_is_completed(p_transfer->xfer))
	{
		// Remove incomplete file for now
		if(purple_xfer_get_type(p_transfer->xfer) == PURPLE_XFER_RECEIVE)
			remove(purple_xfer_get_local_filename(p_transfer->xfer));

		if(!purple_xfer_is_canceled(p_transfer->xfer))
		{
			purple_xfer_set_cancel_recv_fnc(p_transfer->xfer, NULL);
			purple_xfer_set_cancel_send_fnc(p_transfer->xfer, NULL);

			if(p_local_reason)
				purple_xfer_cancel_local(p_transfer->xfer);
			else
				purple_xfer_cancel_remote(p_transfer->xfer);
		}
		else
			purple_xfer_unref(p_transfer->xfer);
	}
	else
		purple_xfer_unref(p_transfer->xfer);

	g_free(p_transfer);
}

void gfire_filetransfer_request_reply(gfire_filetransfer *p_transfer, gboolean p_reply)
{
	if(!p_transfer)
		return;

	if(!p_reply)
	{
		purple_debug_info("gfire", "P2P: file request denied\n");
		gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, FALSE);
	}
	else
	{
		purple_debug_info("gfire", "P2P: file request accepted\n");
		purple_xfer_start(p_transfer->xfer, 0, NULL, 0);
	}
}

void gfire_filetransfer_event(gfire_filetransfer *p_transfer, guint32 p_event, guint32 p_type)
{
	if(!p_transfer)
		return;

	if(p_event == 1 && p_type == 2)
	{
		purple_debug_misc("gfire", "P2P: Buddy aborted transfer\n");
		gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, FALSE);
	}
}

void gfire_filetransfer_transfer_info(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_chunk_size,
									  guint32 p_chunk_count, guint32 p_msgid)
{
	if(!p_transfer)
		return;

	if(!p_transfer->current_chunk)
		p_transfer->current_chunk = g_malloc0(sizeof(gfire_file_chunk));

	// Set new current chunk
	gfire_file_chunk_clear(p_transfer->current_chunk);
	gfire_file_chunk_init(p_transfer->current_chunk, p_transfer, GF_FILE_CHUNK_SEND, p_offset, p_chunk_size);
	gfire_file_chunk_send_info(p_transfer->current_chunk, p_msgid);
}

void gfire_filetransfer_chunk_info(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_size,
								   const gchar *p_checksum)
{
	if(!p_transfer || !p_checksum)
		return;

	guint32 i = 0;
	for(; i < p_transfer->chunk_count; i++)
	{
		if(gfire_file_chunk_is(p_transfer->chunks + i, p_offset, p_size))
		{
			gfire_file_chunk_set_checksum(p_transfer->chunks + i, p_checksum);
			return;
		}
	}

	if(i == p_transfer->chunk_count)
		purple_debug_error("gfire", "gfire_filetransfer_chunk_info: unknown chunk!\n");
}

void gfire_filetransfer_data_packet_request(gfire_filetransfer *p_transfer, guint64 p_offset,
											guint32 p_size, guint32 p_msgid)
{
	if(!p_transfer)
		return;

	gfire_file_chunk_send_data(p_transfer->current_chunk, p_offset, p_size, p_msgid);
}

void gfire_filetransfer_data_packet(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_size,
									const GList *p_data)
{
	if(!p_transfer || !p_data)
		return;

	gfire_file_chunk_got_data(p_transfer->current_chunk, p_offset, p_size, p_data);
}

void gfire_filetransfer_complete(gfire_filetransfer *p_transfer)
{
	if(!p_transfer)
		return;

	purple_xfer_set_completed(p_transfer->xfer, TRUE);
	gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, TRUE);
}

void gfire_filetransfer_next_chunk(gfire_filetransfer *p_transfer)
{
	if(!p_transfer || !p_transfer->chunks)
		return;

	// Check if file is complete
	if(p_transfer->current_chunk == p_transfer->chunks + (p_transfer->chunk_count - 1))
	{
		gfire_p2p_dl_proto_send_file_complete(p_transfer->session, p_transfer->fileid);
		purple_xfer_set_completed(p_transfer->xfer, TRUE);
		gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, TRUE);
		return;
	}

	p_transfer->current_chunk++;
	gfire_file_chunk_start_transfer(p_transfer->current_chunk);
}

gfire_p2p_session *gfire_filetransfer_get_session(const gfire_filetransfer *p_transfer)
{
	return p_transfer ? p_transfer->session : NULL;
}

PurpleXfer *gfire_filetransfer_get_xfer(const gfire_filetransfer *p_transfer)
{
	return p_transfer ? p_transfer->xfer : NULL;
}

guint32 gfire_filetransfer_get_fileid(const gfire_filetransfer *p_transfer)
{
	return p_transfer ? p_transfer->fileid : 0;
}

int gfire_filetransfer_get_file(const gfire_filetransfer *p_transfer)
{
	return p_transfer ? p_transfer->file : -1;
}

guint32 gfire_filetransfer_next_msgid(gfire_filetransfer *p_transfer)
{
	return p_transfer ? p_transfer->msgid++ : 0;
}
