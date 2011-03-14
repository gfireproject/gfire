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

#include "gfire_proto.h"
#include "gf_network.h"
#include "gfire.h"
#include "gf_menus.h"
#include "gf_games.h"
#include "gf_friend_search.h"
#include "gf_purple.h"
#include "gf_chat_proto.h"

#ifdef USE_GAME_DETECTION
#	include "gf_game_detection.h"
#endif // USE_GAME_DETECTION

#ifdef HAVE_GTK
#	include "gf_server_query.h"
#endif // HAVE_GTK

static PurplePlugin *_gfire_plugin = NULL;

static void gfire_purple_blist_node_added_signal(PurpleBlistNode *p_node)
{
	if(!p_node)
		return;

	if(PURPLE_BLIST_NODE_IS_CHAT(p_node))
	{
		PurpleChat *chat = PURPLE_CHAT(p_node);
		PurpleAccount *account = purple_chat_get_account(chat);
		PurpleConnection *connection = purple_account_get_connection(account);

		// Only for our plugin
		if(!g_ascii_strcasecmp(GFIRE_PRPL_ID, purple_account_get_protocol_id(account)))
		{
			if(connection && PURPLE_CONNECTION_IS_CONNECTED(connection))
			{
				gfire_chat *gchat = gfire_find_chat(connection->proto_data,
													g_hash_table_lookup(purple_chat_get_components(chat), "room"),
													GFFC_TOPIC);

				if(!gchat)
					return;

				gfire_chat_set_purple_chat(gchat, chat);
				gfire_chat_set_saved(gchat, TRUE);

				purple_debug_misc("gfire", "chat room added\n");
			}
		}
	}
}

static void gfire_purple_blist_node_removed_signal(PurpleBlistNode *p_node)
{
	if(!p_node)
		return;

	if(PURPLE_BLIST_NODE_IS_CHAT(p_node))
	{
		PurpleChat *chat = PURPLE_CHAT(p_node);
		PurpleAccount *account = purple_chat_get_account(chat);
		PurpleConnection *connection = purple_account_get_connection(account);

		// Only for our plugin
		if(!g_ascii_strcasecmp(GFIRE_PRPL_ID, purple_account_get_protocol_id(account)))
		{
			if(connection && PURPLE_CONNECTION_IS_CONNECTED(connection))
			{
				GHashTable *comp = purple_chat_get_components(chat);
				gfire_chat *gchat = gfire_find_chat(connection->proto_data,
													g_hash_table_lookup(comp, "room"),
													GFFC_TOPIC);

				// Remove the room directly
				if(!gchat && g_hash_table_lookup(comp, "chat_id"))
				{
					guint8 *chat_id = purple_base64_decode(g_hash_table_lookup(comp, "chat_id"), NULL);
					guint16 len = gfire_chat_proto_create_save_chat_room(chat_id, FALSE);
					if(len > 0) gfire_send(connection, len);
					g_free(chat_id);

					purple_debug_misc("gfire", "chat room removed by ID\n");

					return;
				}
				// Insufficient data for a remove available
				else
					return;

				gfire_chat_set_saved(gchat, FALSE);
				purple_debug_misc("gfire", "chat room removed by topic\n");
			}
		}
	}
}

