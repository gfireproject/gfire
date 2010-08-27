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

#ifndef _GF_FILE_CHUNK_H
#define _GF_FILE_CHUNK_H

typedef struct _gfire_file_chunk gfire_file_chunk;

#include "gf_base.h"
#include "gf_util.h"
#include "gf_filetransfer.h"

#define XFIRE_P2P_FT_DATA_PACKET_SIZE 0x0400 // 1024 Byte
#define XFIRE_P2P_FT_MAX_REQUESTS 10
#define XFIRE_P2P_FT_DATA_PACKET_TIMEOUT 20

typedef struct _gfire_file_requested_data
{
	guint32 data_packet;
	glong last_try;
	guint32 msgid;
} gfire_file_requested_data;

typedef enum
{
	GF_FILE_CHUNK_SEND = 0,
	GF_FILE_CHUNK_RECV
} gfire_file_chunk_type;

struct _gfire_file_chunk
{
	gfire_file_chunk_type type;
	guint64 offset;
	guint32 size;
	guint32 data_packet_count;
	guint32 data_packets_processed;
	gfire_bitlist *data_packets;
	guint32 last_requested;
	gfire_file_requested_data *requested;
	gchar *checksum;
	gboolean informed;
	gboolean finished;

	guint timeout;
	guint8 *data;

	gfire_filetransfer *ft;
};

// Initialization and cleanup
void gfire_file_chunk_init(gfire_file_chunk *p_chunk, gfire_filetransfer *p_transfer, gfire_file_chunk_type p_type,
						   guint64 p_offset, guint32 p_size);
void gfire_file_chunk_clear(gfire_file_chunk *p_chunk);

// Setting data
void gfire_file_chunk_make_current(gfire_file_chunk *p_chunk);
void gfire_file_chunk_set_checksum(gfire_file_chunk *p_chunk, const gchar *p_checksum);
void gfire_file_chunk_finalize(gfire_file_chunk *p_chunk);

// Identification
gboolean gfire_file_chunk_contains(const gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size);
gboolean gfire_file_chunk_is(const gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size);

// Sending
void gfire_file_chunk_send_info(gfire_file_chunk *p_chunk, guint32 p_msgid);
void gfire_file_chunk_send_data(gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size, guint32 p_msgid);

// Receiving
void gfire_file_chunk_start_transfer(gfire_file_chunk *p_chunk);
void gfire_file_chunk_got_data(gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size, const GList *p_data);

#endif // _GF_FILE_CHUNK_H
