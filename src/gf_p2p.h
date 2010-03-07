/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
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

#ifndef _GF_P2P_H
#define _GF_P2P_H

typedef struct _gfire_p2p_packet_resend gfire_p2p_packet_resend;
typedef struct _gfire_p2p_connection gfire_p2p_connection;

#include "gf_base.h"
#include "network.h"
#include "gf_p2p_session.h"

#define GFIRE_P2P_BUFFER_LEN 131072 // 128 KB
#define GFIRE_P2P_HEADER_LEN 7
#define GFIRE_P2P_PACKET_TIMEOUT 5 // in sec
#define GFIRE_P2P_MAX_RETRIES 4 // packets will be sent max. +1 time

// P2P Packet Types
#define GFIRE_P2P_TYPE_DATA32			0x0000
#define GFIRE_P2P_TYPE_PING				0x0010
#define GFIRE_P2P_TYPE_PONG				0x0020
#define GFIRE_P2P_TYPE_ACK				0x0040
#define GFIRE_P2P_TYPE_BADCRC			0x0080
#define GFIRE_P2P_TYPE_DATA16			0x0300
#define GFIRE_P2P_TYPE_KEEP_ALIVE_REQ	0x0800
#define GFIRE_P2P_TYPE_KEEP_ALIVE_REP	0x1000

struct _gfire_p2p_packet_resend
{
	guint8 encoding;
	guint8 *moniker;
	guint32 type;
	guint32 msgid;
	guint32 seqid;
	guint32 data_len;
	guint8 *data;
	gchar *category;

	guint8 retries;
	glong last_try;

	gfire_p2p_session *session;
};

struct _gfire_p2p_connection
{
	// Input callback
	gint prpl_inpa;
	// Resend timeout
	guint resend_source;
	// Socket
	PurpleNetworkListenData *listen_data;
	int socket;
	// Buffers
	guint8 *buff_out;
	guint8 *buff_in;

	// State
	guint32 msgid;

	// Packets which are waiting to be resent (no ack yet)
	GList *packets;

	// Sessions bound to this connection
	GList *sessions;
};

// Creation/Freeing
gfire_p2p_connection *gfire_p2p_connection_create();
void gfire_p2p_connection_close(gfire_p2p_connection *p_p2p);

// Status
gboolean gfire_p2p_connection_running(const gfire_p2p_connection *p_p2p);
guint32 gfire_p2p_connection_local_ip(const gfire_p2p_connection *p_p2p);
guint16 gfire_p2p_connection_port(const gfire_p2p_connection *p_p2p);
guint32 gfire_p2p_connection_ip(const gfire_p2p_connection *p_p2p);

// Sessions
void gfire_p2p_connection_add_session(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session);
void gfire_p2p_connection_remove_session(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session);

// Sending
guint32 gfire_p2p_connection_send_ping(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
									   guint32 p_sessid, const struct sockaddr_in *p_addr);
guint32 gfire_p2p_connection_send_pong(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
									   guint32 p_sessid, const struct sockaddr_in *p_addr);
void gfire_p2p_connection_send_keep_alive(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
										  guint32 p_sessid, const struct sockaddr_in *p_addr);
void gfire_p2p_connection_send_keep_alive_reply(gfire_p2p_connection *p_p2p, const guint8 *p_moniker,
												guint32 p_sessid, const struct sockaddr_in *p_addr);
void gfire_p2p_connection_send_data16(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session,
									  guint8 p_encoding, const guint8 *p_moniker,
									  guint32 p_seqid, const guint8 *p_data, guint16 p_data_len,
									  const gchar *p_category, const struct sockaddr_in *p_addr);
void gfire_p2p_connection_send_data32(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session,
									  guint8 p_encoding, const guint8 *p_moniker,
									  guint32 p_seqid, const guint8 *p_data, guint32 p_data_len,
									  const gchar *p_category, const struct sockaddr_in *p_addr);

#endif // _GF_P2P_H
