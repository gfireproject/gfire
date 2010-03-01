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

#include "gf_menus.h"

void gfire_clan_menu_site_cb(PurpleBlistNode *p_node, gpointer *p_data)
{
	if(!p_node || !p_node)
		return;

	gint clanid = purple_blist_node_get_int(p_node, "clanid");
	gfire_clan *clan = gfire_find_clan((gfire_data*)p_data, clanid);
	if(!clan)
		return;

	gchar *uri;
	uri = g_strdup_printf(XFIRE_COMMUNITY_URL, gfire_clan_get_short_name(clan));
	purple_notify_uri(gfire_get_connection((gfire_data*)p_data), uri);
	g_free(uri);
 }

void gfire_buddy_menu_profile_cb(PurpleBlistNode *p_node, gpointer *p_data)
{
	PurpleBuddy *b = (PurpleBuddy*)p_node;
	if (!b || !(b->name)) return;

	gchar *uri;
	uri = g_strdup_printf("%s%s", XFIRE_PROFILE_URL, purple_buddy_get_name(b));
	purple_notify_uri(purple_account_get_connection(b->account), uri);
	g_free(uri);
 }

void gfire_buddy_menu_add_as_friend_cb(PurpleBlistNode *p_node, gpointer *p_data)
{
	PurpleBuddy *b = (PurpleBuddy*)p_node;

	if (!b || !b->account)
		return;

	purple_blist_request_add_buddy(b->account, purple_buddy_get_name(b), GFIRE_DEFAULT_GROUP_NAME, NULL);
}


 /**
  * Callback function for pidgin buddy list right click menu.  This callback
  * is what the interface calls when the Join Game ... menu option is selected
  *
  * @param p_node		BuddyList node provided by interface
  * @param p_data		Gpointer data (not used)
*/
void gfire_buddy_menu_joingame_cb(PurpleBlistNode *p_node, gpointer *p_data)
{
	PurpleBuddy *b = (PurpleBuddy*)p_node;
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;

	if (!b || !b->account || !(gc = purple_account_get_connection(b->account)) ||
		!(gfire = (gfire_data*)gc->proto_data)) return;

	gf_buddy = gfire_find_buddy(gfire, purple_buddy_get_name(b), GFFB_NAME);
	if(!gf_buddy)
		return;

	const gfire_game_data *game_data = gfire_buddy_get_game_data(gf_buddy);
	if (gfire_game_data_is_valid(game_data) && gfire_game_playable(game_data->id))
		gfire_join_game(game_data);
}

/**
  * Callback function for pidgin buddy list right click menu.  This callback
  * is what the interface calls when the Join VoIP ... menu option is selected
  *
  * @param p_node		BuddyList node provided by interface
  * @param p_data		Gpointer data (not used)
*/
void gfire_buddy_menu_joinvoip_cb(PurpleBlistNode *p_node, gpointer *p_data)
{
	PurpleBuddy *b = (PurpleBuddy*)p_node;
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;

	if (!b || !b->account || !(gc = purple_account_get_connection(b->account)) ||
		!(gfire = (gfire_data*)gc->proto_data)) return;

	gf_buddy = gfire_find_buddy(gfire, purple_buddy_get_name(b), GFFB_NAME);
	if(!gf_buddy)
		return;

	const gfire_game_data *voip_data = gfire_buddy_get_voip_data(gf_buddy);
	if (gfire_game_data_is_valid(voip_data) && gfire_game_playable(voip_data->id))
		gfire_join_game(voip_data);
}

void gfire_menu_action_nick_change_cb(PurplePluginAction *p_action)
{
	PurpleConnection *gc = (PurpleConnection *)p_action->context;
	PurpleAccount *account = purple_connection_get_account(gc);

	purple_request_input(gc, NULL, _("Change Xfire nickname"), _("Leaving empty will clear your current nickname."), purple_connection_get_display_name(gc),
		FALSE, FALSE, NULL, _("OK"), G_CALLBACK(gfire_purple_nick_change_cb), _("Cancel"), NULL, account, NULL, NULL, gc);
}

void gfire_menu_action_reload_lconfig_cb(PurplePluginAction *p_action)
{
	if(!gfire_game_load_config_xml(TRUE))
		purple_notify_message(p_action->context, PURPLE_NOTIFY_MSG_ERROR, _("Gfire XML Reload"), _("Reloading gfire_game_config.xml"), _("Operation failed. File not found or content was incorrect."), NULL, NULL);
	else
		purple_notify_message(p_action->context, PURPLE_NOTIFY_MSG_INFO, _("Gfire XML Reload"), _("Reloading gfire_game_config.xml"), _("Reloading was successful."), NULL, NULL);
}

static void gfire_menu_action_website_cb(PurpleConnection *p_gc)
{
	purple_notify_uri((void *)p_gc, GFIRE_WEBSITE);
}

static void gfire_menu_action_wiki_cb(PurpleConnection *p_gc)
{
	purple_notify_uri((void *)p_gc, GFIRE_WIKI);
}

void gfire_menu_action_about_cb(PurplePluginAction *p_action)
{
	PurpleConnection *gc = (PurpleConnection *)p_action->context;
	char *msg = NULL;

	if(gfire_game_have_list())
	{
		gchar *version_str = gfire_game_get_version_str();
		msg = g_strdup_printf(_("Gfire Version:\t\t%s\nGame List Version:\t%s"), GFIRE_VERSION_STRING, version_str);
		g_free(version_str);
	}
	else
	{
		msg = g_strdup_printf(_("Gfire Version:\t\t%s"), GFIRE_VERSION_STRING);
	}

	purple_request_action(gc, _("About Gfire"), _("Xfire Plugin for Pidgin"), msg, PURPLE_DEFAULT_ACTION_NONE,
						  purple_connection_get_account(gc), NULL, NULL, gc, 3, _("Close"), NULL,
						  _("Website"), G_CALLBACK(gfire_menu_action_website_cb),
						  _("Wiki"), G_CALLBACK(gfire_menu_action_wiki_cb));

	if(msg) g_free(msg);
}

void gfire_menu_action_profile_page_cb(PurplePluginAction *p_action)
{
	PurpleConnection *gc = (PurpleConnection *)p_action->context;
	PurpleAccount *a = purple_connection_get_account(gc);

	gchar *uri = g_strdup_printf("%s%s", XFIRE_PROFILE_URL, purple_account_get_username(a));
	purple_notify_uri((void *)gc, uri);
	g_free(uri);
}

void gfire_menu_action_launch_game_cb(PurplePluginAction *p_action)
{
	guint32 game_id = GPOINTER_TO_UINT(p_action->user_data);
	if(game_id)
	{
		gfire_game_data launch_data;
		memset(&launch_data, 0, sizeof(gfire_game_data));

		launch_data.id = game_id;

		gfire_join_game(&launch_data);
	}
}
