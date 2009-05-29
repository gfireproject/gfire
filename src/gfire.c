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

#include "gfire.h"
#include "gf_network.h"
#include "gf_packet.h"
#include "gf_games.h"
#include "gf_chat.h"

static const char *gfire_blist_icon(PurpleAccount *a, PurpleBuddy *b);
static const char *gfire_blist_emblems(PurpleBuddy *b);
static void gfire_blist_tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full);
static GList *gfire_status_types(PurpleAccount *account);
static int gfire_im_send(PurpleConnection *gc, const char *who, const char *what, PurpleMessageFlags flags);
static void gfire_login(PurpleAccount *account);
static void gfire_login_cb(gpointer data, gint source, const gchar *error_message);
static void gfire_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group);
static void gfire_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group);
void gfire_buddy_menu_profile_cb(PurpleBlistNode *node, gpointer *data);
GList *gfire_node_menu(PurpleBlistNode *node);

void gfire_join_game(PurpleConnection *gc, const gchar *server_ip, int server_port, int game_id);
char *gfire_escape_html(const char *html);

#ifdef IS_NOT_WINDOWS
static void gfire_action_manage_games_cb(PurplePluginAction *action);
static void gfire_add_game_cb(manage_games_callback_args *args, GtkWidget *button);
static void gfire_edit_game_cb(manage_games_callback_args *args, GtkWidget *button);
static void gfire_manage_games_edit_update_fields_cb(GtkBuilder *builder, GtkWidget *edit_games_combo);
static void gfire_manage_games_update_executable_toggled_cb(GtkBuilder *builder, GtkWidget *executable_check_button);
static void gfire_remove_game_cb(manage_games_callback_args *args, GtkWidget *button);
static void gfire_reload_lconfig(PurpleConnection *gc);
xmlnode *gfire_manage_game_xml(char *game_id, char *game_name, gboolean game_executable, char *game_argument,
	char *game_prefix, char *game_path, char *game_launch, char *game_connect);

gboolean separe_path(char *path, char **file);
gboolean check_process(char *process, char *process_argument);
#endif

static PurplePlugin *_gfire_plugin = NULL;


static const char *gfire_blist_icon(PurpleAccount *a, PurpleBuddy *b) {
	return "gfire";
}


static const char *gfire_blist_emblems(PurpleBuddy *b)
{
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *tmp = NULL;
	PurplePresence *p = NULL;
	PurpleConnection *gc = NULL;

	if (!b || (NULL == b->account) || !(gc = purple_account_get_connection(b->account)) ||
					     !(gfire = (gfire_data *) gc->proto_data) || (NULL == gfire->buddies))
		return NULL;

	tmp = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)b->name, GFFB_NAME);
	if ((NULL == tmp) || (NULL == tmp->data)) return NULL;
	gf_buddy = (gfire_buddy *)tmp->data;

	p = purple_buddy_get_presence(b);

	if (purple_presence_is_online(p) == TRUE){
		if (0 != gf_buddy->gameid && 0 == gf_buddy->voipid) {
			return "game";
		} else if (0 == gf_buddy->gameid && 0 != gf_buddy->voipid) {
			return "voip";
		} else if (0 != gf_buddy->gameid && 0 != gf_buddy->voipid) {
			return "game-voip";
		}
	}

	return NULL;
}


static gchar *gfire_status_text(PurpleBuddy *buddy)
{
	gchar *msg = NULL;
	GList *gfbl = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurplePresence *p = NULL;
	PurpleConnection *gc = NULL;
	char *game_name = NULL;

	if (!buddy || (NULL == buddy->account) || !(gc = purple_account_get_connection(buddy->account)) ||
		!(gfire = (gfire_data *) gc->proto_data) || (NULL == gfire->buddies))
		return NULL;

	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)buddy->name, GFFB_NAME);
	if (NULL == gfbl) return NULL;

	gf_buddy = (gfire_buddy *)gfbl->data;
	p = purple_buddy_get_presence(buddy);

	if (TRUE == purple_presence_is_online(p)) {
		if (0 != gf_buddy->gameid) {
			game_name = gfire_game_name(gc, gf_buddy->gameid);
			if(game_name)
			{
				msg = g_strdup_printf(N_("Playing %s"), game_name);
				g_free(game_name);
			}
			else
				msg = g_strdup_printf(N_("Playing %u"), gf_buddy->gameid);
			return msg;
		}
		if (gf_buddy->away) {
			msg = g_strdup_printf("%s", gf_buddy->away_msg);
			gchar *escaped = gfire_escape_html(msg);
			g_free(msg);
			return escaped;
		}
	}
	return NULL;
}


static void gfire_blist_tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full)
{
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;
	GList *gfbl = NULL;
	gfire_buddy *gf_buddy = NULL;
	PurplePresence *p = NULL;
	gchar *game_name = NULL;
	guint32 *magic = NULL;
	gchar ipstr[16] = "";

	if (!buddy || (NULL == buddy->account) || !(gc = purple_account_get_connection(buddy->account)) ||
		!(gfire = (gfire_data *) gc->proto_data) || (NULL == gfire->buddies))
		return;

	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)buddy->name, GFFB_NAME);
	if (NULL == gfbl) return;

	gf_buddy = (gfire_buddy *)gfbl->data;
	p = purple_buddy_get_presence(buddy);

	if (TRUE == purple_presence_is_online(p)) {

		if (0 != gf_buddy->gameid) {

			game_name = gfire_game_name(gc, gf_buddy->gameid);
			purple_notify_user_info_add_pair(user_info,N_("Game"),NN(game_name));
			if(game_name) g_free(game_name);

			magic = (guint32 *)gf_buddy->gameip;
			if ((NULL != gf_buddy->gameip) && (0 != *magic)) {

				g_sprintf(ipstr, "%d.%d.%d.%d", gf_buddy->gameip[3], gf_buddy->gameip[2],
						gf_buddy->gameip[1], gf_buddy->gameip[0]);

					gchar * value = g_strdup_printf("%s:%d", ipstr, gf_buddy->gameport);
					purple_notify_user_info_add_pair(user_info,N_("Server"),value);
					g_free(value);

			}
		}
		// VoIP Info
		magic = (guint32 *)gf_buddy->voipip;
		if ((NULL != gf_buddy->voipip) && (0 != *magic)) {
			g_sprintf(ipstr, "%d.%d.%d.%d", gf_buddy->voipip[3], gf_buddy->voipip[2],
				gf_buddy->voipip[1], gf_buddy->voipip[0]);
			gchar *server = g_strdup_printf("%s:%d", ipstr, gf_buddy->voipport);
			game_name = gfire_game_name(gc, gf_buddy->voipid);
			purple_notify_user_info_add_pair(user_info,game_name ? game_name : "VoIP",server);
			if(game_name) g_free(game_name);
			g_free(server);
		}
		if (gf_buddy->away) {
			char * escaped_away = gfire_escape_html(gf_buddy->away_msg);

			purple_notify_user_info_add_pair(user_info,N_("Away"),escaped_away);

			g_free(escaped_away);
		}
	}
}



static GList *gfire_status_types(PurpleAccount *account)
{
	PurpleStatusType *type;
	GList *types = NULL;

	type = purple_status_type_new(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	type = purple_status_type_new_with_attrs(
		PURPLE_STATUS_AWAY, NULL, NULL, FALSE, TRUE, FALSE,
		"message", "Message", purple_value_new(PURPLE_TYPE_STRING),
		NULL);
	types = g_list_append(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	return types;
}


static void gfire_login(PurpleAccount *account)
{
	gfire_data *gfire;
	int	err = 0;

	PurpleConnection *gc = purple_account_get_connection(account);
	/* set connection flags for chats and im's tells purple what we can and can't handle */
	gc->flags |=  PURPLE_CONNECTION_NO_BGCOLOR | PURPLE_CONNECTION_NO_FONTSIZE
				| PURPLE_CONNECTION_NO_URLDESC | PURPLE_CONNECTION_NO_IMAGES;

	purple_connection_update_progress(gc, N_("Connecting"), 0, XFIRE_CONNECT_STEPS);

	gc->proto_data = gfire = g_new0(gfire_data, 1);
	gfire->fd = -1;
	gfire->buff_out = gfire->buff_in = NULL;
	/* load game xml from user dir */
	gfire_parse_games_file(gc, g_build_filename(purple_user_dir(), "gfire_games.xml", NULL));
	gfire_parse_launchinfo_file(gc, g_build_filename(purple_user_dir(), "gfire_launch.xml", NULL));

	if (!purple_account_get_connection(account)) {
			purple_connection_error(gc, N_("Couldn't create socket."));
			return;
	}

	if (purple_proxy_connect(NULL, account, purple_account_get_string(account, "server", XFIRE_SERVER),
				purple_account_get_int(account, "port", XFIRE_PORT),
				gfire_login_cb, gc) == NULL){
			purple_connection_error(gc, N_("Couldn't create socket."));
			return;
	}
}


static void gfire_login_cb(gpointer data, gint source, const gchar *error_message)
{
	PurpleConnection *gc = data;
	guint8 packet[1024];
	int length;
	PurpleAccount *account = purple_connection_get_account(gc);
	gfire_data *gfire = (gfire_data *)gc->proto_data;
	gfire->buddies = NULL;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "connected source=%d\n",source);
	if (!g_list_find(purple_connections_get_all(), gc)) {
		close(source);
		return;
	}

	if (source < 0) {
		purple_connection_error(gc, N_("Unable to connect to host."));
		return;
	}

	gfire->fd = source;

	/* Update the login progress status display */

	purple_connection_update_progress(gc, "Login", 1, XFIRE_CONNECT_STEPS);

	gfire_send(gc, (const guint8 *)"UA01", 4); /* open connection */

	length = gfire_initialize_connection(packet,purple_account_get_int(account, "version", XFIRE_PROTO_VERSION));
	gfire_send(gc, packet, length);

	gc->inpa = purple_input_add(gfire->fd, PURPLE_INPUT_READ, gfire_input_cb, gc);

	/* Create server_ip_mutex */
	gfire->server_mutex = g_mutex_new();
}


void gfire_close(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	GList *buddies = NULL;
	GList *chats = NULL;
	GList *clans = NULL;
	gfire_buddy *b = NULL;
	gfire_chat *gf_chat = NULL;
	gfire_clan *gf_clan = NULL;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONNECTION: close requested.\n");
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: closing, but either no GC or ProtoData.\n");
		return;
	}

	if (gc->inpa) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: removing input handler\n");
		purple_input_remove(gc->inpa);
	}

	if (gfire->det_source > 0) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: removing game detection callback\n");
		g_source_remove(gfire->det_source);
	}

	if (gfire->fd >= 0) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: closing source file descriptor\n");
		close(gfire->fd);
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: freeing buddy list\n");
	buddies = gfire->buddies;
	while (NULL != buddies) {
		b = (gfire_buddy *)buddies->data;
		if (NULL != b->away_msg) g_free(b->away_msg);
		if (NULL != b->name) g_free(b->name);
		if (NULL != b->alias) g_free(b->alias);
		if (NULL != b->userid) g_free(b->userid);
		if (NULL != b->uid_str) g_free(b->uid_str);
		if (NULL != b->sid) g_free(b->sid);
		if (NULL != b->sid_str) g_free(b->sid_str);
		if (NULL != b->gameip) g_free(b->gameip);
		if (NULL != b->voipip) g_free(b->voipip);
		
		g_free(b);
		buddies->data = NULL;
 		buddies = g_list_next(buddies);
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: freeing chat struct\n");
	chats = gfire->chats;
	while (NULL != chats) {
		gf_chat = (gfire_chat *)chats->data;
		if (NULL != gf_chat->members) g_list_free(gf_chat->members);
		if (NULL != gf_chat->chat_id) g_free(gf_chat->chat_id);
		if (NULL != gf_chat->topic) g_free(gf_chat->topic);
		if (NULL != gf_chat->motd) g_free(gf_chat->motd);
		if (NULL != gf_chat->c) g_free(gf_chat->c);
		g_free(gf_chat);
		chats->data = NULL;
 		chats = g_list_next(chats);
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: freeing clan struct\n");
	clans = gfire->clans;
	while (NULL != clans)
	{
		gf_clan = (gfire_clan *)clans->data;
		if(gf_clan->clanLongName) g_free(gf_clan->clanLongName);
		if(gf_clan->clanShortName) g_free(gf_clan->clanShortName);
		g_free(gf_clan);
		clans->data = NULL;
		clans = g_list_next(clans);
	}


	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: freeing gfire struct\n");
	if (NULL != gfire->im) {
		if (NULL != gfire->im->sid_str) g_free(gfire->im->sid_str);
		if (NULL != gfire->im->im_str) g_free(gfire->im->im_str);
		g_free(gfire->im); gfire->im = NULL;
	}

	if (NULL != gfire->email) g_free(gfire->email);
	if (NULL != gfire->buff_out) g_free(gfire->buff_out);
	if (NULL != gfire->buff_in) g_free(gfire->buff_in);
	if (NULL != gfire->buddies) g_list_free(gfire->buddies);
	if (NULL != gfire->chats) g_list_free(gfire->chats);
	if (NULL != gfire->clans) g_list_free(gfire->clans);
	if (NULL != gfire->xml_games_list) xmlnode_free(gfire->xml_games_list);
	if (NULL != gfire->xml_launch_info) xmlnode_free(gfire->xml_launch_info);
	if (NULL != gfire->userid) g_free(gfire->userid);
	if (NULL != gfire->sid) g_free(gfire->sid);
	if (NULL != gfire->alias) g_free(gfire->alias);
	if (NULL != gfire->voipip) g_free(gfire->voipip);

	if (gfire->server_ip != NULL) g_free(gfire->server_ip);
	if (gfire->server_mutex != NULL) g_free(gfire->server_mutex);
	
	g_free(gfire);
	gc->proto_data = NULL;
}


static int gfire_im_send(PurpleConnection *gc, const char *who, const char *what, PurpleMessageFlags flags)
{
	PurplePresence *p = NULL;
	gfire_data *gfire = NULL;
	gfire_buddy *gf_buddy = NULL;
	GList *gfbl = NULL;
	PurpleAccount *account = NULL;
	PurpleBuddy *buddy = NULL;
	int packet_len = 0;

	if (!gc || !(gfire = (gfire_data*)gc->proto_data))
		return 1;

	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)who, GFFB_NAME);
	if (NULL == gfbl) return 1;

	gf_buddy = (gfire_buddy *)gfbl->data;
	account = purple_connection_get_account(gc);
	buddy = purple_find_buddy(account, gf_buddy->name);
	if(buddy == NULL){
		purple_conv_present_error(who, account, N_("Message could not be sent. Buddy not in contact list"));
		return 1;
	}

	p = purple_buddy_get_presence(buddy);

	if (TRUE == purple_presence_is_online(p)) {
		/* in 2.0 the gtkimhtml stuff started escaping special chars & now = &amp;
		** XFire native clients don't handle it. */
		what = purple_unescape_html(what);
		packet_len = gfire_create_im(gc, gf_buddy, what);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(im send): %s: %s\n", NN(buddy->name), NN(what));
		gfire_send(gc, gfire->buff_out, packet_len);
		g_free((void *)what);
		return 1;
	} else {
		purple_conv_present_error(who, account, N_("Message could not be sent. Buddy offline"));
		return 1;
	}

}

