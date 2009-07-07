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

#ifndef _GFIRE_PROTO_H
#define _GFIRE_PROTO_H

#include "gf_base.h"
#include "gf_network.h"
#include "gf_protocol.h"
#include "gf_games.h"
#include "gfire.h"

// Packet creation
guint16 gfire_proto_create_auth(const gchar *p_name, const gchar *p_pw_hash);
guint16 gfire_proto_create_collective_statistics(const gchar *p_lang, const gchar *p_skin, const gchar *p_theme, const gchar *p_partner);
guint16 gfire_proto_create_client_version(guint32 p_version);
guint16 gfire_proto_create_status_text(const gchar *p_status);
guint16 gfire_proto_create_keep_alive();
guint16 gfire_proto_create_invitation(const gchar *p_name, const gchar *p_msg);
guint16 gfire_proto_create_invitation_reject(const gchar *p_name);
guint16 gfire_proto_create_invitation_accept(const gchar *p_name);
guint16 gfire_proto_create_delete_buddy(guint32 p_userid);
guint16 gfire_proto_create_change_alias(const gchar *p_alias);
guint16 gfire_proto_create_join_game(const gfire_game_data *p_game);
guint16 gfire_proto_create_join_voip(const gfire_game_data *p_voip);

// Packet parsing
void gfire_proto_buddy_list(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_proto_login_salt(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_proto_session_info(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_proto_invitation(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_proto_clan_leave(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_proto_clan_list(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_proto_clan_blist(gfire_data *p_gfire, gint16 p_packet_len);

#endif // _GFIRE_PROTO_H
