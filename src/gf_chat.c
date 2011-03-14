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

#include "gf_network.h"
#include "gf_chat.h"

static PurpleChat *gfire_chat_find_in_blist(const gfire_chat *p_chat)
{
	if(!p_chat)
		return NULL;

	PurpleChat *chat = NULL;

	// Check by name first
	if(p_chat->topic)
	{
		chat = purple_blist_find_chat(purple_connection_get_account(gfire_get_connection(p_chat->owner)), p_chat->topic);
		if(chat)
		{
			GHashTable *comp = purple_chat_get_components(chat);
			if(comp && p_chat->chat_id && g_hash_table_lookup(comp, "chat_id"))
			{
				guint8 *id = purple_base64_decode(g_hash_table_lookup(comp, "chat_id"), NULL);
				if(memcmp(id, p_chat->chat_id, XFIRE_CHATID_LEN) == 0)
				{
					g_free(id);
					return chat;
				}
				g_free(id);
			}
			else
				return chat;
		}
	}

	// Check by chat_id
	if(p_chat->chat_id)
	{
		// Iterate through the whole buddy list until we find a chat with given chat_id
		PurpleBlistNode *node = purple_blist_get_root();
		while(node)
		{
			PurpleBlistNode *next = NULL;
			if(!PURPLE_BLIST_NODE_IS_CHAT(node))
			{
				next = purple_blist_node_get_first_child(node);
			}
			else
			{
				chat = PURPLE_CHAT(node);
				// We only care for chats of the current account
				if(purple_chat_get_account(chat) == purple_connection_get_account(gfire_get_connection(p_chat->owner)))
				{
					// Check the chat_id
					GHashTable *comp = purple_chat_get_components(chat);
					if(comp && g_hash_table_lookup(comp, "chat_id"))
					{
						guint8 *id = purple_base64_decode(g_hash_table_lookup(comp, "chat_id"), NULL);
						if(memcmp(id, p_chat->chat_id, XFIRE_CHATID_LEN) == 0)
						{
							g_free(id);
							return chat;
						}
						g_free(id);
					}
				}

				// Not our chat; go on with next sibling
			}

			if(!next)
			{
				next = purple_blist_node_get_sibling_next(node);
			}

			if(!next)
			{
				// No chat found
				if(node == purple_blist_get_root())
					return NULL;

				// Get next sibling of parent
				PurpleBlistNode *parent = purple_blist_node_get_parent(node);
				next = purple_blist_node_get_sibling_next(parent);
				while(!next && parent)
				{
					parent = purple_blist_node_get_parent(parent);
					next = purple_blist_node_get_sibling_next(parent);
				}
			}

			node = next;
		}
	}

	// No chat found
	return NULL;
}

static void gfire_chat_update_purple_chat(gfire_chat *p_chat)
{
	if(!p_chat || !p_chat->purple_chat)
		return;

	GHashTable *comp = purple_chat_get_components(p_chat->purple_chat);
	// Set Chat ID by adding a new chat node / Update name
	if(!g_hash_table_lookup(comp, "chat_id") ||
	   (p_chat->topic && purple_utf8_strcasecmp(g_hash_table_lookup(comp, "room"), p_chat->topic) != 0))
	{
		// Update the topic
		if(p_chat->topic)
			g_hash_table_replace(comp, g_strdup("room"), g_strdup(p_chat->topic));

		// Insert the ID
		gchar *id = purple_base64_encode(p_chat->chat_id, XFIRE_CHATID_LEN);
		g_hash_table_replace(comp, g_strdup("chat_id"), id);
	}
}

