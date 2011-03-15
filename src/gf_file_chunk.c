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

#include "gf_file_chunk.h"
#include "gf_p2p_dl_proto.h"

gfire_file_chunk *gfire_file_chunk_create(gfire_p2p_session *p_session, guint32 p_fileid, guint32 p_msgid,
										  PurpleXfer *p_xfer, gfire_ft_callback p_done_func,
										  gfire_ft_callback p_error_func, gpointer p_data)
{
	gfire_file_chunk *chunk = g_new0(gfire_file_chunk, 1);

	chunk->session = p_session;
	chunk->fileid = p_fileid;
	chunk->msgid = p_msgid;

	chunk->xfer = p_xfer;

	chunk->data = (guint8*)g_malloc(XFIRE_P2P_FT_CHUNK_SIZE);

	chunk->done_func = p_done_func;
	chunk->error_func = p_error_func;
	chunk->user_data = p_data;

	return chunk;
}

void gfire_file_chunk_free(gfire_file_chunk *p_chunk)
{
	if(!p_chunk)
		return;

	if(p_chunk->checksum)
		g_free(p_chunk->checksum);

	g_free(p_chunk->data);
	g_free(p_chunk);
}

void gfire_file_chunk_init(gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size)
{
	if(!p_chunk)
		return;

	p_chunk->offset = p_offset;
	p_chunk->size = (p_size >= XFIRE_P2P_FT_DATA_PACKET_SIZE) ? p_size : XFIRE_P2P_FT_CHUNK_SIZE;

	p_chunk->data_packet_count = p_size / XFIRE_P2P_FT_DATA_PACKET_SIZE;
	if((p_size % XFIRE_P2P_FT_DATA_PACKET_SIZE) != 0)
		p_chunk->data_packet_count++;

	p_chunk->last_requested = p_chunk->data_packet_count;
	guint32 i = 0;
	for(; i < XFIRE_P2P_FT_MAX_REQUESTS; i++)
		p_chunk->requested[i] = p_chunk->data_packet_count;

	if(p_chunk->checksum)
	{
		g_free(p_chunk->checksum);
		p_chunk->checksum = NULL;
	}
}

void gfire_file_chunk_set_checksum(gfire_file_chunk *p_chunk, const gchar *p_checksum)
{
	if(!p_chunk || !p_checksum)
		return;

	if(p_chunk->checksum)
		g_free(p_chunk->checksum);

	p_chunk->checksum = g_strdup(p_checksum);
}

guint64 gfire_file_chunk_get_offset(const gfire_file_chunk *p_chunk)
{
	return p_chunk ? p_chunk->offset : 0;
}

guint32 gfire_file_chunk_get_size(const gfire_file_chunk *p_chunk)
{
	return p_chunk ? p_chunk->size : 0;
}

const guint8 *gfire_file_chunk_get_data(const gfire_file_chunk *p_chunk)
{
	return p_chunk ? p_chunk->data : NULL;
}

void gfire_file_chunk_start_transfer(gfire_file_chunk *p_chunk)
{
	if(!p_chunk || p_chunk->last_requested != p_chunk->data_packet_count)
		return;

	// Request chunk info
	gfire_p2p_dl_proto_send_file_chunk_info_request(p_chunk->session, p_chunk->fileid, p_chunk->offset,
													p_chunk->size, 0, p_chunk->msgid++);

	guint32 requests = (p_chunk->data_packet_count > XFIRE_P2P_FT_MAX_REQUESTS) ?
				XFIRE_P2P_FT_MAX_REQUESTS : p_chunk->data_packet_count;
	guint32 i = 0;
	for(; i < requests; i++)
	{
		p_chunk->requested[i] = (p_chunk->last_requested == p_chunk->data_packet_count) ?
					0 : p_chunk->last_requested + 1;
		p_chunk->last_requested = p_chunk->requested[i];

		guint64 offset = p_chunk->offset + p_chunk->requested[i] * XFIRE_P2P_FT_DATA_PACKET_SIZE;
		guint32 size = (p_chunk->last_requested == p_chunk->data_packet_count - 1) ?
					p_chunk->size % XFIRE_P2P_FT_DATA_PACKET_SIZE : XFIRE_P2P_FT_DATA_PACKET_SIZE;

		gfire_p2p_dl_proto_send_file_data_packet_request(p_chunk->session,
														 p_chunk->fileid,
														 offset, size,
														 p_chunk->msgid++);
	}
}

