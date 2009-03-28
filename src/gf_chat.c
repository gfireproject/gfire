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

#include "gfire.h"
#include "gf_packet.h"
#include "gf_chat.h"
#include "gf_network.h"

void gfire_join_chat(PurpleConnection *gc, GHashTable *components)
{
	gfire_data *gfire = NULL;
	guint8 *xid = NULL;
	gchar *room = NULL;
	gchar *name = NULL;
	gchar *pass = NULL;
	int packet_len = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !components) return;


	name = gfire->alias ? gfire->alias : (gchar *)purple_account_get_username(gc->account);
	room = g_hash_table_lookup(components, "room");
	pass = g_hash_table_lookup(components, "password");
	if (!(xid = g_hash_table_lookup(components, "chat_id"))) {
		/* no xid we need to create this room */
		purple_debug(PURPLE_DEBUG_MISC, "gfire","Attempting to create chat room %s\n", NN(room));
		xid = g_malloc0(XFIRE_CHATID_LEN);
		xid[0] = 0x00;
		xid[1] = 0x00;
		packet_len = gfire_create_join_chat(gc, xid, (room ? room : name), pass);
		g_free(xid);
	} else {
		packet_len = gfire_create_join_chat(gc, xid, name, pass);
	}

	if (packet_len > 0)	{
		gfire_send(gc, gfire->buff_out, packet_len);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(chat): sending join info for room %s\n", NN(room));
		return;
	}

	purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(chat join): failed to join room %s\n", NN(room));

}


void gfire_reject_chat(PurpleConnection *gc, GHashTable *components)
{
	gfire_data *gfire = NULL;
	guint8 *cid = NULL;
	int packet_len = 0;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !components) return;

	if ((cid = g_hash_table_lookup(components, "chat_id"))) packet_len = gfire_create_reject_chat(gc, cid);

	if (packet_len > 0)	{
		gfire_send(gc, gfire->buff_out, packet_len);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(chat): sending reject groupchat invitation\n");
		return;
	}

}


void gfire_read_chat_invite(PurpleConnection *gc, int packet_len)
{
	int index = 7;
	guint16 sl = 0;
	gfire_data *gfire = NULL;
	gchar chanid[XFIRE_CHATID_LEN + 1];
	gchar *inviter_uid = NULL;
	gchar *inviter_login = NULL;
	gchar *inviter_alias = NULL;
	gchar *room = NULL;
	GHashTable *components = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || (index > packet_len)) return;

	/* get chat id */
	memcpy(chanid, gfire->buff_in + index, XFIRE_CHATID_LEN);
	chanid[XFIRE_CHATID_LEN]=0x00;
	index += XFIRE_CHATID_LEN + 8;

	/* get user id of the person inviting us */
	inviter_uid = g_malloc0(XFIRE_USERID_LEN);
	memcpy(inviter_uid, gfire->buff_in + index, XFIRE_USERID_LEN);
	index += XFIRE_USERID_LEN + 2;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(chat invite): chatid: %s, inviter userid: %s\n",
				NN(chanid), NN(inviter_uid));

	/* get string len of login id of inviter */
	memcpy(&sl, gfire->buff_in + index, sizeof(sl));
	sl = GUINT16_FROM_LE(sl);
	index += sizeof(sl);

	if (0 == sl) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(chat invite): string len of loginID is 0!\n");
		return;
	}
	inviter_login = g_malloc0(sl + 1);
	memcpy(inviter_login, gfire->buff_in + index, sl);
	index += sl + 2;

	/* get nick of inviter if they have one */
	memcpy(&sl, gfire->buff_in + index, sizeof(sl));
	sl = GUINT16_FROM_LE(sl);
	index += sizeof(sl);

	if (sl > 0) {
		inviter_alias = g_malloc0(sl + 1);
		memcpy(inviter_alias, gfire->buff_in + index, sl);
	}
	index += sl + 2;

	/* get room name length */
	memcpy(&sl, gfire->buff_in + index, sizeof(sl));
	sl = GUINT16_FROM_LE(sl);
	index += sizeof(sl);
	if (sl > 0) {
		room = g_malloc0(sl + 1);
		memcpy(room, gfire->buff_in + index, sl);
	}
	
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(chat invite): %s with alias %s, invited us to room '%s'\n",
				NN(inviter_login), NN(inviter_alias), NN(room));

	/* assemble ghashtable */
	components = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	if (NULL != room) g_hash_table_replace(components, g_strdup("room"), room);
	if (NULL != inviter_alias) g_hash_table_replace(components, g_strdup("inv_alias"), inviter_alias);
	if (NULL != inviter_login) g_hash_table_replace(components, g_strdup("inv_lid"), inviter_login);
	g_hash_table_replace(components, g_strdup("chat_id"), g_strdup(chanid));
	g_hash_table_replace(components, g_strdup("inv_uid"), inviter_uid);
	g_hash_table_replace(components, g_strdup("members"), g_strdup_printf("%s\n", inviter_login));

	serv_got_chat_invite(gc, room, (inviter_alias ? inviter_alias : inviter_login), "", components);