static void gfire_purple_blist_node_ext_menu_signal(PurpleBlistNode *p_node, GList **p_menu)
{
	if(!p_node)
		return;

	if(PURPLE_BLIST_NODE_IS_GROUP(p_node))
	{
		gint clanid = purple_blist_node_get_int(p_node, "clanid");
		// Skip non-community groups
		if(clanid == 0)
			return;

		// Find the account/gfire data for this group
		GSList *accounts = purple_group_get_accounts(PURPLE_GROUP(p_node));
		PurpleAccount *account = NULL;
		GSList *cur = accounts;
		while(cur)
		{
			if(purple_account_is_connected((PurpleAccount*)cur->data) &&
			   !g_ascii_strcasecmp(GFIRE_PRPL_ID, purple_account_get_protocol_id((PurpleAccount*)cur->data)))
			{
				account = (PurpleAccount*)cur->data;
				break;
			}
			cur = g_slist_next(cur);
		}
		g_slist_free(accounts);

		if(!account)
			return;

		gfire_data *gfire = purple_account_get_connection(account)->proto_data;
		if(!gfire)
			return;

		gfire_clan *clan = gfire_find_clan(gfire, clanid);
		// Invalid ID?
		if(!clan)
			return;

		PurpleMenuAction *me = NULL;
		// Community site
		me = purple_menu_action_new(_("Xfire Community Site"),
										PURPLE_CALLBACK(gfire_clan_menu_site_cb), gfire, NULL);
		if(!me)
			return;

		*p_menu = g_list_append(*p_menu, me);
	}
}

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
	static gchar emblem[100];

	if (!p_b || (NULL == p_b->account) || !(gc = purple_account_get_connection(p_b->account)) ||
						 !(gfire = (gfire_data *) gc->proto_data))
		return NULL;

	gf_buddy = gfire_find_buddy(gfire, purple_buddy_get_name(p_b), GFFB_NAME);

	p = purple_buddy_get_presence(p_b);

	if(purple_presence_is_online(p) == TRUE)
	{
		if(gfire_buddy_is_playing(gf_buddy) && !gfire_buddy_is_talking(gf_buddy))
		{
			const gfire_game_data *game = gfire_buddy_get_game_data(gf_buddy);
			gchar *game_name = gfire_game_short_name(game->id);
			if(game_name)
			{
				g_snprintf(emblem, 100, "game_%s", game_name);
				g_free(game_name);
				gchar *file = g_strdup_printf("%s.png", emblem);
				// See Pidgins gtkblist.c for the following line
				gchar *path = g_build_filename(DATADIR, "pixmaps", "pidgin", "emblems", "16", file, NULL);
				g_free(file);

				if(g_file_test(path, G_FILE_TEST_EXISTS))
				{
					g_free(path);
					return emblem;
				}

				g_free(path);
			}

			return "game";
		}
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
		gchar *status = gfire_buddy_get_status_text(gf_buddy, FALSE);
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

			gchar *game_name = gfire_game_name(game_data->id, TRUE);
			purple_notify_user_info_add_pair(p_user_info, _("Game"), NN(game_name));
			if(game_name) g_free(game_name);

			if(gfire_game_data_has_addr(game_data))
			{
				gchar *addr = gfire_game_data_addr_str(game_data);
				purple_notify_user_info_add_pair(p_user_info, _("Server"), addr);
				g_free(addr);
			}
		}

		if(gfire_buddy_get_game_client_data(gf_buddy))
		{
			const GList *current = gfire_buddy_get_game_client_data(gf_buddy);
			for(; current; current = g_list_next(current))
			{
				if(((game_client_data*)current->data)->value && (*((game_client_data*)current->data)->value != 0))
					purple_notify_user_info_add_pair(p_user_info, NN(((game_client_data*)current->data)->key),
													 ((game_client_data*)current->data)->value);
			}
		}

		// VoIP Info
		if(gfire_buddy_is_talking(gf_buddy))
		{
			const gfire_game_data *voip_data = gfire_buddy_get_voip_data(gf_buddy);

			gchar *voip_name = gfire_game_name(voip_data->id, TRUE);

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
		gchar *status_msg = gfire_buddy_get_status_text(gf_buddy, TRUE);
		if(status_msg)
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
	static gboolean signals_registered = FALSE;
	if(!signals_registered)
	{
		purple_signal_connect(purple_blist_get_handle(), "blist-node-added", _gfire_plugin,
										   PURPLE_CALLBACK(gfire_purple_blist_node_added_signal), NULL);
		purple_signal_connect(purple_blist_get_handle(), "blist-node-removed", _gfire_plugin,
										   PURPLE_CALLBACK(gfire_purple_blist_node_removed_signal), NULL);
		purple_signal_connect(purple_blist_get_handle(), "blist-node-extended-menu", _gfire_plugin,
										   PURPLE_CALLBACK(gfire_purple_blist_node_ext_menu_signal), NULL);

		signals_registered = TRUE;
	}

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

static int gfire_purple_im_send(PurpleConnection *p_gc, const gchar *p_who, const gchar *p_what, PurpleMessageFlags p_flags)
{
	PurplePresence *p = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurpleAccount *account = NULL;
	PurpleBuddy *buddy = NULL;

	if (!p_gc || !(gfire = (gfire_data*)p_gc->proto_data))
		return -ENOTCONN;

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

static void gfire_purple_add_buddy_msg_ok_cb(PurpleBuddy *p_buddy, const gchar *p_msg)
{
	PurpleConnection *gc = purple_account_get_connection(purple_buddy_get_account(p_buddy));
	if(!gc)
		return;

	// Send the invite with a message
	guint16 packet_len = gfire_proto_create_invitation(purple_buddy_get_name(p_buddy), p_msg ? p_msg : "");
	if(packet_len > 0) gfire_send(gc, packet_len);
}

static void gfire_purple_add_buddy_msg_cancel_cb(PurpleBuddy *p_buddy)
{
	PurpleConnection *gc = purple_account_get_connection(purple_buddy_get_account(p_buddy));
	if(!gc)
		return;

	// Just send the invite
	guint16 packet_len = gfire_proto_create_invitation(purple_buddy_get_name(p_buddy), "");
	if(packet_len > 0) gfire_send(gc, packet_len);
}

static void gfire_purple_add_buddy(PurpleConnection *p_gc, PurpleBuddy *p_buddy, PurpleGroup *p_group)
{
	gfire_data *gfire = NULL;
	if(!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_buddy || !purple_buddy_get_name(p_buddy))
		return;

	gfire_group *group = NULL;
	if(p_group && p_group != purple_find_group(GFIRE_DEFAULT_GROUP_NAME) &&
	   p_group != purple_find_group(GFIRE_FRIENDS_OF_FRIENDS_GROUP_NAME))
	{
		group = gfire_find_group(gfire, p_group, GFFG_PURPLE);
		if(!group)
		{
			group = gfire_group_create(gfire, purple_group_get_name(p_group), 0);
			gfire_add_group(gfire, group);
		}
	}

	gfire_buddy *gf_buddy = gfire_find_buddy(gfire, purple_buddy_get_name(p_buddy), GFFB_NAME);
	if(!gf_buddy)
	{
		gf_buddy = gfire_buddy_create(0, purple_buddy_get_name(p_buddy), purple_buddy_get_alias(p_buddy), GFBT_FRIEND);
		if(!gf_buddy)
			return;

		gfire_add_buddy(gfire, gf_buddy, group);
	}
	else
		gfire_buddy_make_friend(gf_buddy, group);

	// Request the invitation message
	purple_request_input(p_gc, _("Xfire Invitation Message"),
						 NULL, _("Please enter the message you want to send your buddy with this invite:"),
						 _("Please add me to your friends list!"), TRUE, FALSE, "",
						 _("Invite with a message"), G_CALLBACK(gfire_purple_add_buddy_msg_ok_cb),
						 _("Invite without a message"), G_CALLBACK(gfire_purple_add_buddy_msg_cancel_cb),
						 purple_connection_get_account(p_gc), NULL, NULL, p_buddy);
}

static void gfire_purple_remove_buddy(PurpleConnection *p_gc, PurpleBuddy *p_buddy, PurpleGroup *p_group)
{
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurpleAccount *account = NULL;

	if(!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_buddy || !p_buddy->name) return;

	if(!(account = purple_connection_get_account(p_gc))) return;

	gf_buddy = gfire_find_buddy(gfire, p_buddy->name, GFFB_NAME);
	if(!gf_buddy)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_purple_remove_buddy: buddy find returned NULL\n");
		return;
	}

	if(gfire_buddy_is_friend(gf_buddy))
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Removing buddy: %s\n", gfire_buddy_get_name(gf_buddy));
		gfire_remove_buddy(gfire, gf_buddy, TRUE, FALSE);
	}
	else
	{
		purple_notify_message((void *)p_gc, PURPLE_NOTIFY_MSG_INFO, _("Xfire Buddy Removal"), _("Xfire Buddy Removal"),
							  _("You have removed a buddy which is not on your friends list, it will be "
								"restored on the next login."), NULL, NULL);
		gfire_remove_buddy(gfire, gf_buddy, FALSE, FALSE);
	}
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

	if(PURPLE_BLIST_NODE_IS_BUDDY(p_node))
	{
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

#ifdef USE_GAME_DETECTION
		if(gfire_buddy_is_playing(gf_buddy) && !gfire_game_detector_is_playing())
#else
		if(gfire_buddy_is_playing(gf_buddy))
#endif // USE_GAME_DETECTION
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

#ifdef HAVE_GTK
			if(game_data->ip.value && gfire_server_query_supports(gfire_game_server_query_type(game_data->id)))
			{
				me = purple_menu_action_new(_("Display Server Details"),
											PURPLE_CALLBACK(gfire_buddy_menu_server_details_cb), NULL, NULL);

				if(!me)
					return NULL;

				ret = g_list_append(ret, me);
			}
#endif // HAVE_GTK
		}

#ifdef USE_GAME_DETECTION
		if(gfire_buddy_is_talking(gf_buddy) && !gfire_game_detector_is_voiping())
#else
		if(gfire_buddy_is_talking(gf_buddy))
#endif // USE_GAME_DETECTION
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

	return NULL;
 }

