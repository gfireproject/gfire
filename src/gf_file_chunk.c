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

#include "gf_file_chunk.h"
#include "gf_p2p_dl_proto.h"

void gfire_file_chunk_init(gfire_file_chunk *p_chunk, gfire_filetransfer *p_transfer, guint64 p_offset, guint32 p_size)
{
	if(!p_chunk)
		return;

	memset(p_chunk, 0, sizeof(gfire_file_chunk));

	p_chunk->offset = p_offset;
	p_chunk->size = p_size;
	p_chunk->data_packet_count = p_size / XFIRE_P2P_FT_DATA_PACKET_SIZE;
	if((p_size % XFIRE_P2P_FT_DATA_PACKET_SIZE) != 0)
		p_chunk->data_packet_count++;
	p_chunk->data_packets = gfire_bitlist_new();
	p_chunk->ft = p_transfer;
}

void gfire_file_chunk_clear(gfire_file_chunk *p_chunk)
{
	if(!p_chunk)
		return;

	if(p_chunk->timeout > 0)
		g_source_remove(p_chunk->timeout);

	if(p_chunk->data_packets)
		gfire_bitlist_free(p_chunk->data_packets);

	if(p_chunk->requested)
		g_free(p_chunk->requested);

	if(p_chunk->checksum)
		g_free(p_chunk->checksum);
}

void gfire_file_chunk_set_checksum(gfire_file_chunk *p_chunk, const gchar *p_checksum)
{
	if(!p_chunk || !p_checksum)
		return;

	if(p_chunk->checksum)
		g_free(p_chunk->checksum);

	p_chunk->checksum = g_strdup(p_checksum);
	p_chunk->informed = TRUE;
}

gboolean gfire_file_chunk_contains(const gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size)
{
	return (p_chunk && (p_offset >= p_chunk->offset) && ((p_offset + p_size) <= (p_chunk->offset + p_chunk->size)));
}

gboolean gfire_file_chunk_is(const gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size)
{
	return (p_chunk && (p_offset == p_chunk->offset) && (p_size == p_chunk->size));
}

void gfire_file_chunk_send_info(gfire_file_chunk *p_chunk, guint32 p_msgid)
{
	if(!p_chunk || p_chunk->informed)
		return;

	fseek(gfire_filetransfer_get_file(p_chunk->ft), p_chunk->offset, SEEK_SET);

	guint8 *data = g_malloc(p_chunk->size);
	guint32 r = fread(data, 1, p_chunk->size, gfire_filetransfer_get_file(p_chunk->ft));

	gchar hash[41];
	hashSha1_bin_to_str(data, r, hash);

	gfire_p2p_dl_proto_send_file_chunk_info(gfire_filetransfer_get_session(p_chunk->ft),
											gfire_filetransfer_get_fileid(p_chunk->ft),
											p_chunk->offset, XFIRE_P2P_FT_CHUNK_SIZE, hash,
											p_msgid);

	g_free(data);

	p_chunk->informed = TRUE;
}

void gfire_file_chunk_send_data(gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size, guint32 p_msgid)
{
	if(!p_chunk || !gfire_file_chunk_contains(p_chunk, p_offset, p_size))
		return;

	fseek(gfire_filetransfer_get_file(p_chunk->ft), p_offset, SEEK_SET);

	guint8 *data = g_malloc(p_size);
	guint32 r = fread(data, 1, p_size, gfire_filetransfer_get_file(p_chunk->ft));

	gfire_p2p_dl_proto_send_file_data_packet(gfire_filetransfer_get_session(p_chunk->ft),
											 gfire_filetransfer_get_fileid(p_chunk->ft), p_offset,
											 r, data, p_msgid);

	g_free(data);

	guint32 data_packet = (p_offset - p_chunk->offset) / XFIRE_P2P_FT_DATA_PACKET_SIZE;
	if(!gfire_bitlist_get(p_chunk->data_packets, data_packet))
	{
		// Update the GUI
		purple_xfer_set_bytes_sent(gfire_filetransfer_get_xfer(p_chunk->ft),
								   purple_xfer_get_bytes_sent(gfire_filetransfer_get_xfer(p_chunk->ft)) + r);
		purple_xfer_update_progress(gfire_filetransfer_get_xfer(p_chunk->ft));

		p_chunk->data_packets_processed++;
		gfire_bitlist_set(p_chunk->data_packets, data_packet, TRUE);

		if(p_chunk->data_packets_processed == p_chunk->data_packet_count)
			p_chunk->finished = TRUE;
	}
}

