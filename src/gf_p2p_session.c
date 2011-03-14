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

#include "gf_p2p_session.h"
#include "gfire.h"
#include "gf_p2p.h"
#include "gf_p2p_im_handler.h"

static void gfire_p2p_session_send_ping(gfire_p2p_session *p_session, gfire_p2p_session_addr_type p_addrType)
{
	if(!p_session || !p_session->con)
		return;

	p_session->need_pong = TRUE;
	p_session->seqid = 0;

	GTimeVal gtv;
	g_get_current_time(&gtv);
	p_session->last_ping = gtv.tv_sec;

	p_session->sessid = gfire_p2p_connection_send_ping(p_session->con, p_session->moniker_self, p_session->sessid,
													   gfire_p2p_session_get_peer_addr(p_session, p_addrType));
}

static void gfire_p2p_session_send_pong(gfire_p2p_session *p_session)
{
	if(!p_session || !p_session->con)
		return;

	// Reset the sequence
	p_session->seqid = 0;
	gfire_bitlist_clear(p_session->rec_seqids);

	// Send the pong
	p_session->sessid = gfire_p2p_connection_send_pong(p_session->con, p_session->moniker_self, p_session->sessid,
													   gfire_p2p_session_get_peer_addr(p_session, GF_P2P_ADDR_TYPE_USE));
}

static void gfire_p2p_session_send_keep_alive(gfire_p2p_session *p_session)
{
	if(!p_session || !p_session->con)
		return;

	p_session->need_keep_alive = TRUE;
	GTimeVal gtv;
	g_get_current_time(&gtv);
	p_session->last_keep_alive = gtv.tv_sec;

	gfire_p2p_connection_send_keep_alive(p_session->con, p_session->moniker_self, p_session->sessid,
										 gfire_p2p_session_get_peer_addr(p_session, GF_P2P_ADDR_TYPE_USE));
}

void gfire_p2p_session_send_data16_packet(gfire_p2p_session *p_session, const guint8 *p_data, guint16 p_len, const gchar *p_category)
{
	if(!p_session || !p_session->con || !p_data || !p_len || !p_category)
		return;

	gfire_p2p_connection_send_data16(p_session->con, p_session, 0, p_session->moniker_self, p_session->seqid++,
									 p_data, p_len, p_category,
									 gfire_p2p_session_get_peer_addr(p_session, GF_P2P_ADDR_TYPE_USE));
}

void gfire_p2p_session_send_data32_packet(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len, const gchar *p_category)
{
	if(!p_session || !p_session->con || !p_data || !p_len || !p_category)
		return;

	gfire_p2p_connection_send_data32(p_session->con, p_session, 0, p_session->moniker_self, p_session->seqid++,
									 p_data, p_len, p_category,
									 gfire_p2p_session_get_peer_addr(p_session, GF_P2P_ADDR_TYPE_USE));
}

static gboolean gfire_p2p_session_check_cb(gfire_p2p_session *p_session)
{
	if(!p_session)
		return FALSE;

	GTimeVal gtv;
	g_get_current_time(&gtv);

	// Pong timeout check
	if(p_session->need_pong && ((gtv.tv_sec - p_session->last_ping) > GFIRE_P2P_PACKET_TIMEOUT))
	{
		if(p_session->ping_retries == GFIRE_P2P_MAX_RETRIES)
		{
			purple_debug_info("gfire", "P2P: Session timed out!\n");
			gfire_buddy_p2p_timedout(p_session->buddy);

			return FALSE;
		}

		p_session->ping_retries++;
		purple_debug_misc("gfire", "P2P: Resending ping packet (try %d of %u)\n", p_session->ping_retries + 1, GFIRE_P2P_MAX_RETRIES + 1);
		if(!gfire_p2p_session_get_peer_ip(p_session, GF_P2P_ADDR_TYPE_USE))
		{
			gfire_p2p_session_send_ping(p_session, GF_P2P_ADDR_TYPE_LOCAL);
			gfire_p2p_session_send_ping(p_session, GF_P2P_ADDR_TYPE_EXTERN);
		}
		else
			gfire_p2p_session_send_ping(p_session, GF_P2P_ADDR_TYPE_USE);
	}

	// Keep-alive timeout check
	if(p_session->need_keep_alive && ((gtv.tv_sec - p_session->last_keep_alive) > GFIRE_P2P_PACKET_TIMEOUT))
	{
		if(p_session->keep_alive_retries == GFIRE_P2P_MAX_RETRIES)
		{
			purple_debug_info("gfire", "P2P: Session timed out!\n");
			gfire_buddy_p2p_timedout(p_session->buddy);

			return FALSE;
		}

		p_session->keep_alive_retries++;
		purple_debug_misc("gfire", "P2P: Resending keep-alive packet (try %d of %u)\n", p_session->keep_alive_retries + 1, GFIRE_P2P_MAX_RETRIES + 1);
		gfire_p2p_session_send_keep_alive(p_session);
	}

	// Keep-alive-handling
	if(!p_session->need_keep_alive && (gtv.tv_sec >= (p_session->last_keep_alive + 60)))
	{
		purple_debug_misc("gfire", "P2P: Sending keep-alive\n");
		gfire_p2p_session_send_keep_alive(p_session);
	}

	return TRUE;
}

