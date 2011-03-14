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

#ifndef _GF_CHAT_PROTO_H
#define _GF_CHAT_PROTO_H

#include "gf_base.h"
#include "gfire.h"

// Packet creation
guint16 gfire_chat_proto_create_join(const guint8 *p_id, const gchar *p_room, const gchar *p_pass);
guint16 gfire_chat_proto_create_leave(const guint8 *p_cid);
guint16 gfire_chat_proto_create_message(const guint8 *p_cid, const gchar *p_message);
guint16 gfire_chat_proto_create_invite(const guint8 *p_cid, guint32 p_userid);
guint16 gfire_chat_proto_create_request_persistent_infos(GList *p_cids);
guint16 gfire_chat_proto_create_change_motd(const guint8 *p_cid, const gchar* p_motd);
guint16 gfire_chat_proto_create_reject(const guint8 *p_cid);
guint16 gfire_chat_proto_create_save_chat_room(const guint8 *p_cid, gboolean p_save);
guint16 gfire_chat_proto_create_change_topic(const guint8 *p_cid, const gchar *p_topic);
guint16 gfire_chat_proto_create_kick_buddy(const guint8 *p_cid, guint32 p_userid);
guint16 gfire_chat_proto_create_change_buddy_permissions(const guint8 *p_cid, guint32 p_userid, guint32 p_permission);
guint16 gfire_chat_proto_create_set_default_permission(const guint8 *p_cid, guint32 p_permission);
guint16 gfire_chat_proto_create_change_password(const guint8 *p_cid, const gchar *p_password);
guint16 gfire_chat_proto_create_change_access(const guint8 *p_cid, guint32 p_access);
guint16 gfire_chat_proto_create_change_silenced(const guint8 *p_cid, gboolean p_silenced);
guint16 gfire_chat_proto_create_change_show_join_leave(const guint8 *p_cid, gboolean p_show);

// Packet parsing
void gfire_chat_proto_persistent_chats(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_persistent_chat_infos(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_join_info(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_room_info(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_msg(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_user_leave(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_user_join(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_motd_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_buddy_permission_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_invite(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_topic_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_buddy_kicked(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_default_permission_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_password_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_accessibility_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_silenced_change(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_chat_proto_show_join_leave_change(gfire_data *p_gfire, guint16 p_packet_len);

#endif // _GF_CHAT_PROTO_H