static gboolean gfire_file_chunk_request_timeout(gfire_file_chunk *p_chunk)
{
	GTimeVal gtv;
	g_get_current_time(&gtv);

	guint32 i = 0;
	for(; i < XFIRE_P2P_FT_MAX_REQUESTS; i++)
	{
		if(((p_chunk->requested + i)->data_packet != p_chunk->data_packet_count) &&
		   ((gtv.tv_sec - (p_chunk->requested + i)->last_try) >= XFIRE_P2P_FT_DATA_PACKET_TIMEOUT))
		{
			(p_chunk->requested + i)->last_try = gtv.tv_sec;

			guint64 offset = (p_chunk->requested + i)->data_packet * XFIRE_P2P_FT_DATA_PACKET_SIZE + p_chunk->offset;
			guint32 size = (((p_chunk->offset + p_chunk->size) - offset) < XFIRE_P2P_FT_DATA_PACKET_SIZE) ?
						   ((p_chunk->offset + p_chunk->size) - offset) : XFIRE_P2P_FT_DATA_PACKET_SIZE;
			gfire_p2p_dl_proto_send_file_data_packet_request(gfire_filetransfer_get_session(p_chunk->ft),
															 gfire_filetransfer_get_fileid(p_chunk->ft),
															 offset, size,
															 (p_chunk->requested + i)->msgid);
		}
	}

	return TRUE;
}

static void gfire_file_chunk_new_requests(gfire_file_chunk *p_chunk)
{
	GTimeVal gtv;
	g_get_current_time(&gtv);

	guint32 i = 0;
	for(; i < XFIRE_P2P_FT_MAX_REQUESTS; i++)
	{
		// Request a further packet
		if((p_chunk->requested + i)->data_packet == p_chunk->data_packet_count)
		{
			// Skip if we've already all requested
			if(p_chunk->last_requested == p_chunk->data_packet_count)
				continue;

			(p_chunk->requested + i)->data_packet = p_chunk->last_requested++;
			(p_chunk->requested + i)->last_try = gtv.tv_sec;
			(p_chunk->requested + i)->msgid = gfire_filetransfer_next_msgid(p_chunk->ft);

			guint64 offset = (p_chunk->requested + i)->data_packet * XFIRE_P2P_FT_DATA_PACKET_SIZE + p_chunk->offset;
			guint32 size = (((p_chunk->offset + p_chunk->size) - offset) < XFIRE_P2P_FT_DATA_PACKET_SIZE) ?
						   ((p_chunk->offset + p_chunk->size) - offset) : XFIRE_P2P_FT_DATA_PACKET_SIZE;
			gfire_p2p_dl_proto_send_file_data_packet_request(gfire_filetransfer_get_session(p_chunk->ft),
															 gfire_filetransfer_get_fileid(p_chunk->ft),
															 offset, size,
															 (p_chunk->requested + i)->msgid);
		}
	}
}

void gfire_file_chunk_start_transfer(gfire_file_chunk *p_chunk)
{
	if(!p_chunk || p_chunk->requested)
		return;

	p_chunk->requested = g_malloc0(XFIRE_P2P_FT_MAX_REQUESTS * sizeof(gfire_file_requested_data));
	// Invalidate all requests
	guint32 i = 0;
	for(; i < XFIRE_P2P_FT_MAX_REQUESTS; i++)
		(p_chunk->requested + i)->data_packet = p_chunk->data_packet_count;

	// Request chunk info
	gfire_p2p_dl_proto_send_file_transfer_info(gfire_filetransfer_get_session(p_chunk->ft),
											   gfire_filetransfer_get_fileid(p_chunk->ft),
											   p_chunk->offset, XFIRE_P2P_FT_CHUNK_SIZE, 0,
											   gfire_filetransfer_next_msgid(p_chunk->ft));

	p_chunk->timeout = g_timeout_add_seconds(XFIRE_P2P_FT_DATA_PACKET_TIMEOUT,
											 (GSourceFunc)gfire_file_chunk_request_timeout, p_chunk);

	gfire_file_chunk_new_requests(p_chunk);
}

