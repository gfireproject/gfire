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

#ifndef _GF_CHAT_H
#define _GF_CHAT_H

#include "gf_base.h"
#include "gf_buddies.h"
#include "gf_chat_proto.h"

// GFIRE CHAT SYSTEM ////////////////////////////////////////////////
// Gfire Chat Struct
typedef struct _gfire_chat
{
	PurpleConnection *gc;
	guint32 purple_id;		// purple chat id
	GList *members;			// glist of _gfire_buddy structs
	guint8 *chat_id;		// xfire chat id
	gchar *topic;			// channel topic
	gchar *motd;			// motd
	PurpleConversation *c;	// purple conv instance
} gfire_chat;

// Creation and freeing
gfire_chat *gfire_chat_create(const guint8 *p_id, const gchar *p_topic, const gchar *p_motd);
void gfire_chat_free(gfire_chat *p_chat);

// Member handling
gfire_buddy *gfire_chat_find_user(gfire_chat *p_chat, guint32 p_userid);
void gfire_chat_add_user(gfire_chat *p_chat, gfire_buddy *p_buddy, guint32 p_perm, gboolean p_joined);
void gfire_chat_user_left(gfire_chat *p_chat, guint32 p_userid);
void gfire_chat_buddy_permission_changed(gfire_chat *p_chat, guint32 p_userid, guint32 p_perm);

// Receiving and sending messages
void gfire_chat_got_msg(gfire_chat *p_chat, guint32 p_userid, const gchar *p_msg);
void gfire_chat_send(gfire_chat *p_chat, const gchar *p_msg);

// Joining and leaving
void gfire_chat_join(const guint8 *p_chat_id, const gchar *p_room, const gchar *p_pass, PurpleConnection *p_gc);
void gfire_chat_leave(gfire_chat *p_chat);

// MotD
void gfire_chat_motd_changed(gfire_chat *p_chat, const gchar *p_motd);
void gfire_chat_change_motd(gfire_chat *p_chat, const gchar *p_motd);

// Inviting
void gfire_chat_invite(gfire_chat *p_chat, const gfire_buddy *p_buddy);
void gfire_chat_reject(guint8 *p_chat_id, PurpleConnection *p_gc);

// Purple
void gfire_chat_show(gfire_chat *p_chat);

#endif // _GF_CHAT_H