static unsigned int gfire_send_typing(PurpleConnection *gc, const char *who, PurpleTypingState state)
{
	gfire_buddy *gf_buddy = NULL;
	gfire_data *gfire = NULL;
	GList *gfbl = NULL;
	int packet_len = 0;
	gboolean typenorm = TRUE;

	if (!gc || !(gfire = (gfire_data*)gc->proto_data))
		return 1;

	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)who, GFFB_NAME);
	if (NULL == gfbl) return 1;

	gf_buddy = (gfire_buddy *)gfbl->data;

	typenorm = purple_account_get_bool(purple_connection_get_account(gc), "typenorm", TRUE);

	if (!typenorm)
		return 0;

	if (state == PURPLE_NOT_TYPING)
		return 0;

	if (state == PURPLE_TYPING){
		packet_len = gfire_send_typing_packet(gc, gf_buddy);
		gfire_send(gc, gfire->buff_out, packet_len);
	}

	return XFIRE_SEND_TYPING_TIMEOUT;
}

static void gfire_get_info_parse_gamerig_cb(PurpleUtilFetchUrlData *url_data, gpointer data, const gchar *buf, gsize len, const gchar *error_message)
{
	get_info_callback_args *args = (get_info_callback_args*)data;

	if (!args || !buf || !len) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "User Info Gamerig XML Download: Download failed.\n");
		if(args)
		{
			purple_notify_user_info_add_section_break(args->user_info);
			purple_notify_user_info_add_pair(args->user_info, N_("Error"), N_("Retrieving gamerig data failed!"));
		}
	}
	else
	{
		purple_notify_user_info_add_section_break(args->user_info);
		xmlnode *gamerig = xmlnode_from_str(buf, len);
		if(!gamerig)
		{
			purple_notify_user_info_add_pair(args->user_info, N_("Error"), N_("Invalid gamerig data received!"));
		}
		else if(xmlnode_get_child(gamerig, "error"))
		{
			gchar *tmp = xmlnode_get_data(xmlnode_get_child(gamerig, "error"));
			purple_notify_user_info_add_pair(args->user_info, "Game Rig", NN(tmp));
			if(tmp) g_free(tmp);
			xmlnode_free(gamerig);
		}
		else
		{
			gchar *tmp = NULL;
			// Manufacturer
			xmlnode *data = xmlnode_get_child(gamerig, "manufacturer");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Manufacturer"), tmp);
				g_free(tmp);
			}

			// CPU
			data = xmlnode_get_child(gamerig, "processor");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Processor"), tmp);
				g_free(tmp);
			}

			// Memory
			data = xmlnode_get_child(gamerig, "memory");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Memory"), tmp);
				g_free(tmp);
			}

			// Video Card
			data = xmlnode_get_child(gamerig, "videocard");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Video Card"), tmp);
				g_free(tmp);
			}

			// Sound Card
			data = xmlnode_get_child(gamerig, "soundcard");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Sound Card"), tmp);
				g_free(tmp);
			}

			// Mainboard
			data = xmlnode_get_child(gamerig, "mainboard");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Mainboard"), tmp);
				g_free(tmp);
			}

			// Hard Drive
			data = xmlnode_get_child(gamerig, "harddrive");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Hard Drive"), tmp);
				g_free(tmp);
			}

			// Monitor
			data = xmlnode_get_child(gamerig, "monitor");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Monitor"), tmp);
				g_free(tmp);
			}

			// Keyboard
			data = xmlnode_get_child(gamerig, "keyboard");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Keyboard"), tmp);
				g_free(tmp);
			}

			// Mouse
			data = xmlnode_get_child(gamerig, "mouse");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Mouse"), tmp);
				g_free(tmp);
			}

			// Mouse Surface
			data = xmlnode_get_child(gamerig, "mousesurface");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Mouse Surface"), tmp);
				g_free(tmp);
			}

			// Speakers
			data = xmlnode_get_child(gamerig, "speakers");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Speakers"), tmp);
				g_free(tmp);
			}

			// Computer Case
			data = xmlnode_get_child(gamerig, "computer_case");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Computer Case"), tmp);
				g_free(tmp);
			}

			// Operating System
			data = xmlnode_get_child(gamerig, "operatingsystem");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Operating System"), tmp);
				g_free(tmp);
			}

			xmlnode_free(gamerig);
		}
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "User Info Gamerig XML Download: Download successful.\n");

	if(args)
	{
		purple_notify_userinfo(args->gc, args->gf_buddy->name, args->user_info, NULL, NULL);

		purple_notify_user_info_destroy(args->user_info);
		g_free(args);
	}
}

static void gfire_get_info_parse_profile_cb(PurpleUtilFetchUrlData *url_data, gpointer data, const gchar *buf, gsize len, const gchar *error_message)
{
	get_info_callback_args *args = (get_info_callback_args*)data;

	if (!args || !buf || !len) {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "User Info Profile XML Download: Download failed.\n");
		if(args)
		{
			purple_notify_user_info_add_section_break(args->user_info);
			purple_notify_user_info_add_pair(args->user_info, N_("Error"), N_("Retrieving profile data failed!"));
		}
	}
	else
	{
		purple_notify_user_info_add_section_break(args->user_info);
		xmlnode *profile = xmlnode_from_str(buf, len);
		if(!profile)
		{
			purple_notify_user_info_add_pair(args->user_info, N_("Error"), N_("Invalid profile data received!"));
		}
		else if(xmlnode_get_child(profile, "error"))
		{
			gchar *tmp = xmlnode_get_data(xmlnode_get_child(profile, "error"));
			purple_notify_user_info_add_pair(args->user_info, N_("Profile"), NN(tmp));
			if(tmp) g_free(tmp);
			xmlnode_free(profile);
		}
		else
		{
			gchar *tmp = NULL;
			// Real Name
			xmlnode *data = xmlnode_get_child(profile, "realname");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Real Name"), tmp);
				g_free(tmp);
			}

			// Age
			data = xmlnode_get_child(profile, "age");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Age"), tmp);
				g_free(tmp);
			}

			// Gender
			data = xmlnode_get_child(profile, "gender");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Gender"), (tmp[0] == 'm') ? N_("Male") : N_("Female"));
				g_free(tmp);
			}

			// Occupation
			data = xmlnode_get_child(profile, "occupation");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Occupation"), tmp);
				g_free(tmp);
			}

			// Country
			data = xmlnode_get_child(profile, "country");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Country"), tmp);
				g_free(tmp);
			}

			// Location
			data = xmlnode_get_child(profile, "location");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Location"), tmp);
				g_free(tmp);
			}

			// Gaming style
			data = xmlnode_get_child(profile, "gaming_style");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Gaming Style"), tmp);
				g_free(tmp);
			}

			// Interests
			data = xmlnode_get_child(profile, "interests");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Interests"), tmp);
				g_free(tmp);
			}

			// Friends
			data = xmlnode_get_child(profile, "friends_count");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Friends"), tmp);
				g_free(tmp);
			}

			// Join Date
			data = xmlnode_get_child(profile, "joindate");
			tmp = xmlnode_get_data(data);
			if(tmp)
			{
				purple_notify_user_info_add_pair(args->user_info, N_("Join Date"), tmp);
				g_free(tmp);
			}

			xmlnode_free(profile);
		}
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "User Info Profile XML Download: Download successful.\n");

	if(args)
	{
		// Fetch Gamerig XML Data
		gchar *infoURL = g_strdup_printf(XFIRE_XML_INFO_URL, args->gf_buddy->name, "gamerig");
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "User Info Gamerig XML Download: Starting download from %s.\n", infoURL);
		purple_util_fetch_url(infoURL, TRUE, "Purple-xfire", TRUE, gfire_get_info_parse_gamerig_cb, (void *)args);
		g_free(infoURL);
	}
}

static void gfire_get_info(PurpleConnection *gc, const char *who)
{
	PurpleAccount *account;
	PurpleBuddy *buddy;
	PurpleNotifyUserInfo *user_info;
	gfire_buddy *gf_buddy;
	gfire_data *gfire;
	GList *gfbl = NULL;
	PurplePresence *p = NULL;
	char *status = NULL;
	guint32 *magic = NULL;
	gchar ipstr[16] = "";
	char *server = NULL;
	gchar *infoURL = NULL;
	get_info_callback_args *download_args = NULL;

	account = purple_connection_get_account(gc);
	buddy = purple_find_buddy(account, who);
	user_info = purple_notify_user_info_new();
	p = purple_buddy_get_presence(buddy);

	if (!gc || !(gfire = (gfire_data*)gc->proto_data))
		return;

	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)who, GFFB_NAME);
	if (NULL == gfbl) return;

	gf_buddy = (gfire_buddy *)gfbl->data;

	if (gfire_status_text(buddy) == NULL){
		if (purple_presence_is_online(p) == TRUE){
			status = "Available";
		} else {
			status = "Offline";
		}
	} else {
		status = gfire_status_text(buddy);
	}

	purple_notify_user_info_add_pair(user_info, N_("Nickname"), gfire_escape_html(gf_buddy->alias));
	purple_notify_user_info_add_pair(user_info, N_("Status"), gfire_escape_html(status));
	if ((0 != gf_buddy->gameid) && (gf_buddy->away))
		purple_notify_user_info_add_pair(user_info, N_("Away"), gfire_escape_html(gf_buddy->away_msg));
	magic = (guint32 *)gf_buddy->gameip;
	if ((NULL != gf_buddy->gameip) && (0 != *magic)) {
		g_sprintf(ipstr, "%d.%d.%d.%d", gf_buddy->gameip[3], gf_buddy->gameip[2],
			gf_buddy->gameip[1], gf_buddy->gameip[0]);
		server = g_strdup_printf("%s:%d", ipstr, gf_buddy->gameport);
		purple_notify_user_info_add_pair(user_info,N_("Server"),server);
		g_free(server);
	}

	// VoIP Info
	magic = (guint32 *)gf_buddy->voipip;
	if ((NULL != gf_buddy->voipip) && (0 != *magic)) {
		g_sprintf(ipstr, "%d.%d.%d.%d", gf_buddy->voipip[3], gf_buddy->voipip[2],
			gf_buddy->voipip[1], gf_buddy->voipip[0]);
		server = g_strdup_printf("%s:%d", ipstr, gf_buddy->voipport);
		gchar *voip_name = gfire_game_name(gc, gf_buddy->voipid);
		purple_notify_user_info_add_pair(user_info,voip_name ? voip_name : "VoIP",server);
		if(voip_name) g_free(voip_name);
		g_free(server);
	}

	download_args = g_new0(get_info_callback_args, 1);
	download_args->gc = gc;
	download_args->user_info = user_info;
	download_args->gf_buddy = gf_buddy;

	// Fetch Profile XML data
	infoURL = g_strdup_printf(XFIRE_XML_INFO_URL, gf_buddy->name, "profile");
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "User Info Profile XML Download: Starting download from %s.\n", infoURL);
	purple_util_fetch_url(infoURL, TRUE, "Purple-xfire", TRUE, gfire_get_info_parse_profile_cb, (void *)download_args);
	g_free(infoURL);
}

static void gfire_set_status(PurpleAccount *account, PurpleStatus *status)
{
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;
	char *msg = NULL;
        int freemsg=0;
	if (!purple_status_is_active(status))
		return;

	gc = purple_account_get_connection(account);
	gfire = (gfire_data *)gc->proto_data;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(status): got status change to name: %s id: %s\n",
		NN(purple_status_get_name(status)),
		NN(purple_status_get_id(status)));

	if (purple_status_is_available(status)) {
		if (gfire->away) msg = "";
		gfire->away = FALSE;
	} else {
		gfire->away = TRUE;
		msg =(char *)purple_status_get_attr_string(status, "message");
		if ((msg == NULL) || (*msg == '\0')) {
			msg = N_("(AFK) Away From Keyboard");
                } else {
			msg = purple_unescape_html(msg);
                        freemsg = 1;
                }
	}
	gfire_send_away(gc, msg);
        if (freemsg) g_free(msg);
}


static void gfire_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group)
{
	gfire_data *gfire = NULL;
	int packet_len = 0;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !buddy || !buddy->name) return;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Adding buddy: %s\n", NN(buddy->name));
	packet_len = gfire_add_buddy_create(gc, buddy->name);
	gfire_send(gc, gfire->buff_out, packet_len);

}

