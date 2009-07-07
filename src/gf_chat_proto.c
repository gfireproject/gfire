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

#include "gf_protocol.h"
#include "gfire.h"
#include "gf_chat.h"
#include "gf_chat_proto.h"

guint16 gfire_chat_proto_create_join(const guint8 *p_id, const gchar *p_room, const gchar *p_pass)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if (!p_room || (strlen(p_room) == 0) || !p_id)
	{

		purple_debug(PURPLE_DEBUG_ERROR, "gfire",
					"gfire_chat_proto_create_join_chat: invalid parameter to _create_join_chat room=%s\n", NN(p_room));
		return 0;
	}

	guint32 climsg = 0xF44C0000;
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 6, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_id, XFIRE_CHATID_LEN, offset);

	guint32 requestIndex = 0;
	offset = gfire_proto_write_attr_bs(0x0B, 0x02, &requestIndex, sizeof(requestIndex), offset);

	guint32 chatRoomType = GUINT32_TO_LE(1);
	offset = gfire_proto_write_attr_bs(0xAA, 0x02, &chatRoomType, sizeof(chatRoomType), offset);

	offset = gfire_proto_write_attr_bs(0x05, 0x01, p_room, strlen(p_room), offset);

	offset = gfire_proto_write_attr_bs(0x5F, 0x01, p_pass, strlen(p_pass), offset);

	guint8 autoName = 0;
	offset = gfire_proto_write_attr_bs(0xA7, 0x08, &autoName, sizeof(autoName), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_leave(const guint8 *p_cid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if (!p_cid) return 0;

	guint32 climsg = 0xF54C0000;
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 1, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_message(const guint8 *p_cid, const gchar *p_message)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if (!p_message || !p_cid || (strlen(p_message) == 0)) return 0;

	guint32 climsg = 0xF64C0000;
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	offset = gfire_proto_write_attr_bs(0x10, 0x09, NULL, 2, offset);

	guint32 unknown = 0;
	offset = gfire_proto_write_attr_bs(0x0E, 0x02, &unknown, sizeof(unknown), offset);

	offset = gfire_proto_write_attr_bs(0x0F, 0x01, p_message, strlen(p_message), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_invite(const guint8 *p_cid, guint32 p_userid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if (!p_cid) return 0;

	guint32 climsg = 0xFC4C0000;
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	GList *users = g_list_append(NULL, &p_userid);
	offset = gfire_proto_write_attr_list_bs(0x18, users, 0x02, sizeof(p_userid), offset);
	g_list_free(users);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_change_motd(const guint8 *p_cid, const gchar* p_motd)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid || !p_motd || (strlen(p_motd) == 0)) return 0;

	guint32 climsg = 0x0C4D0000;
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	offset = gfire_proto_write_attr_bs(0x2E, 0x01, p_motd, strlen(p_motd), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_reject(const guint8 *p_cid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid) return 0;

	guint32 climsg = 0xFF4C0000;
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 1, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}


void gfire_chat_proto_info(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;
	gfire_chat *chat = NULL;
	guint8 *chat_id = NULL;
	gchar *topic = NULL;
	gchar *motd = NULL;
	guint32 allowVoiceChat = 0;
	guint32 defaultPerm = 0;
	guint32 timeStamp = 0;
	guint32 roomType = 0;

	gfire_buddy *m = NULL;
	GList *userids, *perms, *names, *nicks;
	GList *u, *p, *n, *a;

	u = p = n = a = NULL;
	userids = perms = names = nicks =  NULL;
	if (!p_gfire || (p_packet_len == 0)) return;

	// xfire chat id
	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	// chat topic / name
	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &topic, 0x05, offset);
	if(offset == -1 || !topic)
	{
		g_free(chat_id);
		return;
	}

	// Motd
	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &motd, 0x4D, offset);
	if(offset == -1 || !motd)
	{
		g_free(chat_id);
		g_free(topic);
		return;
	}

	chat = gfire_chat_create(chat_id, topic, motd);
	g_free(chat_id); g_free(topic); g_free(motd);
	if(!chat)
		return;

	// voice chat allowed
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &allowVoiceChat, 0x4E, offset);
	if(offset == -1)
	{
		gfire_chat_free(chat);
		return;
	}

	// default permission
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &defaultPerm, 0x49, offset);
	if(offset == -1)
	{
		gfire_chat_free(chat);
		return;
	}

	// time stamp?
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &timeStamp, 0x14, offset);
	if(offset == -1)
	{
		gfire_chat_free(chat);
		return;
	}

	// room type
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &roomType, 0x17, offset);
	if(offset == -1)
	{
		gfire_chat_free(chat);
		return;
	}

	// get user id's of members in chat
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &userids, 0x33, offset);
	if(offset == -1 || !userids)
	{
		gfire_chat_free(chat);
		return;
	}

	// get user permissions of members in chat
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &perms, 0x44, offset);
	if(offset == -1 || !perms)
	{
		//mem cleanup code
		gfire_chat_free(chat);
		if (userids) g_list_free(userids);
		return;
	}

	// get chat member usernames
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &names, 0x42, offset);
	if(offset == -1 || !names)
	{
		//mem cleanup code
		gfire_chat_free(chat);
		if (userids) g_list_free(userids);
		if (perms) g_list_free(perms);
		return;
	}

	// get chat member aliases
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &nicks, 0x43, offset);
	if(offset == -1 || !nicks)
	{
		//mem cleanup code
		gfire_chat_free(chat);
		if (userids) g_list_free(userids);
		if (perms) g_list_free(perms);
		if (names) g_list_free(names);
		return;
	}

	// packet has been parsed

	// Add it to gfire_data and tell purple about the chat
	gfire_add_chat(p_gfire, chat);
	gfire_chat_show(chat);

	n = names; p = perms; a = nicks; u = userids;

	while(u)
	{
		if(!gfire_is_self(p_gfire, *(guint32*)u->data))
		{
			m = gfire_buddy_create(*(guint32*)u->data, (gchar*)n->data, (gchar*)a->data, GFBT_GROUPCHAT);
			gfire_chat_add_member(chat, m, *(guint32*)p->data, FALSE);
		}

		g_free(u->data); g_free(n->data);
		g_free(a->data); g_free(p->data);
		u = g_list_next(u); n = g_list_next(n);
		a = g_list_next(a); p = g_list_next(p);
	}

	g_list_free(names); g_list_free(perms); g_list_free(nicks); g_list_free(userids);
}


