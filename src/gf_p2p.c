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

#include "gf_p2p.h"
#include "gf_p2p_session.h"

static void gfire_p2p_connection_send(gfire_p2p_connection *p_p2p, const struct sockaddr_in *p_addr, guint32 p_len);

static guint32 gfire_p2p_connection_write_header(gfire_p2p_connection *p_p2p, guint8 p_encoding, const guint8 *p_moniker,
												 guint32 p_type, guint32 p_msgid,
												 guint32 p_seqid, guint32 p_data_len)
{
	if(!p_p2p || !p_moniker)
		return 0;

	guint32 offset = 0;

	// Encoding + 3 unknown bytes
	p_p2p->buff_out[0] = p_encoding;
	p_p2p->buff_out[1] = p_p2p->buff_out[2] = p_p2p->buff_out[3] = 0;
	offset += 4;

	// Moniker
	memcpy(p_p2p->buff_out + offset, p_moniker, 20);
	offset += 20;

	// Packet type
	p_type = GUINT32_TO_LE(p_type);
	memcpy(p_p2p->buff_out + offset, &p_type, 4);
	offset += 4;

	// Message ID
	p_msgid = GUINT32_TO_LE(p_msgid);
	memcpy(p_p2p->buff_out + offset, &p_msgid, 4);
	offset += 4;

	// Sequence ID
	p_seqid = GUINT32_TO_LE(p_seqid);
	memcpy(p_p2p->buff_out + offset, &p_seqid, 4);
	offset += 4;

	// Data Length
	p_data_len = GUINT32_TO_LE(p_data_len);
	memcpy(p_p2p->buff_out + offset, &p_data_len, 4);
	offset += 4;

	// 4 Unknown bytes
	guint32 unknown = 0;
	memcpy(p_p2p->buff_out + offset, &unknown, 4);
	offset += 4;

	return offset;
}

static guint32 gfire_p2p_connection_write_data(gfire_p2p_connection *p_p2p, guint8 p_encoding,
												 const guint8 *p_data, guint32 p_data_length,
												 const gchar *p_category, guint32 p_offset)
{
	if(!p_p2p || !p_category || strlen(p_category) > 16 || !p_data || p_data_length == 0)
		return 0;

	guint8 *checksum_start = p_p2p->buff_out + p_offset;

	// 4 unknown bytes
	memset(p_p2p->buff_out + p_offset, 0, 4);
	p_offset += 4;

	// Data
	memcpy(p_p2p->buff_out + p_offset, p_data, p_data_length);
	p_offset += p_data_length;

	// Category
	memcpy(p_p2p->buff_out + p_offset, p_category, strlen(p_category));
	p_offset += strlen(p_category);
	memset(p_p2p->buff_out + p_offset, 0, 16 - strlen(p_category));
	p_offset += 16 - strlen(p_category);

	// CRC32 checksum
	guint32 checksum = GUINT32_TO_LE(gfire_crc32(checksum_start, (p_p2p->buff_out + p_offset) - checksum_start));
	memcpy(p_p2p->buff_out + p_offset, &checksum, 4);
	p_offset += 4;

	// Encode data
	if(p_encoding != 0x0)
	{
		while(checksum_start < (p_p2p->buff_out + p_offset))
		{
			*checksum_start ^= p_encoding;
			checksum_start++;
		}
	}

	return p_offset;
}

static gfire_p2p_packet_resend *gfire_p2p_packet_resend_create(gfire_p2p_session *p_session, guint8 p_encoding,
															   const guint8 *p_moniker, guint32 p_type,
															   guint32 p_msgid, guint32 p_seqid, guint32 p_data_len,
															   const guint8 *p_data, const gchar *p_category)
{
	if(!p_session || !p_moniker)
		return NULL;

	// Abort on invalid data packages
	if((p_type == GFIRE_P2P_TYPE_DATA16 || p_type == GFIRE_P2P_TYPE_DATA32) && (!p_data || !p_data_len || !p_category))
		return NULL;

	gfire_p2p_packet_resend *ret = g_malloc0(sizeof(gfire_p2p_packet_resend));

	ret->encoding = p_encoding;

	ret->moniker = g_malloc0(20);
	memcpy(ret->moniker, p_moniker, 20);

	ret->type = p_type;
	ret->msgid = p_msgid;
	ret->seqid = p_seqid;

	if(p_data_len > 0)
	{
		ret->data_len = p_data_len;
		ret->data = g_malloc0(p_data_len);
		memcpy(ret->data, p_data, p_data_len);

		ret->category = g_strdup(p_category);
	}

	GTimeVal gtv;
	g_get_current_time(&gtv);

	ret->last_try = gtv.tv_sec;
	ret->session = p_session;

	return ret;
}

