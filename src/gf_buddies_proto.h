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

#ifndef _GF_BUDDIES_PROTO_H
#define _GF_BUDDIES_PROTO_H

#include "gf_base.h"
#include "gf_games.h"
#include "gfire.h"

// Packet creation
guint16 gfire_buddy_proto_create_advanced_info_request(guint32 p_userid);
guint16 gfire_buddy_proto_create_typing_notification(const guint8 *p_sid, guint32 p_imindex, gboolean p_typing);
guint16 gfire_buddy_proto_create_send_im(const guint8 *p_sid, guint32 p_imindex, const gchar *p_msg);
guint16 gfire_buddy_proto_create_fof_request(GList *p_sids);
guint16 gfire_buddy_proto_create_ack(const guint8 *p_sid, guint32 p_imindex);

// Packet parsing
void gfire_buddy_proto_on_off(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_buddy_proto_game_status(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_buddy_proto_voip_status(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_buddy_proto_status_msg(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_buddy_proto_alias_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_buddy_proto_changed_avatar(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_buddy_proto_clans(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_buddy_proto_im(gfire_data *p_gfire, guint16 packet_len);
void gfire_buddy_proto_fof_list(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_buddy_proto_clan_alias_change(gfire_data *p_gfire, guint16 p_packet_len);

#endif // _GF_BUDDIES_PROTO_H
