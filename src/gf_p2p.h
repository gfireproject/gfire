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

#ifndef _GF_P2P_H
#define _GF_P2P_H

typedef struct _gfire_p2p_connection gfire_p2p_connection;

#include "gf_base.h"
#include "network.h"
#include "gf_p2p_session.h"

#define GFIRE_P2P_BUFFER_LEN 131072 // 128 KB
#define GFIRE_P2P_HEADER_LEN 7

struct _gfire_p2p_connection
{
	int socket;
	guint8 *buff_out;
	guint8 *buff_in;

	// Sessions bound to this connection
	GList *sessions;

	gint prpl_inpa;
};

// Creation/Freeing
gfire_p2p_connection *gfire_p2p_connection_create(guint16 p_minport, guint16 p_maxport);
void gfire_p2p_connection_close(gfire_p2p_connection *p_p2p);

// Status
gboolean gfire_p2p_connection_running(const gfire_p2p_connection *p_p2p);
guint32 gfire_p2p_connection_local_ip(const gfire_p2p_connection *p_p2p);
guint16 gfire_p2p_connection_port(const gfire_p2p_connection *p_p2p);
guint32 gfire_p2p_connection_ip(const gfire_p2p_connection *p_p2p);

// Sessions
void gfire_p2p_connection_add_session(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session, gboolean p_init);
void gfire_p2p_connection_remove_session(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session);

// Sending
guint8 *gfire_p2p_connection_send_packet(gfire_p2p_connection *p_p2p, const gfire_p2p_session *p_session, guint32 p_type, guint32 p_msgid, guint32 p_seqid, void *p_data, guint32 p_len, const gchar *p_category, guint32 *p_packet_len);
void gfire_p2p_connection_resend_packet(gfire_p2p_connection *p_p2p, gfire_p2p_session *p_session, void *p_data, guint32 p_len);

#endif // _GF_P2P_H