static void gfire_p2p_packet_resend_free(gfire_p2p_packet_resend *p_packet)
{
	if(!p_packet)
		return;

	if(p_packet->moniker)
		g_free(p_packet->moniker);

	if(p_packet->data)
		g_free(p_packet->data);

	if(p_packet->category)
		g_free(p_packet->category);

	g_free(p_packet);
}

static void gfire_p2p_packet_resend_send(gfire_p2p_packet_resend *p_packet, gfire_p2p_connection *p_p2p,
										 guint8 p_encoding)
{
	if(!p_packet || !p_p2p)
		return;

	guint32 offset = 0;
	switch(p_packet->type)
	{
	case GFIRE_P2P_TYPE_DATA16:
	case GFIRE_P2P_TYPE_DATA32:
		offset = gfire_p2p_connection_write_header(p_p2p, p_encoding, p_packet->moniker, p_packet->type,
												   p_packet->msgid, p_packet->seqid, p_packet->data_len);
		if(offset == 0)
			return;

		offset = gfire_p2p_connection_write_data(p_p2p, p_encoding, p_packet->data, p_packet->data_len,
											 p_packet->category, offset);
		if(offset == 0)
			return;

		gfire_p2p_connection_send(p_p2p, gfire_p2p_session_get_peer_addr(p_packet->session, GF_P2P_ADDR_TYPE_USE),
								  offset);
		break;
	default:
		purple_debug_warning("gfire", "P2P: gfire_p2p_packet_resend_send: unknown packet type!");
	}

	p_packet->retries++;
}

static void gfire_p2p_connection_send(gfire_p2p_connection *p_p2p, const struct sockaddr_in *p_addr, guint32 p_len)
{
	if(!p_p2p || !p_addr || !p_len)
		return;

	guint32 sent = sendto(p_p2p->socket, p_p2p->buff_out, p_len, 0, (struct sockaddr*)p_addr, sizeof(struct sockaddr_in));
	if(sent != p_len)
		purple_debug_warning("gfire", "P2P: Sent too less bytes!\n");
#ifdef DEBUG
	else
		purple_debug_misc("gfire", "P2P: %u bytes sent\n", p_len);
#endif // DEBUG
}

// Returns the Session ID (msgid of ping)
guint32 gfire_p2p_connection_send_ping(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
									   guint32 p_sessid, const struct sockaddr_in *p_addr)
{
	if(!p_p2p || !p_moniker || !p_addr)
		return 0;

	guint32 offset = gfire_p2p_connection_write_header(p_p2p, 0, p_moniker, GFIRE_P2P_TYPE_PING,
													   (p_sessid > 0) ? p_sessid : p_p2p->msgid, 0, 0);
	if(offset == 0)
		return 0;

	gfire_p2p_connection_send(p_p2p, p_addr, offset);

	if(p_sessid == 0)
	{
		p_p2p->msgid++;
		return (p_p2p->msgid - 1);
	}
	else
		return p_sessid;
}

// Returns the Session ID (msgid of pong if p_sessid == 0)
guint32 gfire_p2p_connection_send_pong(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
									   guint32 p_sessid, const struct sockaddr_in *p_addr)
{
	if(!p_p2p || !p_moniker || !p_addr)
		return 0;

	guint32 offset = gfire_p2p_connection_write_header(p_p2p, 0, p_moniker, GFIRE_P2P_TYPE_PONG,
													   (p_sessid > 0) ? p_sessid : p_p2p->msgid, 0, 0);
	if(offset == 0)
		return 0;

	gfire_p2p_connection_send(p_p2p, p_addr, offset);

	if(p_sessid == 0)
	{
		p_p2p->msgid++;
		return (p_p2p->msgid - 1);
	}
	else
		return p_sessid;
}

static void gfire_p2p_connection_send_ack(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
										  guint32 p_msgid, guint32 p_seqid, const struct sockaddr_in *p_addr)
{
	if(!p_p2p || !p_moniker || !p_addr)
		return;

	guint32 offset = gfire_p2p_connection_write_header(p_p2p, 0, p_moniker, GFIRE_P2P_TYPE_ACK, p_msgid, p_seqid, 0);
	if(offset == 0)
		return;

	gfire_p2p_connection_send(p_p2p, p_addr, offset);
}