gfire_p2p_session *gfire_p2p_session_create(gfire_buddy *p_buddy, const gchar *p_salt)
{
	if(!p_buddy || !p_salt)
		return NULL;

	gfire_p2p_session *ret = g_malloc0(sizeof(gfire_p2p_session));
	if(!ret)
		return NULL;

	ret->peer_addr[0].sin_family = AF_INET;
	ret->peer_addr[1].sin_family = AF_INET;
	ret->peer_addr[2].sin_family = AF_INET;

	ret->moniker_self = g_malloc0(20);
	ret->moniker_peer = g_malloc0(20);

	ret->need_pong = FALSE;
	ret->need_keep_alive = FALSE;

	GTimeVal gtv;
	g_get_current_time(&gtv);
	ret->last_keep_alive = gtv.tv_sec;

	ret->rec_seqids = gfire_bitlist_new();

	ret->buddy = p_buddy;

	// Generate the monikers
	gchar *sid = gfire_hex_bin_to_str(p_buddy->sid, XFIRE_SID_LEN);
	gchar *tohash = g_strdup_printf("%s%s", sid, p_salt);
	hashSha1_to_bin(tohash, ret->moniker_peer);
	g_free(sid);
	g_free(tohash);

	sid = gfire_hex_bin_to_str(((gfire_data*)p_buddy->gc->proto_data)->sid, XFIRE_SID_LEN);
	tohash = g_strdup_printf("%s%s", sid, p_salt);
	hashSha1_to_bin(tohash, ret->moniker_self);
	g_free(sid);
	g_free(tohash);

	return ret;
}

void gfire_p2p_session_free(gfire_p2p_session *p_session, gboolean p_local_reason)
{
	if(!p_session)
		return;

	if(p_session->check_timer > 0)
		g_source_remove(p_session->check_timer);

	while(p_session->transfers)
	{
		gfire_filetransfer_free(p_session->transfers->data, p_local_reason);
		p_session->transfers = g_list_delete_link(p_session->transfers, p_session->transfers);
	}

	gfire_bitlist_free(p_session->rec_seqids);

	g_free(p_session->moniker_self);
	g_free(p_session->moniker_peer);

	g_free(p_session);
}

void gfire_p2p_session_bind(gfire_p2p_session *p_session, gfire_p2p_connection *p_p2p)
{
	if(p_session && p_p2p)
		p_session->con = p_p2p;
}

void gfire_p2p_session_set_addr(gfire_p2p_session *p_session, gfire_p2p_session_addr_type p_type, guint32 p_ip, guint16 p_port) {
	if(!p_session || !p_ip || !p_port)
		return;

	if(!p_session->peer_addr[(int)p_type].sin_addr.s_addr) {
		p_session->peer_addr[(int)p_type].sin_addr.s_addr = g_htonl(p_ip);
		p_session->peer_addr[(int)p_type].sin_port = g_htons(p_port);

		// Send a handshake for NAT Type 2 and 3 buddies now as we have their address
		if((p_session->peer_natType == 2 || p_session->peer_natType == 3) && p_type == GF_P2P_ADDR_TYPE_USE)
		{
			gfire_p2p_session_send_ping(p_session, GF_P2P_ADDR_TYPE_USE);
			purple_debug_misc("gfire", "P2P: Handshake sent\n");
		}
	}
}

