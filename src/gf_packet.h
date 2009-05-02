/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,		Laurent De Marez <laurentdemarez@gmail.com>
 *
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
#ifndef _GF_PACKET_H
#define _GF_PACKET_H

#include "gfire.h"

void gfire_add_header(guint8 *packet, int length, int type, int atts);
int gfire_add_att_name(guint8 *packet,int packet_length, char *att);
void gfire_packet_130(PurpleConnection *gc, int packet_len);
void gfire_packet_131(PurpleConnection *gc, int packet_len);
int gfire_send_auth(PurpleConnection *gc, int packet_len, int packet_id);
GList *gfire_read_buddy_online(PurpleConnection *gc, int packet_len);
int gfire_get_im(PurpleConnection *gc, int packet_len);
int gfire_create_im(PurpleConnection *gc, gfire_buddy *buddy, const char *msg);
GList *gfire_game_status(PurpleConnection *gc, int packet_len);
int gfire_ka_packet_create(PurpleConnection *gc);
GList *gfire_read_buddy_status(PurpleConnection *gc, int packet_len);
GList *gfire_read_invitation(PurpleConnection *gc, int packet_len);
int gfire_invitation_deny(PurpleConnection *gc, char *name);
int gfire_invitation_accept(PurpleConnection *gc, char *name);
int gfire_add_buddy_create(PurpleConnection *gc, char *name);
int gfire_remove_buddy_create(PurpleConnection *gc, gfire_buddy *b);
int gfire_create_change_alias(PurpleConnection *gc, char *name);
int gfire_join_game_create(PurpleConnection *gc, int game, int port, const char *ip);
int gfire_request_avatar_info(PurpleConnection *gc, gfire_buddy *b);
//void gfire_read_avatar_info(PurpleConnection *gc, int packet_len);
int gfire_send_typing_packet(PurpleConnection *gc, gfire_buddy *buddy);
void gfire_read_alias_change(PurpleConnection *gc, int packet_len);
int gfire_create_join_chat(PurpleConnection *gc, gchar *id, gchar *room, gchar *pass);
GList *gfire_read_chat_info(PurpleConnection *gc, int packet_len, gchar **rtop, gchar **rmotd, guint8 **rcid);
gfire_chat_msg *gfire_read_chat_msg(PurpleConnection *gc, int packet_len);
int gfire_create_chat_message(PurpleConnection *gc, guint8 *cid, const char *message);
gfire_chat_msg *gfire_read_chat_user_leave(PurpleConnection *gc, int packet_len);
int gfire_create_chat_leave(PurpleConnection *gc, const guint8 *cid);
gfire_chat_msg *gfire_read_chat_user_join(PurpleConnection *gc, int packet_len);
int gfire_create_chat_invite(PurpleConnection *gc, const guint8 *cid, const guint8 *sid);
int gfire_create_change_motd(PurpleConnection *gc, const guint8 *cid, gchar* motd);
void gfire_read_chat_motd_change(PurpleConnection *gc, int packet_len);
void read_groupchat_buddy_permission_change(PurpleConnection *gc, int packet_len);
int gfire_create_reject_chat(PurpleConnection *gc, const guint8 *cid);
void gfire_read_clan_blist(PurpleConnection *gc, int packet_len);
int gfire_create_serverlist_request (PurpleConnection *gc, int game);
void gfire_read_serverlist(PurpleConnection *gc, int packet_len);

#endif /* _GF_PACKET_H */