static void gfire_p2p_connection_send_bad_crc32(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
												guint32 p_msgid, guint32 p_seqid, const struct sockaddr_in *p_addr)
{
	if(!p_p2p || !p_moniker || !p_addr)
		return;

	guint32 offset = gfire_p2p_connection_write_header(p_p2p, 0, p_moniker, GFIRE_P2P_TYPE_BADCRC, p_msgid, p_seqid, 0);
	if(offset == 0)
		return;

	gfire_p2p_connection_send(p_p2p, p_addr, offset);
}

void gfire_p2p_connection_send_keep_alive(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
										  guint32 p_sessid, const struct sockaddr_in *p_addr)
{
	if(!p_p2p || !p_moniker || !p_addr)
		return;

	guint32 offset = gfire_p2p_connection_write_header(p_p2p, 0, p_moniker, GFIRE_P2P_TYPE_KEEP_ALIVE_REQ,
													   p_sessid, 0, 0);
	if(offset == 0)
		return;

	gfire_p2p_connection_send(p_p2p, p_addr, offset);
}

void gfire_p2p_connection_send_keep_alive_reply(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
												guint32 p_sessid, const struct sockaddr_in *p_addr)
{
	if(!p_p2p || !p_moniker || !p_addr)
		return;

	guint32 offset = gfire_p2p_connection_write_header(p_p2p, 0, p_moniker, GFIRE_P2P_TYPE_KEEP_ALIVE_REP,
													   p_sessid, 0, 0);
	if(offset == 0)
		return;

	gfire_p2p_connection_send(p_p2p, p_addr, offset);
}

void gfire_p2p_connection_send_data16(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session,
									  guint8 p_encoding, const guint8 *p_moniker,
									  guint32 p_seqid, const guint8 *p_data, guint16 p_data_len,
									  const gchar *p_category, const struct sockaddr_in *p_addr)
{
	if(!p_p2p || !p_session || !p_moniker || !p_data || p_data_len == 0 || !p_category || !p_addr)
		return;

	guint32 offset = gfire_p2p_connection_write_header(p_p2p, p_encoding, p_moniker, GFIRE_P2P_TYPE_DATA16,
													   p_p2p->msgid, p_seqid, p_data_len);
	if(offset == 0)
		return;

	offset = gfire_p2p_connection_write_data(p_p2p, p_encoding, p_data, p_data_len, p_category, offset);
	if(offset == 0)
		return;

	p_p2p->msgid++;

	gfire_p2p_connection_send(p_p2p, p_addr, offset);
	gfire_p2p_packet_resend *packet = gfire_p2p_packet_resend_create(p_session, p_encoding, p_moniker,
																	 GFIRE_P2P_TYPE_DATA16, p_p2p->msgid - 1,
																	 p_seqid, p_data_len, p_data, p_category);
	if(packet)
		p_p2p->packets = g_list_append(p_p2p->packets, packet);
}

void gfire_p2p_connection_send_data32(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session,
									  guint8 p_encoding, const guint8 *p_moniker,
									  guint32 p_seqid, const guint8 *p_data, guint32 p_data_len,
									  const gchar *p_category, const struct sockaddr_in *p_addr)
{
	if(!p_p2p || !p_session || !p_moniker || !p_data || p_data_len == 0 || !p_category || !p_addr)
		return;

	guint32 offset = gfire_p2p_connection_write_header(p_p2p, p_encoding, p_moniker, GFIRE_P2P_TYPE_DATA32,
													   p_p2p->msgid, p_seqid, p_data_len);
	if(offset == 0)
		return;

	offset = gfire_p2p_connection_write_data(p_p2p, p_encoding, p_data, p_data_len, p_category, offset);
	if(offset == 0)
		return;

	p_p2p->msgid++;

	gfire_p2p_connection_send(p_p2p, p_addr, offset);
	gfire_p2p_packet_resend *packet = gfire_p2p_packet_resend_create(p_session, p_encoding, p_moniker,
																	 GFIRE_P2P_TYPE_DATA32, p_p2p->msgid - 1,
																	 p_seqid, p_data_len, p_data, p_category);
	if(packet)
		p_p2p->packets = g_list_append(p_p2p->packets, packet);
}

