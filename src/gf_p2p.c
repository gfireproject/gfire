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

#include "gf_p2p.h"
#include "gf_p2p_session.h"

#include "gf_ipc_server.h"
#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <netinet/in.h>
#endif // _WIN32

static void gfire_p2p_connection_input_cb(gpointer p_data, gint p_fd, PurpleInputCondition p_condition)
{
	if(!p_data || p_condition != PURPLE_INPUT_READ)
		return;

	gfire_p2p_connection *p2p = p_data;

	int length = recv(p2p->socket, p2p->buff_in, GFIRE_P2P_BUFFER_LEN, 0);
	purple_debug_misc("gfire", "P2P: %u bytes received\n", length);

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
	purple_debug_error("gfire", "P2P: Packet: Type = %u; MsgID = %u; SeqID = %u\n", type, msgid, seqid);
#endif // DEBUG

	if(type != 0 && type != 0x300)
	{
		gfire_p2p_session_handle_data(session->data, type, msgid, seqid, NULL, 0, NULL);
	}
	else
	{
		guint32 size;
		memcpy(&size, p2p->buff_in + offset, 4);
		size = GUINT32_FROM_LE(size);
		offset += 4;

		guint32 crc_offset = offset + 4;
		guint8 *crc_data = p2p->buff_in + crc_offset;

		// Decode
		if(encoding != 0x00)
		{
#ifdef DEBUG
			purple_debug_misc("gfire", "Decoding encoded packet with value %u\n", encoding);
#endif // DEBUG
			guint32 i = 0;
			for(; i < (4 + size + 16  + 4/* unknown + data + category + crc32 */); i++)
				*(crc_data + i) ^= encoding;
		}

		// Unknown
		offset += 8;

		void *data = p2p->buff_in + offset;

		offset += size;

		gchar *category = (gchar*)p2p->buff_in + offset;
		offset += 16;

#ifdef DEBUG
		purple_debug_error("gfire", "Category: %s\n", category);
#endif // DEBUG

		guint32 crc32;
		memcpy(&crc32, p2p->buff_in + offset, 4);
		crc32 = GUINT32_FROM_LE(crc32);

		if(crc32 != gfire_crc32(crc_data, offset - crc_offset))
		{
			purple_debug_warning("gfire", "P2P: Received data packet with incorrect CRC-32!\n");
			gfire_p2p_session_send_invalid_crc(session->data, msgid, seqid);
			return;
		}

		gfire_p2p_session_handle_data(session->data, type, msgid, seqid, data, size, category);
	}
}

static void gfire_p2p_connection_listen_cb(int p_fd, gpointer p_data)
{
	if(!p_data)
		return;

	gfire_p2p_connection *p2p = (gfire_p2p_connection*)p_data;

	p2p->socket = p_fd;

	p2p->prpl_inpa = purple_input_add(p_fd, PURPLE_INPUT_READ, gfire_p2p_connection_input_cb, p2p);

	purple_debug_info("gfire", "P2P: Connection created on port %u\n", purple_network_get_port_from_fd(p_fd));
}

gfire_p2p_connection *gfire_p2p_connection_create(guint16 p_minport, guint16 p_maxport)
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

	// Use one of all available ports (for now)
	purple_network_listen_range(p_minport, p_maxport, SOCK_DGRAM, gfire_p2p_connection_listen_cb, ret);
	return ret;
}

void gfire_p2p_connection_close(gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return;

	if(p_p2p->prpl_inpa > 0)
		purple_input_remove(p_p2p->prpl_inpa);

	if(p_p2p->sessions)
		g_list_free(p_p2p->sessions);

	if(p_p2p->socket >= 0)
		close(p_p2p->socket);

	if(p_p2p->buff_in) g_free(p_p2p->buff_in);
	if(p_p2p->buff_out) g_free(p_p2p->buff_out);

	g_free(p_p2p);

	purple_debug_info("gfire", "P2P: Connection closed\n");
}

gboolean gfire_p2p_connection_running(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return FALSE;

	return (p_p2p->socket >= 0);
}

guint32 gfire_p2p_connection_local_ip(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return 0;

	const char *ip_str = purple_network_get_local_system_ip(p_p2p->socket);

	guint32 ip = 0;
	memcpy(&ip, purple_network_ip_atoi(ip_str), 4);
	ip = GUINT32_FROM_BE(ip);

	return ip;
}

