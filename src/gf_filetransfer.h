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

#ifndef _GF_FILETRANSFER_H
#define _GF_FILETRANSFER_H

typedef struct _gfire_filetransfer gfire_filetransfer;

#include "gf_base.h"
#include "gf_p2p_session.h"
#include "gf_file_chunk.h"

#define XFIRE_P2P_FT_PRIVATE_FILEID_START 0x80000000
#define XFIRE_P2P_FT_CHUNK_SIZE 0xC800 // 50 * 1024 Byte: 51200 Byte

struct _gfire_filetransfer
{
	// Related structures
	gfire_p2p_session *session;
	PurpleXfer *xfer;

	// Xfire transfer data
	guint32 fileid;
	guint32 msgid;
	gboolean aborted;

	// Chunks
	guint64 chunk_count;
	guint32 chunk_size;
	guint64 bytes_transferred;
	gfire_file_chunk *chunks;
	gfire_file_chunk *current_chunk;
	guint64 chunks_received;

	// The file itself
	int file;
	guint64 size;
};

gfire_filetransfer *gfire_filetransfer_create(gfire_p2p_session *p_session, PurpleXfer *p_xfer, guint32 p_fileid);
void gfire_filetransfer_start(gfire_filetransfer *p_transfer);
void gfire_filetransfer_free(gfire_filetransfer *p_transfer, gboolean p_local_reason);

// Handler functions
void gfire_filetransfer_request_reply(gfire_filetransfer *p_transfer, gboolean p_reply);
void gfire_filetransfer_event(gfire_filetransfer *p_transfer, guint32 p_event, guint32 p_type);
void gfire_filetransfer_transfer_info(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_chunk_size,
									  guint32 p_chunk_count, guint32 p_msgid);
void gfire_filetransfer_chunk_info(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_size,
								   const gchar *p_checksum);
void gfire_filetransfer_data_packet_request(gfire_filetransfer *p_transfer, guint64 p_offset,
											guint32 p_size, guint32 p_msgid);
void gfire_filetransfer_data_packet(gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_size,
									const GList *p_data);
void gfire_filetransfer_complete(gfire_filetransfer *p_transfer);

// Chunk processing
void gfire_filetransfer_next_chunk(gfire_filetransfer *p_transfer);

// Getting data
gfire_p2p_session *gfire_filetransfer_get_session(const gfire_filetransfer *p_transfer);
PurpleXfer *gfire_filetransfer_get_xfer(const gfire_filetransfer *p_transfer);
guint32 gfire_filetransfer_get_fileid(const gfire_filetransfer *p_transfer);
int gfire_filetransfer_get_file(const gfire_filetransfer *p_transfer);
guint32 gfire_filetransfer_next_msgid(gfire_filetransfer *p_transfer);

#endif // _GF_FILETRANSFER_H