void gfire_check_for_left_clan_members(PurpleConnection *gc, guint32 clanid)
{
	gfire_data *gfire = NULL;
	GList *clans = NULL;
	GList *buddies = NULL;
	gfire_clan *clan = NULL;
	gfire_buddy *clan_member = NULL;
	PurpleGroup *clan_group = NULL;
	PurpleBlistNode *contact_buddy_node = NULL;
	PurpleBlistNode *buddy_node = NULL;
	PurpleBuddy *pbuddy = NULL;
	gfire_buddy *gf_buddy = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !clanid)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_check_for_left_clan_members: invalid purple connection or clanid!\n");
		return;
	}

	clans = gfire->clans;
	while(clans)
	{
		if(clans->data && ((gfire_clan*)clans->data)->clanid == clanid)
			break;
		clans = g_list_next(clans);
	}

	if(!clans)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_check_for_left_clan_members: clan not found!\n");
		return;
	}

	clan = clans->data;

	contact_buddy_node = purple_blist_node_get_first_child(&(clan->group->node));
	if(!contact_buddy_node)
		return;

	while(contact_buddy_node)
	{
		if(!PURPLE_BLIST_NODE_IS_BUDDY(contact_buddy_node) && !PURPLE_BLIST_NODE_IS_CONTACT(contact_buddy_node))
		{
			buddy_node = purple_blist_node_get_sibling_next(contact_buddy_node);
			continue;
		}
		else if(PURPLE_BLIST_NODE_IS_CONTACT(contact_buddy_node))
			buddy_node = purple_blist_node_get_first_child(contact_buddy_node);
		else
			buddy_node = contact_buddy_node;

		gboolean found = FALSE;
		pbuddy = (PurpleBuddy*)buddy_node;

		buddies = gfire->buddies;
		while(buddies)
		{
			if(!buddies->data)
			{
				buddies = g_list_next(buddies);
				continue;
			}

			gf_buddy = (gfire_buddy*)buddies->data;
			if(g_strcmp0(gf_buddy->name, purple_buddy_get_name(pbuddy)) == 0)
			{
				found = TRUE;
				break;
			}

			buddies = g_list_next(buddies);
		}

		if(!found)
		{
			purple_debug(PURPLE_DEBUG_INFO, "gfire", "%s seems to have left his clan, removing buddy\n", purple_buddy_get_name(pbuddy));
			purple_blist_remove_buddy(pbuddy);
		}

		contact_buddy_node = purple_blist_node_get_sibling_next(contact_buddy_node);
	}
}

void gfire_leave_clan(PurpleConnection *gc, guint32 clanid)
{
	gfire_data *gfire = NULL;
	GList *clans = NULL;
	GList *buddies = NULL;
	gfire_clan *clan = NULL;
	gfire_buddy *clan_member = NULL;
	PurpleAccount *account = NULL;
	PurpleBuddy *pbuddy = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !clanid) return;

	account = purple_connection_get_account(gc);

	buddies = gfire->buddies;

	while(buddies)
	{
		if(!buddies->data)
		{
			buddies = g_list_next(buddies);
			continue;
		}

		clan_member = buddies->data;
		if(!clan_member->clan || clan_member->clanid != clanid)
		{
			buddies = g_list_next(buddies);
			continue;
		}

		pbuddy = purple_find_buddy(account, clan_member->name);
		if(!pbuddy)
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_leave_clan: Tried to remove clan member %s, but buddy was not found!\n", clan_member->name);
			buddies = g_list_next(buddies);
			continue;
		}

		purple_blist_remove_buddy(pbuddy);
		gfire->buddies = g_list_delete_link(gfire->buddies, buddies);

		if (clan_member->away_msg) g_free(clan_member->away_msg);
		if (clan_member->name) g_free(clan_member->name);
		if (clan_member->alias) g_free(clan_member->alias);
		if (clan_member->userid) g_free(clan_member->userid);
		if (clan_member->uid_str) g_free(clan_member->uid_str);
		if (clan_member->sid) g_free(clan_member->sid);
		if (clan_member->sid_str) g_free(clan_member->sid_str);
		if (clan_member->gameip) g_free(clan_member->gameip);
		if (clan_member->voipip) g_free(clan_member->voipip);
		g_free(clan_member);

		buddies = g_list_next(buddies);
	}

	clans = gfire->clans;

	while(clans)
	{
		if(clans->data && ((gfire_clan*)clans->data)->clanid == clanid)
			break;
		clans = g_list_next(clans);
	}

	if(!clans)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_leave_clan: clan not found!\n");
		return;
	}

	clan = clans->data;
	purple_debug(PURPLE_DEBUG_INFO, "gfire", "gfire_leave_clan: removing clan %08x\n", clan->clanid);

	if(clan->clanLongName) g_free(clan->clanLongName);
	if(clan->clanShortName) g_free(clan->clanShortName);

	purple_blist_remove_group(clan->group);
	gfire->clans = g_list_delete_link(gfire->clans, clans);

	g_free(clan);
}

void gfire_add_clan(PurpleConnection *gc, gfire_clan *newClan)
{
	gfire_data *gfire = NULL;
	gchar *clan_group_str = NULL;
	PurpleBlistNode *group_node = NULL;
	PurpleGroup *clan_group = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !newClan) return;

	// Check if the clan group exists /////////////////
	// Get first group node
	group_node = purple_blist_get_root();
	while(group_node && !PURPLE_BLIST_NODE_IS_GROUP(group_node))
		group_node = purple_blist_node_get_first_child(group_node);

	// Check for a fitting node
	while(group_node && purple_blist_node_get_int(group_node, "clanid") != newClan->clanid)
		group_node = purple_blist_node_get_sibling_next(group_node);

	// Group found
	if(group_node)
	{
		newClan->group = (PurpleGroup*)group_node;
		// Maybe a TODO: Renaming an existing group if the clan name changed
	}
	// Not found, create new blist clan group
	else if(!group_node)
	{
		clan_group_str = g_strdup_printf(GFIRE_CLAN_GROUP_FORMATTING, NN(newClan->clanLongName), NN(newClan->clanShortName));
		gchar *escaped = gfire_escape_html(clan_group_str);
		if(clan_group_str) g_free(clan_group_str);

		clan_group = purple_group_new(escaped);
		if(escaped) g_free(escaped);

		purple_blist_add_group(clan_group, NULL);
		purple_blist_node_set_int(&clan_group->node, "clanid", newClan->clanid);

		newClan->group = clan_group;
	}

	// Append clan to gfire clan list
	gfire->clans = g_list_append(gfire->clans, newClan);
}


static void gfire_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group)
{
	gfire_data *gfire = NULL;
	int packet_len = 0;
	gfire_buddy *gf_buddy = NULL;
	GList *b = NULL;
	PurpleAccount *account = NULL;
	gboolean buddynorm = TRUE;
	char tmp[255] = "";

	if (!gc || !(gfire = (gfire_data *)gc->proto_data) || !buddy || !buddy->name) return;

	if (!(account = purple_connection_get_account(gc))) return;

	b = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)buddy->name, GFFB_NAME);
	if (b == NULL) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_remove_buddy: buddy find returned NULL\n");
		return;
	}
	gf_buddy = (gfire_buddy *)b->data;
	if (!gf_buddy) return;

	buddynorm = purple_account_get_bool(account, "buddynorm", TRUE);
	if (buddynorm) {
		g_sprintf(tmp, N_("Not Removing %s"), NN(buddy->name));
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_remove_buddy: buddynorm TRUE not removing buddy %s.\n",
					NN(buddy->name));
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, N_("Xfire Buddy Removal Denied"), tmp, N_("Account settings are set to not remove buddies\n" "The buddy will be restored on your next login"), NULL, NULL);
		return;
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Removing buddy: %s\n", NN(gf_buddy->name));
	packet_len = gfire_remove_buddy_create(gc, gf_buddy);
	gfire_send(gc, gfire->buff_out, packet_len);

}


GList *gfire_find_buddy_in_list( GList *blist, gpointer *data, int mode )
{
	gfire_buddy *b = NULL;
	guint8 *u = NULL;
	guint8 *f = NULL;
	gchar *n = NULL;

	if ((NULL == blist) || (NULL == data)) return NULL;

	blist = g_list_first(blist);
	switch(mode)
	{
		case GFFB_NAME:
			n = (gchar *)data;
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if ( 0 == g_ascii_strcasecmp(n, b->name)) return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_ALIAS:
			n = (gchar *)data;
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if ( 0 == g_ascii_strcasecmp(n, b->alias)) return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_USERID:
			n = (gchar *)data;
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if ( 0 == g_ascii_strcasecmp(n, b->uid_str)) return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_UIDBIN:
			u = (guint8 *)data;
			while (NULL != blist) {
				b = (gfire_buddy *)blist->data;
				f = b->userid;
				if ( (u[0] == f[0]) && (u[1] == f[1]) && (u[2] == f[2]) && (u[3] == f[3]))
					return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_SIDS:
			n = (gchar *)data;
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if (!(NULL == b->sid_str) && (0 == g_ascii_strcasecmp(n, b->sid_str)))
					return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		case GFFB_SIDBIN:
			while (NULL != blist){
				b = (gfire_buddy *)blist->data;
				if ((NULL != b->sid) && (memcmp(b->sid, data, XFIRE_SID_LEN) == 0))
					return blist;
				blist = g_list_next(blist);
			}
			return NULL;
		break;
		default:
			/* mode not implemented */
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "ERROR: gfire_find_buddy_in_list, called with invalid mode\n");
			return NULL;
	}
}


void gfire_new_buddy(PurpleConnection *gc, gchar *alias, gchar *name, gboolean friend, guint32 clanid)
{
	PurpleBuddy *buddy = NULL;
	PurpleAccount *account = NULL;
	PurpleGroup *default_purple_group = NULL;
	PurpleGroup *clan_group = NULL;
	gfire_data *gfire = NULL;
	GList *clans = NULL;
	account = purple_connection_get_account(gc);
	default_purple_group = purple_find_group(GFIRE_DEFAULT_GROUP_NAME);
	clan_group = NULL;
	buddy = purple_find_buddy(account, name);

	if(!gc || !(gfire = (gfire_data*)gc->proto_data))
		return;

	clans = gfire->clans;


	if (buddy == NULL && friend == TRUE) {
		if (NULL == default_purple_group) {
			default_purple_group = purple_group_new(GFIRE_DEFAULT_GROUP_NAME);
			purple_blist_add_group(default_purple_group, NULL);
		}
		buddy = purple_buddy_new(account, name, NULL);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(buddylist): buddy %s not found in Pidgin buddy list, adding.\n",
				NN(name));
		purple_blist_add_buddy(buddy, NULL, default_purple_group, NULL);
		serv_got_alias(gc, name, g_strdup(alias));
	} else {
		serv_got_alias(gc, name, g_strdup(alias));
	}
	
	if (buddy == NULL && clanid > 0) {
		while(clans)
		{
			if(clans->data && ((gfire_clan*)clans->data)->clanid == clanid)
			{
				clan_group = ((gfire_clan*)clans->data)->group;
				break;
			}
			clans = g_list_next(clans);
		}

		if(!clan_group)
		{
			clan_group = purple_find_group(GFIRE_CLAN_GROUP_NAME);
			if(!clan_group)
			{
				clan_group = purple_group_new(GFIRE_CLAN_GROUP_NAME);
				purple_blist_add_group(clan_group, NULL);
			}
			purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(buddylist): buddy's clan %08x not found in Pidgin buddy list, adding to default!\n", clanid);
		}

		buddy = purple_buddy_new(account, name, NULL);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(buddylist): buddy %s not found in Pidgin buddy list, adding.\n",
				NN(name));
		purple_blist_add_buddy(buddy, NULL, clan_group, NULL);
		serv_got_alias(gc, name, g_strdup(alias));
	} else {
		serv_got_alias(gc, name, g_strdup(alias));
	}
}



void gfire_new_buddies(PurpleConnection *gc)
{
	gfire_data *gfire = (gfire_data *)gc->proto_data;
	gfire_buddy *b = NULL;
	PurpleBuddy *gbuddy = NULL;
	GList *tmp = gfire->buddies;
	int packet_len = 0;
	guint32 clanid = 0;

	while (NULL != tmp) {
		b = (gfire_buddy *)tmp->data;
		if (!b) return;
		// Check if a member has left during offline time
		if(b->clan && clanid != b->clanid)
		{
			clanid = b->clanid;
			gfire_check_for_left_clan_members(gc, clanid);
		}
		gfire_new_buddy(gc, b->alias, b->name, b->friend, b->clanid);
		tmp = g_list_next(tmp);
		gbuddy = purple_find_buddy(purple_connection_get_account(gc), b->name);
		if(gbuddy != NULL)
			b->avatarnumber = purple_blist_node_get_int(&(gbuddy->node), "avatarnumber");
		packet_len = gfire_request_avatar_info(gc, b);
		if (packet_len > 0)	gfire_send(gc, gfire->buff_out, packet_len);
	}
}


void gfire_handle_im(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	gfire_im	*im = NULL;
	GList		*gfbl = NULL;
	gfire_buddy	*gf_buddy = NULL;
	PurpleAccount *account = NULL;
	PurpleBuddy *buddy = NULL;

	if ( !gc || !(gfire = (gfire_data *)gc->proto_data) || !(im = gfire->im) )
		return;
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "im_handle: looking for sid %s\n", NN(im->sid_str));
	gfbl = gfire_find_buddy_in_list(gfire->buddies, (gpointer *) im->sid_str, GFFB_SIDS);
	if (NULL == gfbl) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "im_handle: buddy find returned NULL\n");
		g_free(im->im_str);
		g_free(im->sid_str);
		g_free(im);
		gfire->im = NULL;
		return;
	}
	gf_buddy = (gfire_buddy *)gfbl->data;
	account = purple_connection_get_account(gc);
	buddy = purple_find_buddy(account, gf_buddy->name);
	if (NULL == buddy) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "im_handle: PIDGIN buddy find returned NULL for %s\n",
					NN(gf_buddy->name));
		g_free(im->im_str);
		g_free(im->sid_str);
		g_free(im);
		gfire->im = NULL;
		return;
	}

	if (PURPLE_BUDDY_IS_ONLINE(buddy)) {
	if (!purple_privacy_check(account, buddy->name))
		return;

		switch (im->type)
		{
			case 0:	/* got an im */
				serv_got_im(gc, buddy->name, gfire_escape_html(im->im_str), 0, time(NULL));
			break;
			case 1: /* got an ack packet... we shouldn't be here */
			break;
			case 2:	/* got a p2p thing... */
			break;
			case 3: /* got typing */
				serv_got_typing(gc, buddy->name, 10, PURPLE_TYPING);
			break;
		}
	}

	if (NULL != im->im_str) g_free(im->im_str);
	if (NULL != im->sid_str) g_free(im->sid_str);
	g_free(im);
	gfire->im = NULL;
}


