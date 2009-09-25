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

#include "gf_p2p_session.h"
#include "gfire.h"
#include "gf_p2p.h"
#include "gf_p2p_im_handler.h"

static void gfire_p2p_session_req_ack_remove_item(gfire_p2p_session *p_session, gfire_p2p_session_req_ack_item *p_item);
static void gfire_p2p_session_send_ping(gfire_p2p_session *p_session);
static void gfire_p2p_session_send_pong(gfire_p2p_session *p_session);
static void gfire_p2p_session_send_ack(gfire_p2p_session *p_session, guint32 p_msgid, guint32 p_seqid);
static void gfire_p2p_session_send_keep_alive(gfire_p2p_session *p_session);
static void gfire_p2p_session_send_keep_alive_reply(gfire_p2p_session *p_session);

static gboolean gfire_p2p_session_check_cb(gfire_p2p_session *p_session)
{
	if(!p_session)
		return FALSE;

	GTimeVal gtv;
	g_get_current_time(&gtv);

	// Pong timeout check
	if(p_session->need_pong && ((gtv.tv_sec - p_session->last_ping) > 10))
	{
		if(p_session->ping_retries == 5)
		{
			purple_debug_info("gfire", "P2P: Session timed out!\n");
			gfire_buddy_p2p_timedout(p_session->buddy);

			return FALSE;
		}

		p_session->ping_retries++;
		purple_debug_misc("gfire", "P2P: Resending ping packet (try %d of 5)\n", p_session->ping_retries);
		gfire_p2p_session_send_ping(p_session);
	}

	// Keep-alive timeout check
	if(p_session->need_keep_alive && ((gtv.tv_sec - p_session->last_keep_alive) > 10))
	{
		if(p_session->keep_alive_retries == 5)
		{
			purple_debug_info("gfire", "P2P: Session timed out!\n");
			gfire_buddy_p2p_timedout(p_session->buddy);

			return FALSE;
		}

		p_session->keep_alive_retries++;
		purple_debug_misc("gfire", "P2P: Resending keep-alive packet (try %d of 5)\n", p_session->keep_alive_retries);
		gfire_p2p_session_send_keep_alive(p_session);
	}

	// Data timeout check
	GList *cur = p_session->req_ack_list;
	while(cur)
	{
		gfire_p2p_session_req_ack_item *item = cur->data;
		if((gtv.tv_sec - item->last_sent) > 10)
		{
			if(item->retries == 5)
			{
				purple_debug_info("gfire", "P2P: Session timed out!\n");
				gfire_buddy_p2p_timedout(p_session->buddy);

				return FALSE;
			}

			item->retries++;
			purple_debug_misc("gfire", "P2P: Resending data packet (try %d of 5)\n", item->retries);

			gfire_p2p_connection_send_packet(p_session->con, p_session, item->type, item->msgid, item->seqid, item->data, item->len, item->category, NULL);
			item->last_sent = gtv.tv_sec;
		}

		cur = g_list_next(cur);
	}

	// Keep-alive-handling
	if(gtv.tv_sec >= (p_session->last_keep_alive + 60))
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

	ret->moniker_self = g_malloc0(20);
	ret->moniker_peer = g_malloc0(20);

	ret->need_pong = FALSE;
	ret->need_keep_alive = FALSE;

	GTimeVal gtv;
	g_get_current_time(&gtv);
	ret->last_keep_alive = gtv.tv_sec;

	ret->ping_retries = 1;
	ret->keep_alive_retries = 1;

	ret->rec_msgids = gfire_bitlist_new();

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

	g_source_remove(p_session->check_timer);

	while(p_session->transfers)
	{
		gfire_filetransfer_free(p_session->transfers->data, p_local_reason);
		p_session->transfers = g_list_delete_link(p_session->transfers, p_session->transfers);
	}

	while(p_session->req_ack_list)
		gfire_p2p_session_req_ack_remove_item(p_session, p_session->req_ack_list->data);

	gfire_bitlist_free(p_session->rec_msgids);

	g_free(p_session->moniker_self);
	g_free(p_session->moniker_peer);

	g_free(p_session);
}

void gfire_p2p_session_bind(gfire_p2p_session *p_session, gfire_p2p_connection *p_p2p, gboolean p_init)
{
	if(!p_session || !p_p2p)
		return;

	p_session->con = p_p2p;

	p_session->init = p_init;
}

