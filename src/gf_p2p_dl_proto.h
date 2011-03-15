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

#ifndef _GF_P2P_DL_PROTO_H
#define _GF_P2P_DL_PROTO_H

#include "gf_p2p_session.h"
#include "gf_base.h"

// Packet sending
guint32 gfire_p2p_dl_proto_send_file_request(gfire_p2p_session *p_session, guint32 p_fileid, guint64 p_size,
											 const gchar *p_name, const gchar *p_desc, guint32 p_mtime);
guint32 gfire_p2p_dl_proto_send_file_request_reply(gfire_p2p_session *p_session, guint32 p_fileid, gboolean p_reply);
guint32 gfire_p2p_dl_proto_send_file_event(gfire_p2p_session *p_session, guint32 p_fileid,
										   guint32 p_event, guint32 p_type);
guint32 gfire_p2p_dl_proto_send_file_chunk_info_request(gfire_p2p_session *p_session, guint32 p_fileid,
												   guint64 p_offset, guint32 p_chunk_size,
												   guint32 p_chunk_count, guint32 p_msgid);
guint32 gfire_p2p_dl_proto_send_file_chunk_info(gfire_p2p_session *p_session, guint32 p_fileid, guint64 p_offset,
										   guint32 p_size, const gchar *p_checksum, guint32 p_msgid);
guint32 gfire_p2p_dl_proto_send_file_data_packet_request(gfire_p2p_session *p_session, guint32 p_fileid, guint64 p_offset,
														 guint32 p_size, guint32 p_msgid);
guint32 gfire_p2p_dl_proto_send_file_data_packet(gfire_p2p_session *p_session, guint32 p_fileid, guint64 p_offset,
												 guint32 p_size, const guint8 *p_data, guint32 p_msgid);
guint32 gfire_p2p_dl_proto_send_file_complete(gfire_p2p_session *p_session, guint32 p_fileid);

// Packet parsing
gboolean gfire_p2p_dl_proto_file_request(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);
gboolean gfire_p2p_dl_proto_file_request_reply(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);
gboolean gfire_p2p_dl_proto_file_event(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);
gboolean gfire_p2p_dl_proto_file_chunk_info_request(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);
gboolean gfire_p2p_dl_proto_file_chunk_info(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);
gboolean gfire_p2p_dl_proto_file_data_packet_request(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);
gboolean gfire_p2p_dl_proto_file_data_packet(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);
gboolean gfire_p2p_dl_proto_file_completion_msg(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);

#endif // _GF_P2P_DL_PROTO_H