/* connection keep alive.  We send a packet to the xfire server
 * saying we are still around.  Otherwise they will forcibly close our connection
 * purple allows for this, but calls us every 30 seconds.  We keep track of *all* sent
 * packets.  We only need to send this keep alive if we haven't sent anything in a long(tm)
 * time.  So we watch and wait.
*/
void gfire_keep_alive(PurpleConnection *gc){
	static int count = 1;
	int packet_len;
	gfire_data *gfire = NULL;
	GTimeVal gtv;

	g_get_current_time(&gtv);
	if ((purple_connection_get_state(gc) != PURPLE_DISCONNECTED) &&
		(NULL != (gfire = (gfire_data *)gc->proto_data)) &&
		((gtv.tv_sec - gfire->last_packet) > XFIRE_KEEPALIVE_TIME))
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "send keep_alive packet (PING)\n");
		packet_len = gfire_ka_packet_create(gc);
		if (packet_len > 0)	gfire_send(gc, gfire->buff_out, packet_len);
		count = 1;
	}
	count++;
}


void gfire_update_buddy_status(PurpleConnection *gc, GList *buddies, int status)
{
	gfire_buddy *gf_buddy = NULL;
	GList *b = g_list_first(buddies);
	PurpleBuddy *gbuddy = NULL;
	int len = 0;

	if (!buddies || !gc || !gc->account) {
		if (buddies) g_list_free(buddies);
		return;
	}

	while ( NULL != b ) {
		gf_buddy = (gfire_buddy *)b->data;
		if ((NULL != (gf_buddy = (gfire_buddy *)b->data)) && (NULL != gf_buddy->name)) {
			gbuddy = purple_find_buddy(gc->account, gf_buddy->name);
			if (NULL == gbuddy) { b = g_list_next(b); continue; }
			switch (status)
			{
				case GFIRE_STATUS_ONLINE:
					if ( 0 == g_ascii_strcasecmp(XFIRE_SID_OFFLINE_STR,gf_buddy->sid_str)) {
						purple_prpl_got_user_status(gc->account, gf_buddy->name, "offline", NULL);
						// Reset the buddy's game and voip status
						gf_buddy->gameid = gf_buddy->voipid = 0;
						gf_buddy->gameport = gf_buddy->voipport = 0;
						if(gf_buddy->gameip) g_free(gf_buddy->gameip);
						if(gf_buddy->voipip) g_free(gf_buddy->voipip);
						gf_buddy->gameip = gf_buddy->voipip = NULL;
					} else {
						if ( gf_buddy->away ) {
							purple_prpl_got_user_status(gc->account, gf_buddy->name, "away", NULL);
						} else {
							purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);
						}
					}

				break;
				case GFIRE_STATUS_GAME:

					if ( gf_buddy->gameid > 0 ) {
						if ( gf_buddy->away ) {
							purple_prpl_got_user_status(gc->account, gf_buddy->name, "away", NULL);
						} else {
							purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);
						}
					} else {
						if ( gf_buddy->away ) {
							purple_prpl_got_user_status(gc->account, gf_buddy->name, "away", NULL);
						} else {
							purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);
						}
					}

				break;
				case GFIRE_STATUS_AWAY:
					if ( gf_buddy->away ) {

						purple_prpl_got_user_status(gc->account, gf_buddy->name, "away", NULL);
					} else {
						purple_prpl_got_user_status(gc->account, gf_buddy->name, "available", NULL);
					}

				break;
				default:
					purple_debug(PURPLE_DEBUG_MISC, "gfire", "update_buddy_status: Unknown mode specified\n");
			}
		}
		b = g_list_next(b);
	}
	g_list_free(buddies);
}



void gfire_buddy_add_authorize_cb(void *data)
{
	PurpleConnection *gc = NULL;
	gfire_buddy *b = (gfire_buddy *)data;
	gfire_data *gfire = NULL;
	int packet_len = 0;

	if (!b) {
		if (b->name) g_free(b->name);
		if (b->alias) g_free(b->alias);
		if (b->uid_str) g_free(b->uid_str);
		g_free(b);
 		return;
	}
	gc = (PurpleConnection *)b->sid;
	b->sid = NULL;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		if (b->name) g_free(b->name);
		if (b->alias) g_free(b->alias);
		if (b->uid_str) g_free(b->uid_str);
		g_free(b);
 		return;
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Authorizing buddy invitation: %s\n", NN(b->name));
	packet_len = gfire_invitation_accept(gc, b->name);
	gfire_send(gc, gfire->buff_out, packet_len);

	if (b->name) g_free(b->name);
	if (b->alias) g_free(b->alias);
	if (b->uid_str) g_free(b->uid_str);
	g_free(b);

}


void gfire_buddy_add_deny_cb(void *data)
{
	PurpleConnection *gc = NULL;
	gfire_buddy *b = (gfire_buddy *)data;
	gfire_data *gfire = NULL;
	int packet_len = 0;

	if (!b) {
		if (b->name) g_free(b->name);
		if (b->alias) g_free(b->alias);
		if (b->uid_str) g_free(b->uid_str);
		g_free(b);
 		return;
	}
	gc = (PurpleConnection *)b->sid;
	b->sid = NULL;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		if (b->name) g_free(b->name);
		if (b->alias) g_free(b->alias);
		if (b->uid_str) g_free(b->uid_str);
		g_free(b);
 		return;
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "Denying buddy invitation: %s\n", NN(b->name));
	packet_len = gfire_invitation_deny(gc, b->name);
	gfire_send(gc, gfire->buff_out, packet_len);

	if (b->name) g_free(b->name);
	if (b->alias) g_free(b->alias);
	if (b->uid_str) g_free(b->uid_str);
	g_free(b);

}


void gfire_buddy_menu_profile_cb(PurpleBlistNode *node, gpointer *data)
{
	PurpleBuddy *b =(PurpleBuddy *)node;
	if (!b || !(b->name)) return;

	char uri[256] = "";
	g_sprintf(uri, "%s%s", XFIRE_PROFILE_URL, b->name);
	purple_notify_uri((void *)_gfire_plugin, uri);
 }


 /**
  * Callback function for pidgin buddy list right click menu.  This callback
  * is what the interface calls when the Join Game ... menu option is selected
  *
  * @param node		BuddyList node provided by interface
  * @param data		Gpointer data (not used)
*/
void gfire_buddy_menu_joingame_cb(PurpleBlistNode *node, gpointer *data)
{
 	int game = 0;
	gchar *serverip = NULL;
 	int serverport = 0;
	PurpleBuddy *b = (PurpleBuddy *)node;
 	PurpleConnection *gc = NULL;

	if (!b || !b->account || !(gc = purple_account_get_connection(b->account))) return;

 	game = gfire_get_buddy_game(gc, b);
	if (game && gfire_game_playable(gc, game)) {
 		serverport = gfire_get_buddy_port(gc, b);
 		if (serverport) serverip = (gchar *)gfire_get_buddy_ip(gc, b);

 		gfire_join_game(gc, serverip, serverport, game);

		if(serverip)
			g_free(serverip);
 	}
}

/**
  * Callback function for pidgin buddy list right click menu.  This callback
  * is what the interface calls when the Join VoIP ... menu option is selected
  *
  * @param node		BuddyList node provided by interface
  * @param data		Gpointer data (not used)
*/
void gfire_buddy_menu_joinvoip_cb(PurpleBlistNode *node, gpointer *data)
{
	PurpleBuddy *b = (PurpleBuddy *)node;
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;
	GList *l = NULL;
	gfire_buddy *gb = NULL;
	gchar *server_ip = NULL;

	if (!b || !b->account || !(gc = purple_account_get_connection(b->account)) || !(gfire = (gfire_data*)gc->proto_data)) return;

	l = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)b->name, GFFB_NAME);
	if (!l || !(l->data)) return;
	gb = (gfire_buddy *)l->data;

	if(gb->voipip)
		server_ip = g_strdup_printf("%u.%u.%u.%u", gb->voipip[3], gb->voipip[2], gb->voipip[1], gb->voipip[0]);

	if (gb->voipid && gfire_game_playable(gc, gb->voipid)) {
		gfire_join_game(gc, server_ip, gb->voipport, gb->voipid);
	}

	if(server_ip)
		g_free(server_ip);
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
GList * gfire_node_menu(PurpleBlistNode *node)
{
	GList *ret = NULL;
	PurpleMenuAction *me = NULL;
	PurpleBuddy *b =(PurpleBuddy *)node;
	gfire_buddy *gb = NULL;
	GList *l = NULL;
	PurpleConnection *gc = NULL;
	gfire_data *gfire = NULL;


	if (PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		
		if (!b || !b->account || !(gc = purple_account_get_connection(b->account)) ||
				     !(gfire = (gfire_data *) gc->proto_data))
		return NULL;
		
		l = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)b->name, GFFB_NAME);
		if (!l) return NULL; /* can't find the buddy? not our plugin? */
		gb = (gfire_buddy *)l->data;

		if (gb && !gfire->gameid && gb->gameid && gfire_game_playable(gc, gb->gameid)){
			 /* don't show menu if we are in game */

			me = purple_menu_action_new(N_("Join Game ..."),
					PURPLE_CALLBACK(gfire_buddy_menu_joingame_cb),NULL, NULL);

			if (!me) {
				return NULL;
			}
			ret = g_list_append(ret, me);
		}

		if (gb && !gfire->voipid && gb->voipid && gfire_game_playable(gc, gb->voipid)){
			 /* don't show menu if we are in VoIP software */

			me = purple_menu_action_new(N_("Join VoIP ..."),
					PURPLE_CALLBACK(gfire_buddy_menu_joinvoip_cb),NULL, NULL);

			if (!me) {
				return NULL;
			}
			ret = g_list_append(ret, me);
		}

		me = purple_menu_action_new(N_("Xfire Profile"),
			PURPLE_CALLBACK(gfire_buddy_menu_profile_cb),NULL, NULL);

		if (!me) {
			return NULL;
		}
		ret = g_list_append(ret, me);
 	}
	return ret;

 }


static void gfire_change_nick(PurpleConnection *gc, const char *entry)
{
	gfire_data *gfire = NULL;
	int packet_len = 0;
	gfire_buddy *b = NULL;
	GList *l = NULL;
	PurpleBuddy *buddy = NULL;
	PurpleAccount *account = purple_connection_get_account(gc);

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	packet_len = gfire_create_change_alias(gc, (char *)entry);
	if (packet_len) {
		gfire_send(gc, gfire->buff_out, packet_len);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Changed server nick (alias) to \"%s\"\n", NN(entry));
	} else {
		purple_debug(PURPLE_DEBUG_ERROR, "gfire",
				"ERROR: During alias change, _create_change_alias returned 0 length\n");
		return;
	}
	purple_connection_set_display_name(gc, entry);

	l = gfire_find_buddy_in_list(gfire->buddies, (gpointer *)gfire->userid, GFFB_UIDBIN);
	if (l) {
		/* we are in our own buddy list... change our server alias :) */
		b = (gfire_buddy *)l->data;
		buddy = purple_find_buddy(account, b->name);
		if (buddy){
			purple_blist_server_alias_buddy(buddy, entry);
		}
	}
}


static void gfire_action_nick_change_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *)action->context;
	PurpleAccount *account = purple_connection_get_account(gc);

	purple_request_input(gc, NULL, N_("Change Xfire nickname"), N_("Leaving empty will clear your current nickname."), purple_connection_get_display_name(gc),
		FALSE, FALSE, NULL, N_("OK"), G_CALLBACK(gfire_change_nick), N_("Cancel"), NULL, account, NULL, NULL, gc);
}

static void gfire_manage_games_update_executable_cb(GtkWidget *executable_button, GtkWidget *path_button)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(executable_button), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(path_button)));
}

/**
 * shows the manage games window
 *
 * @param action: the menu action, passed by the signal connection function
 *
**/
static void gfire_action_manage_games_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *)action->context;
	gfire_data *gfire = NULL;
	GtkBuilder *builder = gtk_builder_new();
	gchar *builder_file;

	if (gc == NULL || (gfire = (gfire_data *)gc->proto_data) == NULL) {
		purple_debug_error("gfire: gfire_action_manage_games_cb", "GC not set and/or couldn't access gfire data.\n");
		return;
	}

	gfire_reload_lconfig(gc);

	builder_file = g_build_filename(DATADIR, "purple", "gfire", "games.glade", NULL);
	gtk_builder_add_from_file(builder, builder_file, NULL);
	g_free(builder_file);

	if (builder == NULL) {
		purple_debug_error("gfire: gfire_action_manage_games_cb", "Couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));
	GtkWidget *add_game_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_game_entry"));
	GtkWidget *add_path_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_path_button"));
	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_executable_check_button"));
	GtkWidget *add_executable_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_executable_button"));
	GtkWidget *add_close_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_close_button"));
	GtkWidget *add_add_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_add_button"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "edit_game_combo"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_executable_check_button"));
	GtkWidget *edit_close_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_close_button"));
	GtkWidget *edit_apply_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_apply_button"));
	GtkWidget *edit_remove_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_remove_button"));
	
	manage_games_callback_args *args;

	args = g_new0(manage_games_callback_args, 1);
	args->gc = gc;
	args->builder = builder;
	
	g_signal_connect_swapped(add_path_button, "current-folder-changed", G_CALLBACK(gfire_manage_games_update_executable_cb), add_executable_button);
	g_signal_connect_swapped(add_executable_check_button, "toggled", G_CALLBACK(gfire_manage_games_update_executable_toggled_cb), builder);
	g_signal_connect_swapped(add_close_button, "clicked", G_CALLBACK(gtk_widget_destroy), manage_games_window);
	g_signal_connect_swapped(add_add_button, "clicked", G_CALLBACK(gfire_add_game_cb), args);
	g_signal_connect_swapped(edit_game_combo, "changed", G_CALLBACK(gfire_manage_games_edit_update_fields_cb), builder);
	g_signal_connect_swapped(edit_executable_check_button, "toggled", G_CALLBACK(gfire_manage_games_update_executable_toggled_cb), builder);
	g_signal_connect_swapped(edit_close_button, "clicked", G_CALLBACK(gtk_widget_destroy), manage_games_window);
	g_signal_connect_swapped(edit_apply_button, "clicked", G_CALLBACK(gfire_edit_game_cb), args);
	g_signal_connect_swapped(edit_remove_button, "clicked", G_CALLBACK(gfire_remove_game_cb), args);

	xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
	
	if (gfire_launch != NULL)
	{

		const char *manager_version = xmlnode_get_attrib(gfire_launch, "version");

		if (g_strcmp0(manager_version, "2") != NULL) {
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"), N_("Incompatible games configuration"),
				N_("Your current games configuration is incompatible with this version of Gfire. Please remove it and try again."), NULL, NULL);
			return;
		}
		else
		{
			xmlnode *node_child;
			for (node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
			     node_child = xmlnode_get_next_twin(node_child)) {
				const char *game_name = xmlnode_get_attrib(node_child, "name");
				gtk_combo_box_append_text(GTK_COMBO_BOX(edit_game_combo), game_name);
			}
		}
	}

	xmlnode *gfire_games;
	GtkEntryCompletion *add_game_completion;
	GtkListStore *add_game_list_store;
	GtkTreeIter add_game_iter;
	
	gfire_games = gfire->xml_games_list;
	add_game_list_store = gtk_list_store_new(1, G_TYPE_STRING);
	
	if (gfire_games != NULL)
	{
		xmlnode *node_child;
		for (node_child = xmlnode_get_child(gfire_games, "game"); node_child != NULL;
		    node_child = xmlnode_get_next_twin(node_child))
		{
			const char *game_name = xmlnode_get_attrib(node_child, "name");

			gtk_list_store_append(add_game_list_store, &add_game_iter);
			gtk_list_store_set(add_game_list_store, &add_game_iter, 0,  game_name, -1);
		}

		add_game_completion = gtk_entry_completion_new();
		gtk_entry_completion_set_model(add_game_completion, GTK_TREE_MODEL(add_game_list_store));
		gtk_entry_completion_set_text_column(add_game_completion, 0);
		gtk_entry_set_completion(GTK_ENTRY(add_game_entry), add_game_completion);
	}
	else {
		purple_debug_error("gfire: gfire_action_manage_games_cb", "Couldn't get games list.\n");
		return;
	}

	gtk_widget_show_all(manage_games_window);
}