gfire_chat *gfire_chat_create(gfire_data *p_owner, const guint8 *p_id, const gchar *p_topic,
							  const gchar *p_motd, gboolean p_force_add)
{
	if(!p_owner)
		return NULL;

	gfire_chat *ret = g_malloc0(sizeof(gfire_chat));
	if(!ret)
		return NULL;

	ret->owner = p_owner;

	ret->chat_id = g_malloc0(XFIRE_CHATID_LEN);
	if(!ret->chat_id)
	{
		g_free(ret);
		return NULL;
	}

	if(p_id)
		memcpy(ret->chat_id, p_id, XFIRE_CHATID_LEN);
	if(p_topic)
		ret->topic = g_strdup(p_topic);
	if(p_motd)
		ret->motd = g_strdup(p_motd);

	ret->purple_chat = gfire_chat_find_in_blist(ret);
	if(ret->purple_chat)
	{
		gfire_chat_update_purple_chat(ret);
		gfire_chat_set_saved(ret, TRUE);
	}
	else if(p_force_add)
	{
		GHashTable *comp = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
		if(p_topic)
			g_hash_table_insert(comp, g_strdup("room"), g_strdup(p_topic));

		gchar *chat_id = purple_base64_encode(ret->chat_id, XFIRE_CHATID_LEN);
		g_hash_table_insert(comp, g_strdup("chat_id"), chat_id);

		ret->purple_chat = purple_chat_new(purple_connection_get_account(gfire_get_connection(p_owner)),
										   NULL, comp);

		PurpleGroup *group = purple_find_group(GFIRE_DEFAULT_GROUP_NAME);
		if(!group)
		{
			group = purple_group_new(GFIRE_DEFAULT_GROUP_NAME);
			purple_blist_add_group(group, NULL);
		}
		purple_blist_add_chat(ret->purple_chat, group, NULL);
	}

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

void gfire_chat_add_user(gfire_chat *p_chat, gfire_buddy *p_buddy, guint32 p_perm, gboolean p_joined)
{
	if(!p_chat || !p_buddy)
		return;

	gchar permissionName[50];
	PurpleConvChatBuddyFlags f;
	switch(p_perm)
	{
	case Muted:
		strcpy(permissionName, _("Permissionless (muted)"));
		f = PURPLE_CBFLAGS_NONE;
		break;

	case Normal:
		strcpy(permissionName, _("Normal"));
		f = PURPLE_CBFLAGS_NONE;
		break;

	case Power_User:
		strcpy(permissionName, _("Power-User"));
		f = PURPLE_CBFLAGS_VOICE;
		break;

	case Moderator:
		strcpy(permissionName, _("Moderator"));
		f = PURPLE_CBFLAGS_HALFOP;
		break;

	case Admin:
		strcpy(permissionName, _("Admin"));
		f = PURPLE_CBFLAGS_OP;
		break;

	default:
		strcpy(permissionName, _("Unknown"));
		f = PURPLE_CBFLAGS_NONE;
	}

	purple_conv_chat_add_user(PURPLE_CONV_CHAT(p_chat->c), gfire_buddy_get_name(p_buddy), NULL, f, p_joined && p_chat->show_join_leave);

	if(gfire_is_self(p_chat->owner, p_buddy->userid))
	{
		p_chat->own_permission = p_perm;
		gchar *tmpmsg = g_strdup_printf(_("You currently have the permission \"%s\"."), permissionName);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}

	p_buddy->chatperm = p_perm;
	p_chat->members = g_list_append(p_chat->members, p_buddy);
}

gfire_buddy *gfire_chat_find_user(gfire_chat *p_chat, guint32 p_userid)
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

	gfire_buddy *buddy = gfire_chat_find_user(p_chat, p_userid);
	if(!buddy)
		return;

	gchar *escaped = gfire_escape_html(p_msg);
	serv_got_chat_in(purple_conversation_get_gc(p_chat->c), p_chat->purple_id, gfire_buddy_get_name(buddy),
					 PURPLE_MESSAGE_RECV, escaped, time(NULL));
	g_free(escaped);
}

void gfire_chat_user_left(gfire_chat *p_chat, guint32 p_userid, gboolean p_kicked)
{
	if(!p_chat)
		return;

	gfire_buddy *buddy = gfire_chat_find_user(p_chat, p_userid);
	if(!buddy)
		return;

	purple_conv_chat_remove_user(PURPLE_CONV_CHAT(p_chat->c), gfire_buddy_get_name(buddy),
								 p_kicked ? _("Buddy has been kicked.") : NULL);

	GList *cur = g_list_find(p_chat->members, buddy);
	if(!cur)
		return;

	p_chat->members = g_list_delete_link(p_chat->members, cur);
	gfire_buddy_free(buddy);
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
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(chat): sending join request for room %s\n", p_room);
		return;
	}
}

void gfire_chat_leave(gfire_chat *p_chat)
{
	if(!p_chat)
		return;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(group chat): leaving room: %s\n", NN(p_chat->topic));
	guint16 len = gfire_chat_proto_create_leave(p_chat->chat_id);
	if(len > 0) gfire_send(gfire_get_connection(p_chat->owner), len);
}

