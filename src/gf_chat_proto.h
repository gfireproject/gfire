/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009,	    Laurent De Marez <laurentdemarez@gmail.com>,
 *						    Warren Dumortier <nwarrenfl@gmail.com>,
 *						    Oliver Ney <oliver@dryder.de>
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

#ifndef _GF_CHAT_PROTO_H
#define _GF_CHAT_PROTO_H

#include "gf_base.h"
#include "gfire.h"

// Packet creation
guint16 gfire_chat_proto_create_join(const guint8 *p_id, const gchar *p_room, const gchar *p_pass);
guint16 gfire_chat_proto_create_leave(const guint8 *p_cid);
guint16 gfire_chat_proto_create_message(const guint8 *p_cid, const gchar *p_message);
guint16 gfire_chat_proto_create_invite(const guint8 *p_cid, guint32 p_userid);
guint16 gfire_chat_proto_create_change_motd(const guint8 *p_cid, const gchar* p_motd);
guint16 gfire_chat_proto_create_reject(const guint8 *p_cid);

// Packet parsing
void gfire_chat_proto_info(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_msg(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_user_leave(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_user_join(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_motd_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_buddy_permission_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_invite(gfire_data *p_gfire, guint16 p_packet_len);

#endif // _GF_CHAT_PROTO_H
