/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
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

#include "gf_network.h"
#include "gf_chat.h"

gfire_chat *gfire_chat_create(const guint8 *p_id, const gchar *p_topic, const gchar *p_motd)
{
	gfire_chat *ret = g_malloc0(sizeof(gfire_chat));
	if(!ret)
		return NULL;

	ret->chat_id = g_malloc0(XFIRE_CHATID_LEN);
	if(!ret->chat_id)
	{
		g_free(ret);
		return NULL;
	}

	if(p_topic)
		ret->topic = g_strdup(p_topic);
	if(p_motd)
		ret->motd = g_strdup(p_motd);

	return ret;
}

void gfire_chat_free(gfire_chat *p_chat)
{
	if(!p_chat)
		return;

	if(p_chat->chat_id) g_free(p_chat->chat_id);
	if(p_chat->topic) g_free(p_chat->topic);
	if(p_chat->motd) g_free(p_chat->motd);

	GList *cur_member = p_chat->members;
	while(cur_member)
	{
		gfire_buddy_free((gfire_buddy*)cur_member->data);
		cur_member = g_list_next(cur_member);
	}

	if(p_chat->members) g_list_free(p_chat->members);
}

void gfire_chat_add_member(gfire_chat *p_chat, gfire_buddy *p_buddy, guint32 p_perm, gboolean p_joined)
{
	if(!p_chat || !p_buddy)
		return;

	PurpleConvChatBuddyFlags f;
	switch(p_perm)
	{
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

	purple_conv_chat_add_user(PURPLE_CONV_CHAT(p_chat->c), gfire_buddy_get_name(p_buddy), NULL, f, p_joined);

	p_buddy->chatperm = p_perm;
	p_chat->members = g_list_append(p_chat->members, p_buddy);
}

gfire_buddy *gfire_chat_find_member(gfire_chat *p_chat, guint32 p_userid)
{
	if(!p_chat)
		return NULL;

	GList *cur = p_chat->members;
	while(cur)
	{
		if(gfire_buddy_is_by_userid((gfire_buddy*)cur->data, p_userid))
			return (gfire_buddy*)cur->data;

		cur = g_list_next(cur);
	}

	return NULL;
}

void gfire_chat_got_msg(gfire_chat *p_chat, guint32 p_userid, const gchar *p_msg)
{
	if(!p_chat || !p_msg)
		return;

	gfire_buddy *m = gfire_chat_find_member(p_chat, p_userid);
	if(!m)
		return;

	gchar *escaped = gfire_escape_html(p_msg);
	serv_got_chat_in(purple_conversation_get_gc(p_chat->c), p_chat->purple_id, gfire_buddy_get_name(m),
					 PURPLE_MESSAGE_RECV, escaped, time(NULL));
	g_free(escaped);
}

void gfire_chat_user_left(gfire_chat *p_chat, guint32 p_userid)
{
	if(!p_chat)
		return;

	gfire_buddy *m = gfire_chat_find_member(p_chat, p_userid);
	if(!m)
		return;

	purple_conv_chat_remove_user(PURPLE_CONV_CHAT(p_chat->c), gfire_buddy_get_name(m), NULL);

	GList *cur = g_list_find(p_chat->members, m);
	if(!cur)
		return;

	gfire_buddy_free(m);
	p_chat->members = g_list_delete_link(p_chat->members, cur);
}

void gfire_chat_join(const guint8 *p_chat_id, const gchar *p_room, const gchar *p_pass, PurpleConnection *p_gc)
{
	if(!p_gc)
		return;

	if(!p_room)
		p_room = gfire_get_name((gfire_data *)p_gc->proto_data);


	guint16 packet_len = gfire_chat_proto_create_join(p_chat_id, p_room, p_pass);
	if(packet_len > 0)
	{
		gfire_send(p_gc, packet_len);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(chat): sending join info for room %s\n", NN(p_room));
		return;
	}
}

void gfire_chat_leave(gfire_chat *p_chat)
{
	if(!p_chat)
		return;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(group chat): leaving room: %s\n", NN(p_chat->topic));
	guint16 len = gfire_chat_proto_create_leave(p_chat->chat_id);
	if(len > 0) gfire_send(p_chat->gc, len);
}

void gfire_chat_reject(guint8 *p_chat_id, PurpleConnection *p_gc)
{
	if(!p_chat_id || !p_gc)
		return;

	guint16 packet_len = gfire_chat_proto_create_reject(p_chat_id);
	if(packet_len > 0)
	{
		gfire_send(p_gc, packet_len);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(chat): sending reject groupchat invitation\n");
	}
}

void gfire_chat_motd_changed(gfire_chat *p_chat, const gchar *p_motd)
{
	if(!p_chat || !p_motd)
		return;

	if(p_chat->motd) g_free(p_chat->motd);
	p_chat->motd = g_strdup(p_motd);

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "new motd for room %s: %s\n", p_chat->topic, p_motd);

	purple_conv_chat_set_topic(PURPLE_CONV_CHAT(p_chat->c), "", p_motd);
	gchar *tmpmsg = g_strdup_printf(N_("Today's message changed to:\n%s"), p_motd);
	purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
	g_free(tmpmsg);
}