guint16 gfire_p2p_connection_port(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return 0;

	return purple_network_get_port_from_fd(p_p2p->socket);
}

guint32 gfire_p2p_connection_ip(const gfire_p2p_connection *p_p2p)
{
	if(!p_p2p)
		return 0;

	const char *ip_str = purple_network_get_my_ip(p_p2p->socket);

	guint32 ip = 0;
	memcpy(&ip, purple_network_ip_atoi(ip_str), 4);
	ip = GUINT32_FROM_BE(ip);

	return ip;
}

void gfire_p2p_connection_add_session(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session, gboolean p_init)
{
	if(!p_p2p || !p_session)
		return;

	gfire_p2p_session_bind(p_session, p_p2p, p_init);
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

	p_p2p->sessions = g_list_delete_link(p_p2p->sessions, session);

	purple_debug_info("gfire", "P2P: Session removed (%u left)\n", g_list_length(p_p2p->sessions));
}

guint8 *gfire_p2p_connection_send_packet(gfire_p2p_connection *p_p2p, const gfire_p2p_session *p_session, guint32 p_type, guint32 p_msgid, guint32 p_seqid, void *p_data, guint32 p_len, const gchar *p_category, guint32 *p_packet_len)
{
	if(!p_p2p || !p_session || (p_type == 0 && (!p_data || !p_category)))
		return NULL;

	memset(p_p2p->buff_out, 0, 4);
	guint32 offset = 4;

	memcpy(p_p2p->buff_out + offset, gfire_p2p_session_get_moniker_self(p_session), 20);
	offset += 20;

	p_type = GUINT32_TO_LE(p_type);
	memcpy(p_p2p->buff_out + offset, &p_type, 4);
	offset += 4;

	p_msgid = GUINT32_TO_LE(p_msgid);
	memcpy(p_p2p->buff_out + offset, &p_msgid, 4);
	offset += 4;

	p_seqid = GUINT32_TO_LE(p_seqid);
	memcpy(p_p2p->buff_out + offset, &p_seqid, 4);
	offset += 4;

	if(p_type != 0 && p_type != 0x300)
	{
		memset(p_p2p->buff_out + offset, 0, 8);
		offset += 8;
	}
	else
	{
		p_len = GUINT32_TO_LE(p_len);
		memcpy(p_p2p->buff_out + offset, &p_len, 4);
		offset += 4;

		// Following data needs to be hashed
		guint32 checksum_start = offset + 4;
		guint8 *checksum_data = p_p2p->buff_out + checksum_start;

		memset(p_p2p->buff_out + offset, 0, 8);
		offset += 8;

		memcpy(p_p2p->buff_out + offset, p_data, GUINT32_FROM_LE(p_len));
		offset += GUINT32_FROM_LE(p_len);

		memcpy(p_p2p->buff_out + offset, p_category, strlen(p_category));
		offset += strlen(p_category);

		memset(p_p2p->buff_out + offset, 0, 16 - strlen(p_category));
		offset += 16 - strlen(p_category);

		guint32 checksum = GUINT32_TO_LE(gfire_crc32(checksum_data, offset - checksum_start));
		memcpy(p_p2p->buff_out + offset, &checksum, 4);
		offset += 4;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(gfire_p2p_session_get_peer_ip(p_session));
	addr.sin_port = htons(gfire_p2p_session_get_peer_port(p_session));

	int sent = sendto(p_p2p->socket, p_p2p->buff_out, offset, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	if(sent != offset)
		purple_debug_warning("gfire", "P2P: Sent too less bytes!\n");
	else
		purple_debug_misc("gfire", "P2P: %u bytes sent\n", offset);

	// Wants the buffer?
	if(p_packet_len)
	{
		*p_packet_len = offset;
		guint8 *ret = g_malloc0(offset);
		memcpy(ret, p_p2p->buff_out, offset);

		return ret;
	}

	return NULL;
}

void gfire_p2p_connection_resend_packet(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session, void *p_data, guint32 p_len)
{
	if(!p_p2p || !p_data || !p_len)
		return;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(gfire_p2p_session_get_peer_ip(p_session));
	addr.sin_port = htons(gfire_p2p_session_get_peer_port(p_session));

	int sent = sendto(p_p2p->socket, p_data, p_len, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	if(sent != p_len)
		purple_debug_warning("gfire", "P2P: Sent too less bytes!\n");
	else
		purple_debug_misc("gfire", "P2P: %u bytes sent\n", p_len);
}