void gfire_purple_nick_change_cb(PurpleConnection *p_gc, const gchar *p_entry)
{
	gfire_data *gfire = NULL;

	if (!p_gc || !(gfire = (gfire_data *)p_gc->proto_data) || !p_entry) return;

	gfire_set_nick(gfire, p_entry);

	purple_connection_set_display_name(p_gc, p_entry);
}

static gboolean gfire_purple_actions_game_cb(const gfire_game_configuration *p_gconf, GList **p_list)
{
	if(p_gconf && p_list)
	{
		const gfire_game *game = gfire_game_by_id(p_gconf->game_id);
		if(game)
		{
			gchar *label = g_strdup_printf(_("Launch %s"), game->name);
			PurplePluginAction *act = purple_plugin_action_new(label, gfire_menu_action_launch_game_cb);
			if(act)
			{
				act->user_data = GUINT_TO_POINTER(p_gconf->game_id);
				*p_list = g_list_append(*p_list, act);
			}
		}
	}

	return FALSE;
}

static GList *gfire_purple_actions(PurplePlugin *p_plugin, gpointer p_context)
{
	GList *m = NULL;
	PurplePluginAction *act;

	// General things
	act = purple_plugin_action_new(_("Change Nickname"),
			gfire_menu_action_nick_change_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(_("My Profile Page"),
			gfire_menu_action_profile_page_cb);
	m = g_list_append(m, act);
	m = g_list_append(m, NULL);
	act = purple_plugin_action_new(_("Friend Search"),
			gfire_show_friend_search_cb);
	m = g_list_append(m, act);

	// Game configuration
	act = purple_plugin_action_new(_("Reload Game Config"),
			gfire_menu_action_reload_lconfig_cb);
	m = g_list_append(m, act);

#ifdef HAVE_GTK
	if(strcmp(purple_core_get_ui(), "gnt-purple") != 0)
	{
		act = purple_plugin_action_new(_("Manage Games"), gfire_game_manager_show);
		m = g_list_append(m, act);

		// Server browser
		act = purple_plugin_action_new(_("Server Browser"), gfire_show_server_browser);
		m = g_list_append(m, act);
	}
#endif // HAVE_GTK

	// Game launchers
	m = g_list_append(m, NULL);
	gfire_game_config_foreach(G_CALLBACK(gfire_purple_actions_game_cb), &m);
	if(g_list_last(m)->data)
		m = g_list_append(m, NULL);

	// About
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

	gchar *cid_base64 = NULL;
	guint8 *cid = NULL;
	gchar *room = (gchar*)g_hash_table_lookup(p_table, "room");
	gchar *pass = (gchar*)g_hash_table_lookup(p_table, "password");
	if(!(cid_base64 = g_hash_table_lookup(p_table, "chat_id")))
	{
		// no cid, we need to create this room
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Attempting to create chat room %s\n", NN(room));
		cid = g_malloc0(XFIRE_CHATID_LEN);
	}
	else
	{
		cid = purple_base64_decode(cid_base64, NULL);
	}

	gfire_chat_join(cid, room, pass, p_gc);

	g_free(cid);
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
	pce->label = _("_Room:");
	pce->identifier = "room";
	pce->required = TRUE;
	m = g_list_append(m, pce);

	pce = g_malloc0(sizeof(struct proto_chat_entry));
	pce->label = _("_Password:");
	pce->identifier = "password";
	pce->secret = TRUE;
	m = g_list_append(m, pce);

	return m;
}

static GHashTable *gfire_purple_chat_info_defaults(PurpleConnection *p_gc, const gchar *p_chat_name)
{
	GHashTable *defaults;

	defaults = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	if (p_chat_name != NULL)
		g_hash_table_insert(defaults, g_strdup("room"), g_strdup(p_chat_name));

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

	gfire_chat_send(chat, p_message);
	return 0;
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
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_WARNING, _("Xfire Groupchat"), _("MotD change failed"),
							  _("The MotD contains more than 200 characters."), NULL, NULL);
		g_free(unescaped);
		return;
	}

	gfire_chat_change_motd(chat, unescaped);
	g_free(unescaped);
}