void gfire_chat_set_topic(gfire_chat *p_chat, const gchar *p_topic, gboolean p_notify)
{
	if(!p_chat || !p_topic)
		return;

	gchar *oldtopic = p_chat->topic;
	p_chat->topic = g_strdup(p_topic);

	// Update chat node
	gfire_chat_update_purple_chat(p_chat);

	if(p_notify && p_chat->c && oldtopic)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "new topic for room %s: %s\n", oldtopic, p_topic);

		purple_conversation_set_title(p_chat->c, p_chat->topic);

		gchar *tmpmsg = g_strdup_printf(_("This room's name has been changed to \"%s\"."), p_topic);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}

	if(oldtopic)
		g_free(oldtopic);
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

void gfire_chat_set_motd(gfire_chat *p_chat, const gchar *p_motd, gboolean p_notify)
{
	if(!p_chat || !p_motd)
		return;

	if(p_chat->motd) g_free(p_chat->motd);
	p_chat->motd = g_strdup(p_motd);

	if(p_notify && p_chat->c)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "new motd for room %s: %s\n", p_chat->topic, p_motd);

		purple_conv_chat_set_topic(PURPLE_CONV_CHAT(p_chat->c), "", p_motd);
		gchar *tmpmsg = g_strdup_printf(_("Today's message changed to:\n%s"), p_motd);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}
}

void gfire_chat_buddy_permission_changed(gfire_chat *p_chat, guint32 p_userid, guint32 p_perm)
{
	if(!p_chat)
		return;

	gfire_buddy *gf_buddy = gfire_chat_find_user(p_chat, p_userid);
	if(!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_chat_buddy_permission_changed: Unknown buddy!\n");
		return;
	}

	gchar permissionName[50];
	PurpleConvChatBuddyFlags f;
	switch(p_perm)
	{
	case 01:
		strcpy(permissionName, _("Permissionless (muted)"));
		f = PURPLE_CBFLAGS_NONE;
		break;

	case 02:
		strcpy(permissionName, _("Normal"));
		f = PURPLE_CBFLAGS_NONE;
		break;

	case 03:
		strcpy(permissionName, _("Power-User"));
		f = PURPLE_CBFLAGS_VOICE;
		break;

	case 04:
		strcpy(permissionName, _("Moderator"));
		f = PURPLE_CBFLAGS_HALFOP;
		break;

	case 05:
		strcpy(permissionName, _("Admin"));
		f = PURPLE_CBFLAGS_OP;
		break;

	default:
		strcpy(permissionName, _("Unknown"));
		f = PURPLE_CBFLAGS_NONE;
	}

	if(gfire_is_self(p_chat->owner, gf_buddy->userid))
	{
		p_chat->own_permission = p_perm;
		gchar *tmpmsg = g_strdup_printf(_("Your permission has been changed to \"%s\"."), permissionName);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}
	else
	{
		gchar *tmpmsg = g_strdup_printf(_("%s's permission has been changed to \"%s\"."), gfire_buddy_get_alias(gf_buddy), permissionName);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}

	purple_conv_chat_user_set_flags(PURPLE_CONV_CHAT(p_chat->c), gfire_buddy_get_name(gf_buddy), f);
	gf_buddy->chatperm = p_perm;
}

void gfire_chat_set_saved(gfire_chat *p_chat, gboolean p_save)
{
	if(!p_chat)
		return;

	guint16 len = gfire_chat_proto_create_save_chat_room(p_chat->chat_id, p_save);
	if(len > 0) gfire_send(gfire_get_connection(p_chat->owner), len);
}

void gfire_chat_show(gfire_chat *p_chat)
{
	if(!p_chat)
		return;

	// Create conversation
	p_chat->c = serv_got_joined_chat(gfire_get_connection(p_chat->owner), p_chat->purple_id, p_chat->topic);

	// Set motd
	purple_conv_chat_set_topic(PURPLE_CONV_CHAT(p_chat->c), NULL, p_chat->motd);

	// Join message
	gchar *tmpmsg = g_strdup_printf(_("You are now chatting in %s."), p_chat->topic);
	purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
	g_free(tmpmsg);

	// MotD
	if(p_chat->motd && strlen(p_chat->motd))
	{
		gchar *tmpmsg = g_strdup_printf(_("Today's message:\n%s"), p_chat->motd);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}

	purple_conversation_present(p_chat->c);
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
		gfire_send(gfire_get_connection(p_chat->owner), len);
	}
}

