/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009-2010  Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009-2010  Oliver Ney <oliver@dryder.de>
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
#include "gf_network.h"

guint16 gfire_chat_proto_create_join(const guint8 *p_id, const gchar *p_room, const gchar *p_pass)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if (!p_room || (strlen(p_room) == 0) || !p_id)
	{

		purple_debug(PURPLE_DEBUG_ERROR, "gfire",
					"gfire_chat_proto_create_join_chat: invalid parameter to gfire_chat_proto_create_join room=%s\n", NN(p_room));
		return 0;
	}

	guint32 climsg = GUINT32_TO_LE(0x4CF4);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 6, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_id, XFIRE_CHATID_LEN, offset);

	guint32 requestIndex = GUINT32_TO_LE(1);
	offset = gfire_proto_write_attr_bs(0x0B, 0x02, &requestIndex, sizeof(requestIndex), offset);

	guint32 chatRoomType = GUINT32_TO_LE(1);
	offset = gfire_proto_write_attr_bs(0xAA, 0x02, &chatRoomType, sizeof(chatRoomType), offset);

	offset = gfire_proto_write_attr_bs(0x05, 0x01, p_room, strlen(p_room), offset);

	offset = gfire_proto_write_attr_bs(0x5F, 0x01, p_pass, p_pass ? strlen(p_pass) : 0, offset);

	guint8 autoName = 0;
	offset = gfire_proto_write_attr_bs(0xA7, 0x08, &autoName, sizeof(autoName), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_leave(const guint8 *p_cid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if (!p_cid) return 0;

	guint32 climsg = GUINT32_TO_LE(0x4CF5);
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

	guint32 climsg = GUINT32_TO_LE(0x4CF6);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	offset = gfire_proto_write_attr_bs(0x2E, 0x01, p_message, strlen(p_message), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_invite(const guint8 *p_cid, guint32 p_userid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if (!p_cid) return 0;

	guint32 climsg = GUINT32_TO_LE(0x4CFC);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	GList *users = g_list_append(NULL, &p_userid);
	offset = gfire_proto_write_attr_list_bs(0x18, users, 0x02, sizeof(p_userid), offset);
	g_list_free(users);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_request_persistent_infos(GList *p_cids)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cids)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4CFA);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 1, offset);

	offset = gfire_proto_write_attr_list_bs(0x04, p_cids, 0x06, XFIRE_CHATID_LEN, offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_change_motd(const guint8 *p_cid, const gchar* p_motd)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid || !p_motd || (strlen(p_motd) == 0)) return 0;

	guint32 climsg = GUINT32_TO_LE(0x4D0C);
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

	if(!p_cid)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4CFF);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 1, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_save_chat_room(const guint8 *p_cid, gboolean p_save)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4CFD);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	guint8 save = p_save ? 1 : 0;
	offset = gfire_proto_write_attr_bs(0x2A, 0x08, &save, sizeof(save), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_change_topic(const guint8 *p_cid, const gchar *p_topic)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid || !p_topic)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4CF8);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	offset = gfire_proto_write_attr_bs(0x05, 0x01, p_topic, strlen(p_topic), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_kick_buddy(const guint8 *p_cid, guint32 p_userid)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4CFB);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	p_userid = GUINT32_TO_LE(p_userid);
	offset = gfire_proto_write_attr_bs(0x18, 0x02, &p_userid, sizeof(p_userid), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_change_buddy_permissions(const guint8 *p_cid, guint32 p_userid, guint32 p_permission)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4CF9);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 3, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	p_userid = GUINT32_TO_LE(p_userid);
	offset = gfire_proto_write_attr_bs(0x18, 0x02, &p_userid, sizeof(p_userid), offset);

	p_permission = GUINT32_TO_LE(p_permission);
	offset = gfire_proto_write_attr_bs(0x13, 0x02, &p_permission, sizeof(p_permission), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_set_default_permission(const guint8 *p_cid, guint32 p_permission)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4D08);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	p_permission = GUINT32_TO_LE(p_permission);
	offset = gfire_proto_write_attr_bs(0x13, 0x02, &p_permission, sizeof(p_permission), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_change_password(const guint8 *p_cid, const gchar *p_password)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid || !p_password)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4D15);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	offset = gfire_proto_write_attr_bs(0x5F, 0x01, p_password, strlen(p_password), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_change_access(const guint8 *p_cid, guint32 p_access)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4D16);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	p_access = GUINT32_TO_LE(p_access);
	offset = gfire_proto_write_attr_bs(0x17, 0x02, &p_access, sizeof(p_access), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_change_silenced(const guint8 *p_cid, gboolean p_silenced)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4D17);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	guint8 silenced = p_silenced ? 1 : 0;
	offset = gfire_proto_write_attr_bs(0x16, 0x08, &silenced, sizeof(silenced), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

guint16 gfire_chat_proto_create_change_show_join_leave(const guint8 *p_cid, gboolean p_show)
{
	guint32 offset = XFIRE_HEADER_LEN;

	if(!p_cid)
		return 0;

	guint32 climsg = GUINT32_TO_LE(0x4D18);
	offset = gfire_proto_write_attr_ss("climsg", 0x02, &climsg, sizeof(climsg), offset);

	offset = gfire_proto_write_attr_ss("msg", 0x09, NULL, 2, offset);

	offset = gfire_proto_write_attr_bs(0x04, 0x06, p_cid, XFIRE_CHATID_LEN, offset);

	guint8 show = p_show ? 1 : 0;
	offset = gfire_proto_write_attr_bs(0x1B, 0x08, &show, sizeof(show), offset);

	gfire_proto_write_header(offset, 0x19, 2, 0);

	return offset;
}

void gfire_chat_proto_persistent_chats(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;
	GList *chat_ids = NULL;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &chat_ids, 0x04, offset);
	if(offset == -1 || !chat_ids)
		return;

	// Directly request further information on the rooms
	guint16 len = gfire_chat_proto_create_request_persistent_infos(chat_ids);
	if(len > 0) gfire_send(gfire_get_connection(p_gfire), len);

	gfire_list_clear(chat_ids);
}