// By default everyone can receive a file
static gboolean gfire_purple_can_receive_file(PurpleConnection *p_gc, const gchar *p_who)
{
	if(!p_gc || !p_gc->proto_data || !p_who)
		return FALSE;

	gfire_data *gfire = p_gc->proto_data;

	gfire_buddy *gf_buddy = gfire_find_buddy(gfire, p_who, GFFB_NAME);
	if(!gf_buddy)
	{
		purple_debug_warning("gfire", "gfire_purple_can_receive_file: called on invalid buddy\n");
		return FALSE;
	}

	return (gfire_has_p2p(gfire) && gfire_buddy_is_online(gf_buddy) && gfire_buddy_has_p2p(gf_buddy));
}

static PurpleXfer *gfire_purple_new_xfer(PurpleConnection *p_gc, const gchar *p_who)
{
	if(!p_gc || !p_gc->proto_data || !p_who)
		return NULL;

	gfire_data *gfire = p_gc->proto_data;

	gfire_buddy *gf_buddy = gfire_find_buddy(gfire, p_who, GFFB_NAME);
	if(!gf_buddy)
	{
		purple_debug_warning("gfire", "gfire_purple_new_xfer: called on invalid buddy\n");
		return NULL;
	}

	if(!gfire_buddy_uses_p2p(gf_buddy))
		gfire_buddy_request_p2p(gf_buddy, TRUE);

	PurpleXfer *xfer = purple_xfer_new(purple_connection_get_account(p_gc), PURPLE_XFER_SEND, p_who);
	if(!xfer)
	{
		purple_debug_warning("gfire", "gfire_purple_new_xfer: xfer creation failed\n");
		return NULL;
	}

	xfer->data = gf_buddy;
	purple_xfer_set_init_fnc(xfer, gfire_buddy_p2p_ft_init);

	return xfer;
}