void gfire_chat_send(gfire_chat *p_chat, const gchar *p_msg)
{
	if(!p_chat || !p_msg)
		return;

	gchar *no_html = purple_markup_strip_html(p_msg);
	gchar *unescaped = purple_unescape_html(no_html);
	g_free(no_html);

	guint16 len = gfire_chat_proto_create_message(p_chat->chat_id, unescaped);
	if(len > 0) gfire_send(gfire_get_connection(p_chat->owner), len);

	g_free(unescaped);
}

void gfire_chat_change_motd(gfire_chat *p_chat, const gchar *p_motd)
{
	if(!p_chat || !p_motd)
		return;

	guint16 len = gfire_chat_proto_create_change_motd(p_chat->chat_id, p_motd);
	if(len > 0) gfire_send(gfire_get_connection(p_chat->owner), len);
}

void gfire_chat_set_default_permission(gfire_chat *p_chat, guint32 p_permission, gboolean p_notify)
{
	if(!p_chat)
		return;

	p_chat->def_permission = p_permission;

	if(p_notify)
	{
		gchar permissionName[50];
		switch(p_permission)
		{
		case Muted:
			strcpy(permissionName, _("Permissionless (muted)"));
			break;

		case Normal:
			strcpy(permissionName, _("Normal"));
			break;

		case Power_User:
			strcpy(permissionName, _("Power-User"));
			break;

		case Moderator:
			strcpy(permissionName, _("Moderator"));
			break;

		case Admin:
			strcpy(permissionName, _("Admin"));
			break;

		default:
			strcpy(permissionName, _("Unknown"));
		}

		gchar *tmpmsg = g_strdup_printf(_("This room's default permission has been changed to \"%s\"."), permissionName);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}
}

void gfire_chat_set_accessibility(gfire_chat *p_chat, guint32 p_accessibility, gboolean p_notify)
{
	if(!p_chat)
		return;

	p_chat->accessibility = p_accessibility;

	if(p_notify)
	{
		const gchar *accessibility = NULL;
		switch(p_accessibility)
		{
		case 1:
			accessibility = _("Public");
			break;
		case 2:
			accessibility = _("Friends only");
			break;
		default:
			accessibility = _("Unknown");
		}

		gchar *tmpmsg = g_strdup_printf(_("This room's visibility has been changed to \"%s\"."), accessibility);
		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}
}

void gfire_chat_set_secure(gfire_chat *p_chat, gboolean p_secure, gboolean p_notify)
{
	if(!p_chat)
		return;

	if(p_notify)
	{
		gchar *tmpmsg = NULL;
		if(!p_chat->secure && p_secure)
			tmpmsg = g_strdup_printf(_("This room is now password protected."));
		else if(p_chat->secure && !p_secure)
			tmpmsg = g_strdup_printf(_("This room is no longer password protected."));
		else if(p_chat->secure && p_secure)
			tmpmsg = g_strdup_printf(_("This room's password has been changed."));
		else
			return;

		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}

	p_chat->secure = p_secure;
}

void gfire_chat_set_silenced(gfire_chat *p_chat, gboolean p_silenced, gboolean p_notify)
{
	if(!p_chat)
		return;

	if(p_notify)
	{
		gchar *tmpmsg = NULL;
		if(!p_chat->silenced && p_silenced)
			tmpmsg = g_strdup_printf(_("This room is now silenced."));
		else if(p_chat->silenced && !p_silenced)
			tmpmsg = g_strdup_printf(_("This room is no longer silenced."));
		else
			return;

		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}

	p_chat->silenced = p_silenced;
}

void gfire_chat_set_show_join_leave(gfire_chat *p_chat, gboolean p_show, gboolean p_notify)
{
	if(!p_chat)
		return;

	if(p_notify)
	{
		gchar *tmpmsg = NULL;
		if(!p_chat->show_join_leave && p_show)
			tmpmsg = g_strdup_printf(_("Buddy join-/leave-messages will be displayed now."));
		else if(p_chat->show_join_leave && !p_show)
			tmpmsg = g_strdup_printf(_("Buddy join-/leave-messages will no longer be displayed."));
		else
			return;

		purple_conv_chat_write(PURPLE_CONV_CHAT(p_chat->c), "", tmpmsg, PURPLE_MESSAGE_SYSTEM, time(NULL));
		g_free(tmpmsg);
	}

	p_chat->show_join_leave = p_show;
}