//	serv_got_chat_invite(gc, room, inviter_login, "", components);

}


GList *gfire_chat_info(PurpleConnection *gc)
{
	GList *m = NULL;
	struct proto_chat_entry *pce;

	pce = g_new0(struct proto_chat_entry, 1);
	pce->label = "_Room:";
	pce->identifier = "room";
	pce->required = TRUE;
	m = g_list_append(m, pce);
	
	pce = g_new0(struct proto_chat_entry, 1);
	pce->label = "_Password:";
	pce->identifier = "password";
	pce->secret = TRUE;
	m = g_list_append(m, pce);

	return m;
}


GHashTable *gfire_chat_info_defaults(PurpleConnection *gc, const char *chat_name)
{
	GHashTable *defaults;

	defaults = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

	if (chat_name != NULL)
		g_hash_table_insert(defaults, "room", g_strdup(chat_name));

	return defaults;
}


char *gfire_get_chat_name(GHashTable *data)
{
	return g_strdup(g_hash_table_lookup(data, "room"));
}


void gfire_chat_invite(PurpleConnection *gc, int id, const char *message, const char *who)
{
	gfire_data *gfire = NULL;
	gfire_chat *gfchat = NULL;
	gfire_buddy *buddy = NULL;
	GList *t = NULL;
	int len = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !who) return;

	t = gfire_find_chat(gfire->chats, (gpointer *)GINT_TO_POINTER(id), GFFC_PURPLEID);
	if (!t || !(gfchat = (gfire_chat *)t->data)) return;

	t = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)who, GFFB_NAME);
	if (!t || !(buddy = (gfire_buddy *)t->data)) return;

	/* need chat id, and buddy's sid for invite message */
	len = gfire_create_chat_invite(gc, gfchat->chat_id, buddy->userid);
	if (len) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(group chat): inviting %s to %s\n",
					NN(buddy->name), NN(gfchat->topic));
		gfire_send(gc, gfire->buff_out, len);
	}
}


void gfire_chat_leave(PurpleConnection *gc, int id)
{
	gfire_data *gfire = NULL;
	gfire_chat *gfchat = NULL;
	GList *t = NULL;
	int len = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	t = gfire_find_chat(gfire->chats, (gpointer *)GINT_TO_POINTER(id), GFFC_PURPLEID);
	if (!t || !(gfchat = (gfire_chat *)t->data)) return;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(group chat): leaving room: %s\n", NN(gfchat->topic));
	len = gfire_create_chat_leave(gc, gfchat->chat_id);
	if (len) gfire_send(gc, gfire->buff_out, len);
/* FIXME: add cleanup code to chat leave() */
	return;
}


int gfire_chat_send(PurpleConnection *gc, int id, const char *message, PurpleMessageFlags flags)
{
	gfire_data *gfire = NULL;
	gfire_chat *gfchat = NULL;
	GList *t = NULL;
	int len = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !message) return -1;

	t = gfire_find_chat(gfire->chats, (gpointer *)GINT_TO_POINTER(id), GFFC_PURPLEID);
	if (!t || !(gfchat = (gfire_chat *)t->data)) return -1;

	message = purple_unescape_html(message);

	if ((len = gfire_create_chat_message(gc, gfchat->chat_id, message))) {
		gfire_send(gc, gfire->buff_out, len);
		return 1;
	}

	return -1;
}