void gfire_p2p_session_start(gfire_p2p_session *p_session, guint32 p_natType)
{
	if(!p_session)
		return;

	p_session->peer_natType = p_natType;

	// Send ping/handshake if we can actually send any data
	if(!p_session->connected && p_natType != 2 && p_natType != 3)
	{
		if(!gfire_p2p_session_get_peer_ip(p_session, GF_P2P_ADDR_TYPE_USE))
		{
			gfire_p2p_session_send_ping(p_session, GF_P2P_ADDR_TYPE_LOCAL);
			gfire_p2p_session_send_ping(p_session, GF_P2P_ADDR_TYPE_EXTERN);
		}
		else
			gfire_p2p_session_send_ping(p_session, GF_P2P_ADDR_TYPE_USE);

		purple_debug_misc("gfire", "P2P: Handshake sent\n");
	}

	p_session->check_timer = g_timeout_add_seconds(1, (GSourceFunc)gfire_p2p_session_check_cb, p_session);
}

const guint8 *gfire_p2p_session_get_moniker_peer(const gfire_p2p_session *p_session)
{
	if(!p_session)
		return NULL;

	return p_session->moniker_peer;
}

const guint8 *gfire_p2p_session_get_moniker_self(const gfire_p2p_session *p_session)
{
	if(!p_session)
		return NULL;

	return p_session->moniker_self;
}

guint32 gfire_p2p_session_get_peer_ip(const gfire_p2p_session *p_session, gfire_p2p_session_addr_type p_type)
{
	if(!p_session)
		return 0;

	return g_ntohl(p_session->peer_addr[(int)p_type].sin_addr.s_addr);
}

guint16 gfire_p2p_session_get_peer_port(const gfire_p2p_session *p_session, gfire_p2p_session_addr_type p_type)
{
	if(!p_session)
		return 0;

	return g_ntohs(p_session->peer_addr[(int)p_type].sin_port);
}

const struct sockaddr_in *gfire_p2p_session_get_peer_addr(const gfire_p2p_session *p_session, gfire_p2p_session_addr_type p_type)
{
	if(!p_session)
		return NULL;

	return &(p_session->peer_addr[(int)p_type]);
}

gfire_buddy *gfire_p2p_session_get_buddy(const gfire_p2p_session *p_session)
{
	if(!p_session)
		return NULL;

	return p_session->buddy;
}

gboolean gfire_p2p_session_is_by_moniker_peer(const gfire_p2p_session *p_session, const guint8 *p_moniker)
{
	if(!p_session)
		return FALSE;

	return (memcmp(p_session->moniker_peer, p_moniker, 20) == 0);
}

gboolean gfire_p2p_session_is_by_moniker_self(const gfire_p2p_session *p_session, const guint8 *p_moniker)
{
	if(!p_session)
		return FALSE;

	return (memcmp(p_session->moniker_self, p_moniker, 20) == 0);
}

void gfire_p2p_session_ping(gfire_p2p_session *p_session)
{
	if(!p_session)
		return;

	gfire_p2p_session_send_pong(p_session);
	gfire_bitlist_clear(p_session->rec_seqids);

	if(!p_session->connected)
		gfire_p2p_session_send_ping(p_session, GF_P2P_ADDR_TYPE_USE);
}

void gfire_p2p_session_pong(gfire_p2p_session *p_session)
{
	if(!p_session)
		return;

	if(!p_session->connected)
	{
		// Start requested file transfers as soon as we are connected
		GList *cur = p_session->transfers;
		while(cur)
		{
			gfire_filetransfer_start(cur->data);
			cur = g_list_next(cur);
		}

		p_session->connected = TRUE;
		gfire_buddy_p2p_connected(p_session->buddy);
	}
	p_session->need_pong = FALSE;
}

void gfire_p2p_session_keep_alive_request(gfire_p2p_session *p_session)
{
	if(p_session && p_session->con)
		gfire_p2p_connection_send_keep_alive_reply(p_session->con, p_session->moniker_self, p_session->sessid,
												   gfire_p2p_session_get_peer_addr(p_session, GF_P2P_ADDR_TYPE_USE));
}

