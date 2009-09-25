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

#ifndef _GF_P2P_DL_HANDLER_H
#define _GF_P2P_DL_HANDLER_H

typedef struct _gfire_file_chunk gfire_file_chunk;
typedef struct _gfire_filetransfer gfire_filetransfer;

#include "gf_base.h"
#include "gf_p2p_session.h"

#define XFIRE_P2P_FT_UNKNOWN_CHUNK_SIZE 0xC800
#define XFIRE_P2P_FT_DEFAULT_CHUNK_SIZE 0x0400
#define GFIRE_P2P_FT_CHUNK_REQUEST_COUNT 10

// State of a single file chunk
struct _gfire_file_chunk
{
	guint64 offset;
	guint32 size;
	gboolean requested;
	glong request_time;
	gboolean received;
};

struct _gfire_filetransfer
{
	// Related structures
	gfire_p2p_session *session;
	PurpleXfer *xfer;

	// Xfire transfer data
	guint32 fileid;
	guint32 msgid;
	guint32 chunk_size;
	guint64 bytes_transferred;
	guint32 chunks_requested;
	guint32 chunks_received;
	GList *chunks;
	gboolean first_sent;

	gchar *file_checksum;

	// The file itself
	FILE *file;
	guint64 size;
};

// GFIRE FILETRANSFER
gfire_filetransfer *gfire_filetransfer_init(gfire_p2p_session *p_session, PurpleXfer *p_xfer);
void gfire_filetransfer_free(gfire_filetransfer *p_transfer, gboolean p_local_reason);

// GFIRE P2P DL DATA HANDLER
void gfire_p2p_dl_handler_handle(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);

#endif // _GF_P2P_DL_HANDLER_H