static void gfire_purple_send_file(PurpleConnection *p_gc, const gchar *p_who, const gchar *p_filename)
{
	if(!p_gc || !p_gc->proto_data || !p_who)
		return;

	gfire_buddy *gf_buddy = gfire_find_buddy(p_gc->proto_data, p_who, GFFB_NAME);
	if(!gf_buddy || !gfire_buddy_is_online(gf_buddy))
			return;

	purple_debug_info("gfire", "request for a file transfer!\n");

	PurpleXfer *xfer = gfire_purple_new_xfer(p_gc, p_who);
	if(!xfer)
		return;

	if(p_filename)
		purple_xfer_request_accepted(xfer, p_filename);
	else
		purple_xfer_request(xfer);
}

static void gfire_purple_rename_group(PurpleConnection *p_gc, const gchar *p_old_name, PurpleGroup *p_group, GList *p_buddies)
{
	// Handle FoF group renamings
	if(g_utf8_collate(p_old_name, GFIRE_FRIENDS_OF_FRIENDS_GROUP_NAME) == 0)
	{
		purple_debug_info("gfire", "FoF group has been renamed, restoring the name...\n");
		purple_blist_rename_group(p_group, GFIRE_FRIENDS_OF_FRIENDS_GROUP_NAME);

		purple_notify_info(p_gc, _("Friends of friends group name restored"), _("Group name restored"), _("You have renamed Xfire's FoF group name. Unfortunately we had to restore this groups name."));

		return;
	}
	// Handle Clan group renamings
	else if(purple_blist_node_get_int(PURPLE_BLIST_NODE(p_group), "clanid") != 0)
	{
		gfire_clan *clan = gfire_find_clan(p_gc->proto_data, purple_blist_node_get_int(PURPLE_BLIST_NODE(p_group), "clanid"));
		if(!clan)
			return;

		// Was the name really changed?
		if(g_utf8_collate(purple_group_get_name(p_group), gfire_clan_get_name(clan)) == 0)
			return;

		purple_debug_info("gfire", "Clan group has been renamed, restoring the name...\n");
		purple_blist_rename_group(p_group, gfire_clan_get_name(clan));

		purple_notify_info(p_gc, _("Clan's group name restored"), _("Group name restored"), _("You have renamed the group name of a Xfire clan. Unfortunately we had to restore this groups name."));
		return;
	}

	// Rename custom groups
	gfire_group *group = gfire_find_group(p_gc->proto_data, p_old_name, GFFB_NAME);
	if(!group)
		return;

	gfire_group_rename(group, purple_group_get_name(p_group));
}