static gboolean gfire_p2p_connection_resend(gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return FALSE;

	GTimeVal gtv;
	g_get_current_time(&gtv);

	GList *cur = p_p2p->packets;
	while(cur)
	{
		gfire_p2p_packet_resend *packet = (gfire_p2p_packet_resend*)cur->data;
		if(gtv.tv_sec - GFIRE_P2P_PACKET_TIMEOUT >= packet->last_try)
		{
			if(packet->retries == GFIRE_P2P_MAX_RETRIES)
			{
				gfire_p2p_packet_resend_free(packet);
				p_p2p->packets = g_list_delete_link(p_p2p->packets, cur);
				cur = p_p2p->packets;

				continue;
			}
			gfire_p2p_packet_resend_send(packet, p_p2p, 0x33 + packet->retries * 0x33);
		}

		cur = g_list_next(cur);
	}

	return TRUE;
}

static void gfire_p2p_connection_input_cb(gpointer p_data, gint p_fd, PurpleInputCondition p_condition)
{
	if(!p_data || !(p_condition & PURPLE_INPUT_READ))
		return;

	gfire_p2p_connection *p2p = p_data;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	socklen_t addrlen = sizeof(addr);
	int length = recvfrom(p2p->socket, p2p->buff_in, GFIRE_P2P_BUFFER_LEN, 0, (struct sockaddr*)&addr, &addrlen);
#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: %u bytes received\n", length);
#endif // DEBUG

	if(length < 44)
	{
		purple_debug_warning("gfire", "P2P: received invalid packet\n");
		return;
	}

	guint8 encoding = *p2p->buff_in;
	guint32 offset = 4;

	GList *session = p2p->sessions;
	while(session)
	{
		if(gfire_p2p_session_is_by_moniker_peer(session->data, p2p->buff_in + offset))
			break;

		session = g_list_next(session);
	}

	if(!session)
	{
		purple_debug_warning("gfire", "P2P: packet for unknown session\n");
		return;
	}

	guint32 ip = g_ntohl(addr.sin_addr.s_addr);
	guint16 port = g_ntohs(addr.sin_port);
	gfire_p2p_session_set_addr(session->data, GF_P2P_ADDR_TYPE_USE, ip, port);

	offset += 20;

	guint32 type;
	memcpy(&type, p2p->buff_in + offset, 4);
	type = GUINT32_FROM_LE(type);
	offset += 4;

	guint32 msgid;
	memcpy(&msgid, p2p->buff_in + offset, 4);
	msgid = GUINT32_FROM_LE(msgid);
	offset += 4;

	guint32 seqid;
	memcpy(&seqid, p2p->buff_in + offset, 4);
	seqid = GUINT32_FROM_LE(seqid);
	offset += 4;

#ifdef DEBUG
	purple_debug_misc("gfire", "P2P: Packet: Type = %u; MsgID = %u; SeqID = %u\n", type, msgid, seqid);
#endif // DEBUG

	switch(type)
	{
	case GFIRE_P2P_TYPE_PING:
		gfire_p2p_session_ping(session->data, msgid);
		break;
	case GFIRE_P2P_TYPE_PONG:
		gfire_p2p_session_pong(session->data, msgid);
		break;
	case GFIRE_P2P_TYPE_ACK:
		{
			// Remove packet from resend queue
			GList *cur = p2p->packets;
			while(cur)
			{
				gfire_p2p_packet_resend *packet = cur->data;
				if(packet->session == session->data && packet->msgid == msgid && packet->seqid == seqid)
				{
					gfire_p2p_packet_resend_free(packet);
					p2p->packets = g_list_delete_link(p2p->packets, cur);
					break;
				}

				cur = g_list_next(cur);
			}
			break;
		}
	case GFIRE_P2P_TYPE_BADCRC:
		{
			// Force immediate rebuilding & resending
			GList *cur = p2p->packets;
			while(cur)
			{
				gfire_p2p_packet_resend *packet = cur->data;
				if(packet->session == session->data && packet->msgid == msgid && packet->seqid == seqid)
				{
					gfire_p2p_packet_resend_send(packet, p2p, 0);
					if(packet->retries == GFIRE_P2P_MAX_RETRIES)
					{
						gfire_p2p_packet_resend_free(packet);
						p2p->packets = g_list_delete_link(p2p->packets, cur);
					}
					break;
				}

				cur = g_list_next(cur);
			}
			break;
		}
	case GFIRE_P2P_TYPE_KEEP_ALIVE_REQ:
		gfire_p2p_session_keep_alive_request(session->data, msgid);
		break;
	case GFIRE_P2P_TYPE_KEEP_ALIVE_REP:
		gfire_p2p_session_keep_alive_response(session->data, msgid);
		break;
	case GFIRE_P2P_TYPE_DATA16:
	case GFIRE_P2P_TYPE_DATA32:
		{
			guint32 size;
			memcpy(&size, p2p->buff_in + offset, 4);
			size = GUINT32_FROM_LE(size);
			offset += 4;

			if(length < (68 + size))
			{
				purple_debug_warning("gfire", "P2P: Received too short packet\n");
				break;
			}

			guint32 crc_offset = offset + 4;
			guint8 *crc_data = p2p->buff_in + crc_offset;

			// Decode
			if(encoding != 0x00)
			{
#ifdef DEBUG
				purple_debug_misc("gfire", "P2P: Decoding encoded packet with value %u\n", encoding);
#endif // DEBUG
				guint32 i = 0;
				for(; i < (4 + size + 16 + 4/* unknown + data + category + crc32 */); i++)
					*(crc_data + i) ^= encoding;
			}

			// Unknown
			offset += 8;

			void *data = p2p->buff_in + offset;

			offset += size;

			gchar category[17];
			category[16] = 0;
			memcpy(category, p2p->buff_in + offset, 16);
			offset += 16;

#ifdef DEBUG_VERBOSE
			purple_debug_misc("gfire", "P2P: Category: %s\n", category);
#endif // DEBUG_VERBOSE

			guint32 crc32;
			memcpy(&crc32, p2p->buff_in + offset, 4);
			crc32 = GUINT32_FROM_LE(crc32);

			if(crc32 != gfire_crc32(crc_data, offset - crc_offset))
			{
				purple_debug_warning("gfire", "P2P: Received data packet with incorrect CRC-32\n");
				gfire_p2p_connection_send_bad_crc32(p2p, gfire_p2p_session_get_moniker_self(session->data),
													msgid, seqid,
													gfire_p2p_session_get_peer_addr(session->data,
																					GF_P2P_ADDR_TYPE_USE));
				return;
			}

			gfire_p2p_session_handle_data(session->data, type, msgid, seqid, data, size, category);

			gfire_p2p_connection_send_ack(p2p, gfire_p2p_session_get_moniker_self(session->data), msgid, seqid,
										  gfire_p2p_session_get_peer_addr(session->data, GF_P2P_ADDR_TYPE_USE));
			break;
		}
	default:
		purple_debug_warning("gfire", "P2P: unknown packet type (0x%x) received\n", type);
		break;
	}
}