void gfire_p2p_session_keep_alive_response(gfire_p2p_session *p_session)
{
	if(p_session)
		p_session->need_keep_alive = FALSE;
}

gboolean gfire_p2p_session_handle_data(gfire_p2p_session *p_session, guint32 p_type, guint32 p_seqid, void *p_data, guint32 p_len, const gchar *p_category)
{
	if(!p_session || !p_data || !p_category)
		return FALSE;

	// Check for duplicate messages
	if(gfire_bitlist_get(p_session->rec_seqids, p_seqid))
	{
		purple_debug_misc("gfire", "P2P: Received duplicate message, ignoring it\n");
		return TRUE;
	}

	gboolean result = TRUE;

	// Data 32bit size
	if(p_type == GFIRE_P2P_TYPE_DATA32)
	{
#ifdef DEBUG_VERBOSE
		purple_debug_misc("gfire", "P2P: Received 32bit data packet\n");
#endif // DEBUG_VERBOSE

		if(g_utf8_collate(p_category, "DL") == 0)
		{
#ifdef DEBUG
			purple_debug_misc("gfire", "P2P: received DL packet\n");
#endif // DEBUG
			result = gfire_p2p_dl_handler_handle(p_session, p_data, p_len);
		}
		else
			purple_debug_warning("gfire", "P2P: received unknown data packet (category \"%s\")\n", p_category);
	}
	// Data 16bit size
	else if(p_type == GFIRE_P2P_TYPE_DATA16)
	{
#ifdef DEBUG_VERBOSE
		purple_debug_misc("gfire", "P2P: Received 16bit data packet\n");
#endif // DEBUG_VERBOSE

		if(g_utf8_collate(p_category, "IM") == 0)
		{
#ifdef DEBUG
			purple_debug_misc("gfire", "P2P: received IM packet\n");
#endif // DEBUG
			result = gfire_p2p_im_handler_handle(p_session, p_data, p_len);
		}
		else
			purple_debug_warning("gfire", "P2P: received unknown data packet (category \"%s\")\n", p_category);
	}

	// Add this packet to the received ones
	if(result)
		gfire_bitlist_set(p_session->rec_seqids, p_seqid, TRUE);

	return result;
}

void gfire_p2p_session_add_file_transfer(gfire_p2p_session *p_session, PurpleXfer *p_xfer)
{
	if(!p_session || !p_xfer)
		return;

	gfire_filetransfer *ft = gfire_filetransfer_create(p_session, p_xfer, 0);
	if(!ft)
		return;

	if(p_session->connected)
		gfire_filetransfer_start(ft);
	else
		purple_xfer_conversation_write(p_xfer, _("Please wait until a connection with your buddy has been established!"), FALSE);

	p_session->transfers = g_list_append(p_session->transfers, ft);
}

void gfire_p2p_session_add_recv_file_transfer(gfire_p2p_session *p_session, gfire_filetransfer *p_transfer)
{
	if(!p_session || !p_transfer)
		return;

	p_session->transfers = g_list_append(p_session->transfers, p_transfer);
}

void gfire_p2p_session_remove_file_transfer(gfire_p2p_session *p_session, gfire_filetransfer *p_transfer, gboolean p_local_reason)
{
	if(!p_session || !p_transfer)
		return;

	GList *ft = g_list_find(p_session->transfers, p_transfer);
	if(ft)
	{
		gfire_filetransfer_free(p_transfer, p_local_reason);
		p_session->transfers = g_list_delete_link(p_session->transfers, ft);
	}
}

gfire_filetransfer *gfire_p2p_session_find_file_transfer(gfire_p2p_session *p_session, guint32 p_fileid)
{
	if(!p_session)
		return NULL;

	GList *ft = g_list_first(p_session->transfers);
	for(; ft; ft = g_list_next(ft))
	{
		if(((gfire_filetransfer*)ft->data)->fileid == p_fileid)
			return ft->data;
	}

	return NULL;
}

gboolean gfire_p2p_session_connected(const gfire_p2p_session *p_session)
{
	return (p_session && p_session->connected);
}
