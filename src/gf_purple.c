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

#include "gfire_proto.h"
#include "gf_network.h"
#include "gfire.h"
#include "gf_menus.h"
#include "gf_server_browser.h"
#include "gf_games.h"
#include "gf_friend_search.h"
#include "gf_purple.h"

static const gchar *gfire_purple_blist_icon(PurpleAccount *p_a, PurpleBuddy *p_b)
{
	return "gfire";
}

static const gchar *gfire_purple_blist_emblems(PurpleBuddy *p_b)
{
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurplePresence *p = NULL;
	PurpleConnection *gc = NULL;

	if (!p_b || (NULL == p_b->account) || !(gc = purple_account_get_connection(p_b->account)) ||
						 !(gfire = (gfire_data *) gc->proto_data))
		return NULL;

	gf_buddy = gfire_find_buddy(gfire, purple_buddy_get_name(p_b), GFFB_NAME);

	p = purple_buddy_get_presence(p_b);

	if(purple_presence_is_online(p) == TRUE)
	{
		if(gfire_buddy_is_playing(gf_buddy) && !gfire_buddy_is_talking(gf_buddy))
			return "game";
		else if(!gfire_buddy_is_playing(gf_buddy) && gfire_buddy_is_talking(gf_buddy))
			return "voip";
		else if (gfire_buddy_is_playing(gf_buddy) && gfire_buddy_is_talking(gf_buddy))
			return "game-voip";
	}

	return NULL;
}

static gchar *gfire_purple_status_text(PurpleBuddy *p_buddy)
{
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurplePresence *p = NULL;
	PurpleConnection *gc = NULL;

	if(!p_buddy || !p_buddy->account || !(gc = purple_account_get_connection(p_buddy->account)) ||
	   !(gfire = (gfire_data *)gc->proto_data) || (!gfire->buddies))
		return NULL;

	gf_buddy = gfire_find_buddy(gfire, p_buddy->name, GFFB_NAME);
	if (!gf_buddy)
		return NULL;

	p = purple_buddy_get_presence(p_buddy);

	if(purple_presence_is_online(p))
	{
		gchar *status = gfire_buddy_get_status_text(gf_buddy);
		if(!status)
			return NULL;

		gchar *escaped = gfire_escape_html(status);
		g_free(status);

		return escaped;
	}
	return NULL;
}

static void gfire_purple_blist_tooltip_text(PurpleBuddy *p_buddy, PurpleNotifyUserInfo *p_user_info, gboolean p_full)
{
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurplePresence *p = NULL;

	if (!p_buddy || !p_buddy->account || !(gc = purple_account_get_connection(p_buddy->account)) ||
		!(gfire = (gfire_data*)gc->proto_data) || !gfire->buddies)
		return;

	gf_buddy = gfire_find_buddy(gfire, p_buddy->name, GFFB_NAME);
	if (!gf_buddy)
		return;

	p = purple_buddy_get_presence(p_buddy);

	if(purple_presence_is_online(p))
	{
		gfire_buddy_request_info(gf_buddy);

		// Game Info
		if(gfire_buddy_is_playing(gf_buddy))
		{
			const gfire_game_data *game_data = gfire_buddy_get_game_data(gf_buddy);

			gchar *game_name = gfire_game_name(game_data->id);
			purple_notify_user_info_add_pair(p_user_info, _("Game"), NN(game_name));
			if(game_name) g_free(game_name);

			if(gfire_game_data_has_addr(game_data))
			{
				gchar *addr = gfire_game_data_addr_str(game_data);
				purple_notify_user_info_add_pair(p_user_info, _("Server"), addr);
				g_free(addr);
			}
		}

		// VoIP Info
		if(gfire_buddy_is_talking(gf_buddy))
		{
			const gfire_game_data *voip_data = gfire_buddy_get_voip_data(gf_buddy);

			gchar *voip_name = gfire_game_name(voip_data->id);

			if(gfire_game_data_has_addr(voip_data))
			{
				gchar *addr = gfire_game_data_addr_str(voip_data);
				purple_notify_user_info_add_pair(p_user_info, NN(voip_name), addr);
				g_free(addr);
			}
			else
				purple_notify_user_info_add_pair(p_user_info, NN(voip_name), _("unknown"));

			if(voip_name) g_free(voip_name);
		}

		// Status
		gchar *status_msg = gfire_buddy_get_status_text(gf_buddy);
		if(status_msg && !gfire_buddy_is_playing(gf_buddy))
		{
			gchar *tmp = gfire_escape_html(status_msg);
			g_free(status_msg);
			purple_notify_user_info_add_pair(p_user_info, gfire_buddy_get_status_name(gf_buddy), tmp);
			if(tmp) g_free(tmp);
		}
		else
			purple_notify_user_info_add_pair(p_user_info, _("Status"), gfire_buddy_get_status_name(gf_buddy));

		// FoF common friends
		if(gfire_buddy_is_friend_of_friend(gf_buddy))
		{
			gchar *common_friends = gfire_buddy_get_common_buddies_str(gf_buddy);
			if(common_friends)
			{
				gchar *escaped_cf = gfire_escape_html(common_friends);
				g_free(common_friends);
				purple_notify_user_info_add_pair(p_user_info, _("Common Friends"), escaped_cf);
				g_free(escaped_cf);
			}
		}
	}
}