void gfire_chat_joined(PurpleConnection *gc, GList *members, guint8 *chat_id, gchar *topic, gchar *motd)
{
	gchar *tmpmsg = NULL;
	gfire_buddy *m = NULL;
	gfire_chat *gfchat = NULL;
	GList *cl = NULL;
	gfire_data *gfire = NULL;
	PurpleConversation *c = NULL;
	PurpleConvChatBuddyFlags f;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !chat_id) return;

	cl = gfire_find_chat(gfire->chats, (gpointer *)chat_id, GFFC_CID);

	if (!cl || !(gfchat = (gfire_chat *)cl->data)) {
		gfchat = g_new0(gfire_chat, 1);
		gfchat->purple_id = gfire->chat;
		gfchat->chat_id = chat_id;
		gfire->chat++;
		gfire->chats = g_list_append(gfire->chats, gfchat);
	}

	gfchat->members = members;
	gfchat->topic = topic;
	gfchat->motd = motd;
	//prefix the window title with "xfire groupchat-" to get arround a nasty 1.5 bug that causes a crash
	//if groupchat window title matches users username.
	c = gfchat->c = serv_got_joined_chat(gc, gfchat->purple_id, g_strdup_printf("xfire groupchat-%s",topic));
	purple_conv_chat_set_topic(PURPLE_CONV_CHAT(c), NULL, topic);

	tmpmsg = g_strdup_printf("You are now chatting in %s.", topic);
	purple_conv_chat_write(PURPLE_CONV_CHAT(c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
	g_free(tmpmsg);

	if (NULL != motd) {
		purple_conv_chat_set_topic(PURPLE_CONV_CHAT(c), "", motd);
		tmpmsg = g_strdup_printf("Today's Message:\n%s", motd);
		purple_conv_chat_write(PURPLE_CONV_CHAT(c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}
	
	for (cl = members; NULL != cl; cl = g_list_next(cl)) {
		m = (gfire_buddy *)cl->data;
		if (!m) continue;
		switch(m->chatperm) {
			case 01:
				f = PURPLE_CBFLAGS_NONE;
			break;
				
			case 02:
				f = PURPLE_CBFLAGS_NONE;
			break;
			
			case 03:
				f = PURPLE_CBFLAGS_VOICE;
			break;
			
			case 04:
				f = PURPLE_CBFLAGS_HALFOP;
			break;
			
			case 05:
				f = PURPLE_CBFLAGS_OP;
			break;
			
			default:
				f = PURPLE_CBFLAGS_NONE;
		}
		purple_conv_chat_add_user(PURPLE_CONV_CHAT(c), m->name, NULL, f, FALSE);

	}	

	return;
}


GList *gfire_find_chat(GList *chats, gpointer *data, int mode)
{
	GList *t = chats;
	gfire_chat *c = NULL;

	if (!chats) return NULL;

	switch (mode)
	{
		case GFFC_CID:
			while (NULL != t){
				c = (gfire_chat *)t->data;
				if ((NULL != c->chat_id) && (memcmp(c->chat_id, data, XFIRE_CHATID_LEN) == 0))
					break;
				t = g_list_next(t);
			}
		break;

		case GFFC_PURPLEID:
			while (NULL != t){
				c = (gfire_chat *)t->data;
				if ( c->purple_id == GPOINTER_TO_INT(data))
					break;
				t = g_list_next(t);
			}
		break;

		default:
			purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_find_chat() unknown mode specified\n");
			return NULL;
	}

	return t;
}


void gfire_chat_got_msg(PurpleConnection *gc, gfire_chat_msg *gcm)
{
	gfire_data *gfire = NULL;
	gfire_chat *gfchat = NULL;
	GList *t = NULL;
	gfire_buddy *m = NULL;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !gfire->chats
		|| !gcm || !(gcm->chat_id) ) return;

	t = gfire_find_chat(gfire->chats, (gpointer *)gcm->chat_id, GFFC_CID);
	if (t && (gfchat = (gfire_chat *)t->data)) {

		t = gfire_find_buddy_in_list(gfchat->members, (gpointer *)gcm->uid, GFFB_UIDBIN);
		if ( t && (m = (gfire_buddy *)t->data)) {
			serv_got_chat_in(gc, gfchat->purple_id, m->name, PURPLE_MESSAGE_RECV,
								gfire_escape_html(gcm->im_str), time(NULL));
		}
	}
	//free gcm
	if (gcm->chat_id) g_free(gcm->chat_id);
	if (gcm->uid) g_free(gcm->uid);
	if (gcm->im_str) g_free(gcm->im_str);
	g_free(gcm);
}



void gfire_chat_user_leave(PurpleConnection *gc, gfire_chat_msg *gcm)
{
	gfire_data *gfire = NULL;
	gfire_chat *gfchat = NULL;
	GList *t = NULL;
	gfire_buddy *m = NULL;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !gfire->chats
		|| !gcm || !(gcm->chat_id)) return;

	t = gfire_find_chat(gfire->chats, (gpointer *)gcm->chat_id, GFFC_CID);
	if (t && (gfchat = (gfire_chat *)t->data)) {

		t = gfire_find_buddy_in_list(gfchat->members, (gpointer *)gcm->uid, GFFB_UIDBIN);
		if ( t && (m = (gfire_buddy *)t->data)) {
			purple_conv_chat_remove_user(PURPLE_CONV_CHAT(gfchat->c), m->name, NULL);
		}
	}
	/* free gcm */
	if (gcm->chat_id) g_free(gcm->chat_id);
	if (gcm->uid) g_free(gcm->uid);
	if (gcm->im_str) g_free(gcm->im_str);
	g_free(gcm);
}


void gfire_chat_user_join(PurpleConnection *gc, gfire_chat_msg *gcm)
{
	gfire_data *gfire = NULL;
	gfire_chat *gfchat = NULL;
	GList *t = NULL;
	gfire_buddy *m = NULL;
	PurpleConvChatBuddyFlags f;
	
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !gfire->chats
		|| !gcm ||!(gcm->chat_id)) return;
		
	t = gfire_find_chat(gfire->chats, (gpointer *)gcm->chat_id, GFFC_CID);
	if (t && (gfchat = (gfire_chat *)t->data)) {
		m = gcm->b;
		/* we need to supress our own join messages, otherwise we show up on the userlist twice */
		if (memcmp(m->userid, gfire->userid, XFIRE_USERID_LEN) != 0) {
			switch(m->chatperm) {
				case 01:
					f = PURPLE_CBFLAGS_NONE;
				break;
					
				case 02:
					f = PURPLE_CBFLAGS_NONE;
				break;
			
				case 03:
					f = PURPLE_CBFLAGS_VOICE;
				break;
			
				case 04:
					f = PURPLE_CBFLAGS_HALFOP;
				break;
			
				case 05:
					f = PURPLE_CBFLAGS_OP;
				break;
			
				default:
					f = PURPLE_CBFLAGS_NONE;
		}
			purple_conv_chat_add_user(PURPLE_CONV_CHAT(gfchat->c), m->name, NULL, f, TRUE);
			gfchat->members = g_list_append(gfchat->members, m);
		} else {
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(group chat): supressing own join message\n");
			/* free the buddy we already have it in the list */
			if (NULL != m->name) g_free(m->name);
			if (NULL != m->alias) g_free(m->alias);
			if (NULL != m->userid) g_free(m->userid);
			g_free(m);
		}
	}
	
	/* free gcm */
	if (gcm->chat_id) g_free(gcm->chat_id);
	if (gcm->uid) g_free(gcm->uid);
	if (gcm->im_str) g_free(gcm->im_str);
	g_free(gcm);

}

void gfire_chat_change_motd(PurpleConnection *gc, int id, const char *topic)
{
	gfire_data *gfire = NULL;
	gfire_chat *gfchat = NULL;
	GList *t = NULL;
	int len = 0;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !topic) return -1;

	t = gfire_find_chat(gfire->chats, (gpointer *)GINT_TO_POINTER(id), GFFC_PURPLEID);
	if (!t || !(gfchat = (gfire_chat *)t->data)) return -1;

	topic = purple_unescape_html(topic);

	if ((len = gfire_create_change_motd(gc, gfchat->chat_id, topic))) {
		gfire_send(gc, gfire->buff_out, len);
		return 1;
	}

	return -1;

}