/**
 * adds a game by getting the values from the manage games window
 * 
**/
static void gfire_add_game_cb(manage_games_callback_args *args, GtkWidget *button)
{
	PurpleConnection *gc = args->gc;
	GtkBuilder *builder = args->builder;

	if (gc == NULL || builder == NULL) {
		purple_debug_error("gfire: gfire_add_game_cb", "GC not set and/or couldn't access interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));
	GtkWidget *add_game_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_game_entry"));
	GtkWidget *add_path_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_path_button"));
	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_executable_check_button"));
	GtkWidget *add_executable_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_executable_button"));
	GtkWidget *add_connect_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_connect_entry"));

	GtkWidget *add_prefix_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_prefix_entry"));
	GtkWidget *add_launch_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_launch_entry"));
	GtkWidget *add_argument_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_argument_entry"));
	
	const gchar *game_name = gtk_entry_get_text(GTK_ENTRY(add_game_entry));
	gchar *game_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(add_path_button));
	gboolean game_executable_use_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_executable_check_button));
	gchar *game_executable = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(add_executable_button));
	const gchar *game_connect = gtk_entry_get_text(GTK_ENTRY(add_connect_entry));

	const gchar *game_prefix = gtk_entry_get_text(GTK_ENTRY(add_prefix_entry));
	const gchar *game_launch = gtk_entry_get_text(GTK_ENTRY(add_launch_entry));
	const gchar *game_argument = gtk_entry_get_text(GTK_ENTRY(add_argument_entry));

	if (game_name != NULL && game_path != NULL && game_connect != NULL
	    && ((game_executable_use_path == FALSE && game_executable != NULL) || game_executable_use_path == TRUE))
	{
		int game_id_tmp;
		char *game_id;
		gboolean game_check;

		game_id_tmp = gfire_game_id(gc, game_name);
		game_id = g_strdup_printf("%d", game_id_tmp);
		game_check = gfire_game_playable(gc, game_id_tmp);

		if (game_check == FALSE)
 		{
			xmlnode *gfire_launch_new = xmlnode_new("launch_info");
			xmlnode_set_attrib(gfire_launch_new, "version", "2");
			xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
			xmlnode *game_node;
			
			if (gfire_launch != NULL)
			{
				xmlnode *node_child;
				for (node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
					node_child = xmlnode_get_next_twin(node_child)) {
					xmlnode_insert_child(gfire_launch_new, node_child);
				}
			}

			if (game_executable_use_path == TRUE) {
				game_node = gfire_manage_game_xml(game_id, game_name, game_path, game_argument, game_prefix, game_path,
												  game_launch, game_connect);
			}
			else {
				game_node = gfire_manage_game_xml(game_id, game_name, game_executable, game_argument, game_prefix, game_path,
												  game_launch, game_connect);
			}
			 
			g_free(game_path);

			if (game_node == NULL) purple_debug_error("gfire: gfire_add_game_cb", "Couldn't create the new game node.\n");
			else
			{
				xmlnode_insert_child(gfire_launch_new, game_node);
				char *gfire_launch_new_str = xmlnode_to_formatted_str(gfire_launch_new, NULL);

				gboolean write_xml = purple_util_write_data_to_file("gfire_launch.xml", gfire_launch_new_str, -1);
				if (write_xml == FALSE) {
					purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"), N_("Couldn't add game"),
										  N_("Please try again. An error occured while adding the game."), NULL, NULL);
				}
				else {
					gfire_reload_lconfig(gc);
					purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: game added"),
										  game_name, N_("The game has been successfully added."), NULL, NULL);
				}
				xmlnode_free(gfire_launch_new);
			}
		}
 		else {
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: warning"), N_("Game already added"),
								  N_("This game is already added, you can configure it if you want."), NULL, NULL);
		}
	}	
	else {
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"),
							  N_("Couldn't add game"), N_("Please try again. Make sure you fill in all fields."), NULL, NULL);
	}
	
	gtk_widget_destroy(manage_games_window);
}

/**
 * edits a game in gfire_launch.xml by getting the new values from
 * the manage games window
 *
**/
static void gfire_edit_game_cb(manage_games_callback_args *args, GtkWidget *button)
{
	PurpleConnection *gc = args->gc;
	GtkBuilder *builder = args->builder;
	
	if (!gc || !builder) {
		purple_debug_error("gfire: gfire_edit_game_cb", "GC not set and/or couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "edit_game_combo"));
	GtkWidget *edit_path_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_path_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_executable_check_button"));
	GtkWidget *edit_executable_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_executable_button"));
	GtkWidget *edit_connect_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_connect_entry"));

	GtkWidget *edit_prefix_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_prefix_entry"));
	GtkWidget *edit_launch_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_launch_entry"));
	GtkWidget *edit_argument_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_argument_entry"));
	
	const gchar *game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));
	gchar *game_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(edit_path_button));
	gboolean game_executable_use_path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit_executable_check_button));
	gchar *game_executable = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(edit_executable_button));
	const gchar *game_connect = gtk_entry_get_text(GTK_ENTRY(edit_connect_entry));

	const gchar *game_prefix = gtk_entry_get_text(GTK_ENTRY(edit_prefix_entry));
	const gchar *game_launch = gtk_entry_get_text(GTK_ENTRY(edit_launch_entry));
	const gchar *game_argument = gtk_entry_get_text(GTK_ENTRY(edit_argument_entry));

	if (game_name != NULL && game_path != NULL && game_connect != NULL
	    && ((game_executable_use_path == FALSE && game_executable != NULL) || game_executable_use_path == TRUE))
	{
		xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
		xmlnode *gfire_launch_new = xmlnode_new("launch_info");
		xmlnode_set_attrib(gfire_launch_new, "version", "2");
		
		if (gfire_launch != NULL)
		{
			xmlnode *node_child;
			for(node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
				node_child = xmlnode_get_next_twin(node_child))
			{
				const char *game_name_tmp = xmlnode_get_attrib(node_child, "name");
				const char *game_id = xmlnode_get_attrib(node_child, "id");
				const char *game_type = xmlnode_get_attrib(node_child, "type");
					
				if (g_strcmp0(game_name, game_name_tmp) == NULL)
				{
					gboolean game_native;
					xmlnode *game_node ;
					
					if (g_strcmp0(game_type, "Native game") == NULL) game_native = TRUE;
					else game_native = FALSE;

					if (game_executable_use_path == TRUE) {
						game_node = gfire_manage_game_xml(game_id, game_name, game_path, game_argument, game_prefix, game_path,
														  game_launch, game_connect);
					}
					else {
						game_node = gfire_manage_game_xml(game_id, game_name, game_executable, game_argument, game_prefix, game_path,
														  game_launch, game_connect);
					}
					
					xmlnode_insert_child(gfire_launch_new, game_node);
					g_free(game_path);
				}
				else xmlnode_insert_child(gfire_launch_new, node_child);
			}

			char *gfire_launch_new_str = xmlnode_to_formatted_str(gfire_launch_new, NULL);
			gboolean write_xml = purple_util_write_data_to_file("gfire_launch.xml", gfire_launch_new_str, -1);
			
			if (write_xml == TRUE) {
				gfire_reload_lconfig(gc);
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: game edited"), N_("Game edited"),
					N_("The game has been successfully edited."), NULL, NULL);
			}
			else {
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"), N_("Couldn't edit game'"),
					N_("Please try again. An error occured while editing the game."), NULL, NULL);
			}
			
			xmlnode_free(gfire_launch_new);
		}
	}
	else {
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"),
			N_("Couldn't edit game"), N_("Please try again. Make sure you fill in all required fields."), NULL, NULL);
	}

	gtk_widget_destroy(manage_games_window);
}

/**
 * gets and shows the current values from gfire_launch.xml
 * for editing the selected game in the manage games window
 *
**/
static void gfire_manage_games_edit_update_fields_cb(GtkBuilder *builder, GtkWidget *edit_game_combo)
{
	if (builder == NULL) {
		purple_debug_error("gfire: gfire_manage_games_edit_update_fields_cb", "Couldn't access interface.\n");
		return;
	}

	GtkWidget *edit_path_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_path_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_executable_check_button"));
	GtkWidget *edit_executable_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_executable_button"));
	GtkWidget *edit_connect_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_connect_entry"));

	GtkWidget *edit_prefix_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_prefix_entry"));
	GtkWidget *edit_launch_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_launch_entry"));
	GtkWidget *edit_argument_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_argument_entry"));
	
	gchar *selected_game = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));
	gchar *game_executable = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(edit_executable_button));

 	xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
	if (gfire_launch != NULL)
	{
		xmlnode *node_child;
		for (node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
		     node_child = xmlnode_get_next_twin(node_child))
		{
			const char *game = xmlnode_get_attrib(node_child, "name");
			if (g_strcmp0(selected_game, game) == NULL)
			{
				xmlnode *command_node = xmlnode_get_child(node_child, "command");
				xmlnode *executable_node = xmlnode_get_child(command_node, "executable");
				xmlnode *prefix_node = xmlnode_get_child(command_node, "prefix");
				xmlnode *path_node = xmlnode_get_child(command_node, "path");
				xmlnode *launch_node = xmlnode_get_child(command_node, "launch");
				xmlnode *connect_node = xmlnode_get_child(command_node, "connect");

				char *game_executable = xmlnode_get_data(executable_node);
				char *game_prefix = xmlnode_get_data(prefix_node);
				char *game_path = xmlnode_get_data(path_node);
				char *game_launch = xmlnode_get_data(launch_node);
				char *game_argument = xmlnode_get_attrib(executable_node, "argument");
				char *game_connect = xmlnode_get_data(connect_node);

				if (g_strcmp0(game_path, game_executable) == NULL) {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_executable_check_button), TRUE);
				}
				else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(edit_executable_check_button), FALSE);
				
				gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(edit_executable_button), game_executable);
				gtk_entry_set_text(GTK_ENTRY(edit_prefix_entry), game_prefix);
				gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(edit_path_button), game_path);
				gtk_entry_set_text(GTK_ENTRY(edit_launch_entry), game_launch);
				gtk_entry_set_text(GTK_ENTRY(edit_argument_entry), game_argument);
				gtk_entry_set_text(GTK_ENTRY(edit_connect_entry), game_connect);

				xmlnode_free(command_node);
			}
		}
	}
}

static void gfire_manage_games_update_executable_toggled_cb(GtkBuilder *builder, GtkWidget *executable_check_button)
{
	if (builder == NULL) {
		purple_debug_error("gfire", "Couldn't access interface.");
		return;
	}

	GtkWidget *add_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_executable_check_button"));
	GtkWidget *add_executable_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_executable_button"));
	GtkWidget *edit_executable_check_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_executable_check_button"));
	GtkWidget *edit_executable_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_executable_button"));

	gboolean check_button_state;
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(add_executable_check_button)) == TRUE) check_button_state = FALSE;
	else check_button_state = TRUE;

	gtk_widget_set_sensitive(add_executable_button, check_button_state);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(edit_executable_check_button)) == TRUE) check_button_state = FALSE;
	else check_button_state = TRUE;

	gtk_widget_set_sensitive(edit_executable_button, check_button_state);	
}


/**
 * removes the selected game from gfire_launch.xml in the manage games window
 *
**/
static void gfire_remove_game_cb(manage_games_callback_args *args, GtkWidget *button)
{
	PurpleConnection *gc = args->gc;
	GtkBuilder *builder = args->builder;
	if (!gc || !builder) {
		purple_debug_error("gfire", "GC not set and/or couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));
	GtkWidget *edit_game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "edit_game_combo"));

	gchar *selected_game = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_game_combo));
	if (selected_game != NULL)
	{
		xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
		xmlnode *gfire_launch_new = xmlnode_new("launch_info");
		xmlnode_set_attrib(gfire_launch_new, "version", "2");
		
		if (gfire_launch != NULL)
		{
			xmlnode *node_child = xmlnode_get_child(gfire_launch, "game");	
			for (node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
			     node_child = xmlnode_get_next_twin(node_child))
			{
				const char *game_name = xmlnode_get_attrib(node_child, "name");
				if(g_strcmp0(game_name, selected_game) != NULL) {
					xmlnode_insert_child(gfire_launch_new, node_child);
				}
			}

			char *gfire_launch_new_str = xmlnode_to_formatted_str(gfire_launch_new, NULL);
			gboolean write_xml = purple_util_write_data_to_file("gfire_launch.xml", gfire_launch_new_str, -1);

			if(write_xml == TRUE) {
				gfire_reload_lconfig(gc);
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Manage Games: game removed"),
					N_("Game removed"), N_("The game has been successfully removed."), NULL, NULL);
			}
			else {
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"),
					N_("Couldn't remove game"), N_("Please try again. An error occured while removing the game."), NULL, NULL);
			}
		}
	}
	else {
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, N_("Manage Games: error"),
			N_("Couldn't remove game"), N_("Please try again. Make sure you select a game to remove."), NULL, NULL);
	}

	gtk_widget_destroy(manage_games_window);
}