void gfire_chat_set_purple_chat(gfire_chat *p_chat, PurpleChat *p_purple_chat)
{
	if(!p_chat || !p_purple_chat)
		return;

	p_chat->purple_chat = p_purple_chat;
	gfire_chat_update_purple_chat(p_chat);
}

// Gfire Chat Commands
static PurpleCmdRet gfire_chat_command_handler(PurpleConversation *p_conv, const gchar *p_cmd,
											   gchar **p_args, gchar **p_error, void *p_data)
{
	gfire_data *gfire = purple_conversation_get_gc(p_conv)->proto_data;
	if(!gfire)
		return PURPLE_CMD_RET_FAILED;

	gint id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(p_conv));
	gfire_chat *chat = gfire_find_chat(gfire, &id, GFFC_PURPLEID);
	if(!chat)
		return PURPLE_CMD_RET_FAILED;

	// Save command
	if(!purple_utf8_strcasecmp(p_cmd, "save"))
	{
		if(!purple_utf8_strcasecmp(p_args[0], "yes"))
		{
			if(!chat->purple_chat)
				purple_blist_request_add_chat(purple_conversation_get_account(p_conv), NULL, NULL, chat->topic);
			else
				gfire_chat_set_saved(chat, TRUE);

			return PURPLE_CMD_RET_OK;
		}
		else if(!purple_utf8_strcasecmp(p_args[0], "no"))
		{
			gfire_chat_set_saved(chat, FALSE);
			return PURPLE_CMD_RET_OK;
		}
		else
		{
			*p_error = g_strdup_printf(_("Unknown argument: %s"), p_args[0]);
			return PURPLE_CMD_RET_FAILED;
		}
	}
	// Change room name command
	else if(!purple_utf8_strcasecmp(p_cmd, "rename"))
	{
		if(!purple_utf8_strcasecmp(chat->topic, p_args[0]))
		{
			*p_error = g_strdup(_("New and old name are identical. Please note that chat room names have no case."));
			return PURPLE_CMD_RET_FAILED;
		}
		else
		{
			guint16 len = gfire_chat_proto_create_change_topic(chat->chat_id, p_args[0]);
			if(len > 0) gfire_send(gfire_get_connection(gfire), len);
			return PURPLE_CMD_RET_OK;
		}
	}
	// Change password command
	else if(!purple_utf8_strcasecmp(p_cmd, "password"))
	{
		guint16 len = gfire_chat_proto_create_change_password(chat->chat_id, p_args[0]);
		if(len > 0) gfire_send(gfire_get_connection(gfire), len);
		return PURPLE_CMD_RET_OK;
	}
	// Change visibility command
	else if(!purple_utf8_strcasecmp(p_cmd, "visibility"))
	{
		if(!purple_utf8_strcasecmp(p_args[0], "public"))
		{
			guint16 len = gfire_chat_proto_create_change_access(chat->chat_id, Public);
			if(len > 0) gfire_send(gfire_get_connection(gfire), len);
			return PURPLE_CMD_RET_OK;
		}
		else if(!purple_utf8_strcasecmp(p_args[0], "friends"))
		{
			guint16 len = gfire_chat_proto_create_change_access(chat->chat_id, Friends);
			if(len > 0) gfire_send(gfire_get_connection(gfire), len);
			return PURPLE_CMD_RET_OK;
		}
		else
		{
			*p_error = g_strdup_printf(_("Unknown visibility: %s"), p_args[0]);
			return PURPLE_CMD_RET_FAILED;
		}
	}
	// Silence command
	else if(!purple_utf8_strcasecmp(p_cmd, "silence"))
	{
		if(!purple_utf8_strcasecmp(p_args[0], "on"))
		{
			guint16 len = gfire_chat_proto_create_change_silenced(chat->chat_id, TRUE);
			if(len > 0) gfire_send(gfire_get_connection(gfire), len);
			return PURPLE_CMD_RET_OK;
		}
		else if(!purple_utf8_strcasecmp(p_args[0], "off"))
		{
			guint16 len = gfire_chat_proto_create_change_silenced(chat->chat_id, FALSE);
			if(len > 0) gfire_send(gfire_get_connection(gfire), len);
			return PURPLE_CMD_RET_OK;
		}
		else
		{
			*p_error = g_strdup_printf(_("Unknown argument: %s"), p_args[0]);
			return PURPLE_CMD_RET_FAILED;
		}
	}
	// User-Join/Leave-Messages command
	else if(!purple_utf8_strcasecmp(p_cmd, "userjoinmsg"))
	{
		if(!purple_utf8_strcasecmp(p_args[0], "on"))
		{
			guint16 len = gfire_chat_proto_create_change_show_join_leave(chat->chat_id, TRUE);
			if(len > 0) gfire_send(gfire_get_connection(gfire), len);
			return PURPLE_CMD_RET_OK;
		}
		else if(!purple_utf8_strcasecmp(p_args[0], "off"))
		{
			guint16 len = gfire_chat_proto_create_change_show_join_leave(chat->chat_id, FALSE);
			if(len > 0) gfire_send(gfire_get_connection(gfire), len);
			return PURPLE_CMD_RET_OK;
		}
		else
		{
			*p_error = g_strdup_printf(_("Unknown argument: %s"), p_args[0]);
			return PURPLE_CMD_RET_FAILED;
		}
	}
	// Change user permission command
	else if(!purple_utf8_strcasecmp(p_cmd, "permission"))
	{
		if(chat->own_permission < Moderator)
		{
			*p_error = g_strdup(_("You are not allowed to grant/revoke any permissions."));
			return PURPLE_CMD_RET_FAILED;
		}

		gfire_buddy *buddy = NULL;
		GList *cur = chat->members;
		while(cur)
		{
			if(!purple_utf8_strcasecmp(((gfire_buddy*)cur->data)->name, p_args[0]))
			{
				buddy = (gfire_buddy*)cur->data;
				break;
			}

			cur = g_list_next(cur);
		}

		if(!buddy)
		{
			*p_error = g_strdup_printf(_("Unknown buddy: %s"), p_args[0]);
			return PURPLE_CMD_RET_FAILED;
		}

		if(gfire_is_self(gfire, buddy->userid))
		{
			*p_error = g_strdup(_("You can't change your own permission!"));
			return PURPLE_CMD_RET_FAILED;
		}

		guint32 permission;
		if(!purple_utf8_strcasecmp("muted", p_args[1]))
			permission = Muted;
		else if(!purple_utf8_strcasecmp("normal", p_args[1]))
			permission = Normal;
		else if(!purple_utf8_strcasecmp("power", p_args[1]))
			permission = Power_User;
		else if(!purple_utf8_strcasecmp("moderator", p_args[1]))
		{
			if(chat->own_permission != Admin)
			{
				*p_error = g_strdup(_("You are not allowed to grant this permission."));
				return PURPLE_CMD_RET_FAILED;
			}
			permission = Moderator;
		}
		else if(!purple_utf8_strcasecmp("admin", p_args[1]))
		{
			if(chat->own_permission != Admin)
			{
				*p_error = g_strdup(_("You are not allowed to grant this permission."));
				return PURPLE_CMD_RET_FAILED;
			}
			permission = Admin;
		}
		else
		{
			*p_error = g_strdup_printf(_("Unknown permission: %s"), p_args[1]);
			return PURPLE_CMD_RET_FAILED;
		}

		guint16 len = gfire_chat_proto_create_change_buddy_permissions(chat->chat_id, buddy->userid, permission);
		if(len > 0) gfire_send(gfire_get_connection(gfire), len);
		return PURPLE_CMD_RET_OK;
	}
	// Kick buddy command
	else if(!purple_utf8_strcasecmp(p_cmd, "kick"))
	{
		if(chat->own_permission < Moderator)
		{
			*p_error = g_strdup(_("You are not allowed to kick buddies."));
			return PURPLE_CMD_RET_FAILED;
		}

		gfire_buddy *buddy = NULL;
		GList *cur = chat->members;
		while(cur)
		{
			if(!purple_utf8_strcasecmp(((gfire_buddy*)cur->data)->name, p_args[0]))
			{
				buddy = (gfire_buddy*)cur->data;
				break;
			}

			cur = g_list_next(cur);
		}

		if(!buddy)
		{
			*p_error = g_strdup_printf(_("Unknown buddy: %s"), p_args[0]);
			return PURPLE_CMD_RET_FAILED;
		}

		if(gfire_is_self(gfire, buddy->userid))
		{
			*p_error = g_strdup(_("You can't kick yourself!"));
			return PURPLE_CMD_RET_FAILED;
		}

		guint16 len = gfire_chat_proto_create_kick_buddy(chat->chat_id, buddy->userid);
		if(len > 0) gfire_send(gfire_get_connection(gfire), len);
		return PURPLE_CMD_RET_OK;
	}
	// Change default permission command
	else if(!purple_utf8_strcasecmp(p_cmd, "def_permission"))
	{
		if(chat->own_permission < Moderator)
		{
			*p_error = g_strdup(_("You are not allowed to change the default permission."));
			return PURPLE_CMD_RET_FAILED;
		}

		guint32 permission;
		if(!purple_utf8_strcasecmp("muted", p_args[0]))
			permission = Muted;
		else if(!purple_utf8_strcasecmp("normal", p_args[0]))
			permission = Normal;
		else if(!purple_utf8_strcasecmp("power", p_args[0]))
			permission = Power_User;
		else if(!purple_utf8_strcasecmp("moderator", p_args[0]))
		{
			if(chat->own_permission != Admin)
			{
				*p_error = g_strdup(_("You are not allowed to set this default permission."));
				return PURPLE_CMD_RET_FAILED;
			}
			permission = Moderator;
		}
		else if(!purple_utf8_strcasecmp("admin", p_args[0]))
		{
			if(chat->own_permission != Admin)
			{
				*p_error = g_strdup(_("You are not allowed to set this default permission."));
				return PURPLE_CMD_RET_FAILED;
			}
			permission = Admin;
		}
		else
		{
			*p_error = g_strdup_printf(_("Unknown permission: %s"), p_args[1]);
			return PURPLE_CMD_RET_FAILED;
		}

		guint16 len = gfire_chat_proto_create_set_default_permission(chat->chat_id, permission);
		if(len > 0) gfire_send(gfire_get_connection(gfire), len);
		return PURPLE_CMD_RET_OK;
	}
	return PURPLE_CMD_RET_FAILED;
}