static gboolean gfire_file_chunk_check_checksum(gfire_file_chunk *p_chunk)
{
	if(!p_chunk)
		return FALSE;

	// If we don't have a checksum yet, ignore the check for now
	if(!p_chunk->checksum)
		return TRUE;

	gchar hash[41];
	hashSha1_bin_to_str(p_chunk->data, p_chunk->size, hash);
	hash[40] = 0;

	return (!strcmp(hash, p_chunk->checksum));
}

void gfire_file_chunk_got_data(gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size, const GList *p_data)
{
	if(!p_chunk || !p_data)
		return;

	if(p_offset < p_chunk->offset || (p_offset + p_size) > (p_chunk->offset + p_chunk->size) ||
			((p_offset - p_chunk->offset) % XFIRE_P2P_FT_DATA_PACKET_SIZE) != 0)
	{
		purple_debug_warning("gfire", "P2P: Got unrequested data packet!\n");
		return;
	}

	const GList *cur = p_data;
	guint32 pos = 0;
	while(cur)
	{
		*(p_chunk->data + p_offset - p_chunk->offset + pos) = *((guint8*)cur->data);
		cur = g_list_next(cur);
		pos++;
	}

	guint32 data_packet = (p_offset - p_chunk->offset) / XFIRE_P2P_FT_DATA_PACKET_SIZE;
	guint32 i = 0;
	for(; i < XFIRE_P2P_FT_MAX_REQUESTS; i++)
	{
		if(p_chunk->requested[i] == data_packet)
		{
			// Update the GUI
			if((purple_xfer_get_bytes_sent(p_chunk->xfer) + p_size) > purple_xfer_get_size(p_chunk->xfer))
				purple_xfer_set_size(p_chunk->xfer, purple_xfer_get_bytes_sent(p_chunk->xfer) + p_size);

			purple_xfer_set_bytes_sent(p_chunk->xfer, purple_xfer_get_bytes_sent(p_chunk->xfer) + p_size);
			purple_xfer_update_progress(p_chunk->xfer);

			// Is this chunk done? If so process the next one
			if(p_chunk->last_requested == p_chunk->data_packet_count - 1)
			{
				// Check checksum and restart chunk if failed
				if(!gfire_file_chunk_check_checksum(p_chunk))
				{
					purple_debug_warning("gfire", "P2P: bad checksum for chunk at offset %lu\n", p_chunk->offset);
					gfire_file_chunk_init(p_chunk, p_chunk->offset, p_chunk->size);
					gfire_file_chunk_start_transfer(p_chunk);
					return;
				}

				if(p_chunk->done_func)
					(*p_chunk->done_func)(p_chunk->user_data);

				return;
			}

			if(p_chunk->last_requested < p_chunk->data_packet_count - 1)
			{
				p_chunk->requested[i] = ++p_chunk->last_requested;

				guint64 offset = p_chunk->offset + p_chunk->requested[i] * XFIRE_P2P_FT_DATA_PACKET_SIZE;
				guint32 size = (p_chunk->last_requested == p_chunk->data_packet_count - 1) ?
							p_chunk->size % XFIRE_P2P_FT_DATA_PACKET_SIZE : XFIRE_P2P_FT_DATA_PACKET_SIZE;

				gfire_p2p_dl_proto_send_file_data_packet_request(p_chunk->session,
																 p_chunk->fileid,
																 offset, size,
																 p_chunk->msgid++);
			}
			else
				p_chunk->requested[i] = p_chunk->data_packet_count;

			return;
		}
	}

	purple_debug_warning("gfire", "P2P: Got unrequested data packet!\n");
}