void gfire_chat_buddy_permission_changed(gfire_chat *p_chat, guint32 p_userid, guint32 p_perm)
{
	if(!p_chat)
		return;

	gfire_buddy *gf_buddy = gfire_chat_find_member(p_chat, p_userid);
	if(!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_chat_buddy_permission_changed: Unknown buddy!\n");
		return;
	}

	PurpleConvChatBuddyFlags f;
	switch(p_perm)
	{
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

	purple_conv_chat_user_set_flags(PURPLE_CONV_CHAT(p_chat->c), gfire_buddy_get_name(gf_buddy), f);
	gf_buddy->chatperm = p_perm;
}

void gfire_chat_show(gfire_chat *p_chat)
{
	if(!p_chat)
		return;

	// Create conversation
	gchar *wnd_title = g_strdup_printf(N_("%s [Xfire Chat]"), p_chat->topic);
	p_chat->c = serv_got_joined_chat(p_chat->gc, p_chat->purple_id, wnd_title);
	g_free(wnd_title);

	// Set topic
	purple_conv_chat_set_topic(PURPLE_CONV_CHAT(p_chat->c), NULL, p_chat->topic);

	// Join message
	gchar *tmpmsg = g_strdup_printf(N_("You are now chatting in %s."), p_chat->topic);
	purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
	g_free(tmpmsg);

	// MotD
	if(p_chat->motd)
	{
		gchar *tmpmsg = g_strdup_printf(N_("Today's message:\n%s."), p_chat->motd);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}
}

void gfire_chat_invite(gfire_chat *p_chat, const gfire_buddy *p_buddy)
{
	if(!p_chat || !p_buddy)
		return;

	// need chat id, and buddy's userid for invite message
	guint32 len = gfire_chat_proto_create_invite(p_chat->chat_id, p_buddy->userid);
	if(len > 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(group chat): inviting %s to %s\n",
					NN(p_buddy->name), NN(p_chat->topic));
		gfire_send(p_chat->gc, len);
	}
}

void gfire_chat_send(gfire_chat *p_chat, const gchar *p_msg)
{
	if(!p_chat || !p_msg)
		return;

	guint16 len = gfire_chat_proto_create_message(p_chat->chat_id, p_msg);
	if(len > 0) gfire_send(p_chat->gc, len);
}

void gfire_chat_change_motd(gfire_chat *p_chat, const gchar *p_motd)
{
	if(!p_chat || !p_motd)
		return;
	
	guint16 len = gfire_chat_proto_create_change_motd(p_chat->chat_id, p_motd);
	if(len > 0) gfire_send(p_chat->gc, len);
}