void gfire_chat_proto_persistent_chat_infos(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;
	GList *chat_ids = NULL;
	GList *chat_types = NULL;
	GList *chat_names = NULL;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &chat_ids, 0x04, offset);
	if(offset == -1 || !chat_ids)
		return;

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &chat_types, 0xAA, offset);
	if(offset == -1 || !chat_types)
	{
		gfire_list_clear(chat_ids);
		return;
	}

	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &chat_names, 0x05, offset);
	if(offset == -1 || !chat_names)
	{
		gfire_list_clear(chat_ids);
		gfire_list_clear(chat_types);
		return;
	}

	GList *chat_id = chat_ids;
	GList *chat_type = chat_types;
	GList *chat_name = chat_names;
	while(chat_id && chat_type && chat_name)
	{
		// We only want normal chats
		if(*((guint32*)chat_type->data) == 1)
		{
			gfire_chat *chat = gfire_find_chat(p_gfire, chat_id->data, GFFC_CID);
			if(!chat)
			{
				gfire_chat_create(p_gfire, chat_id->data, chat_name->data, NULL, TRUE);
				gfire_add_chat(p_gfire, chat);
			}
			else
			{
				gfire_chat_set_topic(chat, chat_name->data, FALSE);
			}
		}

		chat_id = g_list_next(chat_id);
		chat_type = g_list_next(chat_type);
		chat_name = g_list_next(chat_name);
	}

	gfire_list_clear(chat_ids);
	gfire_list_clear(chat_types);
	gfire_list_clear(chat_names);
}