/**
 * reloads gfire_launch.xml, the difference with gfire_action_reload_gconfig_cb()
 * is that this function doesn't give feedback to the user.
 *
 * @param gc: the purple connection
 *
**/
static void gfire_reload_lconfig(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	if (gc == NULL || (gfire = (gfire_data *)gc->proto_data) == NULL) {
		purple_debug_error("gfire: gfire_reload_lconfig", "GC not set or found.\n");
		return;
	}

	if (gfire->xml_launch_info != NULL) xmlnode_free(gfire->xml_launch_info);

	gfire->xml_launch_info = NULL;
	gfire_parse_launchinfo_file(gc, g_build_filename(purple_user_dir(), "gfire_launch.xml", NULL));

	if (gfire->xml_launch_info == NULL) {
		purple_debug_error("gfire: gfire_reload_lconfig", "Couldn't reload launch config.\n");
	}
	else purple_debug_info("gfire: gfire_reload_lconfig", "Launch config successfully reloaded.\n");
}

/**
 * creates a new xmlnode containing the game information, the returned node must be inserted
 * in the main launch_info node
 *
 * @param game_id: the game ID
 * @param game_name: the game name
 * @param game_executable: the game executable
 * @param game_prefix: the game launch prefix (environment variables in most cases)
 * @param game_path: the full path to the game executable/launcher
 * @param game_launch: the game launch options
 * @param game_connect: the game connection options
 * 
 * @return: the new xmlnode
 *
**/
xmlnode *gfire_manage_game_xml(char *game_id, char *game_name, gboolean game_executable, char *game_argument,
	char *game_prefix, char *game_path, char *game_launch, char *game_connect)
{
	xmlnode *game_node = xmlnode_new("game");
	xmlnode_set_attrib(game_node, "id", game_id);
	xmlnode_set_attrib(game_node, "name", game_name);
	
	xmlnode *xqf_node = xmlnode_new_child(game_node, "xqf");
	xmlnode_set_attrib(xqf_node, "name", game_name);
	
	xmlnode *command_node = xmlnode_new_child(game_node, "command");
	xmlnode *executable_node = xmlnode_new_child(command_node, "executable");
	xmlnode_insert_data(executable_node, game_executable, -1);
	xmlnode_set_attrib(executable_node, "argument", game_argument);
	xmlnode *prefix_node = xmlnode_new_child(command_node, "prefix");
	xmlnode_insert_data(prefix_node, game_prefix, -1);
	xmlnode *path_node = xmlnode_new_child(command_node, "path");
	xmlnode_insert_data(path_node, game_path, -1);
	xmlnode *launch_node = xmlnode_new_child(command_node, "launch");
	xmlnode_insert_data(launch_node, game_launch, -1);
	xmlnode *connect_node = xmlnode_new_child(command_node, "connect");
	xmlnode_insert_data(connect_node, game_connect, -1);

	return game_node;
}

void gfire_handle_game_detection(PurpleConnection *gc, int gameid, gboolean running, const char *executable)
{
	gfire_data *gfire = NULL;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug_error("gfire", "gfire_handle_game_detection: GC not set.\n");
		return;
	}

	char *game_name = gfire_game_name(gc, gameid);
	int len = 0;

	if (running == TRUE)
	{
		g_thread_create(gfire_detect_game_server, gc, TRUE, NULL);
		
		if (gfire->server_ip != NULL && gfire->server_port != NULL && gfire->gameid != NULL)
		{
			purple_debug_info("gfire_handle_game_detection", "Playing on server (%s:%d)", gfire->server_ip, gfire->server_port);
			
			int packet_len = gfire_join_game_create(gc, gfire->gameid, gfire->server_port,
			                                        (guint8*)gfire_ipstr_to_bin(gfire->server_ip));
			if (packet_len != 0) gfire_send(gc, gfire->buff_out, packet_len);
		}
		else
		{
			purple_debug_info("gfire_handle_game_detection", "Not playing on a server.", gfire->server_ip, gfire->server_port);

			int packet_len = gfire_join_game_create(gc, gfire->gameid, 0, NULL);
			if (packet_len != 0) gfire_send(gc, gfire->buff_out, packet_len);
		}
		
		if (gfire->game_running == FALSE)
		{
			gboolean norm = purple_account_get_bool(purple_connection_get_account(gc), "ingamenotificationnorm", FALSE);
			purple_debug_info("gfire",
							"gfire_handle_game_detection: %s is running. Telling Xfire ingame status.\n", NN(game_name));

			if (norm == TRUE) purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("Ingame status"),
										NN(game_name), N_("Your status has been changed."), NULL, NULL);

			len = gfire_join_game_create(gc, gameid, 0, NULL);
			if (len != FALSE) gfire_send(gc, gfire->buff_out, len);

			gfire->game_running = TRUE;
			gfire->gameid = gameid;
		}
	}
	else
	{
		if(gfire->game_running == TRUE && gfire->gameid == gameid)
		{
			purple_debug(PURPLE_DEBUG_MISC, "gfire",
						"gfire_handle_game_detection: Game not running anymore, sending out of game status.\n");
			gfire->gameid = 0;

			len = gfire_join_game_create(gc, 0, 0, NULL);
			if(len) gfire_send(gc, gfire->buff_out, len);

			gfire->game_running = FALSE;
		}
	}

	if(game_name) g_free(game_name);
}

void gfire_handle_voip_detection(PurpleConnection *gc, int voipid, gboolean running, const char *executable)
{
	gfire_data *gfire = NULL;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug_error("gfire", "gfire_handle_voip_detection: GC not set.\n");
		return;
	}

	char *voip_name = gfire_game_name(gc, voipid);
	int len = 0;

	if (running == TRUE)
	{
		if (gfire->voipid == 0)
		{
			// Get Server IP and Port
			char *executable_name = g_path_get_basename(executable);
			guint8 *voip_ip = NULL;
			guint32 voip_port = 0;

			if(voipid == 32)
				gfire_detect_teamspeak_server(&voip_ip, &voip_port);
			else if(voipid == 34)
				gfire_detect_mumble_server(executable_name, &voip_ip, &voip_port);

			if(executable_name) g_free(executable_name);

			if(!voip_ip)
			{
				purple_debug(PURPLE_DEBUG_MISC, "gfire",
							 "gfire_handle_voip_detection: %s is running but it is not connected.\n", NN(voip_name));
				if(voip_name) g_free(voip_name);
				return;
			}

			gboolean norm = purple_account_get_bool(purple_connection_get_account(gc), "ingamenotificationnorm", FALSE);
			purple_debug(PURPLE_DEBUG_MISC, "gfire",
						 "gfire_handle_voip_detection: %s is running. Telling Xfire voip software status.\n", NN(voip_name));

			if (norm == TRUE) purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, N_("VoIP software status"),
													NN(voip_name), N_("Your status has been changed."), NULL, NULL);

			len = gfire_join_voip_create(gc, voipid, voip_port, voip_ip);
			if (len) gfire_send(gc, gfire->buff_out, len);

			gfire->voipid = voipid;
			gfire->voipip = voip_ip;
			gfire->voipport = voip_port;

			if(voip_name) g_free(voip_name);
			return;
		}
		else if(gfire->voipid == voipid)
		{
			// Get Server IP and Port
			char *executable_name = g_path_get_basename(executable);
			guint8 *voip_ip = NULL;
			guint32 voip_port = 0;

			purple_debug(PURPLE_DEBUG_MISC, "gfire",
						 "gfire_handle_voip_detection: Checking for %ss server state...\n", NN(voip_name));
			if(voipid == 32)
				gfire_detect_teamspeak_server(&voip_ip, &voip_port);
			else if(voipid == 34)
				gfire_detect_mumble_server(executable_name, &voip_ip, &voip_port);

			if(executable_name) g_free(executable_name);

			if(!voip_ip)
			{
				purple_debug(PURPLE_DEBUG_MISC, "gfire",
							 "gfire_handle_voip_detection: %s is not connected anymore. Resetting Xfire voip software status.\n", NN(voip_name));

				if(gfire->voipip) g_free(gfire->voipip);

				gfire->voipid = 0;
				gfire->voipip = NULL;
				gfire->voipport = 0;

				len = gfire_join_voip_create(gc, 0, 0, NULL);
				if (len) gfire_send(gc, gfire->buff_out, len);

				if(voip_name) g_free(voip_name);
				return;
			}
			else if((*((guint32*)gfire->voipip) != *((guint32*)voip_ip)) || gfire->voipport != voip_port)
			{
				purple_debug(PURPLE_DEBUG_MISC, "gfire",
							 "gfire_handle_voip_detection: %ss server changed. Updating Xfire voip software status\n", NN(voip_name));

				if(gfire->voipip) g_free(gfire->voipip);

				gfire->voipip = voip_ip;
				gfire->voipport = voip_port;

				len = gfire_join_voip_create(gc, voipid, voip_port, voip_ip);
				if (len) gfire_send(gc, gfire->buff_out, len);

				if(voip_name) g_free(voip_name);
				return;
			}
			else
			{
				purple_debug(PURPLE_DEBUG_MISC, "gfire",
							 "gfire_handle_voip_detection: %ss server hasn't changed.\n", NN(voip_name));

				if(voip_name) g_free(voip_name);
				return;
			}
		}
	}
	else
	{
		if(gfire->voipid  == voipid)
		{
			purple_debug(PURPLE_DEBUG_MISC, "gfire",
						"gfire_handle_voip_detection: voip software not running anymore. Resetting Xfire voip software status.\n");

			if(gfire->voipip) g_free(gfire->voipip);

			gfire->voipid = 0;
			gfire->voipip = NULL;
			gfire->voipport = 0;

			len = gfire_join_voip_create(gc, 0, 0, NULL);
			if(len) gfire_send(gc, gfire->buff_out, len);

			if(voip_name) g_free(voip_name);
		}
	}
}

int gfire_detect_running_processes_cb(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug_error("gfire", "gfire_detect_running_processes_cb: GC not set.\n");
		return FALSE;
	}

	gboolean norm = purple_account_get_bool(purple_connection_get_account(gc), "ingamedetectionnorm", TRUE);
	if (norm == FALSE)
		return TRUE;

	xmlnode *gfire_launch = gfire->xml_launch_info;
	if (gfire_launch != NULL)
	{
		xmlnode *node_child;
		for (node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
			 node_child = xmlnode_get_next_twin(node_child))
		{
			xmlnode *command_node = xmlnode_get_child(node_child, "command");
			xmlnode *executable_node = xmlnode_get_child(command_node, "executable");
			xmlnode *path_node = xmlnode_get_child(command_node, "path");

			const char *game_executable = xmlnode_get_data(executable_node);
			const char *game_executable_argument = xmlnode_get_attrib(executable_node, "argument");
			const char *game_path = xmlnode_get_data(path_node);
			gchar *game_id = xmlnode_get_attrib(node_child, "id");
			int game_id_int = atoi(game_id);

			char *game_executable_name = g_path_get_basename(game_executable);
			gboolean process_running = check_process(game_executable_name, game_executable_argument);
			if(game_executable_name) g_free(game_executable_name);

			if(game_id_int < 32 || game_id_int > 34)
				gfire_handle_game_detection(gc, game_id_int, process_running, game_executable);
			else
				gfire_handle_voip_detection(gc, game_id_int, process_running, game_executable);

			if(game_executable) g_free(game_executable);
			if(game_path) g_free(game_path);
		}
	}

	return TRUE;
}

/**
 * Checks if an argument has been used on a running process.
 * This function was made to find game mods.
 *
 * @param process_id: the process id
 * @param process_argument: the argument to check
 *
 * @return: TRUE if the process has been launched with the given argument, FALSE if not or if an error occured.
 *
**/ 
gboolean check_process_argument(int process_id, char *process_argument)
{
	char command[256];
	sprintf(command, "cat /proc/%d/cmdline | gawk -F[[:cntrl:]] \'{ for (i = 1; i < NF + 1; i++) printf(\"%%s \", $i) }\'", process_id);

	char buf[256];
	int c;
	int count = 0;

	memset(buf, 0, sizeof(buf));
	FILE *cmd = popen(command, "r");
	
	while(((c = getc(cmd)) != EOF) && (count < (sizeof(buf) - 1))) {
		if(c == '\n') break;
		buf[count++] = c;
	}
	
	pclose(cmd);

	char *argument_match = strstr(&buf, process_argument);
	if (argument_match == NULL) return FALSE;
	else return TRUE;
}


/**
 * checks if a process is running.
 *
 * @param process: the process name
 *
 * @return: TRUE if the process is running, FALSE if not or if an error occured.
 *
**/
gboolean check_process(char *process, char *process_argument)
{
	#ifdef IS_WINDOWS
	return FALSE;
	#else
	char command[256];

	sprintf(command, "ps -ef | grep -i %s | grep -v grep", process);
	//sprintf(command, "lsof \"%s\"", process);
	
	char buf[256];
	int c;
	int count = 0;

	memset(buf, 0, sizeof(buf));
	FILE *cmd = popen(command, "r");
	while(((c = getc(cmd)) != EOF) && (count < (sizeof(buf) - 1))) {
		if(c == '\n') break;
		buf[count++] = c;
	}
	
	pclose(cmd);

	if (g_strcmp0(buf, "") != NULL)
	{
		if (process_argument != NULL)
		{
			int process_id;

			sprintf(command, "pidof %s", g_path_get_basename(process));
			memset(buf, 0, sizeof(buf));
			FILE *cmd = popen(command, "r");

			count = 0;
			while(((c = getc(cmd)) != EOF) && (count < (sizeof(buf) - 1))) {
				if(c == '\n') break;
				buf[count++] = c;
			}

			pclose(cmd);
			process_id = atoi(&buf);
			gboolean process_running_argument = check_process_argument(process_id, process_argument);

			return process_running_argument;
		}
		else return TRUE;
	}
	else return FALSE;
	#endif
}

