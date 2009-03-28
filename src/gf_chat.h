/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef _GF_CHAT_H
#define _GF_CHAT_H

#include "gfire.h"

typedef struct _gfire_chat	gfire_chat;

struct _gfire_chat {
	int purple_id;			/* purple chat id */
	GList *members;			/* glist of _gfire_buddy structs */
	guint8 *chat_id;		/* xfire chat id */
	gchar *topic;			/* channel topic */
	gchar *motd;			/* motd */
	PurpleConversation *c;	/* purple conv instance */
};

/* gfire_find_chat() modes */
#define GFFC_CID	0
#define GFFC_PURPLEID	1


void gfire_join_chat(PurpleConnection *gc, GHashTable *components);
void gfire_reject_chat(PurpleConnection *gc, GHashTable *components);

void gfire_read_chat_invite(PurpleConnection *gc, int packet_len);
GList *gfire_chat_info(PurpleConnection *gc);
GHashTable *gfire_chat_info_defaults(PurpleConnection *gc, const char *chat_name);
char *gfire_get_chat_name(GHashTable *data);
void gfire_chat_invite(PurpleConnection *gc, int id, const char *message, const char *who);
void gfire_chat_leave(PurpleConnection *gc, int id);
int gfire_chat_send(PurpleConnection *gc, int id, const char *message, PurpleMessageFlags flags);
void gfire_chat_joined(PurpleConnection *gc, GList *members, guint8 *chat_id, gchar *topic, gchar *motd);
GList *gfire_find_chat(GList *chats, gpointer *data, int mode);
void gfire_chat_got_msg(PurpleConnection *gc, gfire_chat_msg *gcm);
void gfire_chat_user_leave(PurpleConnection *gc, gfire_chat_msg *gcm);
void gfire_chat_user_join(PurpleConnection *gc, gfire_chat_msg *gcm);
void gfire_chat_change_motd(PurpleConnection *gc, int id, const char *topic);

#endif /* _GF_CHAT_H */