void gfire_chat_proto_join_info(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;
	gfire_chat *chat = NULL;
	guint8 *chat_id = NULL;
	guint32 requestID = 0;
	guint32 response = 0;
	guint32 my_permission = 0;
	guint32 access = 0;
	guint32 type = 0;
	gchar *topic = NULL;
	gchar *motd = NULL;
	gboolean new_room = FALSE;
	gboolean secure = FALSE;
	gboolean silenced = FALSE;
	gboolean show_join_leave = FALSE;

	if (!p_gfire || (p_packet_len == 0)) return;

	// xfire chat id
	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	// requestID
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &requestID, 0x0B, offset);
	if(offset == -1)
		return;

	// response
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &response, 0x0C, offset);
	if(offset == -1)
		return;

	// Password required but not given
	if(response == 4)
	{
		// Chat ID will be freed by the function
		gfire_chat_request_password_rejoin(p_gfire, chat_id, FALSE);
		return;
	}
	// Wrong password given
	else if(response == 5)
	{
		// Chat ID will be freed by the function
		gfire_chat_request_password_rejoin(p_gfire, chat_id, TRUE);
		return;
	}
	// Other failure
	else if(response != 0)
	{
		purple_notify_error(gfire_get_connection(p_gfire), _("Chat room join error"), _("Unknown error"),
							_("Unknown join error. You might be blocked from this chat room or are already in 5 rooms."));
		g_free(chat_id);
		return;
	}

	// own permission
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &my_permission, 0x12, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		return;
	}

	// accessibility (public / private)
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &access, 0x17, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		return;
	}

	// chat room type (normal / live broadcast)
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &type, 0xAA, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		return;
	}

	// topic
	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &topic, 0x05, offset);
	if(offset == -1 || !topic)
	{
		g_free(chat_id);
		return;
	}

	// message of the day
	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &motd, 0x4D, offset);
	if(offset == -1 || !motd)
	{
		g_free(chat_id);
		g_free(topic);
		return;
	}

	// new room
	offset = gfire_proto_read_attr_boolean_bs(p_gfire->buff_in, &new_room, 0xA5, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		g_free(topic);
		g_free(motd);
		return;
	}

	// password protected
	offset = gfire_proto_read_attr_boolean_bs(p_gfire->buff_in, &secure, 0xA6, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		g_free(topic);
		g_free(motd);
		return;
	}

	// silenced
	offset = gfire_proto_read_attr_boolean_bs(p_gfire->buff_in, &silenced, 0x16, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		g_free(topic);
		g_free(motd);
		return;
	}

	// show join/leave messages
	offset = gfire_proto_read_attr_boolean_bs(p_gfire->buff_in, &show_join_leave, 0x1B, offset);
	if(offset == -1)
	{
		g_free(chat_id);
		g_free(topic);
		g_free(motd);
		return;
	}

	// packet has been parsed

	gboolean created = FALSE;
	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		// Create a new chat with the correct values
		created = TRUE;
		chat = gfire_chat_create(p_gfire, chat_id, topic, motd, FALSE);
	}
	else
	{
		// Set current values
		gfire_chat_set_topic(chat, topic, FALSE);
		gfire_chat_set_motd(chat, motd, FALSE);
	}
	g_free(chat_id); g_free(topic); g_free(motd);

	gfire_chat_set_accessibility(chat, access, FALSE);
	gfire_chat_set_secure(chat, secure, FALSE);
	gfire_chat_set_silenced(chat, silenced, FALSE);
	gfire_chat_set_show_join_leave(chat, show_join_leave, FALSE);

	// Add it to gfire_data and tell purple about the chat
	if(created)
			gfire_add_chat(p_gfire, chat);
	gfire_chat_show(chat);
}

void gfire_chat_proto_room_info(gfire_data *p_gfire, guint16 p_packet_len)
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

	// Get the correct chat
	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
		return;

	gfire_chat_set_topic(chat, topic, FALSE);
	gfire_chat_set_motd(chat, motd, FALSE);
	g_free(chat_id); g_free(topic); g_free(motd);

	// voice chat allowed
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &allowVoiceChat, 0x4E, offset);
	if(offset == -1)
		return;

	// default permission
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &defaultPerm, 0x49, offset);
	if(offset == -1)
		return;

	gfire_chat_set_default_permission(chat, defaultPerm, FALSE);

	// time stamp?
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &timeStamp, 0x14, offset);
	if(offset == -1)
		return;

	// room type
	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &roomType, 0x17, offset);
	if(offset == -1)
		return;

	// get user id's of members in chat
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &userids, 0x33, offset);
	if(offset == -1 || !userids)
		return;

	// get user permissions of members in chat
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &perms, 0x44, offset);
	if(offset == -1 || !perms)
	{
		//mem cleanup code
		if(userids)
			g_list_free(userids);
		return;
	}

	// get chat member usernames
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &names, 0x42, offset);
	if(offset == -1 || !names)
	{
		//mem cleanup code
		if(userids)
			g_list_free(userids);
		if(perms)
			g_list_free(perms);
		return;
	}

	// get chat member aliases
	offset = gfire_proto_read_attr_list_bs(p_gfire->buff_in, &nicks, 0x43, offset);
	if(offset == -1 || !nicks)
	{
		//mem cleanup code
		if(userids)
			g_list_free(userids);
		if(perms)
			g_list_free(perms);
		if(names)
			g_list_free(names);
		return;
	}

	// packet has been parsed

	n = names; p = perms; a = nicks; u = userids;

	while(u && p && a && u)
	{
		if(!gfire_is_self(p_gfire, *(guint32*)u->data))
		{
			m = gfire_buddy_create(*(guint32*)u->data, (gchar*)n->data, (gchar*)a->data, GFBT_GROUPCHAT);
			gfire_chat_add_user(chat, m, *(guint32*)p->data, FALSE);
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

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &msg, 0x2E, offset);

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

	gfire_chat_user_left(chat, userid, FALSE);
}


