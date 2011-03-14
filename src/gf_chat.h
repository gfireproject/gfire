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

#ifndef _GF_CHAT_H
#define _GF_CHAT_H

typedef struct _gfire_chat gfire_chat;

#include "gf_base.h"
#include "gf_buddies.h"
#include "gf_chat_proto.h"
#include "gfire.h"

// GFIRE CHAT SYSTEM ////////////////////////////////////////////////
typedef enum
{
	Muted = 1,
	Normal = 2,
	Power_User = 3,
	Moderator = 4,
	Admin = 5
} gfire_chat_privilege;

typedef enum
{
	Public = 1,
	Friends = 2
} gfire_chat_visibility;

// Gfire Chat Struct
struct _gfire_chat
{
	gfire_data *owner;
	guint32 purple_id;		// purple chat id
	GList *members;			// glist of _gfire_buddy structs
	guint8 *chat_id;		// xfire chat id
	gchar *topic;			// channel topic
	gchar *motd;			// motd
	PurpleConversation *c;	// purple conv instance
	PurpleChat *purple_chat;// purple buddy list chat
	guint32 own_permission;
	guint32 def_permission;
	guint32 accessibility;
	gboolean secure;
	gboolean silenced;
	gboolean show_join_leave;
};

// Creation and freeing
gfire_chat *gfire_chat_create(gfire_data *p_owner, const guint8 *p_id, const gchar *p_topic,
							  const gchar *p_motd, gboolean p_force_add);
void gfire_chat_free(gfire_chat *p_chat);

// Member handling
gfire_buddy *gfire_chat_find_user(gfire_chat *p_chat, guint32 p_userid);
void gfire_chat_add_user(gfire_chat *p_chat, gfire_buddy *p_buddy, guint32 p_perm, gboolean p_joined);
void gfire_chat_user_left(gfire_chat *p_chat, guint32 p_userid, gboolean p_kicked);
void gfire_chat_buddy_permission_changed(gfire_chat *p_chat, guint32 p_userid, guint32 p_perm);

// Receiving and sending messages
void gfire_chat_got_msg(gfire_chat *p_chat, guint32 p_userid, const gchar *p_msg);
void gfire_chat_send(gfire_chat *p_chat, const gchar *p_msg);

// Joining and leaving
void gfire_chat_join(const guint8 *p_chat_id, const gchar *p_room, const gchar *p_pass, PurpleConnection *p_gc);
void gfire_chat_leave(gfire_chat *p_chat);

// Topic
void gfire_chat_set_topic(gfire_chat *p_chat, const gchar *p_topic, gboolean p_notify);

// MotD
void gfire_chat_set_motd(gfire_chat *p_chat, const gchar *p_motd, gboolean p_notify);
void gfire_chat_change_motd(gfire_chat *p_chat, const gchar *p_motd);

// Flags
//	Internal
void gfire_chat_set_default_permission(gfire_chat *p_chat, guint32 p_permission, gboolean p_notify);
void gfire_chat_set_accessibility(gfire_chat *p_chat, guint32 p_accessibility, gboolean p_notify);
void gfire_chat_set_secure(gfire_chat *p_chat, gboolean p_secure, gboolean p_notify);
void gfire_chat_set_silenced(gfire_chat *p_chat, gboolean p_silenced, gboolean p_notify);
void gfire_chat_set_show_join_leave(gfire_chat *p_chat, gboolean p_show, gboolean p_notify);

// Inviting
void gfire_chat_invite(gfire_chat *p_chat, const gfire_buddy *p_buddy);
void gfire_chat_reject(guint8 *p_chat_id, PurpleConnection *p_gc);

// Saving
void gfire_chat_set_saved(gfire_chat *p_chat, gboolean p_save);

// Purple
void gfire_chat_show(gfire_chat *p_chat);
void gfire_chat_set_purple_chat(gfire_chat *p_chat, PurpleChat *p_purple_chat);

// Chat commands
void gfire_chat_register_commands();
void gfire_chat_unregister_commands();

// Identification
gboolean gfire_chat_is_by_topic(const gfire_chat *p_chat, const gchar *p_topic);
gboolean gfire_chat_is_by_chat_id(const gfire_chat *p_chat, const guint8 *p_chat_id);
gboolean gfire_chat_is_by_purple_id(const gfire_chat *p_chat, const gint p_purple_id);
gboolean gfire_chat_is_by_purple_chat(const gfire_chat *p_chat, const PurpleChat *p_purple_chat);

// Internal
void gfire_chat_request_password_rejoin(gfire_data *p_gfire, guint8 *p_chat_id, gboolean p_password_given);

#endif // _GF_CHAT_H