static GList *gfire_purple_status_types(PurpleAccount *p_account)
{
	PurpleStatusType *type;
	GList *types = NULL;

	type = purple_status_type_new_with_attrs(
		PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE, TRUE, FALSE,
		"message", "Message", purple_value_new(PURPLE_TYPE_STRING),
		NULL);
	types = g_list_append(types, type);

	type = purple_status_type_new_with_attrs(
		PURPLE_STATUS_AWAY, NULL, NULL, TRUE, TRUE, FALSE,
		"message", "Message", purple_value_new(PURPLE_TYPE_STRING),
		NULL);
	types = g_list_append(types, type);

	type = purple_status_type_new_with_attrs(
		PURPLE_STATUS_UNAVAILABLE, NULL, NULL, TRUE, TRUE, FALSE,
		"message", "Message", purple_value_new(PURPLE_TYPE_STRING),
		NULL);
	types = g_list_append(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	return types;
}

static void gfire_purple_login(PurpleAccount *p_account)
{
	gfire_data *gfire;

	PurpleConnection *gc = purple_account_get_connection(p_account);
	/* set connection flags for chats and im's tells purple what we can and can't handle */
	gc->flags |=  PURPLE_CONNECTION_NO_BGCOLOR | PURPLE_CONNECTION_NO_FONTSIZE
				| PURPLE_CONNECTION_NO_URLDESC | PURPLE_CONNECTION_NO_IMAGES;

	gfire = gfire_create(gc);
	if(!gfire)
	{
		purple_connection_error(gc, _("Protocol initialization failed."));
		return;
	}

	gc->proto_data = gfire;

	gfire_login(gfire);
}

static void gfire_purple_close(PurpleConnection *p_gc)
{
	gfire_close((gfire_data*)p_gc->proto_data);
}

static gint32 gfire_purple_im_send(PurpleConnection *p_gc, const gchar *p_who, const gchar *p_what, PurpleMessageFlags p_flags)
{
	PurplePresence *p = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurpleAccount *account = NULL;
	PurpleBuddy *buddy = NULL;

	if (!p_gc || !(gfire = (gfire_data*)p_gc->proto_data))
		return -1;

	gf_buddy = gfire_find_buddy(gfire, p_who, GFFB_NAME);
	if (!gf_buddy)
		return -1;

	account = purple_connection_get_account(p_gc);
	buddy = purple_find_buddy(account, gf_buddy->name);
	if(!buddy)
	{
		purple_conv_present_error(p_who, account, _("Message could not be sent. Buddy not in contact list"));
		return 1;
	}

	p = purple_buddy_get_presence(buddy);

	if(purple_presence_is_online(p))
	{
		gfire_buddy_send(gf_buddy, p_what);
		return 1;
	}
	else
	{
		purple_conv_present_error(p_who, account, _("Message could not be sent. Buddy offline"));
		return 1;
	}
}

static unsigned int gfire_purple_send_typing(PurpleConnection *p_gc, const gchar *p_who, PurpleTypingState p_state)
{
	gfire_buddy *gf_buddy = NULL;
	gfire_data *gfire = NULL;
	gboolean typenorm = TRUE;

	if (!p_gc || !(gfire = (gfire_data*)p_gc->proto_data))
		return 1;

	gf_buddy = gfire_find_buddy(gfire, p_who, GFFB_NAME);
	if (!gf_buddy) return 1;

	typenorm = purple_account_get_bool(purple_connection_get_account(p_gc), "typenorm", TRUE);

	if (!typenorm)
		return 0;

	gfire_buddy_send_typing(gf_buddy, p_state == PURPLE_TYPING);

	if(p_state == PURPLE_TYPING)
		return XFIRE_SEND_TYPING_TIMEOUT;
	else
		return 0;
}

static void gfire_purple_get_info(PurpleConnection *p_gc, const gchar *p_who)
{
	gfire_show_buddy_info((gfire_data*)p_gc->proto_data, p_who);
}

static void gfire_purple_set_status(PurpleAccount *p_account, PurpleStatus *p_status)
{
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;

	if (!purple_status_is_active(p_status))
		return;

	gc = purple_account_get_connection(p_account);
	gfire = (gfire_data *)gc->proto_data;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(status): got status change to name: %s id: %s\n",
		NN(purple_status_get_name(p_status)),
		NN(purple_status_get_id(p_status)));

	gfire_set_status(gfire, p_status);
}