void gfire_chat_proto_msg(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	guint32 userid = 0;
	gchar *msg = NULL;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_msg: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &userid, 0x01, offset);

	guint8 attr_len = 0;
	offset = gfire_proto_read_attr_children_count_bs(p_gfire->buff_in, &attr_len, 0x10, offset);

	guint32 unknown = 0;
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &unknown, 0x0E, offset);

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &msg, 0x0F, offset);

	gfire_chat_got_msg(chat, userid, msg);

	g_free(msg);
}


void gfire_chat_proto_user_leave(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	guint32 userid = 0;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_user_leave: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &userid, 0x01, offset);

	gfire_chat_user_left(chat, userid);
}


void gfire_chat_proto_user_join(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	guint32 userid = 0;
	guint8 *usersid = NULL;
	gchar *name = NULL;
	gchar *nick = NULL;
	guint32 perm = 0;
	gfire_buddy *gf_buddy = NULL;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_user_leave: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &userid, 0x01, offset);
	if(offset == -1)
		return;

	if(gfire_is_self(p_gfire, userid))
		return;

	offset = gfire_proto_read_attr_sid_bs(p_gfire->buff_in, &usersid, 0x11, offset);
	if(offset == -1 || !usersid)
		return;

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &name, 0x02, offset);
	if(offset == -1 || !name)
	{
		g_free(usersid);
		return;
	}

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &nick, 0x0D, offset);
	if(offset == -1 || !nick)
	{
		g_free(usersid);
		g_free(name);
		return;
	}

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &perm, 0x12, offset);
	if(offset == -1)
		return;

	gf_buddy = gfire_buddy_create(userid, name, nick, GFBT_GROUPCHAT);
	gfire_chat_add_member(chat, gf_buddy, perm, TRUE);

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "groupchat join, userid: %u, username: %s, alias: %s\n",
				 userid, NN(name), NN(nick));
}



void gfire_chat_proto_motd_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	gchar *motd = NULL;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_motd_change: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &motd, 0x2E, offset);
	if(offset == -1 || !motd)
		return;

	gfire_chat_motd_changed(chat, motd);
	g_free(motd);
}

void gfire_chat_proto_buddy_permission_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	guint32 userid = 0;
	guint32 perm = 0;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_buddy_permission_change: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &userid, 0x18, offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &perm, 0x13, offset);
	if(offset == -1)
		return;

	gfire_chat_buddy_permission_changed(chat, userid, perm);
}

void gfire_chat_proto_invite(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	guint32 unknown = 0;
	guint32 userid = 0;
	gchar *name = NULL;
	gchar *nick = NULL;
	gchar *room = NULL;
	GHashTable *components = NULL;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &unknown, 0xAA, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		return;
	}

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &userid, 0x01, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		return;
	}

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &name, 0x02, offset);
	if(offset == -1 || !name)
	{
		g_free(chat_id);
		return;
	}

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &nick, 0x0D, offset);
	if(offset == -1 || !nick)
	{
		g_free(chat_id);
		g_free(name);
		return;
	}

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &room, 0x05, offset);
	if(offset == -1 || !room)
	{
		g_free(chat_id);
		g_free(name);
		g_free(nick);
		return;
	}

	if(strlen(nick) == 0)
	{
		g_free(nick);
		nick = g_strdup(name);
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(chat invite): %s with alias %s invited us to room '%s'\n",
				NN(name), NN(nick), NN(room));

	// assemble ghashtable
	components = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	g_hash_table_replace(components, g_strdup("room"), room);
	g_hash_table_replace(components, g_strdup("inv_alias"), nick);
	g_hash_table_replace(components, g_strdup("inv_lid"), name);
	g_hash_table_replace(components, g_strdup("chat_id"), chat_id);
	g_hash_table_replace(components, g_strdup("inv_uid"), g_strdup_printf("%u", userid));
	g_hash_table_replace(components, g_strdup("members"), g_strdup_printf("%s\n", name));

	serv_got_chat_invite(gfire_get_connection(p_gfire), room, nick, "", components);
}