static void gfire_action_reload_lconfig_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	if (NULL != gfire->xml_launch_info) xmlnode_free(gfire->xml_launch_info);
	gfire->xml_launch_info = NULL;
	gfire_parse_launchinfo_file(gc, g_build_filename(purple_user_dir(), "gfire_launch.xml", NULL));

	if (NULL == gfire->xml_launch_info) {
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_ERROR, N_("Gfire XML Reload"), N_("Reloading gfire_launch.xml"), N_("Operation failed. File not found or content was incorrect."), NULL, NULL);
	} else {
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, N_("Gfire XML Reload"), N_("Reloading gfire_launch.xml"),N_("Reloading was successful."), NULL, NULL);

	}

}

static void gfire_action_reload_gconfig_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	gfire_data *gfire = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	if (NULL != gfire->xml_games_list) xmlnode_free(gfire->xml_games_list);
	gfire->xml_games_list = NULL;
	gfire_parse_games_file(gc, g_build_filename(purple_user_dir(), "gfire_games.xml", NULL));

	if (NULL == gfire->xml_games_list) {
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_ERROR, N_("Gfire XML Reload"), N_("Reloading gfire_games.xml"), N_("Operation failed. File not found or content was incorrect."), NULL, NULL);
	}
	else {
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, N_("Gfire XML Reload"), N_("Reloading gfire_games.xml"),N_("Reloading was successful."), NULL, NULL);
	}
}

static void gfire_action_website_cb() {
	purple_notify_uri((void *)_gfire_plugin, GFIRE_WEBSITE);
}

static void gfire_action_wiki_cb() {
	purple_notify_uri((void *)_gfire_plugin, GFIRE_WIKI);
}

static void gfire_action_about_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	char *msg = NULL;

	gchar *version_str = gfire_game_name(gc, 100);

	if(version_str && g_strcmp0(version_str, "100") != 0) {
		msg = g_strdup_printf(N_("Gfire Version:\t\t%s\nGame List Version:\t%s"), GFIRE_VERSION, version_str);
		g_free(version_str);
	} else {
		if(version_str) g_free(version_str);
		msg = g_strdup_printf(N_("Gfire Version: %s"), GFIRE_VERSION);
	}

	purple_request_action(gc, N_("About Gfire"), N_("Xfire Plugin for Pidgin"), msg, PURPLE_DEFAULT_ACTION_NONE,
						  purple_connection_get_account(gc), NULL, NULL, gc, 3, N_("Close"), NULL,
						  N_("Website"), G_CALLBACK(gfire_action_website_cb),
						  N_("Wiki"), G_CALLBACK(gfire_action_wiki_cb));

	if(msg) g_free(msg);
}

static void gfire_action_get_gconfig_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	gfire_data *gfire = NULL;
	const char *filename = g_build_filename(purple_user_dir(), "gfire_games.xml", NULL);

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

	purple_util_fetch_url(GFIRE_GAMES_XML_URL, TRUE, "Purple-xfire", TRUE, gfire_xml_download_cb, (void *)gc);

	if (NULL != gfire->xml_games_list) xmlnode_free(gfire->xml_games_list);
	gfire->xml_games_list = NULL;
	gfire_parse_games_file(gc, filename);
}

static void gfire_action_profile_page_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	PurpleAccount *a = purple_connection_get_account(gc);
	if (!a || !(a->username)) return;

	char uri[256] = "";
	g_sprintf(uri, "%s%s", XFIRE_PROFILE_URL, a->username);
	purple_notify_uri((void *)_gfire_plugin, uri);
}

static void gfire_query_server_list_cb(manage_games_callback_args *args, GtkWidget *button)
{
	PurpleConnection *gc = args->gc;
	GtkBuilder *builder = args->builder;
	
	if (!gc || !builder) {
		purple_debug_error("gfire_query_server_list_cb", "GC not set and/or couldn't get interface.\n");
		return;
	}

	gfire_data *gfire = NULL;
	if (!(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug_error("gfire_query_server_list_cb", "Couldn't access gfire data.\n");
		return;
	}

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(builder, "server_browser_window"));
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "game_combo"));

	const gchar *game_name;
	int game_id;

	game_name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	game_id = gfire_game_id(gc, game_name);
	
	if (game_id != 0)
	{
        int packet_len = 0; 
		
		gfire->server_browser = builder;
		packet_len = gfire_create_serverlist_request(gc, game_id); 
		if (packet_len != 0) gfire_send(gc, gfire->buff_out, packet_len);
	}
}

static void gfire_server_browser_connect_cb(manage_games_callback_args *args, GtkWidget *sender)
{
	PurpleConnection *gc = args->gc;
	GtkBuilder *builder = args->builder;
	
	if (!gc || !builder) {
		purple_debug_error("gfire_server_browser_connect_cb", "GC not set and/or couldn't get interface.\n");
		return;
	}

	gfire_data *gfire = NULL;
	if (!(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug_error("gfire_server_browser_connect_cb", "Couldn't access gfire data.\n");
		return;
	}

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(builder, "server_browser_window"));
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "game_combo"));
	GtkWidget *servers_tree_view = GTK_WIDGET(gtk_builder_get_object(builder, "servers_tree_view"));
	
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	gchar *game = gtk_combo_box_get_active_text(GTK_COMBO_BOX(game_combo));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(servers_tree_view));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(servers_tree_view));
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *server;
		gchar *server_ip;
		gchar **server_tok;
		int server_port, game_id;
		
		gtk_tree_model_get(model, &iter, 1, &server, -1);
		server_tok = g_strsplit(server, ":", -1);
		
		server_ip = g_strdup_printf("%s", server_tok[0]);
		server_port = atoi(server_tok[1]);
		game_id = gfire_game_id(gc, game);
		
		gfire_join_game(gc, server_ip, server_port, game_id);
	}
	else purple_debug_error("gfire_server_browser_connect_cb", "Couldn't get selected server.\n");
}

/**
 * shows the server browser window.
 *
 * @param action: the menu action, passed by the signal connection function
 *
**/
static void gfire_action_server_browser_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *)action->context;
	gfire_data *gfire = NULL;

	if (gc == NULL || (gfire = (gfire_data *)gc->proto_data) == NULL) {
		purple_debug_error("gfire: gfire_action_manage_games_cb", "GC not set and/or couldn't access gfire data.\n");
		return;
	}

	GtkBuilder *builder = gtk_builder_new();
	gchar *builder_file;

	builder_file = g_build_filename(DATADIR, "purple", "gfire", "servers.glade", NULL);
	gtk_builder_add_from_file(builder, builder_file, NULL);
	g_free(builder_file);

	if (builder == NULL) {
		purple_debug_error("gfire: gfire_action_manage_games_cb", "Couldn't build interface.\n");
		return;
	}

	GtkWidget *server_browser_window = GTK_WIDGET(gtk_builder_get_object(builder, "server_browser_window"));
	GtkWidget *refresh_button = GTK_WIDGET(gtk_builder_get_object(builder, "refresh_button"));
	GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
	GtkWidget *favorite_button = GTK_WIDGET(gtk_builder_get_object(builder, "favorite_button"));
	GtkWidget *informations_button = GTK_WIDGET(gtk_builder_get_object(builder, "informations_button"));
	GtkWidget *favorites_button = GTK_WIDGET(gtk_builder_get_object(builder, "favorites_button"));
	GtkWidget *quit_button = GTK_WIDGET(gtk_builder_get_object(builder, "quit_button"));
	GtkWidget *game_combo = GTK_WIDGET(gtk_builder_get_object(builder, "game_combo"));
	GtkWidget *status_bar = GTK_WIDGET(gtk_builder_get_object(builder, "status_bar"));

	manage_games_callback_args *args;

	args = g_new0(manage_games_callback_args, 1);
	args->gc = gc;
	args->builder = builder;
	
	g_signal_connect_swapped(quit_button, "clicked", G_CALLBACK(gtk_widget_destroy), server_browser_window);
	g_signal_connect_swapped(refresh_button, "clicked", G_CALLBACK(gfire_query_server_list_cb), args);
	g_signal_connect_swapped(connect_button, "clicked", G_CALLBACK(gfire_server_browser_connect_cb), args);

	xmlnode *gfire_launch = gfire->xml_launch_info;
	if (gfire_launch != NULL)
	{
		xmlnode *node_child;
		for (node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
		    node_child = xmlnode_get_next_twin(node_child))
		{
			const char *game_name = xmlnode_get_attrib(node_child, "name");
			gtk_combo_box_append_text(GTK_COMBO_BOX(game_combo), game_name);
		}
	}

	gtk_widget_show_all(server_browser_window);
}

