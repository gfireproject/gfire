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

#include "gf_menus.h"

#ifdef HAVE_GTK
#	include "gf_server_browser.h"
#endif // HAVE_GTK

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

#ifdef HAVE_GTK

void gfire_buddy_menu_server_details_cb(PurpleBlistNode *p_node, gpointer *p_data)
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
	if(gfire_game_data_is_valid(game_data))
		gfire_server_browser_show_single(game_data->id, game_data->ip.value, game_data->port);
}

#endif // HAVE_GTK

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

        purple_request_input(gc, NULL, _("Change Xfire nickname"), _("Leaving empty will clear your current nickname.\nMaximum length is 25 characters."), purple_connection_get_display_name(gc),
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
	gfire_data *gfire = NULL;
	char *msg = NULL;

	if(!(gfire = (gfire_data*)gc->proto_data)) return;

	const gchar *natTypeString = NULL;
	int natType = 0;

	gfire_p2p_connection *p2p = gfire_get_p2p(gfire);
	if(p2p)
		natType = gfire_p2p_connection_natType(p2p);

	switch(natType) {
	case 1:
		natTypeString = _("Full Cone NAT");
		break;
	case 2:
	case 3:
		natTypeString = _("Symmetric NAT");
		break;
	case 4:
		natTypeString = _("Restricted Cone NAT");
		break;
	default:
		natTypeString = _("No P2P available");
		break;
	}

	if(gfire_game_have_list())
	{
		gchar *version_str = gfire_game_get_version_str();
        msg = g_strdup_printf(_("Gfire Version: %s\nGfire Revision: %s\nGame List Version: %s\nNAT Type: %d (%s)"),
                              GFIRE_VERSION_STRING, GFIRE_REVISION, version_str, natType, natTypeString);
		g_free(version_str);
	}
	else
	{
        msg = g_strdup_printf(_("Gfire Version: %s\nGfire Revision: %s\nNAT Type: %d (%s)"), GFIRE_VERSION_STRING, GFIRE_REVISION,
                              natType, natTypeString);
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