void gfire_p2p_session_set_addr(gfire_p2p_session *p_session, guint32 p_ip, guint16 p_port)
{
	if(!p_session)
		return;

	p_session->peer_ip = p_ip;
	p_session->peer_port = p_port;

	if(p_session->need_pong)
	{
		purple_debug_misc("gfire", "P2P: Sending handshake response\n");
		gfire_p2p_session_send_pong(p_session);
		p_session->need_pong = FALSE;
	}

	if(p_session->init)
	{
		// Send Ping as handshake
		purple_debug_misc("gfire", "P2P: Sending handshake\n");
		gfire_p2p_session_send_ping(p_session);
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

guint32 gfire_p2p_session_get_peer_ip(const gfire_p2p_session *p_session)
{
	if(!p_session)
		return 0;

	return p_session->peer_ip;
}

guint16 gfire_p2p_session_get_peer_port(const gfire_p2p_session *p_session)
{
	if(!p_session)
		return 0;

	return p_session->peer_port;
}

gfire_buddy *gfire_p2p_session_get_buddy(const gfire_p2p_session *p_session)
{
	if(!p_session)
		return NULL;

	return p_session->buddy;
}

static void gfire_p2p_session_req_ack_add(gfire_p2p_session *p_session, guint32 p_type, guint32 p_msgid, guint32 p_seqid, guint8 *p_data, guint32 p_len, const gchar *p_category)
{
	if(!p_session || (p_type != 0x00 && p_type != 0x300) || !p_data || !p_len || !p_category)
		return;

	gfire_p2p_session_req_ack_item *item = g_malloc0(sizeof(gfire_p2p_session_req_ack_item));
	if(!item)
		return;

	item->category = g_strdup(p_category);
	item->data = g_malloc0(p_len);
	memcpy(item->data, p_data, p_len);
	item->len = p_len;

	item->msgid = p_msgid;
	item->seqid = p_seqid;
	item->type = p_type;

	item->retries = 1;

	GTimeVal gtv;
	g_get_current_time(&gtv);
	item->last_sent = gtv.tv_sec;

	p_session->req_ack_list = g_list_append(p_session->req_ack_list, item);
}

static void gfire_p2p_session_req_ack_remove_item(gfire_p2p_session *p_session, gfire_p2p_session_req_ack_item *p_item)
{
	if(!p_session || !p_session->req_ack_list || !p_item)
		return;

	GList *list_item = g_list_find(p_session->req_ack_list, p_item);
	if(!list_item)
		return;

	if(p_item->data) g_free(p_item->data);
	if(p_item->category) g_free(p_item->category);

	g_free(p_item);

	p_session->req_ack_list = g_list_delete_link(p_session->req_ack_list, list_item);
}

static void gfire_p2p_session_send_ping(gfire_p2p_session *p_session)
{
	if(!p_session || !p_session->con)
		return;

	p_session->need_pong = TRUE;
	p_session->seqid = 0;

	GTimeVal gtv;
	g_get_current_time(&gtv);
	p_session->last_ping = gtv.tv_sec;

	if(p_session->ping_retries == 1)
	{
		p_session->ping_msgid = p_session->msgid;
		p_session->msgid++;
	}

	gfire_p2p_connection_send_packet(p_session->con, p_session, 0x10, p_session->ping_msgid, p_session->seqid, NULL, 0, NULL, NULL);
}

static void gfire_p2p_session_send_pong(gfire_p2p_session *p_session)
{
	if(!p_session || !p_session->con)
		return;

	p_session->seqid = 0;

	gfire_p2p_connection_send_packet(p_session->con, p_session, 0x20, p_session->msgid, p_session->seqid, NULL, 0, NULL, NULL);

	p_session->msgid++;
}

static void gfire_p2p_session_send_ack(gfire_p2p_session *p_session, guint32 p_msgid, guint32 p_seqid)
{
	if(!p_session || !p_session->con)
		return;

	gfire_p2p_connection_send_packet(p_session->con, p_session, 0x40, p_msgid, p_seqid, NULL, 0, NULL, NULL);
}

void gfire_p2p_session_send_invalid_crc(gfire_p2p_session *p_session, guint32 p_msgid, guint32 p_seqid)
{
	if(!p_session || !p_session->con)
		return;

	gfire_p2p_connection_send_packet(p_session->con, p_session, 0x80, p_msgid, p_seqid, NULL, 0, NULL, NULL);
}

static void gfire_p2p_session_send_keep_alive(gfire_p2p_session *p_session)
{
	if(!p_session || !p_session->con)
		return;

	p_session->need_keep_alive = TRUE;
	GTimeVal gtv;
	g_get_current_time(&gtv);
	p_session->last_keep_alive = gtv.tv_sec;
	if(p_session->keep_alive_retries == 1)
	{
		p_session->keep_alive_msgid = p_session->msgid;
		p_session->keep_alive_seqid = p_session->seqid;
	}

	gfire_p2p_connection_send_packet(p_session->con, p_session, 0x800, p_session->keep_alive_msgid, p_session->keep_alive_seqid, NULL, 0, NULL, NULL);
}

static void gfire_p2p_session_send_keep_alive_reply(gfire_p2p_session *p_session)
{
	if(!p_session || !p_session->con)
		return;

	gfire_p2p_connection_send_packet(p_session->con, p_session, 0x1000, p_session->msgid, p_session->seqid, NULL, 0, NULL, NULL);
}

void gfire_p2p_session_send_data32_packet(gfire_p2p_session *p_session, void *p_data, guint32 p_len, const gchar *p_category)
{
	if(!p_session || !p_session->con || !p_data || !p_len || !p_category)
		return;

	p_session->seqid++;

	gfire_p2p_connection_send_packet(p_session->con, p_session, 0x00, p_session->msgid, p_session->seqid, p_data, p_len, p_category, NULL);
	gfire_p2p_session_req_ack_add(p_session, 0x00, p_session->msgid, p_session->seqid, p_data, p_len, p_category);

	p_session->msgid++;
}

void gfire_p2p_session_send_data16_packet(gfire_p2p_session *p_session, void *p_data, guint16 p_len, const gchar *p_category)
{
	if(!p_session || !p_session->con || !p_data || !p_len || !p_category)
		return;

	p_session->seqid++;

	gfire_p2p_connection_send_packet(p_session->con, p_session, 0x300, p_session->msgid, p_session->seqid, p_data, p_len, p_category, NULL);
	gfire_p2p_session_req_ack_add(p_session, 0x300, p_session->msgid, p_session->seqid, p_data, p_len, p_category);

	p_session->msgid++;
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

void gfire_p2p_session_handle_data(gfire_p2p_session *p_session, guint32 p_type, guint32 p_msgid, guint32 p_seqid, void *p_data, guint32 p_len, const gchar *p_category)
{
	if(!p_session || (((p_type == 0) || (p_type == 0x300)) && (!p_data || !p_category)))
		return;

	// Check for duplicate messages
	if(p_type == 0 || p_type == 0x300)
	{
		if(gfire_bitlist_get(p_session->rec_msgids, p_msgid))
		{
			purple_debug_misc("gfire", "P2P: Received duplicate message, ignoring and acking\n");
			gfire_p2p_session_send_ack(p_session, p_msgid, p_seqid);
			return;
		}
	}

	// Ping
	if(p_type == 0x10)
	{
		if(p_session->peer_ip)
		{
			purple_debug_misc("gfire", "P2P: Got Ping, sending pong...\n");
			gfire_p2p_session_send_pong(p_session);
		}
		else
		{
			purple_debug_misc("gfire", "P2P: Got Handshake\n");
			p_session->need_pong = TRUE;
			p_session->connected = TRUE;
		}
	}
	// Pong
	else if(p_type == 0x20)
	{
		purple_debug_misc("gfire", "P2P: Got Pong\n");
		p_session->need_pong = FALSE;
		p_session->ping_retries = 1;
		p_session->connected = TRUE;
	}
	// Acknowledgement
	else if(p_type == 0x40)
	{
		GList *cur = p_session->req_ack_list;
		while(cur)
		{
			gfire_p2p_session_req_ack_item *item = cur->data;
			if(item->msgid == p_msgid && item->seqid == p_seqid)
			{
				purple_debug_misc("gfire", "P2P: Got Ack\n");
				gfire_p2p_session_req_ack_remove_item(p_session, item);

				break;
			}
			cur = g_list_next(cur);
		}

		if(!cur)
			purple_debug_warning("gfire", "P2P: Received Ack with invalid id!\n");
	}
	// Invalid CRC-32
	else if(p_type == 0x80)
	{
		purple_debug_warning("gfire", "P2P: Received invalid CRC32 packet!\n");
	}
	// Keep-alive
	else if(p_type == 0x800)
	{
		purple_debug_misc("gfire", "P2P: Received keep-alive; sending response\n");
		gfire_p2p_session_send_keep_alive_reply(p_session);
	}
	// Keep-alive response
	else if(p_type == 0x1000)
	{
		purple_debug_misc("gfire", "P2P: Received keep-alive response\n");
		p_session->need_keep_alive = FALSE;
		p_session->keep_alive_retries = 1;
	}
	// Data 32bit size
	else if(p_type == 0x00)
	{
		purple_debug_misc("gfire", "P2P: Received 32bit data packet\n");

		gfire_p2p_session_send_ack(p_session, p_msgid, p_seqid);

		if(g_utf8_collate(p_category, "DL") == 0)
		{
			purple_debug_misc("gfire", "P2P: received DL packet\n");
			gfire_p2p_dl_handler_handle(p_session, p_data, p_len);
		}
		else
			purple_debug_warning("gfire", "P2P: received unknown data packet (category \"%s\")\n", p_category);
	}
	// Data 16bit size
	else if(p_type == 0x300)
	{
		purple_debug_misc("gfire", "P2P: Received 16bit data packet\n");

		gfire_p2p_session_send_ack(p_session, p_msgid, p_seqid);

		if(g_utf8_collate(p_category, "IM") == 0)
		{
			purple_debug_misc("gfire", "P2P: received IM packet\n");
			gfire_p2p_im_handler_handle(p_session, p_data, p_len);
		}
		else
			purple_debug_warning("gfire", "P2P: received unknown data packet (category \"%s\")\n", p_category);
	}
	else
	{
		purple_debug_warning("gfire", "P2P: Received unknown packet type (%u)\n", p_type);
	}

	// Add this packet to the received ones if it wasn't a ack-packet (which uses the ID of our own packets)
	if(p_type != 0x40)
		gfire_bitlist_set(p_session->rec_msgids, p_msgid, TRUE);
}

void gfire_p2p_session_add_file_transfer(gfire_p2p_session *p_session, PurpleXfer *p_xfer)
{
	if(!p_session || !p_xfer)
		return;

	gfire_filetransfer *ft = gfire_filetransfer_init(p_session, p_xfer);
	if(!ft)
		return;

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