static void gfire_purple_remove_group(PurpleConnection *p_gc, PurpleGroup *p_group)
{
	if(!p_gc || !p_group)
		return;

	gfire_group *group = gfire_find_group(p_gc->proto_data, p_group, GFFG_PURPLE);
	if(group)
	{
		gfire_remove_group(p_gc->proto_data, group, TRUE);
	}
}

static void gfire_purple_group_buddy(PurpleConnection *p_gc, const char *p_who,
									 const char *p_old_group, const char *p_new_group)
{
	if(!p_gc || !p_who || !p_new_group)
		return;

	gfire_data *gfire = p_gc->proto_data;

	gfire_buddy *buddy = gfire_find_buddy(gfire, p_who, GFFB_NAME);
	if(!buddy)
		return;

	if(!gfire_buddy_is_friend(buddy))
	{
		// TODO: Ask if the user wants to add this buddy
		return;
	}

	if(p_old_group)
	{
		gfire_group *old_group = gfire_find_group(gfire, p_old_group, GFFG_NAME);
		if(old_group)
		{
			gfire_group_remove_buddy(old_group, buddy->userid);
		}
	}

	if(strcmp(p_new_group, GFIRE_DEFAULT_GROUP_NAME) &&
	   strcmp(p_new_group, GFIRE_FRIENDS_OF_FRIENDS_GROUP_NAME))
	{
		gfire_group *new_group = gfire_find_group(gfire, p_new_group, GFFG_NAME);
		if(!new_group)
		{
			new_group = gfire_group_create(gfire, p_new_group, 0);
			gfire_add_group(gfire, new_group);
		}

		gfire_group_add_buddy(new_group, buddy->userid, TRUE);
	}
}

