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

#ifdef _WIN32
#	include <io.h>
#	include <share.h>
#	include <sys/stat.h>
#else
#	include <fcntl.h>
#	include <sys/stat.h>
#endif // _WIN32

#include "gf_filetransfer.h"
#include "gf_p2p_dl_proto.h"

static guint32 gfire_transfer_count = 0;

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

static void gfire_filetransfer_chunk_done(gfire_filetransfer *p_transfer)
{
	// Write the data to disk
	const guint8 *chunk_data = gfire_file_chunk_get_data(p_transfer->chunk);

    lseek(p_transfer->file, gfire_file_chunk_get_offset(p_transfer->chunk), SEEK_SET);
	if(write(p_transfer->file, chunk_data, gfire_file_chunk_get_size(p_transfer->chunk)) < 0)
	{
		purple_xfer_error(PURPLE_XFER_RECEIVE, purple_xfer_get_account(p_transfer->xfer),
						  purple_xfer_get_remote_user(p_transfer->xfer),
						  _("Writing a chunk failed! Make sure you have enough drive space "
							"and appropriate permissions!"));

		// Abort the transfer
		gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, TRUE);
		return;
	}

	// Check if we're done
	if(++p_transfer->current_chunk == p_transfer->chunk_count)
	{
		gfire_p2p_dl_proto_send_file_complete(p_transfer->session, p_transfer->fileid);
		purple_xfer_set_completed(p_transfer->xfer, TRUE);
		gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, TRUE);
		return;
	}

	// Request the next chunk
	guint64 offset = p_transfer->current_chunk * XFIRE_P2P_FT_CHUNK_SIZE;
	guint32 size = (p_transfer->current_chunk == p_transfer->chunk_count - 1) ?
				p_transfer->size % XFIRE_P2P_FT_CHUNK_SIZE : XFIRE_P2P_FT_CHUNK_SIZE;

	gfire_file_chunk_init(p_transfer->chunk, offset, size);
	gfire_file_chunk_start_transfer(p_transfer->chunk);
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
	if((ft->file = open(purple_xfer_get_local_filename(p_xfer), O_CREAT | O_WRONLY | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
#endif // _WIN32
	{
		purple_debug_error("gfire", "gfire_filetransfer_request_accepted: Couldn't open file for writing\n");
		ft->aborted = TRUE;
		gfire_p2p_dl_proto_send_file_request_reply(ft->session, ft->fileid, FALSE);
		purple_xfer_cancel_local(p_xfer);
		gfire_p2p_session_remove_file_transfer(ft->session, ft, TRUE);
		return;
	}

	// Grow file to the full size
#ifdef _WIN32
	if(_chsize(ft->file, ft->size) != 0)
#else
    if(ftruncate(ft->file, ft->size) != 0)
#endif // _WIN32
		purple_debug_warning("gfire", "P2P: setting the files size failed\n");

	ft->size = purple_xfer_get_size(p_xfer);
	ft->current_chunk = 0;
	ft->chunk_count = ft->size / XFIRE_P2P_FT_CHUNK_SIZE;
	if((ft->size % XFIRE_P2P_FT_CHUNK_SIZE) != 0)
		ft->chunk_count++;

	purple_xfer_start(p_xfer, -1, NULL, 0);

	gfire_p2p_dl_proto_send_file_request_reply(ft->session, ft->fileid, TRUE);

	// Request first chunk
	ft->chunk = gfire_file_chunk_create(ft->session, ft->fileid, ft->msgid, p_xfer,
										(gfire_ft_callback)gfire_filetransfer_chunk_done, NULL, ft);

	gfire_file_chunk_init(ft->chunk, 0, (ft->chunk_count == 1) ? ft->size : XFIRE_P2P_FT_CHUNK_SIZE);
	gfire_file_chunk_start_transfer(ft->chunk);
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

	gfire_filetransfer *ret = g_new0(gfire_filetransfer, 1);
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
		if((ret->file = open(purple_xfer_get_local_filename(p_xfer), O_RDONLY)) == -1)
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

	if(p_transfer->chunk)
		gfire_file_chunk_free(p_transfer->chunk);

	if(p_transfer->file >= 0)
#ifdef _WIN32
		_close(p_transfer->file);
#else
		close(p_transfer->file);
#endif // _WIN32

	if(!purple_xfer_is_completed(p_transfer->xfer))
	{
		if(!purple_xfer_is_canceled(p_transfer->xfer))
		{
			purple_xfer_set_cancel_recv_fnc(p_transfer->xfer, NULL);
			purple_xfer_set_cancel_send_fnc(p_transfer->xfer, NULL);

			if(p_local_reason)
				purple_xfer_cancel_local(p_transfer->xfer);
			else
				purple_xfer_cancel_remote(p_transfer->xfer);
		}

		// Remove incomplete file for now
		if(purple_xfer_get_type(p_transfer->xfer) == PURPLE_XFER_RECEIVE)
			remove(purple_xfer_get_local_filename(p_transfer->xfer));
	}
	else
		purple_xfer_end(p_transfer->xfer);

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

void gfire_filetransfer_chunk_info_request(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_chunk_size,
									  guint32 p_chunk_count, guint32 p_msgid)
{
	if(!p_transfer || purple_xfer_get_type(p_transfer->xfer) != PURPLE_XFER_SEND ||
			p_offset >= p_transfer->size || p_chunk_size > 0x01E00000 /* 30 MB */)
		return;

	guint8 *chunk = g_malloc(p_chunk_size);
    lseek(p_transfer->file, p_offset, SEEK_SET);
	int size = read(p_transfer->file, chunk, p_chunk_size);
	if(size <= 0)
	{
		g_free(chunk);

		purple_xfer_error(PURPLE_XFER_SEND, purple_xfer_get_account(p_transfer->xfer),
						  purple_xfer_get_remote_user(p_transfer->xfer),
						  _("Reading a chunk failed! Make sure you have appropriate permissions!"));

		// Abort the transfer
		gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, TRUE);
		return;
	}

	gchar hash[41];
	hashSha1_bin_to_str(chunk, size, hash);
	hash[40] = 0;

	g_free(chunk);

	gfire_p2p_dl_proto_send_file_chunk_info(p_transfer->session, p_transfer->fileid, p_offset, size, hash,
											p_msgid);
}

void gfire_filetransfer_chunk_info(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_size,
								   const gchar *p_checksum)
{
	if(!p_transfer || !p_checksum || !p_transfer->chunk)
		return;

	if(p_offset == gfire_file_chunk_get_offset(p_transfer->chunk))
		gfire_file_chunk_set_checksum(p_transfer->chunk, p_checksum);
	else
		purple_debug_warning("gfire", "P2P: Got chunk information for unknown chunk!\n");
}

void gfire_filetransfer_data_packet_request(gfire_filetransfer *p_transfer, guint64 p_offset,
											guint32 p_size, guint32 p_msgid)
{
	if(!p_transfer || purple_xfer_get_type(p_transfer->xfer) != PURPLE_XFER_SEND ||
			p_offset >= p_transfer->size || p_size > 0x01E00000 /* 30 MB */)
		return;

	guint8 *data = g_malloc(p_size);
    lseek(p_transfer->file, p_offset, SEEK_SET);
	int size = read(p_transfer->file, data, p_size);
	if(size <= 0)
	{
		g_free(data);

		purple_xfer_error(PURPLE_XFER_SEND, purple_xfer_get_account(p_transfer->xfer),
						  purple_xfer_get_remote_user(p_transfer->xfer),
						  _("Reading a data segment failed! Make sure you have appropriate permissions!"));

		// Abort the transfer
		gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, TRUE);
		return;
	}

	gfire_p2p_dl_proto_send_file_data_packet(p_transfer->session, p_transfer->fileid, p_offset,
											 size, data, p_msgid);

	g_free(data);

	// Update GUI
	if((purple_xfer_get_bytes_sent(p_transfer->xfer) + size) > purple_xfer_get_size(p_transfer->xfer))
		purple_xfer_set_size(p_transfer->xfer, purple_xfer_get_bytes_sent(p_transfer->xfer) + size);

	purple_xfer_set_bytes_sent(p_transfer->xfer, purple_xfer_get_bytes_sent(p_transfer->xfer) + size);
	purple_xfer_update_progress(p_transfer->xfer);
}

void gfire_filetransfer_data_packet(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_size,
									const GList *p_data)
{
	if(!p_transfer || !p_data || !p_transfer->chunk)
		return;

	gfire_file_chunk_got_data(p_transfer->chunk, p_offset, p_size, p_data);
}

void gfire_filetransfer_complete(gfire_filetransfer *p_transfer)
{
	if(!p_transfer || purple_xfer_get_type(p_transfer->xfer) != PURPLE_XFER_SEND)
		return;

	purple_xfer_set_completed(p_transfer->xfer, TRUE);
	gfire_p2p_session_remove_file_transfer(p_transfer->session, p_transfer, TRUE);
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