static PurpleCmdId gfire_chat_save_cmd;
static PurpleCmdId gfire_chat_rename_cmd;
static PurpleCmdId gfire_chat_password_cmd;
static PurpleCmdId gfire_chat_visibility_cmd;
static PurpleCmdId gfire_chat_silence_cmd;
static PurpleCmdId gfire_chat_userjoinmsg_cmd;
static PurpleCmdId gfire_chat_permission_cmd;
static PurpleCmdId gfire_chat_kick_cmd;
static PurpleCmdId gfire_chat_def_perm_cmd;

void gfire_chat_register_commands()
{
	gfire_chat_save_cmd = purple_cmd_register("save", "w", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("save &lt;yes|no&gt;:<br />Save the current chat room on Xfire. "
												"This preserves all of the chat rooms settings and privileges. "
												"On &quot;yes&quot;, you will be asked to save the chat room to "
												"your buddy list if it is not already there. If you decline this "
												"request the save will NOT be performed."), NULL);

	gfire_chat_rename_cmd = purple_cmd_register("rename", "w", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("rename &lt;new-chat-name&gt;:<br />Changes the current name for this "
												"room.<br/><br />Requires &quot;Admin&quot; permission."), NULL);

	gfire_chat_password_cmd = purple_cmd_register("password", "s", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("password [&lt;new-password&gt;]:<br />Changes the current password for "
												"this room. &quot;/password&quot; results in removing the current "
												"password.<br /><br />Requires &quot;Admin&quot; permission."), NULL);

	gfire_chat_visibility_cmd = purple_cmd_register("visibility", "w", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("visibilty &lt;public|friends&gt;:<br />Changes the current visibility "
												"for this room.<br /><br />Requires &quot;Admin&quot; permission."),
											  NULL);

	gfire_chat_silence_cmd = purple_cmd_register("silence", "w", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("silence &lt;on|off&gt;:<br />Sets whether non-Moderators and non-Admins "
												"should not be able to talk in this room.<br /><br />Requires "
												"&quot;Moderator&quot; or higher permission."), NULL);

	gfire_chat_userjoinmsg_cmd = purple_cmd_register("userjoinmsg", "w", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("userjoinmsg &lt;on|off&gt;:<br />Set whether &quot;&lt;User&gt; "
												"joined&quot; and &quot;&lt;User&gt; left&quot; messages should be "
												"displayed in this room.<br /><br />Requires "
												"&quot;Moderator&quot; or higher permission."), NULL);

	gfire_chat_permission_cmd = purple_cmd_register("permission", "ww", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("permission &lt;username&gt; &lt;muted|normal|power|moderator|admin&gt;:"
												"<br />Set <i>username</i>'s permission. Only admins may give other users "
												"the &quot;Admin&quot; permission.<br />Please note, that you can't "
												"change your own permission.<br /><br />Requires &quot;Moderator&quot; "
												"or higher permission."), NULL);

	gfire_chat_kick_cmd = purple_cmd_register("kick", "w", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("kick &lt;username&gt;:<br />Kicks <i>username</i> from the channel."
												"<br /><br />Requires &quot;Moderator&quot; "
												"or higher permission."), NULL);

	gfire_chat_def_perm_cmd = purple_cmd_register("def_permission", "w", PURPLE_CMD_P_PRPL,
											  PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
											  GFIRE_PRPL_ID, gfire_chat_command_handler,
											  _("def_permission &lt;muted|normal|power|moderator|admin&gt;:"
												"<br />Set this room's default permission. New buddies who join this "
												"room will have this permission<br /><br />Requires "
												"&quot;Moderator&quot; or higher permission."), NULL);
}