static GList *gfire_actions(PurplePlugin *plugin, gpointer context)
{
	GList *m = NULL;
	PurplePluginAction *act;

	act = purple_plugin_action_new(N_("Change Nickname"),
			gfire_action_nick_change_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(N_("My Profile Page"),
			gfire_action_profile_page_cb);
	m = g_list_append(m, act);
	m = g_list_append(m, NULL);
	act = purple_plugin_action_new(N_("Reload Launch Config"),
			gfire_action_reload_lconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(N_("Reload Game ID List"),
			gfire_action_reload_gconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(N_("Get Game ID List"),
			gfire_action_get_gconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(N_("Manage Games"),
			gfire_action_manage_games_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new(N_("Server Browser"),
		gfire_action_server_browser_cb);
	m = g_list_append(m, act);
	m = g_list_append(m, NULL);
	act = purple_plugin_action_new(N_("About"),
			gfire_action_about_cb);
	m = g_list_append(m, act);
	return m;
}

/**
 * Joins the game a buddy is playing.
 * This function launches the game and tells the game to connect to the corresponding server if needed.
 *
 * @param gc: the purple connection
 * @param server_ip: the server ip to join (quad dotted notation)
 * @param server_port: the server port
 * @param game_id: the game ID to launch
 *
**/
void gfire_join_game(PurpleConnection *gc, const gchar *server_ip, int server_port, int game_id)
{
	gfire_data *gfire = NULL;
	if ((gfire = (gfire_data *)gc->proto_data) == NULL) {
		purple_debug_error("gfire: gfire_join_game()", "Couldn't access gfire data.\n");
		return;
	}
	
	gfire_linfo *game_launch_info = NULL;
	const char null_ip[4] = {0x00, 0x00, 0x00, 0x00};
	gchar *game_launch_command;

 	game_launch_info = gfire_linfo_get(gc, game_id);
 	if (game_launch_info == NULL) {
 		purple_debug_error("gfire: gfire_join_game()", "Game launch info struct not defined!\n");
 		return;
 	}

 	if (server_ip == NULL) server_ip = (char *)&null_ip;

	gchar *tmp = gfire_ipstr_to_bin(server_ip);
	
	game_launch_command = gfire_linfo_get_cmd(game_launch_info, (guint8 *)tmp, server_port, NULL);
	g_spawn_command_line_async(game_launch_command, NULL);
}

void gfire_avatar_download_cb( PurpleUtilFetchUrlData *url_data, gpointer data, const char *buf, gsize len, const gchar *error_message)
{
	PurpleConnection *gc = NULL;
	PurpleBuddy *pbuddy = (PurpleBuddy*)data;
	PurpleBuddyIcon *buddy_icon = NULL;
	guchar *temp = NULL;

	if (data == NULL || buf == NULL || len == 0)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_avatar_download_cb: download of avatar failed (%s)\n", NN(error_message));
		return;
	}

	if((unsigned char)buf[0] != 0xFF || (unsigned char)buf[1] != 0xD8)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "gfire_avatar_download_cb: invalid jpeg file (first two bytes: %02x%02x)\n", buf[0], buf[1]);
		return;
	}

	gc = pbuddy->account->gc;
	if ( PURPLE_CONNECTION_IS_VALID(gc) && PURPLE_CONNECTION_IS_CONNECTED(gc) )
	{
		temp = g_new0(guchar, len);
		memcpy(temp, buf, len);

		buddy_icon = purple_buddy_get_icon(pbuddy);
		if(buddy_icon == NULL)
			buddy_icon = purple_buddy_icon_new(gc->account, pbuddy->name, temp, len, NULL);
		else
			purple_buddy_icon_set_data(buddy_icon, temp, len, NULL);

		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_avatar_download_cb: avatar successfully changed\n");
	}
}

/**
 * Replaces a substring by another substring in a string
 * and returns the new string.
**/
char *str_replace (char *string, char *before, char *after)
{
	const char *pos;
	char *replaced;
	size_t replaced_pos;

	size_t len;
	size_t allocated_size;

	pos = strstr(string, before);
	if (pos == NULL) return string;

	len = (size_t)pos - (size_t)string;
	allocated_size = len + strlen (after) + 1;
	replaced = malloc (allocated_size);
	replaced_pos = 0;

	strncpy (replaced + replaced_pos, string, len);
	replaced_pos += len;
	string = pos + strlen (before);

	len = strlen (after);
	strncpy (replaced + replaced_pos, after, len);
	replaced_pos += len;

	pos = strstr (string, before);
	while (pos != NULL)
	{
		len = (size_t)pos - (size_t)string;
		allocated_size += len + strlen (after);
		replaced = (char *)realloc (replaced, allocated_size);

		strncpy (replaced + replaced_pos, string, len);
		replaced_pos += len;

		string = pos + strlen (before);

		len = strlen (after);
		strncpy (replaced + replaced_pos, after, len);
		replaced_pos += len;

		pos = strstr (string, before);
	}

	len = strlen (string) + 1;
	allocated_size += len;
	replaced = realloc (replaced, allocated_size);
	strncpy (replaced + replaced_pos, string, len);

	return replaced;
}

/**
 * Replaces a character by a string in a string
 * and returns the new string.
**/
gchar *str_replace_c(const gchar *p_src, gchar p_find, const gchar *p_replace)
{
	if(!p_src || !p_replace)
		return NULL;

	guint16 len_replace = strlen(p_replace);

	guint16 alloc_len = strlen(p_src) + 11;
	gchar *ret = (gchar*)g_malloc0(alloc_len);

	int pos = 0;
	int ret_pos = 0;
	while(p_src[pos] != 0)
	{
		if(p_src[pos] == p_find)
		{
			if((alloc_len - len_replace - ret_pos) < 0)
			{
				alloc_len += len_replace + 1;
				ret = (gchar*)g_realloc(ret, alloc_len);
			}
			memcpy(ret + ret_pos, p_replace, len_replace);
			ret_pos += len_replace;
		}
		else
		{
			ret[ret_pos] = p_src[pos];
			ret_pos++;
		}

		pos++;
	}

	ret[ret_pos] = 0;

	return ret;
}

/**
 * Removes color codes from a string,
 * used in the server browser to display the server names properly.
**/
char *gfire_escape_color_codes(char *string)
{
	char color_codes[] = {
		'0', 'P', '9', 'Y', 'Z', '7', 'W',
		'J', '1', 'Q', 'I',
		'H', '2', 'R', 'G',
		'M', '3', 'S', 'O', 'N',
		'F', 'D', '4', 'T',
		'B', '5', 'U',
		'C', 'E', '6', 'V',
		'K', 'L', '8', 'Y', 'A',
		'?', '+', '@', '-', '/',
		'&', 'X'
	};
	
	gchar *escaped = g_strdup_printf("%s", string);

	if (escaped != NULL)
	{	
		int i;
		for (i = 0; i < 43; i++)
		{
			gchar *code, *code_lower_case;

			code = g_strdup_printf("^%c", color_codes[i]);
			code_lower_case = g_ascii_strdown(code, -1);

			escaped = str_replace(escaped, code, "");
			escaped = str_replace(escaped, code_lower_case, "");
		}
	}

	return escaped;
}

char *gfire_escape_html(const char *html)
{
	if (html != NULL) {
		gchar *tmp = NULL;
		gchar *tmp2 = NULL;

		tmp = str_replace_c(html, '&', "&amp;");

		tmp2 = str_replace_c(tmp, '<', "&lt;");
		if(tmp) g_free(tmp);

		tmp = str_replace_c(tmp2, '>', "&gt;");
		if(tmp2) g_free(tmp2);

		tmp2 = str_replace_c(tmp, '"', "&quot;");
		if(tmp) g_free(tmp);

		tmp = str_replace_c(tmp2, '\n', "<br />");
		if(tmp2) g_free(tmp2);

		return tmp;

		/* Temporary commented

		char *c = html;
		GString *ret = g_string_new("");
		while (*c) {
			if (*c == '&') {
				ret = g_string_append(ret, "&amp;");
				c++;
			} else if (*c == '<') {
				ret = g_string_append(ret, "&lt;");
				c++;
			} else if (*c == '>') {
				ret = g_string_append(ret, "&gt;");
				c++;
			} else if (*c == '\"') {
				ret = g_string_append(ret, "&quot;");
				c++;
			} else if (*c == '\n') {
				ret = g_string_append(ret, "<br />");
				c++;
			} else {
				ret = g_string_append_c(ret, *c);
				c++;
			}
		}

		gchar *copy = g_strdup(ret->str);
		g_string_free(ret, TRUE);
		return copy;
		*/
	}
	else
		return NULL;
}

void gfire_detect_teamspeak_server(guint8 **voip_ip, guint32 *voip_port)
{
	if(!voip_ip || !voip_port)
		return;

	int unix_socket = -1;
	struct sockaddr_un unix_addr;

	unix_addr.sun_family = AF_UNIX;
	strcpy(unix_addr.sun_path, "/tmp/.teamspeakclient");

	unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if(unix_socket < 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Failed to create socket.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		return;
	}

	// Connect to TeamSpeak client
	if(connect(unix_socket, (struct sockaddr*)&unix_addr, strlen(unix_addr.sun_path) + sizeof(unix_addr.sun_family)) != 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Failed to connect to unix socket.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		return;
	}

	// Send Server Data Request (Format: Message length (incl. zero), 3x zero, Message, zero)
	// Message: "GET_SERVER_INFO" => Length = 16
	char request[20] = {0x10, 0x00, 0x00, 0x00, 'G', 'E', 'T', '_', 'S', 'E', 'R', 'V', 'E', 'R', '_', 'I', 'N', 'F', 'O', 0x00};
	if(send(unix_socket, request, sizeof(request), 0) <= 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Failed to send server data request.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		close(unix_socket);
		return;
	}

	// Receive response
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));

	int recBytes = 0;
	int r = 0;
	do
	{
		r = recv(unix_socket, &buffer[recBytes], sizeof(buffer) - 1 - recBytes, 0);
		recBytes += r;
	} while(r > 0);

	if(recBytes <= 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Failed to receive server data.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		close(unix_socket);
		return;
	}

	buffer[recBytes] = 0;
	char *data = &buffer[4]; // Skip the size and zeros
	if(strncmp(data, "ERROR", 5) == 0)
	{
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: TeamSpeak doesn't seem to be connected.\n");
		*voip_ip = NULL;
		*voip_port = 0;
		close(unix_socket);
		return;
	}

	// Extract Server Address
	char *tmp = data;
	char *strings = NULL;
	int count = 0;
	while((strings = strtok(tmp, "\r\n")))
	{
		if(count == 5)
		{
			char *split = strchr(strings, ':');
			if(!split)
			{
				purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: Received invalid data.\n");
				*voip_ip = NULL;
				*voip_port = 0;
				close(unix_socket);
			}

			*split = ' ';
			char ip[16];
			guint32 port;
			sscanf(strings, "%s %d", ip, &port);
			purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_teamspeak_server: TeamSpeak Server: %s:%d\n", ip, port);

			*voip_ip = (guint8*)gfire_ipstr_to_bin(ip);
			*voip_port = port;
		}

		count = count + 1;
		tmp = NULL;
	}

	close(unix_socket);
}

void gfire_detect_mumble_server(const gchar *executable, guint8 **voip_ip, guint32 *voip_port)
{
	if(!voip_ip || !voip_port || !executable)
		return;

	// Build command line
	char cmd[1024];
	sprintf(cmd, "netstat -tnp | grep -i %s", executable);

	// Read output
	FILE *netstat = popen(cmd, "r");
	char data[100];
	if(fgets(data, 100, netstat) == NULL && feof(netstat))
	{
		// No data. Mumble is not connected.
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_mumble_server: Mumble doesn't seem to be connected.\n");
		pclose(netstat);
		*voip_ip = NULL;
		*voip_port = 0;
		return;
	}
	pclose(netstat);

	// Extract "IP:Port"
	char addr[22];
	sscanf(data, "%*s\t%*d\t%*d\t%*s\t%s", addr);
	// Replace ':' with ' ' for sscanf
	(*strchr(addr, ':')) = ' ';
	// Extract IP as a string and port
	char ip[16];
	guint32 port;
	sscanf(addr, "%s %u", ip, &port);
	purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_detect_mumble_server: Mumble Server: %s:%d\n", ip, port);
	*voip_ip = (guint8*)gfire_ipstr_to_bin(ip);
	*voip_port = port;
}

static void gfire_detect_game_server(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	if (gc == NULL || (gfire = (gfire_data *)gc->proto_data) == NULL) {
		purple_debug_error("gfire_detect_game_server", "GC not set and/or couldn't access gfire data.\n");
		return;
	}

	int game_id = gfire->gameid;
	xmlnode *gfire_games = gfire->xml_games_list;

	if (game_id != NULL && gfire_games != NULL)
	{
		xmlnode *node_child;
		gboolean *server_detect;
		char *server_ip;
		
		for (node_child = xmlnode_get_child(gfire_games, "game"); node_child != NULL; node_child = xmlnode_get_next_twin(node_child))
		{
 			const char *game_id_tmp = xmlnode_get_attrib(node_child, "id");
			xmlnode *server_detect_node = xmlnode_get_child(node_child, "server_detect");

			if (atoi(game_id_tmp) == game_id) {
				if (g_strcmp0(xmlnode_get_data(server_detect_node), "true") == NULL) server_detect = TRUE;
				else server_detect = FALSE;
			}
		}

		if (server_detect == TRUE)
		{
			char command[128], line[2048];
			int c, count;
		
			memset(line, 0, sizeof(line));
		
			sprintf(command, "tcpdump -f -c 30");
			FILE *command_pipe = popen(command, "r");
		
			if (command_pipe != NULL)
			{
				GRegex *regex = NULL;
				GError *regex_error = NULL;
				GMatchInfo *regex_matches = NULL;
				gboolean regex_match = FALSE;

				regex = g_regex_new("(\\d{1,3}\\.){4}\\d{4,5}", G_REGEX_OPTIMIZE, 0, &regex_error);
			
				while (fgets(line, sizeof line, command_pipe) != NULL)
				{
					gchar *current_line = g_strdup_printf("%s", line);
					regex_match = g_regex_match(regex, current_line, 0, &regex_matches);
				
					if (regex_match == TRUE) server_ip = g_match_info_fetch(regex_matches, 0);
					else server_ip = NULL;
				}
				
				fclose(command_pipe);

				if (server_ip != NULL)
				{
					gchar **server_ip_split = g_strsplit(server_ip, ".", -1);

					if (server_ip_split != NULL)
					{
						char *server_ip_str = g_strdup_printf("%s.%s.%s.%s", server_ip_split[0], server_ip_split[1],
						                                      server_ip_split[2], server_ip_split[3]);
						int server_port_tmp = atoi(server_ip_split[4]);
						
						g_mutex_lock(gfire->server_mutex);
						gfire->server_ip = server_ip_str;
						gfire->server_port = server_port_tmp;
						g_mutex_unlock(gfire->server_mutex);
					}
				}
				else
				{
					g_mutex_lock(gfire->server_mutex);
					gfire->server_ip = NULL;
					gfire->server_port = NULL;
					g_mutex_unlock(gfire->server_mutex);
				}
			}
		}
	}
}

/**
 *
 * Plugin initialization section
 *
*/
static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_CHAT_TOPIC,		/* Protocol options  */
	NULL,						/* user_splits */
	NULL,						/* protocol_options */
	NO_BUDDY_ICONS,				/* icon_spec */
	gfire_blist_icon,			/* list_icon */
	gfire_blist_emblems,		/* list_emblems */
	gfire_status_text,			/* status_text */
	gfire_blist_tooltip_text,	/* tooltip_text */
	gfire_status_types,			/* away_states */
	gfire_node_menu,			/* blist_node_menu */
	gfire_chat_info,			/* chat_info */
	gfire_chat_info_defaults,	/* chat_info_defaults */
	gfire_login,				/* login */
	gfire_close,				/* close */
	gfire_im_send,				/* send_im */
	NULL,						/* set_info */
	gfire_send_typing,			/* send_typing */
	gfire_get_info,				/* get_info */
	gfire_set_status,			/* set_status */
	NULL,						/* set_idle */
	NULL,						/* change_passwd */
	gfire_add_buddy,			/* add_buddy */
	NULL,						/* add_buddies */
	gfire_remove_buddy,			/* remove_buddy */
	NULL,						/* remove_buddies */
	NULL,						/* add_permit */
	NULL,						/* add_deny */
	NULL,						/* rem_permit */
	NULL,						/* rem_deny */
	NULL,						/* set_permit_deny */
	gfire_join_chat,			/* join_chat */
	gfire_reject_chat,			/* reject chat invite */
	gfire_get_chat_name,		/* get_chat_name */
	gfire_chat_invite,			/* chat_invite */
	gfire_chat_leave,			/* chat_leave */
	NULL,						/* chat_whisper */
	gfire_chat_send,			/* chat_send */
	gfire_keep_alive,			/* keepalive */
	NULL,						/* register_user */
	NULL,						/* get_cb_info */
	NULL,						/* get_cb_away */
	NULL,						/* alias_buddy */
	NULL,						/* group_buddy */
	NULL,						/* rename_group */
	NULL,						/* buddy_free */
	NULL,						/* convo_closed */
	purple_normalize_nocase,	/* normalize */
	NULL,						/* set_buddy_icon */
	NULL,						/* remove_group */
	NULL,						/* get_cb_real_name */
	gfire_chat_change_motd,		/* set_chat_topic */
	NULL,						/* find_blist_chat */
	NULL,						/* roomlist_get_list */
	NULL,						/* roomlist_cancel */
	NULL,						/* roomlist_expand_category */
	NULL,						/* can_receive_file */
	NULL,						/* send_file */
	NULL,						/* new_xfer */
	NULL,						/* offline_message */
	NULL,						/* whiteboard_prpl_ops */
	NULL,						/* send_raw */
	NULL,						/* roomlist_room_serialize */
	NULL,						/* unregister_user */
	NULL,						/* send_attention */
	NULL,						/* attention_types */

	/* padding */
	NULL,
	NULL
};


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,		/* type */
	NULL,						/* ui_requirement */
	0,							/*flags */
	NULL,						/* dependencies */
	PURPLE_PRIORITY_DEFAULT,	/* priority */
	"prpl-xfire",				/* id */
	"Xfire",					/* name */
	GFIRE_VERSION,				/* version */
	"Xfire Protocol Plugin",	/* summary */
	"Xfire Protocol Plugin",	/*  description */
	NULL,						/* author */
	GFIRE_WEBSITE,				/* homepage */
	NULL,						/* load */
	NULL,						/* unload */
	NULL,						/* destroy */
	NULL,						/* ui_info */
	&prpl_info,					/* extra_info */
	NULL,						/* prefs_info */
	gfire_actions,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void _init_plugin(PurplePlugin *plugin)
{
	PurpleAccountOption *option;

	option = purple_account_option_string_new(N_("Server"), "server",XFIRE_SERVER);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_int_new(N_("Port"), "port", XFIRE_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_int_new(N_("Version"), "version", XFIRE_PROTO_VERSION);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(N_("Don't delete buddies from server"), "buddynorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(N_("Buddies can see if I'm typing"), "typenorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);
	
	option = purple_account_option_bool_new(N_("Auto detect for ingame status"), "ingamedetectionnorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new(N_("Notify me when my status is ingame"), "ingamenotificationnorm", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	setlocale(LC_ALL, "");
	bindtextdomain("gfire", LOCALEDIR);
	textdomain("gfire");

	_gfire_plugin = plugin;
}

PURPLE_INIT_PLUGIN(gfire, _init_plugin, info);