static void gfire_p2p_connection_nat_cb(int p_type, guint32 p_ip, guint16 p_port, gpointer p_data)
{
	if(!p_data)
		return;

	gfire_p2p_connection *p2p = (gfire_p2p_connection*)p_data;

	gfire_p2p_natcheck_destroy(p2p->nat_check);
	p2p->nat_check = NULL;

	p2p->natType = p_type;
	p2p->ext_ip = p_ip;
	p2p->ext_port = p_port;

	// Check successfull, start listening
	if(p2p->natType > 0)
	{
		p2p->prpl_inpa = purple_input_add(p2p->socket, PURPLE_INPUT_READ, gfire_p2p_connection_input_cb, p2p);

		p2p->resend_source = g_timeout_add_seconds(GFIRE_P2P_PACKET_TIMEOUT,
												   (GSourceFunc)gfire_p2p_connection_resend, p2p);
	}
	// Check unsuccessfull -> no P2P possible -> close the socket, we don't need it anyway
	else
	{
		close(p2p->socket);
		p2p->socket = -1;
	}
}

static void gfire_p2p_connection_listen_cb(int p_fd, gpointer p_data)
{
	if(!p_data)
		return;

	gfire_p2p_connection *p2p = (gfire_p2p_connection*)p_data;

	p2p->listen_data = NULL;

	p2p->socket = p_fd;

	purple_debug_info("gfire", "P2P: Connection created on port %u\n", purple_network_get_port_from_fd(p_fd));

	// Start NAT Type check
	p2p->nat_check = gfire_p2p_natcheck_create();
	gfire_p2p_natcheck_start(p2p->nat_check, p_fd, gfire_p2p_connection_nat_cb, p2p);
}

