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

#ifndef _GF_FILE_CHUNK_H
#define _GF_FILE_CHUNK_H

typedef struct _gfire_file_chunk gfire_file_chunk;

#include "gf_util.h"
#include "gf_p2p_session.h"

#define XFIRE_P2P_FT_DATA_PACKET_SIZE 0x0400 // 1024 Byte
#define XFIRE_P2P_FT_MAX_REQUESTS 10

typedef void (*gfire_ft_callback)(gpointer*);

struct _gfire_file_chunk
{
	// Connection
	gfire_p2p_session *session;
	guint32 fileid;
	guint32 msgid;

	// Xfer
	PurpleXfer *xfer;

	// Chunk "geometry"
	guint64 offset;
	guint32 size;

	// Data packets
	guint32 data_packet_count;
	guint32 last_requested;
	guint32 requested[XFIRE_P2P_FT_MAX_REQUESTS];

	// Chunk data
	gchar *checksum;
	guint8 *data;

	// Callbacks
	gfire_ft_callback done_func;
	gfire_ft_callback error_func;
	gpointer user_data;
};

// Initialization and cleanup
gfire_file_chunk *gfire_file_chunk_create(gfire_p2p_session *p_session, guint32 p_fileid, guint32 p_msgid,
										  PurpleXfer *p_xfer, gfire_ft_callback p_done_func,
										  gfire_ft_callback p_error_func, gpointer p_data);
void gfire_file_chunk_free(gfire_file_chunk *p_chunk);
void gfire_file_chunk_init(gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size);

// Setting data
void gfire_file_chunk_set_checksum(gfire_file_chunk *p_chunk, const gchar *p_checksum);

// Getting data
guint64 gfire_file_chunk_get_offset(const gfire_file_chunk *p_chunk);
guint32 gfire_file_chunk_get_size(const gfire_file_chunk *p_chunk);
const guint8 *gfire_file_chunk_get_data(const gfire_file_chunk *p_chunk);

// Receiving
void gfire_file_chunk_start_transfer(gfire_file_chunk *p_chunk);
void gfire_file_chunk_got_data(gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size, const GList *p_data);

#endif // _GF_FILE_CHUNK_H