void gfire_chat_unregister_commands()
{
	purple_cmd_unregister(gfire_chat_save_cmd);
	purple_cmd_unregister(gfire_chat_rename_cmd);
	purple_cmd_unregister(gfire_chat_password_cmd);
	purple_cmd_unregister(gfire_chat_visibility_cmd);
	purple_cmd_unregister(gfire_chat_silence_cmd);
	purple_cmd_unregister(gfire_chat_userjoinmsg_cmd);
	purple_cmd_unregister(gfire_chat_permission_cmd);
	purple_cmd_unregister(gfire_chat_kick_cmd);
	purple_cmd_unregister(gfire_chat_def_perm_cmd);
}

gboolean gfire_chat_is_by_topic(const gfire_chat *p_chat, const gchar *p_topic)
{
	return (p_chat && p_topic && !purple_utf8_strcasecmp(p_chat->topic, p_topic));
}

gboolean gfire_chat_is_by_chat_id(const gfire_chat *p_chat, const guint8 *p_chat_id)
{
	return (p_chat && p_chat_id && !memcmp(p_chat->chat_id, p_chat_id, XFIRE_CHATID_LEN));
}

gboolean gfire_chat_is_by_purple_id(const gfire_chat *p_chat, const gint p_purple_id)
{
	return (p_chat && p_chat->c && purple_conv_chat_get_id(PURPLE_CONV_CHAT(p_chat->c)) == p_purple_id);
}