static void gfire_purple_add_buddy(PurpleConnection *p_gc, PurpleBuddy *p_buddy, PurpleGroup *p_group)
{
	gfire_data *gfire = NULL;
	guint16 packet_len = 0;
	if(!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_buddy || !purple_buddy_get_name(p_buddy))
		return;

	gfire_buddy *gf_buddy = gfire_find_buddy(gfire, purple_buddy_get_name(p_buddy), GFFB_NAME);
	if(!gf_buddy)
	{
		gf_buddy = gfire_buddy_create(0, purple_buddy_get_name(p_buddy), purple_buddy_get_alias(p_buddy), GFBT_FRIEND);
		if(!gf_buddy)
			return;
	}
	else
		gfire_buddy_make_friend(gf_buddy, p_group);

	gfire_add_buddy(gfire, gf_buddy, p_group);

	packet_len = gfire_proto_create_invitation(p_buddy->name, "");
	if(packet_len > 0) gfire_send(p_gc, packet_len);
}

static void gfire_purple_remove_buddy(PurpleConnection *p_gc, PurpleBuddy *p_buddy, PurpleGroup *p_group)
{
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurpleAccount *account = NULL;
	gboolean buddynorm = TRUE;

	if(!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_buddy || !p_buddy->name) return;

	if(!(account = purple_connection_get_account(p_gc))) return;

	gf_buddy = gfire_find_buddy(gfire, p_buddy->name, GFFB_NAME);
	if(!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_purple_remove_buddy: buddy find returned NULL\n");
		return;
	}

	buddynorm = purple_account_get_bool(account, "buddynorm", TRUE);
	if(buddynorm && gfire_buddy_is_friend(gf_buddy))
	{
		gchar *tmp = g_strdup_printf(_("Not Removing %s"), gfire_buddy_get_name(gf_buddy));
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_purple_remove_buddy: buddynorm TRUE not removing buddy %s.\n", gfire_buddy_get_name(gf_buddy));
		purple_notify_message((void *)p_gc, PURPLE_NOTIFY_MSG_INFO, _("Xfire Buddy Removal Denied"), tmp, _("Account settings are set to not remove buddies\n"
																													   "The buddy will be restored on your next login"), NULL, NULL);
		if(tmp) g_free(tmp);
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Removing buddy: %s\n", gfire_buddy_get_name(gf_buddy));
	gfire_remove_buddy(gfire, gf_buddy, !buddynorm, FALSE);
}

/* connection keep alive.  We send a packet to the xfire server
 * saying we are still around.  Otherwise they will forcibly close our connection
 * purple allows for this, but calls us every 30 seconds.  We keep track of *all* sent
 * packets.  We only need to send this keep alive if we haven't sent anything in a long
 * time. So we watch and wait.
*/
static void gfire_purple_keep_alive(PurpleConnection *p_gc)
{
	gfire_data *gfire = NULL;


	if ((purple_connection_get_state(p_gc) != PURPLE_DISCONNECTED) &&
		(NULL != (gfire = (gfire_data *)p_gc->proto_data)))
		gfire_keep_alive(gfire);
}

/*
 *	purple callback function.  Not used directly.  Purple calls this callback
 *	when user right clicks on Xfire buddy (but before menu is displayed)
 *	Function adds right click "Join Game . . ." menu option.  If game is
 *	playable (configured through launch.xml), and user is not already in
 *	a game.
 *
 *	@param	node		Pidgin buddy list node entry that was right clicked
 *
 *	@return	Glist		list of menu items with callbacks attached (or null)
*/
static GList *gfire_purple_node_menu(PurpleBlistNode *p_node)
{
	GList *ret = NULL;
	PurpleMenuAction *me = NULL;
	PurpleBuddy *b = (PurpleBuddy*)p_node;
	gfire_buddy *gf_buddy = NULL;
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;


	if(!PURPLE_BLIST_NODE_IS_BUDDY(p_node))
		return NULL;

	if (!b || !b->account || !(gc = purple_account_get_connection(b->account)) ||
		!(gfire = (gfire_data *)gc->proto_data))
		return NULL;

	gf_buddy = gfire_find_buddy(gfire, b->name, GFFB_NAME);
	if(!gf_buddy)
		return NULL;

	if(!gfire_buddy_is_friend(gf_buddy))
	{
		me = purple_menu_action_new(_("Add as friend"),
									PURPLE_CALLBACK(gfire_buddy_menu_add_as_friend_cb),NULL, NULL);

		if (!me)
			return NULL;

		ret = g_list_append(ret, me);
	}

	if(gfire_buddy_is_playing(gf_buddy) && !gfire_is_playing(gfire))
	{
		const gfire_game_data *game_data = gfire_buddy_get_game_data(gf_buddy);

		if(gfire_game_playable(game_data->id))
		{
			me = purple_menu_action_new(_("Join Game ..."),
										PURPLE_CALLBACK(gfire_buddy_menu_joingame_cb),NULL, NULL);

			if(!me)
				return NULL;

			ret = g_list_append(ret, me);
		}
	}

	if(gfire_buddy_is_talking(gf_buddy) && !gfire_is_talking(gfire))
	{
		const gfire_game_data *voip_data = gfire_buddy_get_voip_data(gf_buddy);

		if(gfire_game_playable(voip_data->id))
		{
			me = purple_menu_action_new(_("Join VoIP ..."),
										PURPLE_CALLBACK(gfire_buddy_menu_joinvoip_cb),NULL, NULL);

			if (!me)
				return NULL;

			ret = g_list_append(ret, me);
		}
	}

	me = purple_menu_action_new(_("Xfire Profile"),
								PURPLE_CALLBACK(gfire_buddy_menu_profile_cb),NULL, NULL);

	if (!me)
		return NULL;

	ret = g_list_append(ret, me);

	return ret;
 }

void gfire_purple_nick_change_cb(PurpleConnection *p_gc, const gchar *p_entry)
{
	gfire_data *gfire = NULL;

	if (!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_entry) return;

	gfire_set_nick(gfire, p_entry);

	purple_connection_set_display_name(p_gc, p_entry);
}

static GList *gfire_purple_actions(PurplePlugin *p_plugin, gpointer p_context)
{
	GList *m = NULL;
	PurplePluginAction *act;

	act = purple_plugin_action_new(_("Change Nickname"),
			gfire_menu_action_nick_change_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(_("My Profile Page"),
			gfire_menu_action_profile_page_cb);
	m = g_list_append(m, act);
	m = g_list_append(m, NULL);
	act = purple_plugin_action_new(_("Reload Game Config"),
			gfire_menu_action_reload_lconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(_("Reload Game ID List"),
			gfire_menu_action_reload_gconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(_("Get Game ID List"),
			gfire_menu_action_get_gconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(_("Friend Search"),
			gfire_show_friend_search_cb);
	m = g_list_append(m, act);

#ifdef HAVE_GTK
	if(strcmp(purple_core_get_ui(), "gnt-purple") != 0)
	{
		act = purple_plugin_action_new(_("Manage Games"),
									   gfire_game_manager_show);
		m = g_list_append(m, act);
		act = purple_plugin_action_new(_("Server Browser"),
									   gfire_server_browser_show);
		m = g_list_append(m, act);
	}
#endif // HAVE_GTK

	m = g_list_append(m, NULL);
	act = purple_plugin_action_new(_("About"),
			gfire_menu_action_about_cb);
	m = g_list_append(m, act);
	return m;
}

static void gfire_purple_join_chat(PurpleConnection *p_gc, GHashTable *p_table)
{
	gfire_data *gfire = NULL;
	if (!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_table)
		return;

	guint8 *xid = NULL;
	gchar *room = (gchar*)g_hash_table_lookup(p_table, "room");
	gchar *pass = (gchar*)g_hash_table_lookup(p_table, "password");
	if(!(xid = g_hash_table_lookup(p_table, "chat_id")))
	{
		// no xid, we need to create this room
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Attempting to create chat room %s\n", NN(room));
		xid = g_malloc0(XFIRE_CHATID_LEN);
	}

	gfire_chat_join(xid, room, pass, p_gc);

	g_free(xid);
	g_free(room);
	g_free(pass);
}

static void gfire_purple_chat_leave(PurpleConnection *p_gc, int p_prpl_id)
{
	gfire_data *gfire = NULL;
	if (!p_gc || !(gfire = (gfire_data *)p_gc->proto_data))
		return;

	gfire_chat *chat = gfire_find_chat(gfire, &p_prpl_id, GFFC_PURPLEID);
	if(!chat)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_purple_chat_leave: Unknown purple chat id!\n");
		return;
	}

	gfire_leave_chat(gfire, chat);
}

static GList *gfire_purple_chat_info(PurpleConnection *p_gc)
{
	GList *m = NULL;
	struct proto_chat_entry *pce;

	pce = g_malloc0(sizeof(struct proto_chat_entry));
	pce->label = "_Room:";
	pce->identifier = "room";
	pce->required = TRUE;
	m = g_list_append(m, pce);

	pce = g_malloc0(sizeof(struct proto_chat_entry));
	pce->label = "_Password:";
	pce->identifier = "password";
	pce->secret = TRUE;
	m = g_list_append(m, pce);

	return m;
}

static GHashTable *gfire_purple_chat_info_defaults(PurpleConnection *p_gc, const gchar *p_chat_name)
{
	GHashTable *defaults;

	defaults = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

	if (p_chat_name != NULL)
		g_hash_table_insert(defaults, "room", g_strdup(p_chat_name));

	return defaults;
}

static gchar *gfire_purple_get_chat_name(GHashTable *p_data)
{
	return g_strdup(g_hash_table_lookup(p_data, "room"));
}

static void gfire_purple_reject_chat(PurpleConnection *p_gc, GHashTable *p_components)
{
	guint8 *cid = NULL;

	if (!p_gc || !p_components) return;

	if ((cid = g_hash_table_lookup(p_components, "chat_id")))
		gfire_chat_reject(cid, p_gc);
}

static void gfire_purple_chat_invite(PurpleConnection *p_gc, int p_id, const gchar *p_message, const gchar *p_who)
{
	gfire_data *gfire = NULL;
	gfire_chat *chat = NULL;
	gfire_buddy *buddy = NULL;

	if (!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_who) return;

	chat = gfire_find_chat(gfire, &p_id, GFFC_PURPLEID);
	if(!chat)
		return;

	buddy = gfire_find_buddy(gfire, p_who, GFFB_NAME);
	if(!buddy)
		return;

	gfire_chat_invite(chat, buddy);
}

static int gfire_purple_chat_send(PurpleConnection *p_gc, int p_id, const gchar *p_message, PurpleMessageFlags p_flags)
{
	gfire_data *gfire = NULL;
	gfire_chat *chat = NULL;

	if (!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_message) return -1;

	chat = gfire_find_chat(gfire, &p_id, GFFC_PURPLEID);
	if(!chat)
		return -1;

	gchar *unescaped = purple_unescape_html(p_message);

	gfire_chat_send(chat, unescaped);

	g_free(unescaped);

	return 0;
}

static void gfire_purple_chat_joined(PurpleConnection *p_gc, GList *p_members, guint8 *p_chat_id, gchar *p_topic, gchar *p_motd)
{
	gfire_chat *chat = NULL;
	gfire_data *gfire = NULL;

	if (!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_chat_id) return;

	chat = gfire_find_chat(gfire, p_chat_id, GFFC_CID);
	if(chat)
		return;

	chat = gfire_chat_create(p_chat_id, p_topic, p_motd);
	if(!chat)
		return;

	gfire_add_chat(gfire, chat);
	gfire_chat_show(chat);
}

static void gfire_purple_chat_change_motd(PurpleConnection *p_gc, int p_id, const gchar *p_motd)
{
	gfire_data *gfire = NULL;
	gfire_chat *chat = NULL;

	if (!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_motd) return;

	chat = gfire_find_chat(gfire, &p_id, GFFC_PURPLEID);
	if(!chat)
		return;

	gchar *unescaped = purple_unescape_html(p_motd);

	if(strlen(unescaped) > 200)
	{
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_WARNING, _("Xfire Groupchat"), _("MotD change failed"), _("The MotD contains more than 200 characters."), NULL, NULL);
		g_free(unescaped);
		return;
	}

	gfire_chat_change_motd(chat, unescaped);
	g_free(unescaped);
}

/**
 *
 * Plugin initialization section
 *
*/
static PurplePlugin *_gfire_plugin = NULL;

static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_CHAT_TOPIC,				/* Protocol options  */
	NULL,								/* user_splits */
	NULL,								/* protocol_options */
	NO_BUDDY_ICONS,						/* icon_spec */
	gfire_purple_blist_icon,			/* list_icon */
	gfire_purple_blist_emblems,			/* list_emblems */
	gfire_purple_status_text,			/* status_text */
	gfire_purple_blist_tooltip_text,	/* tooltip_text */
	gfire_purple_status_types,			/* away_states */
	gfire_purple_node_menu,				/* blist_node_menu */
	gfire_purple_chat_info,				/* chat_info */
	gfire_purple_chat_info_defaults,	/* chat_info_defaults */
	gfire_purple_login,					/* login */
	gfire_purple_close,					/* close */
	gfire_purple_im_send,				/* send_im */
	NULL,								/* set_info */
	gfire_purple_send_typing,			/* send_typing */
	gfire_purple_get_info,				/* get_info */
	gfire_purple_set_status,			/* set_status */
	NULL,								/* set_idle */
	NULL,								/* change_passwd */
	gfire_purple_add_buddy,				/* add_buddy */
	NULL,								/* add_buddies */
	gfire_purple_remove_buddy,			/* remove_buddy */
	NULL,								/* remove_buddies */
	NULL,								/* add_permit */
	NULL,								/* add_deny */
	NULL,								/* rem_permit */
	NULL,								/* rem_deny */
	NULL,								/* set_permit_deny */
	gfire_purple_join_chat,				/* join_chat */
	gfire_purple_reject_chat,			/* reject chat invite */
	gfire_purple_get_chat_name,			/* get_chat_name */
	gfire_purple_chat_invite,			/* chat_invite */
	gfire_purple_chat_leave,			/* chat_leave */
	NULL,								/* chat_whisper */
	gfire_purple_chat_send,				/* chat_send */
	gfire_purple_keep_alive,			/* keepalive */
	NULL,								/* register_user */
	NULL,								/* get_cb_info */
	NULL,								/* get_cb_away */
	NULL,								/* alias_buddy */
	NULL,								/* group_buddy */
	NULL,								/* rename_group */
	NULL,								/* buddy_free */
	NULL,								/* convo_closed */
	purple_normalize_nocase,			/* normalize */
	NULL,								/* set_buddy_icon */
	NULL,								/* remove_group */
	NULL,								/* get_cb_real_name */
	gfire_purple_chat_change_motd,		/* set_chat_topic */
	NULL,								/* find_blist_chat */
	NULL,								/* roomlist_get_list */
	NULL,								/* roomlist_cancel */
	NULL,								/* roomlist_expand_category */
	NULL,								/* can_receive_file */
	NULL,								/* send_file */
	NULL,								/* new_xfer */
	NULL,								/* offline_message */
	NULL,								/* whiteboard_prpl_ops */
	NULL,								/* send_raw */
	NULL,								/* roomlist_room_serialize */
	NULL,								/* unregister_user */
	NULL,								/* send_attention */
	NULL,								/* attention_types */

	/* padding */
	sizeof(PurplePluginProtocolInfo),
	NULL
};


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,		/* type */
	NULL,						/* ui_requirement */
	0,							/* flags */
	NULL,						/* dependencies */
	PURPLE_PRIORITY_DEFAULT,	/* priority */
	"prpl-xfire",				/* id */
	NULL,						/* name (done for NLS in _init_plugin) */
	GFIRE_VERSION,				/* version */
	NULL,						/* summary (done for NLS in _init_plugin) */
	NULL,						/* description (done for NLS in _init_plugin) */
	NULL,						/* author */
	GFIRE_WEBSITE,				/* homepage */
	NULL,						/* load */
	NULL,						/* unload */
	NULL,						/* destroy */
	NULL,						/* ui_info */
	&prpl_info,					/* extra_info */
	NULL,						/* prefs_info */
	gfire_purple_actions,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void _init_plugin(PurplePlugin *plugin)
{	
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif // ENABLE_NLS

	PurpleAccountOption *option;

	option = purple_account_option_string_new(_("Server"), "server",XFIRE_SERVER);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_int_new(_("Port"), "port", XFIRE_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_int_new(_("Version"), "version", XFIRE_PROTO_VERSION);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(_("Don't delete buddies from server"), "buddynorm", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(_("Buddies can see if I'm typing"), "typenorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(_("Auto detect for ingame status"), "ingamedetectionnorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(_("Notify me when my status is ingame"), "ingamenotificationnorm", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(_("Use server detection"), "server_detection_option", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	info.name = _("Xfire");
	info.summary = _("Xfire Protocol Plugin");
	info.description = _("Xfire Protocol Plugin");

	_gfire_plugin = plugin;
}

PURPLE_INIT_PLUGIN(gfire, _init_plugin, info)