gfire_p2p_connection *gfire_p2p_connection_create()
{
	gfire_p2p_connection *ret = g_malloc0(sizeof(gfire_p2p_connection));
	if(!ret)
		return NULL;

	ret->buff_in = g_malloc0(GFIRE_P2P_BUFFER_LEN);
	if(!ret->buff_in)
	{
		g_free(ret);
		return NULL;
	}

	ret->buff_out = g_malloc0(GFIRE_P2P_BUFFER_LEN);
	if(!ret->buff_out)
	{
		g_free(ret->buff_in);
		g_free(ret);
		return NULL;
	}

	ret->socket = -1;
	ret->msgid = 1;

	// Use random port (or from a range specified in Pidgins settings)
	if(!(ret->listen_data = purple_network_listen_range(0, 0, SOCK_DGRAM, gfire_p2p_connection_listen_cb, ret)))
	{
		g_free(ret->buff_in);
		g_free(ret->buff_out);
		g_free(ret);
		return NULL;
	}

	return ret;
}

void gfire_p2p_connection_close(gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return;

	if(p_p2p->listen_data)
		purple_network_listen_cancel(p_p2p->listen_data);

	gfire_p2p_natcheck_destroy(p_p2p->nat_check);

	if(p_p2p->prpl_inpa > 0)
		purple_input_remove(p_p2p->prpl_inpa);

	if(p_p2p->resend_source > 0)
		g_source_remove(p_p2p->resend_source);

	if(p_p2p->sessions)
		g_list_free(p_p2p->sessions);

	while(p_p2p->packets)
	{
		gfire_p2p_packet_resend_free(p_p2p->packets->data);
		p_p2p->packets = g_list_delete_link(p_p2p->packets, p_p2p->packets);
	}

	if(p_p2p->socket >= 0)
		close(p_p2p->socket);

	if(p_p2p->buff_in) g_free(p_p2p->buff_in);
	if(p_p2p->buff_out) g_free(p_p2p->buff_out);

	g_free(p_p2p);

	purple_debug_info("gfire", "P2P: Connection closed\n");
}

int gfire_p2p_connection_natType(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return 0;

	return p_p2p->natType;
}

gboolean gfire_p2p_connection_running(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return FALSE;

	return (p_p2p->socket >= 0 && p_p2p->natType > 0);
}

guint32 gfire_p2p_connection_local_ip(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return 0;

	const char *ip_str = purple_network_get_local_system_ip(p_p2p->socket);

	guint32 ip = 0;
	memcpy(&ip, purple_network_ip_atoi(ip_str), 4);
	return ip;
}

guint16 gfire_p2p_connection_local_port(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return 0;

	return purple_network_get_port_from_fd(p_p2p->socket);
}

guint16 gfire_p2p_connection_port(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return 0;

	return p_p2p->ext_port;
}

guint32 gfire_p2p_connection_ip(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return 0;

	return p_p2p->ext_ip;
}

void gfire_p2p_connection_add_session(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session)
{
	if(!p_p2p || !p_session)
		return;

	gfire_p2p_session_bind(p_session, p_p2p);
	p_p2p->sessions = g_list_append(p_p2p->sessions, p_session);

	purple_debug_info("gfire", "P2P: New session added (%u active)\n", g_list_length(p_p2p->sessions));
}

void gfire_p2p_connection_remove_session(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session)
{
	if(!p_p2p || !p_p2p->sessions || !p_session)
		return;

	GList *session = g_list_find(p_p2p->sessions, p_session);
	if(!session)
		return;

	// Remove all pending packets belonging to this session
	GList *cur_packet = p_p2p->packets;
	while(cur_packet)
	{
		gfire_p2p_packet_resend *packet = cur_packet->data;
		if(packet->session == p_session)
		{
			gfire_p2p_packet_resend_free(packet);
			p_p2p->packets = g_list_delete_link(p_p2p->packets, cur_packet);
			cur_packet = p_p2p->packets;
			continue;
		}

		cur_packet = g_list_next(cur_packet);
	}

	p_p2p->sessions = g_list_delete_link(p_p2p->sessions, session);

	purple_debug_info("gfire", "P2P: Session removed (%u left)\n", g_list_length(p_p2p->sessions));
}