void gfire_chat_proto_user_join(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	guint32 userid = 0;
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

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &name, 0x02, offset);
	if(offset == -1 || !name)
	{
		return;
	}

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &nick, 0x0D, offset);
	if(offset == -1 || !nick)
	{
		g_free(name);
		return;
	}

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &perm, 0x12, offset);
	if(offset == -1)
		return;

	gf_buddy = gfire_buddy_create(userid, name, nick, GFBT_GROUPCHAT);
	gfire_chat_add_user(chat, gf_buddy, perm, TRUE);

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "groupchat join, userid: %u, username: %s, alias: %s\n",
				 userid, NN(name), NN(nick));

	g_free(name);
	g_free(nick);
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

	gfire_chat_set_motd(chat, motd, TRUE);
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
	g_hash_table_replace(components, g_strdup("chat_id"), purple_base64_encode(chat_id, XFIRE_CHATID_LEN));

	g_free(chat_id);

	serv_got_chat_invite(gfire_get_connection(p_gfire), room, nick, "", components);
}

void gfire_chat_proto_topic_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	gchar *topic = NULL;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_topic_change: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_string_bs(p_gfire->buff_in, &topic, 0x05, offset);
	if(offset == -1 || !topic)
		return;

	gfire_chat_set_topic(chat, topic, TRUE);
	g_free(topic);
}

void gfire_chat_proto_buddy_kicked(gfire_data *p_gfire, guint16 p_packet_len)
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
		purple_debug_error("gfire", "gfire_chat_proto_buddy_kicked: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &userid, 0x18, offset);
	if(offset == -1)
		return;

	gfire_chat_user_left(chat, userid, TRUE);
}

void gfire_chat_proto_default_permission_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	guint32 permission = 0;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_default_permission_change: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &permission, 0x13, offset);
	if(offset == -1)
		return;

	gfire_chat_set_default_permission(chat, permission, TRUE);
}

void gfire_chat_proto_password_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	gboolean secured = FALSE;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_password_change: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_boolean_bs(p_gfire->buff_in, &secured, 0x5F, offset);
	if(offset == -1)
		return;

	gfire_chat_set_secure(chat, secured, TRUE);
}

void gfire_chat_proto_accessibility_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	guint32 access = 0;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_accessibility_change: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &access, 0x17, offset);
	if(offset == -1)
		return;

	gfire_chat_set_accessibility(chat, access, TRUE);
}

void gfire_chat_proto_silenced_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	guint32 userid = 0;
	gboolean silenced = FALSE;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_silenced_change: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_int32_bs(p_gfire->buff_in, &userid, 0x01, offset);
	if(offset == -1)
		return;

	offset = gfire_proto_read_attr_boolean_bs(p_gfire->buff_in, &silenced, 0x16, offset);
	if(offset == -1)
		return;

	gfire_chat_set_silenced(chat, silenced, TRUE);
}

void gfire_chat_proto_show_join_leave_change(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	guint8 *chat_id = NULL;
	gfire_chat *chat = NULL;
	gboolean show = FALSE;

	offset = gfire_proto_read_attr_chatid_bs(p_gfire->buff_in, &chat_id, 0x04, offset);
	if(offset == -1 || !chat_id)
		return;

	chat = gfire_find_chat(p_gfire, chat_id, GFFC_CID);
	if(!chat)
	{
		g_free(chat_id);
		purple_debug_error("gfire", "gfire_chat_proto_show_join_leave_change: Unknown chat id!\n");
		return;
	}

	g_free(chat_id);

	offset = gfire_proto_read_attr_boolean_bs(p_gfire->buff_in, &show, 0x1B, offset);
	if(offset == -1)
		return;

	gfire_chat_set_show_join_leave(chat, show, TRUE);
}