gboolean gfire_chat_is_by_purple_chat(const gfire_chat *p_chat, const PurpleChat *p_purple_chat)
{
	return (p_chat && p_purple_chat && p_chat->purple_chat == p_purple_chat);
}

typedef struct
{
	gfire_data *gfire;
	guint8 *chat_id;
} gfire_chat_rejoin_data;

static void gfire_chat_rejoin_cb(gfire_chat_rejoin_data *p_data, const gchar *p_password)
{
	if(!p_data || !p_password)
		return;

	gfire_chat_join(p_data->chat_id, NULL, p_password, gfire_get_connection(p_data->gfire));

	g_free(p_data->chat_id);
	g_free(p_data);
}

static void gfire_chat_rejoin_cancel_cb(gfire_chat_rejoin_data *p_data)
{
	if(p_data)
	{
		g_free(p_data->chat_id);
		g_free(p_data);
	}
}

void gfire_chat_request_password_rejoin(gfire_data *p_gfire, guint8 *p_chat_id, gboolean p_password_given)
{
	if(!p_gfire || !p_chat_id)
		return;

	const gchar *msg = NULL;
	const gchar *title = NULL;
	if(p_password_given)
	{
		msg = _("You attempted to join a chat room using an invalid password. Please try again.");
		title = _("Invalid password");
	}
	else
	{
		msg = _("You attempted to join a chat room that is password protected. Please try again.");
		title = _("Password required");
	}

	gfire_chat_rejoin_data *data = g_malloc(sizeof(gfire_chat_rejoin_data));
	data->chat_id = p_chat_id;
	data->gfire = p_gfire;

	purple_request_input(gfire_get_connection(p_gfire), title,
						 NULL, msg, NULL, FALSE, TRUE, NULL, _("Join"), G_CALLBACK(gfire_chat_rejoin_cb),
						 _("Cancel"), G_CALLBACK(gfire_chat_rejoin_cancel_cb),
						 purple_connection_get_account(gfire_get_connection(p_gfire)),
						 NULL, NULL, data);
}