static gboolean gfire_file_chunk_check_checksum(gfire_file_chunk *p_chunk)
{
	if(!p_chunk)
		return FALSE;

	// If we don't have a checksum yet, ignore the check for now
	if(!p_chunk->checksum)
		return TRUE;

	fseek(gfire_filetransfer_get_file(p_chunk->ft), p_chunk->offset, SEEK_SET);

	guint8 *data = g_malloc(p_chunk->size);
	guint32 r = fread(data, 1, p_chunk->size, gfire_filetransfer_get_file(p_chunk->ft));

	gchar hash[41];
	hashSha1_bin_to_str(data, r, hash);
	g_free(data);

	return (!strcmp(hash, p_chunk->checksum));
}

void gfire_file_chunk_got_data(gfire_file_chunk *p_chunk, guint64 p_offset, guint32 p_size, const GList *p_data)
{
	if(!p_chunk || !p_data || !gfire_file_chunk_contains(p_chunk, p_offset, p_size))
		return;

	fseek(p_chunk->ft->file, p_offset, SEEK_SET);

	const GList *cur = p_data;
	while(cur)
	{
		fputc(*((guint8*)cur->data), gfire_filetransfer_get_file(p_chunk->ft));
		cur = g_list_next(cur);
	}

	guint32 data_packet = (p_offset - p_chunk->offset) / XFIRE_P2P_FT_DATA_PACKET_SIZE;
	if(!gfire_bitlist_get(p_chunk->data_packets, data_packet))
	{
		// Update the GUI
		purple_xfer_set_bytes_sent(gfire_filetransfer_get_xfer(p_chunk->ft),
								   purple_xfer_get_bytes_sent(gfire_filetransfer_get_xfer(p_chunk->ft)) + p_size);
		purple_xfer_update_progress(gfire_filetransfer_get_xfer(p_chunk->ft));

		p_chunk->data_packets_processed++;
		gfire_bitlist_set(p_chunk->data_packets, data_packet, TRUE);

		// Is this chunk done? If so process the next one
		if(p_chunk->data_packets_processed == p_chunk->data_packet_count)
		{
			// Check checksum and restart chunk if failed
			if(!gfire_file_chunk_check_checksum(p_chunk))
			{
				purple_debug_warning("gfire", "bad checksum for chunk at offset %lu\n", p_chunk->offset);
				gfire_bitlist_clear(p_chunk->data_packets);
				p_chunk->last_requested = 0;
				p_chunk->data_packets_processed = 0;

				guint32 i = 0;
				for(; i < XFIRE_P2P_FT_MAX_REQUESTS; i++)
				{
					if((p_chunk->requested + i)->data_packet == data_packet)
					{
						(p_chunk->requested + i)->data_packet = p_chunk->data_packet_count;
						break;
					}
				}

				gfire_file_chunk_new_requests(p_chunk);
				return;
			}

			p_chunk->finished = TRUE;
			g_source_remove(p_chunk->timeout);
			p_chunk->timeout = 0;
			g_free(p_chunk->requested);
			p_chunk->requested = 0;

			gfire_filetransfer_next_chunk(p_chunk->ft);

			return;
		}

		// Mark request as being done
		guint32 i = 0;
		for(; i < XFIRE_P2P_FT_MAX_REQUESTS; i++)
		{
			if((p_chunk->requested + i)->data_packet == data_packet)
			{
				(p_chunk->requested + i)->data_packet = p_chunk->data_packet_count;
				gfire_file_chunk_new_requests(p_chunk);
				break;
			}
		}
	}
}