static gboolean gfire_purple_offline_message(const PurpleBuddy *p_buddy)
{
	return FALSE;
}

/**
 *
 * Plugin initialization section
 *
*/

static gboolean _load_plugin(PurplePlugin *p_plugin)
{
	gfire_chat_register_commands();
	return TRUE;
}

static gboolean _unload_plugin(PurplePlugin *p_plugin)
{
	gfire_chat_unregister_commands();

#ifdef USE_NOTIFICATIONS
	gfire_notify_uninit();
#endif // USE_NOTIFICATION

	// Delete all game data
	gfire_game_config_cleanup();
	gfire_game_cleanup();

	purple_signals_disconnect_by_handle(p_plugin);
	return TRUE;
}

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
	gfire_purple_group_buddy,			/* group_buddy */
	gfire_purple_rename_group,			/* rename_group */
	NULL,								/* buddy_free */
	NULL,								/* convo_closed */
	purple_normalize_nocase,			/* normalize */
	NULL,								/* set_buddy_icon */
	gfire_purple_remove_group,			/* remove_group */
	NULL,								/* get_cb_real_name */
	gfire_purple_chat_change_motd,		/* set_chat_topic */
	NULL,								/* find_blist_chat */
	NULL,								/* roomlist_get_list */
	NULL,								/* roomlist_cancel */
	NULL,								/* roomlist_expand_category */
	gfire_purple_can_receive_file,		/* can_receive_file */
	gfire_purple_send_file,				/* send_file */
	gfire_purple_new_xfer,				/* new_xfer */
	gfire_purple_offline_message,		/* offline_message */
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
	GFIRE_PRPL_ID,				/* id */
	NULL,						/* name (done for NLS in _init_plugin) */
	GFIRE_VERSION_STRING,		/* version */
	NULL,						/* summary (done for NLS in _init_plugin) */
	NULL,						/* description (done for NLS in _init_plugin) */
	NULL,						/* author */
	GFIRE_WEBSITE,				/* homepage */
	_load_plugin,				/* load */
	_unload_plugin,				/* unload */
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

static void _init_plugin(PurplePlugin *p_plugin)
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

	option = purple_account_option_bool_new(_("Buddies can see if I'm typing"), "typenorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

#ifdef USE_GAME_DETECTION
	option = purple_account_option_bool_new(_("Auto detect for ingame status"), "ingamedetectionnorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(_("Change my status for other protocols as well"), "use_global_status", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Notify me when my status is ingame"), "ingamenotificationnorm", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(_("Enable server detection"), "server_detection_option", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
#endif // USE_GAME_DETECTION

	option = purple_account_option_bool_new(_("Use Xfires P2P features"), "p2p_option", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

#ifdef USE_NOTIFICATIONS
	option = purple_account_option_bool_new(_("Display notifications for certain events"), "use_notify", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
#endif // USE_NOTIFICATIONS

	option = purple_account_option_bool_new(_("Show Friends of Friends"), "show_fofs", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	info.name = _("Xfire");
	info.summary = _("Xfire Protocol Plugin");
	info.description = _("Xfire Protocol Plugin");

	_gfire_plugin = p_plugin;
}

PURPLE_INIT_PLUGIN(gfire, _init_plugin, info)
