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
static void gfire_login_cb(gpointer data, gint source, PurpleInputCondition cond);
static void gfire_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group);
static void gfire_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group);
void gfire_buddy_menu_profile_cb(PurpleBlistNode *node, gpointer *data);
GList * gfire_node_menu(PurpleBlistNode *node);
gboolean separe_path(char *path, char **file);

void gfire_join_game(PurpleConnection *gc, const gchar *sip, int sport, int game);
void gfire_game_watch_cb(GPid pid, int status, gpointer *data);
char *gfire_escape_html(const char *html);

static void gfire_action_manage_games_cb(PurplePluginAction *action);
static void gfire_add_game_cb(manage_games_callback_args *args, GtkWidget *button);
static void gfire_edit_game_cb(manage_games_callback_args *args, GtkWidget *button);
static void gfire_manage_games_add_update_label_cb(GtkWidget *add_path_label, GtkWidget *add_type_combo_box);
static void gfire_manage_games_edit_update_fields_cb(GtkBuilder *builder, GtkWidget *edit_games_combo_box);
static void gfire_remove_game_cb(manage_games_callback_args *args, GtkWidget *button);
static void gfire_reload_lconfig(PurpleConnection *gc);
xmlnode *gfire_manage_game_xml(char *game_id, char *game_name, char *game_type, char *game_bin,
	char *game_dir, char *game_mod, char *game_connect, char *game_launch, char *unix_process, char *windows_process);

gboolean separe_path(char *path, char **file);
gboolean check_process(char *process);

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
	if (0 != gf_buddy->gameid)
		return "game";
	}

	return NULL;
}


static char *gfire_status_text(PurpleBuddy *buddy)
{
	char msg[100];
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
			g_sprintf(msg, "Playing %s", game_name);
			g_free(game_name);
			return g_strdup(msg);
		}
		if (gf_buddy->away) {
			g_sprintf(msg,"%s", gf_buddy->away_msg);
			return gfire_escape_html(msg);
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

				purple_notify_user_info_add_pair(user_info,"Game",game_name);

			g_free(game_name);
			magic = (guint32 *)gf_buddy->gameip;
			if ((NULL != gf_buddy->gameip) && (0 != *magic)) {

				g_sprintf(ipstr, "%d.%d.%d.%d", gf_buddy->gameip[3], gf_buddy->gameip[2],
						gf_buddy->gameip[1], gf_buddy->gameip[0]);

					gchar * value = g_strdup_printf("%s:%d", ipstr, gf_buddy->gameport);
					purple_notify_user_info_add_pair(user_info,"Server",value);
					g_free(value);

			}
		}
		if (gf_buddy->away) {
			char * escaped_away = gfire_escape_html(gf_buddy->away_msg);

				purple_notify_user_info_add_pair(user_info,"Away",escaped_away);

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

	purple_connection_update_progress(gc, "Connecting", 0, XFIRE_CONNECT_STEPS);

	gc->proto_data = gfire = g_new0(gfire_data, 1);
	gfire->fd = -1;
	gfire->buff_out = gfire->buff_in = NULL;
	/* load game xml from user dir */
	gfire_parse_games_file(gc, g_build_filename(purple_user_dir(), "gfire_games.xml", NULL));
	gfire_parse_launchinfo_file(gc, g_build_filename(purple_user_dir(), "gfire_launch.xml", NULL));

	err = purple_proxy_connect(
				NULL,
				account,
				purple_account_get_string(account, "server", XFIRE_SERVER),
				purple_account_get_int(account, "port", XFIRE_PORT),
				gfire_login_cb, gc);
	err = !err;


	if (err || !purple_account_get_connection(account)) {
			purple_connection_error(gc, "Couldn't create socket.");
			return;
	}
}


static void gfire_login_cb(gpointer data, gint source, PurpleInputCondition cond)
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
		purple_connection_error(gc, "Unable to connect to host.");
		return;
	}

	gfire->fd = source;

	/* Update the login progress status display */

	purple_connection_update_progress(gc, "Login", 1, XFIRE_CONNECT_STEPS);

	gfire_send(gc, (const guint8 *)"UA01", 4); /* open connection */

	length = gfire_initialize_connection(packet,purple_account_get_int(account, "version", XFIRE_PROTO_VERSION));
	gfire_send(gc, packet, length);

	gc->inpa = purple_input_add(gfire->fd, PURPLE_INPUT_READ, gfire_input_cb, gc);
}


void gfire_close(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	GList *buddies = NULL;
	GList *chats = NULL;
	gfire_buddy *b = NULL;
	gfire_chat *gf_chat = NULL;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONNECTION: close requested.\n");
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: closing, but either no GC or ProtoData.\n");
		return;
	}

	if (gc->inpa) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: removing input handler\n");
		purple_input_remove(gc->inpa);
	}

	if (gfire->xqf_source > 0) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: removing xqf file watch callback\n");
		g_source_remove(gfire->xqf_source);
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
		g_free(b);
		buddies->data = NULL;
 		buddies = g_list_next(buddies);
	}

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "CONN: freeing chat sturct\n");
	chats = gfire->chats;
	while (NULL != buddies) {
		gf_chat = (gfire_chat *)chats->data;
		if (NULL != gf_chat->purple_id) g_free(gf_chat->purple_id);
		if (NULL != gf_chat->members) g_free(gf_chat->members);
		if (NULL != gf_chat->chat_id) g_free(gf_chat->chat_id);
		if (NULL != gf_chat->topic) g_free(gf_chat->topic);
		if (NULL != gf_chat->motd) g_free(gf_chat->motd);
		if (NULL != gf_chat->c) g_free(gf_chat->c);
		g_free(gf_chat);
		chats->data = NULL;
 		chats = g_list_next(chats);
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
	if (NULL != gfire->xml_games_list) xmlnode_free(gfire->xml_games_list);
	if (NULL != gfire->xml_launch_info) xmlnode_free(gfire->xml_launch_info);
	if (NULL != gfire->userid) g_free(gfire->userid);
	if (NULL != gfire->sid) g_free(gfire->sid);
	if (NULL != gfire->alias) g_free(gfire->alias);

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
		purple_conv_present_error(who, account, "Message could not be sent. Buddy not in contact list");
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
		purple_conv_present_error(who, account, "Message could not be sent. Buddy offline");
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

	purple_notify_user_info_add_pair(user_info, "Nickname", gf_buddy->alias);
	purple_notify_user_info_add_pair(user_info, "Status", status);
	if ((NULL != gf_buddy->gameid) && (NULL != gf_buddy->away))
		purple_notify_user_info_add_pair(user_info, "Away", gf_buddy->away_msg);
	magic = (guint32 *)gf_buddy->gameip;
	if ((NULL != gf_buddy->gameip) && (0 != *magic)) {
		g_sprintf(ipstr, "%d.%d.%d.%d", gf_buddy->gameip[3], gf_buddy->gameip[2],
			gf_buddy->gameip[1], gf_buddy->gameip[0]);
		server = g_strdup_printf("%s:%d", ipstr, gf_buddy->gameport);
		purple_notify_user_info_add_pair(user_info,"Server",server);
		g_free(server);
	}

	purple_notify_user_info_add_section_break(user_info);


	purple_notify_userinfo(gc, who, user_info, NULL, NULL);
	purple_notify_user_info_destroy(user_info);

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
			msg = "(AFK) Away From Keyboard";
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
		g_sprintf(tmp, "Not Removing %s", NN(buddy->name));
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "gfire_remove_buddy: buddynorm TRUE not removing buddy %s.\n",
					NN(buddy->name));
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, "Xfire Buddy Removal Denied", tmp, "Account settings are set to not remove buddies\n" "The buddy will be restored on your next login", NULL, NULL);
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


void gfire_new_buddy(PurpleConnection *gc, gchar *uid_str, gchar *alias, gchar *name)
{
	PurpleBuddy *buddy = NULL;
	PurpleAccount *account = NULL;
	PurpleGroup *default_purple_group = NULL;

	account = purple_connection_get_account(gc);
	default_purple_group = purple_find_group(GFIRE_DEFAULT_GROUP_NAME);
	buddy = purple_find_buddy(account, name);
	if (buddy == NULL) {
		if (NULL == default_purple_group) {
			default_purple_group = purple_group_new(GFIRE_DEFAULT_GROUP_NAME);
			purple_blist_add_group(default_purple_group, NULL);
		}
		buddy = purple_buddy_new(account, g_strdup(name), NULL);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(buddylist): buddy %s not found in Pidgin buddy list, adding.\n",
				NN(name));
		purple_blist_add_buddy(buddy, NULL, default_purple_group, NULL);
		serv_got_alias(gc, g_strdup(name), g_strdup(alias));
	} else {
		serv_got_alias(gc, g_strdup(name), g_strdup(alias));
	}
}


void gfire_new_buddies(PurpleConnection *gc)
{
	gfire_data *gfire = (gfire_data *)gc->proto_data;
	gfire_buddy *b = NULL;
	GList *tmp = gfire->buddies;
	int packet_len = 0;

	while (NULL != tmp) {
		b = (gfire_buddy *)tmp->data;
		if (!b) return;
		gfire_new_buddy(gc, b->uid_str, b->alias, b->name);
//		packet_len = gfire_request_avatar_info(gc, b);
//		purple_debug(PURPLE_DEBUG_MISC, "gfire", "avatar: sending avatar info request\n");
//		gfire_send(gc, gfire->buff_out, packet_len);
		tmp = g_list_next(tmp);
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
 	}
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

			me = purple_menu_action_new("Join Game ...",
					PURPLE_CALLBACK(gfire_buddy_menu_joingame_cb),NULL, NULL);

			if (!me) {
				return NULL;
			}
			ret = g_list_append(ret, me);
		}

		me = purple_menu_action_new("Xfire Profile",
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

	purple_request_input(gc, NULL, "Change Xfire nickname", "Leaving empty will clear your current nickname.", purple_connection_get_display_name(gc),
		FALSE, FALSE, NULL, "OK", G_CALLBACK(gfire_change_nick), "Cancel", NULL, account, NULL, NULL, gc);
}


/*
 * Callback function showing the manage games window.
 *
 * @param action	The menu action, passed by the signal connection function
 *
*/
static void gfire_action_manage_games_cb(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *)action->context;
	GtkBuilder *builder = gtk_builder_new();

	if(!gc) {
		purple_debug_error("gfire: gfire_action_manage_games_cb", "GC not set.\n");
		return;
	}

	gfire_reload_lconfig(gc);
	gtk_builder_add_from_file(builder, g_build_filename(purple_user_dir(), "gfire_manage.ui", NULL), NULL);
	if(!builder) {
		purple_debug_error("gfire: gfire_action_manage_games_cb", "Couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));
	GtkWidget *add_path_label = GTK_WIDGET(gtk_builder_get_object(builder, "add_path_label"));
	GtkWidget *add_type_combo_box = GTK_WIDGET(gtk_builder_get_object(builder, "add_type_combo_box"));
	GtkWidget *add_close_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_close_button"));
	GtkWidget *add_add_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_add_button"));
	GtkWidget *edit_games_combo_box = GTK_WIDGET(gtk_builder_get_object(builder, "edit_games_combo_box"));
	GtkWidget *edit_games_list_store = GTK_WIDGET(gtk_builder_get_object(builder, "edit_games_list_store"));
	GtkWidget *edit_close_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_close_button"));
	GtkWidget *edit_apply_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_apply_button"));
	GtkWidget *edit_remove_button = GTK_WIDGET(gtk_builder_get_object(builder, "edit_remove_button"));

	manage_games_callback_args *args;

	args = g_new0(manage_games_callback_args, 1);
	args->gc = gc;
	args->builder = builder;

	g_signal_connect_swapped(add_type_combo_box, "changed", G_CALLBACK(gfire_manage_games_add_update_label_cb), add_path_label);
	g_signal_connect_swapped(add_close_button, "clicked", G_CALLBACK(gtk_widget_destroy), manage_games_window);
	g_signal_connect_swapped(add_add_button, "clicked", G_CALLBACK(gfire_add_game_cb), args);
	g_signal_connect_swapped(edit_games_combo_box, "changed", G_CALLBACK(gfire_manage_games_edit_update_fields_cb), builder);
	g_signal_connect_swapped(edit_close_button, "clicked", G_CALLBACK(gtk_widget_destroy), manage_games_window);
	g_signal_connect_swapped(edit_apply_button, "clicked", G_CALLBACK(gfire_edit_game_cb), args);
	g_signal_connect_swapped(edit_remove_button, "clicked", G_CALLBACK(gfire_remove_game_cb), args);

	xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");

	if(gfire_launch)
	{
		GtkTreeIter iter;
		xmlnode *node_child;

		for(node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
			node_child = xmlnode_get_next_twin(node_child))
		{
			const char *game_name = xmlnode_get_attrib(node_child, "name");

			gtk_list_store_append(GTK_LIST_STORE(edit_games_list_store), &iter);
			gtk_list_store_set(GTK_LIST_STORE(edit_games_list_store), &iter, 0, game_name, -1);
		}
	}

	#if IS_WINDOWS
	gtk_combo_box_remove_text(GTK_COMBO_BOX(add_type_combo_box), 1);
	#endif
	gtk_window_set_keep_above(GTK_WINDOW(manage_games_window), TRUE);
	gtk_widget_show_all(manage_games_window);
}


/*
 * Callback function to add a game by getting the values from the manage games window.
 *
*/
static void gfire_add_game_cb(manage_games_callback_args *args, GtkWidget *button)
{
	PurpleConnection *gc = args->gc;
	GtkBuilder *builder = args->builder;
	
	if(!gc || !builder) {
		purple_debug_error("gfire: gfire_add_game_cb", "GC not set and/or couldn't access interface.\n");
		return;
	}

	GtkWidget *id_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_id_entry"));
	GtkWidget *path_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_path_entry"));
	GtkWidget *connect_entry = GTK_WIDGET(gtk_builder_get_object(builder, "add_connect_entry"));
	GtkWidget *type_combo_box = GTK_WIDGET(gtk_builder_get_object(builder, "add_type_combo_box"));
	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));

	const gchar *id_value = gtk_entry_get_text(GTK_ENTRY(id_entry));
	const gchar *path_value = gtk_entry_get_text(GTK_ENTRY(path_entry));
	const gchar *connect_value = gtk_entry_get_text(GTK_ENTRY(connect_entry));
	gchar *type_value = gtk_combo_box_get_active_text(GTK_COMBO_BOX(type_combo_box));

	if(id_value && path_value && connect_value && type_value)
	{
		char *file;
		char *path = path_value;
		
		if(strcmp(type_value, "Native game") == NULL)
		{
			gboolean separe = separe_path(path, &file);
			if(!separe) {
				purple_debug_error("gfire: gfire_add_game_cb", "Couldn't separe path and file.\n");
				return;
			}
		}
		else {
			file = path_value;
			path = "";
		}

		int game_id = atoi(id_value);
		char *game_name = gfire_game_name(gc, game_id);
		gboolean game_check = gfire_game_playable(gc, game_id);
		
		if(!game_check)
 		{
			xmlnode *gfire_launch_new = xmlnode_new("launchinfo");
			xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
			if(gfire_launch)
			{
				xmlnode *node_child;
				for(node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
					node_child = xmlnode_get_next_twin(node_child)) {
					xmlnode_insert_child(gfire_launch_new, node_child);
				}
			}
			
			char *unix_process = "";
			char *windows_process = "";
			xmlnode *gfire_processes = purple_util_read_xml_from_file("gfire_processes.xml", "gfire_processes.xml");
			xmlnode *node_child;
			
			for(node_child = xmlnode_get_child(gfire_processes, "game"); node_child != NULL;
				node_child = xmlnode_get_next_twin(node_child)) {
					char *game_id_processes = xmlnode_get_attrib(node_child, "id");
					if(game_id == atoi(game_id_processes)) {
						unix_process = xmlnode_get_attrib(node_child, "unix_process");
						windows_process = xmlnode_get_attrib(node_child, "windows_process");
					}
			}
			
			xmlnode *game_node = gfire_manage_game_xml(id_value, game_name, type_value, file, path,
				"", connect_value, "@options@ @gamemod@ @connect@", unix_process, windows_process);
			if(!game_node) {
				purple_debug_error("gfire: gfire_add_game_cb", "Couldn't create the new game node.\n");
			}
			else
			{
				xmlnode_insert_child(gfire_launch_new, game_node);
				char *gfire_launch_new_str = xmlnode_to_formatted_str(gfire_launch_new, NULL);

				gboolean write_xml = purple_util_write_data_to_file("gfire_launch.xml", gfire_launch_new_str, -1);
				if(!write_xml) {
					purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, "Manage games: error",
						"Couldn't add game", "Please try again. An error occured while adding the game.", NULL, NULL);
				}
				else
				{
					gfire_reload_lconfig(gc);
					purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, "Manage games: game added",
						game_name, "The game has been successfully added.", NULL, NULL);
				}
				xmlnode_free(gfire_launch_new);
			}
		}
 		else {
			purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, "Manage games: warning", "Game already added",
				"This game is already added, you can configure it if you want.", NULL, NULL);
		}
	}
	else {
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, "Manage games: error",
			"Couldn't add game", "Please try again. Make sure you fill in all fields.", NULL, NULL);
	}
	gtk_widget_destroy(manage_games_window);
}


/*
 * Callback function editing the game in gfire_launch.xml by getting the new values from
 * the manage games window.
 *
*/
static void gfire_edit_game_cb(manage_games_callback_args *args, GtkWidget *button)
{
	PurpleConnection *gc = args->gc;
	GtkBuilder *builder = args->builder;
	if(!gc || !builder) {
		purple_debug_error("gfire: gfire_edit_game_cb", "GC not set and/or couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));
	GtkWidget *edit_path_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_path_entry"));
	GtkWidget *edit_connect_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_connect_entry"));
	GtkWidget *edit_launch_entry = GTK_WIDGET(gtk_builder_get_object(builder, "edit_launch_entry"));
	GtkWidget *edit_games_combo_box = GTK_WIDGET(gtk_builder_get_object(builder, "edit_games_combo_box"));

	const gchar *edit_path = gtk_entry_get_text(GTK_ENTRY(edit_path_entry));
	const gchar *edit_connect = gtk_entry_get_text(GTK_ENTRY(edit_connect_entry));
	const gchar *edit_launch = gtk_entry_get_text(GTK_ENTRY(edit_launch_entry));
	gchar *selected_item = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_games_combo_box));

	if(edit_path && edit_connect && edit_launch)
	{
		xmlnode *gfire_launch_new = xmlnode_new("launchinfo");
		xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
		if(gfire_launch)
		{
			xmlnode *node_child;
			for(node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
				node_child = xmlnode_get_next_twin(node_child))
			{
				char *game_id = xmlnode_get_attrib(node_child, "id");
				int game_id_int = atoi(game_id);
				char *game_name = gfire_game_name(gc, game_id_int);
				const char *game_type = xmlnode_get_attrib(node_child, "type");
				
				xmlnode *processes_node = xmlnode_get_child(node_child, "processes");
				char *unix_process = xmlnode_get_attrib(processes_node, "unix_process");
				char *windows_process = xmlnode_get_attrib(processes_node, "windows_process");

				if(strcmp(game_name, selected_item) == NULL)
				{
					char *game_path = edit_path;
					char *game_bin;

					if(strcmp(game_type, "Native game") == NULL)
					{
						gboolean separe = separe_path(game_path, &game_bin);
						if(!separe) {
							purple_debug_error("gfire: gfire_add_game_cb", "Couldn't separe path and file.\n");
							return;
						}
					}
					else {
						game_bin = edit_path;
						game_path = "";
					}

					xmlnode *game_node = gfire_manage_game_xml(game_id, game_name, game_type, game_bin, game_path, "", edit_connect, edit_launch, unix_process, windows_process);
					xmlnode_insert_child(gfire_launch_new, game_node);
				}
				else {
					xmlnode_insert_child(gfire_launch_new, node_child);
				}
			}

			char *gfire_launch_new_str = xmlnode_to_formatted_str(gfire_launch_new, NULL);
			gboolean write_xml = purple_util_write_data_to_file("gfire_launch.xml", gfire_launch_new_str, -1);
			if(!write_xml) {
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, "Manage games: error", "Couldn't edit game'",
					"Please try again. An error occured while editing the game.", NULL, NULL);
			}
			else
			{
				gfire_reload_lconfig(gc);
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, "Manage games: game edited", "Game edited",
					"The game has been successfully edited.", NULL, NULL);
			}
			xmlnode_free(gfire_launch_new);
		}
	}
	else {
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, "Manage games: error",
			"Couldn't edit game", "Please try again. Make sure you fill in all fields.", NULL, NULL);
	}
	gtk_widget_destroy(manage_games_window);
}


/*
 * Callback function updating the label in the manage games window, depending on the game type.
 *
*/
static void gfire_manage_games_add_update_label_cb(GtkWidget *add_path_label, GtkWidget *add_type_combo_box)
{
	gchar *selected_item = gtk_combo_box_get_active_text(GTK_COMBO_BOX(add_type_combo_box));
	if(strcmp(selected_item, "Native game") == NULL) {
		gtk_label_set_text(GTK_LABEL(add_path_label), "Game path:");
	}
	else {
		gtk_label_set_text(GTK_LABEL(add_path_label), "Game command:");
	}
}


/*
 * Callback function getting and showing the current values from gfire_launch.xml
 * for editing the selected game in te manage games window.
 *
*/
static void gfire_manage_games_edit_update_fields_cb(GtkBuilder *builder, GtkWidget *edit_games_combo_box)
{
	if(!builder) {
		purple_debug_error("gfire: gfire_manage_games_edit_update_fields_cb", "Couldn't access interface.\n");
		return;
	}

	GtkWidget *edit_path_label = GTK_WIDGET(gtk_builder_get_object(builder, "edit_path_label"));
	GtkWidget *edit_path = GTK_WIDGET(gtk_builder_get_object(builder, "edit_path_entry"));
	GtkWidget *edit_connect = GTK_WIDGET(gtk_builder_get_object(builder, "edit_connect_entry"));
	GtkWidget *edit_launch = GTK_WIDGET(gtk_builder_get_object(builder, "edit_launch_entry"));
	gchar *selected_item = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_games_combo_box));

 	xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
	if(gfire_launch)
	{
		xmlnode *node_child;
		for(node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
			node_child = xmlnode_get_next_twin(node_child))
		{
			const char *game = xmlnode_get_attrib(node_child, "name");
			gchar *game_name = (gchar *)game;
			if(strcmp(selected_item, game_name) == NULL)
			{
				xmlnode *command_node = xmlnode_get_child(node_child, "command");
				xmlnode *dir_node = xmlnode_get_child(command_node, "dir");
				xmlnode *bin_node = xmlnode_get_child(command_node, "bin");
				xmlnode *connect_node = xmlnode_get_child(command_node, "connect");
				xmlnode *launch_node = xmlnode_get_child(command_node, "launch");

				const char *path_dir = xmlnode_get_data(dir_node);
				const char *path_bin = xmlnode_get_data(bin_node);
				const char *game_type = xmlnode_get_attrib(node_child, "type");

				gchar *path;
				gchar *connect = xmlnode_get_data(connect_node);
				gchar *launch = xmlnode_get_data(launch_node);

				if(strcmp(game_type, "Native game") == NULL) {
					gtk_label_set_text(GTK_LABEL(edit_path_label), "Game path:");
					path = g_strdup_printf("%s/%s", path_dir, path_bin);
				}
				else {
					gtk_label_set_text(GTK_LABEL(edit_path_label), "Game command:");
					path = path_bin;
				}

				gtk_entry_set_text(GTK_ENTRY(edit_path), path);
				gtk_entry_set_text(GTK_ENTRY(edit_launch), launch);
				gtk_entry_set_text(GTK_ENTRY(edit_connect), connect);

				xmlnode_free(command_node);
			}
		}
	}
}


/*
 * Callback function removing the selected game from gfire_launch.xml in the manage games window.
 *
*/
static void gfire_remove_game_cb(manage_games_callback_args *args, GtkWidget *button)
{
	PurpleConnection *gc = args->gc;
	GtkBuilder *builder = args->builder;
	if(!gc || !builder) {
		purple_debug_error("gfire", "GC not set and/or couldn't build interface.\n");
		return;
	}

	GtkWidget *manage_games_window = GTK_WIDGET(gtk_builder_get_object(builder, "manage_games_window"));
	GtkWidget *edit_games_combo_box = GTK_WIDGET(gtk_builder_get_object(builder, "edit_games_combo_box"));

	gchar *selected_game = gtk_combo_box_get_active_text(GTK_COMBO_BOX(edit_games_combo_box));
	if(selected_game)
	{
		xmlnode *gfire_launch_new = xmlnode_new("launchinfo");
		const xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
		if(gfire_launch)
		{
			xmlnode *node_child;
			for(node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
				node_child = xmlnode_get_next_twin(node_child))
			{
				const char *game_name = xmlnode_get_attrib(node_child, "name");
				if(strcasecmp(game_name, selected_game) != NULL) {
					xmlnode_insert_child(gfire_launch_new, node_child);
				}
			}

			char *gfire_launch_new_str = xmlnode_to_formatted_str(gfire_launch_new, NULL);
			purple_debug_info("gfire", "XML: %s", gfire_launch_new_str);
			gboolean write_xml = purple_util_write_data_to_file("gfire_launch.xml", gfire_launch_new_str, -1);
			if(!write_xml) {
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, "Manage games: error",
					"Couldn't remove game'", "Please try again. An error occured while removing the game.", NULL, NULL);
			}
			else {
				gfire_reload_lconfig(gc);
				purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, "Manage games: game removed",
					"Game removed", "The game has been successfully removed.", NULL, NULL);
			}
		}
	}
	else {
		purple_notify_message(NULL, PURPLE_NOTIFY_MSG_ERROR, "Manage games: error",
			"Couldn't remove game", "Please try again. Make sure you select a game to remove.", NULL, NULL);
	}
	gtk_widget_destroy(manage_games_window);
}


/*
 * Reloads the launch config file, the difference with gfire_action_reload_gconfig_cb()
 * is that this function doesn't say what happened to the user.
 *
 * @param gc	The purple connection
 *
*/
static void gfire_reload_lconfig(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	if(gc == NULL || (gfire = (gfire_data *)gc->proto_data) == NULL) {
		purple_debug_error("gfire: gfire_reload_lconfig", "GC not set or found.\n");
		return;
	}

	if(gfire->xml_launch_info != NULL) {
		xmlnode_free(gfire->xml_launch_info);
	}

	gfire->xml_launch_info = NULL;
	gfire_parse_launchinfo_file(gc, g_build_filename(purple_user_dir(), "gfire_launch.xml", NULL));

	if(gfire->xml_launch_info == NULL) {
		purple_debug_error("gfire: gfire_reload_lconfig", "Couldn't reload launch config.\n");
	}
	else {
		purple_debug_info("gfire: gfire_reload_lconfig", "Launch config successfully reloaded.\n");
	}
}


/*
 * Creates a new xmlnode containing the game information, the returned node must be inserted in the main launchinfo node.
 *
 * @param game_id			The game ID
 * @param game_name			The game name
 * @param game_type			The game type (Native or Non-native)
 * @param game_bin			The game binary
 * @param game_dir			The game directory (to the binary)
 * @param game_connect		The game connection options
 * @param game_launch		The game launch structure
 * @param unix_process		The game process for Unix
 * @param windows_process	The game process for Windows
 *
 * @return the new xmlnode.
 *
*/
xmlnode *gfire_manage_game_xml(char *game_id, char *game_name, char *game_type, char *game_bin,
	char *game_dir, char *game_mod, char *game_connect, char *game_launch, char *unix_process, char *windows_process)
{
	xmlnode *game_node = xmlnode_new("game");
	xmlnode_set_attrib(game_node, "id", game_id);
	xmlnode_set_attrib(game_node, "name", game_name);
	xmlnode_set_attrib(game_node, "type", game_type);
	xmlnode *xqf_node = xmlnode_new_child(game_node, "xqf");
	xmlnode_set_attrib(xqf_node, "name", "");
	xmlnode *processes_node = xmlnode_new_child(game_node, "processes");
	xmlnode_set_attrib(processes_node, "unix_process", unix_process);
	xmlnode_set_attrib(processes_node, "windows_process", windows_process);
	xmlnode *command_node = xmlnode_new_child(game_node, "command");
	xmlnode *bin_node = xmlnode_new_child(command_node, "bin");
	xmlnode_insert_data(bin_node, game_bin, -1);
	xmlnode *dir_node = xmlnode_new_child(command_node, "dir");
	xmlnode_insert_data(dir_node, game_dir, -1);
	xmlnode *gamemod_node = xmlnode_new_child(command_node, "gamemod");
	xmlnode_insert_data(gamemod_node, game_mod, -1);
	xmlnode *connect_node = xmlnode_new_child(command_node, "connect");
	xmlnode_insert_data(connect_node, game_connect, -1);
	xmlnode *launch_node = xmlnode_new_child(command_node, "launch");
	xmlnode_insert_data(launch_node, game_launch, -1);

	return game_node;
}


/*
 * Extracts the path and file from the full path to a file.
 *
 * @param path		The full path to the file
 * @param file		The variable (pointer) to store the file
 *
 * @return TRUE if the path was successfully extracted, FALSE if an error occured.
 *
*/
gboolean separe_path(char *path, char **file)
{
	char *separator = "/";
	char *str;
	#if IS_WINDOWS
	separator = "\\";
	#endif

	if(!path) return FALSE;

 	str = strrchr(path, *separator);
	if(!str) return FALSE;

	*str = '\0';
	*file = str + 1;
	return TRUE;
}


/*
 * Detects running games by checking running processes.
 *
 * @param gc	The purple connection
 *
 * @return TRUE (this function always returns TRUE).
 *
*/
int gfire_detect_running_games_cb(PurpleConnection *gc)
{
	gfire_data *gfire = NULL;
	if(!gc || !(gfire = (gfire_data *)gc->proto_data)) {
		purple_debug_error("gfire: gfire_detect_running_games_cb", "GC not set.\n");
		return;
	}

	gboolean norm;
	norm = purple_account_get_bool(purple_connection_get_account(gc), "ingamedetectionnorm", TRUE);
	if (!norm) return;

	xmlnode *gfire_launch = purple_util_read_xml_from_file("gfire_launch.xml", "gfire_launch.xml");
	if(gfire_launch)
	{
		xmlnode *node_child;
		for(node_child = xmlnode_get_child(gfire_launch, "game"); node_child != NULL;
			node_child = xmlnode_get_next_twin(node_child))
		{
			const char *game_id = xmlnode_get_attrib(node_child, "id");
			const char *game_unix_process = xmlnode_get_attrib(node_child, "unix_process");
			const char *game_windows_process = xmlnode_get_attrib(node_child, "windows_process");
			char *game_unix_process_tmp = game_unix_process;
			char *game_windows_process_tmp = game_windows_process;

			char *delim = ";";
			char *token;

			gboolean process_running = FALSE;
			#if IS_WINDOWS
			token = strtok(game_windows_process_tmp, delim);
			while(token != NULL) {
				process_running = check_process(token);
				token = strtok(NULL, delim);
			}
			#else
			token = strtok(game_unix_process_tmp, delim);
			while(token != NULL) {
				process_running = check_process(token);
				token = strtok(NULL, delim);
			}
			if(!process_running) {
				token = strtok(game_windows_process_tmp, delim);
				while(token != NULL) {
					process_running = check_process(token);
					token = strtok(NULL, delim);
				}
			}
			#endif

			int len = 0;
			int game_running_id = gfire->gameid;
			int game_id_int = atoi(game_id);
			char *game_name = gfire_game_name(gc, game_id_int);
			gboolean game_running = gfire->game_running;
			if(process_running)
			{
				if(!game_running)
				{
					gboolean norm = purple_account_get_bool(purple_connection_get_account(gc), "ingamenotificationnorm", FALSE);
					purple_debug_info("gfire: gfire_detect_running_games_cb", "%s is running. Telling Xfire ingame status.\n", game_name);
					if(norm) purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, "Ingame status", game_name, "Your status has been changed.", NULL, NULL);

					len = gfire_join_game_create(gc, game_id_int, 0, NULL);
					if(len) gfire_send(gc, gfire->buff_out, len);
		 			gfire->game_running = TRUE;
					gfire->gameid = game_id_int;
				}
			}
			else
			{
				if(game_running && game_running_id == game_id_int)
				{
					purple_debug(PURPLE_DEBUG_MISC, "gfire: gfire_detect_running_games_cb", "Game not running anymore, sending out of game status.\n");
					gfire->gameid = 0;
		 			len = gfire_join_game_create(gc, 0, 0, NULL);
		 			if(len) gfire_send(gc, gfire->buff_out, len);
					gfire->game_running = FALSE;
				}
			}
		}
	}
	return TRUE;
}

/*
 * Checks if a process is running.
 *
 * @param process	The process name
 *
 * @return TRUE if the process is running, FALSE if not or if an error occured.
 *
*/
gboolean check_process(char *process)
{
	#if IS_WINDOWS
	return FALSE;
	#else
	char command[256];

	strlwr(process);
	sprintf(command, "ps -ef | grep -i %s | grep -v grep", process);

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

	if(strcmp(buf, "") == NULL) return FALSE;
	else return TRUE;
	#endif
}

/*
 * Makes a string lowercase.
 *
 * @param string	String to make lowercase
 *
*/
void strlwr(char string[])
{
	int i = 0;

	while(string[i]) {
		string[i] = tolower(string[i]);
		i++;
	}

   return;
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
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_ERROR, "Gfire XML Reload", "Reloading gfire_launch.xml", "Operation failed. File not found or content was incorrect.", NULL, NULL);
	} else {
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, "Gfire XML Reload", "Reloading gfire_launch.xml","Reloading was successful.", NULL, NULL);

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
		purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_ERROR, "Gfire XML Reload", "Reloading gfire_games.xml", "Operation failed. File not found or content was incorrect.", NULL, NULL);
	} else {
purple_notify_message((void *)_gfire_plugin, PURPLE_NOTIFY_MSG_INFO, "Gfire XML Reload", "Reloading gfire_games.xml","Reloading was successful.", NULL, NULL);
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

	if(strcmp(gfire_game_name(gc, 100), "100")) {
		msg = g_strdup_printf("Gfire Version:\t\t%s\nGame List Version:\t%s", GFIRE_VERSION, gfire_game_name(gc, 100));
	}
	else {
		msg = g_strdup_printf("Gfire Version: %s", GFIRE_VERSION);
	}

	purple_request_action(gc, "About Gfire", "Xfire Plugin for Pidgin", msg, PURPLE_DEFAULT_ACTION_NONE,
		purple_connection_get_account(gc), NULL, NULL, gc, 3, "Close", NULL,
		"Website", G_CALLBACK(gfire_action_website_cb),
		"Wiki", G_CALLBACK(gfire_action_wiki_cb));
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

static GList *gfire_actions(PurplePlugin *plugin, gpointer context)
{
	GList *m = NULL;
	PurplePluginAction *act;

	act = purple_plugin_action_new("Change Nickname",
			gfire_action_nick_change_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new("My Profile Page",
			gfire_action_profile_page_cb);
	m = g_list_append(m, act);
	m = g_list_append(m, NULL);
	act = purple_plugin_action_new("Reload Launch Config",
			gfire_action_reload_lconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new("Reload Game ID List",
			gfire_action_reload_gconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new("Get Game ID List",
			gfire_action_get_gconfig_cb);
	m = g_list_append(m, act);
	act = purple_plugin_action_new("Manage Games",
			gfire_action_manage_games_cb);
	m = g_list_append(m, act);
	m = g_list_append(m, NULL);
	act = purple_plugin_action_new("About",
			gfire_action_about_cb);
	m = g_list_append(m, act);
	return m;
}


 /**
  * Joins a game in progress with buddy. spawns process, puts us in game with xfire
  * and adds a gsource watch on the child process so when it exits we can take
  * ourselves out of game.
  *
  * @param gc			pointer to our PurpleConnection for sending network traffic.
  * @param sip			server ip (quad dotted notation) of where the game is
  * @param sport		server port of where the game is located
  * @param game			the xfire game ID to join (looked up in launch options)
  *
  * asking to join a game that is not playable (not configured through launch options
  * or any other reason for an unplayable game is not defined.  You must check
  * the game playability with gfire_game_playable()
  *
  * @see gfire_game_playable
 */

 void gfire_join_game(PurpleConnection *gc, const gchar *sip, int sport, int game)
 {
 	/* test stuff */
 	int len = 0;
 	gfire_linfo *linfo =NULL;
 	gchar *command = NULL;
 	char **argvp;
 	int argcp = 0;
 	gboolean worked = FALSE;
 	GPid pid;
 	GError *gerr;
	gfire_data *gfire = NULL;
	const gchar nullip[4] = {0x00, 0x00, 0x00, 0x00};
	gboolean sworked = FALSE;
	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

 	linfo = gfire_linfo_get(gc, game);
 	if (!linfo) {
 		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "Launch info struct not defined!\n");
 		return;
 	}
 	if (!sip) sip = (char *)&nullip;
 	command = gfire_linfo_get_cmd(linfo, (guint8 *)sip, sport, NULL);
	gerr = 0;
 	worked=sworked=g_shell_parse_argv(command, &argcp, &argvp, &gerr);
 	if (worked) {
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "Attempting to join game %d, on server %d.%d.%d.%d , at port %d\n",
				game, NNA(sip, sip[3]), NNA(sip, sip[2]), NNA(sip, sip[1]), NNA(sip, sip[0]), sport);
 		purple_debug(PURPLE_DEBUG_MISC, "gfire", "launch xml command parsed to:\n");
 		purple_debug(PURPLE_DEBUG_MISC, "gfire", "%s\n", NN(command));
 		gerr = 0;
		worked = g_spawn_async(linfo->c_wdir, argvp, NULL,
 						(GSpawnFlags)G_SPAWN_DO_NOT_REAP_CHILD,
 						NULL, NULL, &pid, &gerr);
	}
 	if (!worked) {
 		/* something went wrong! */
 		purple_debug(PURPLE_DEBUG_ERROR, "gfire", "Launch failed, message: %s\n", NN(gerr->message));
 		g_free(command);
 		if (sworked) {
			g_strfreev(argvp);
		}
 		g_error_free (gerr);
		return;
 	}

 	/* program seems to be running! */
 	gfire->gameid = game;
 	len = gfire_join_game_create(gc, game, sport, sip);
 	if (len) {
		gfire_send(gc, gfire->buff_out, len);
		purple_debug(PURPLE_DEBUG_MISC, "gfire", "(game join): telling xfire our game info\n");
	}
 	/* need to watch pid so we can get out of game when game closes */
 	g_child_watch_add(pid, (GChildWatchFunc)gfire_game_watch_cb, (gpointer *)gc);
 	g_free(command);
 	g_strfreev(argvp);
//	g_free(sip);

}


 /**
  *  GSource watch pid callback for glib.  This function waits for a join game
  *  PID to exit.  This tells gfire to remove user out of game when the user
  *  quits the game. This function is not called directly but by glib.  The
  *  function prototype is set by glib.  This function does not return anything
  *
  *  @param      pid     Integer PID of the process to watch (wait for to die)
  *  @param      status  status of exiting pid
  *  @param      data    Pointer to data passed in by setup function
  *
 */
void gfire_game_watch_cb(GPid pid, int status, gpointer *data)
{
 	/* were now out of game, clean up pid send network message */

 	int len = 0;
	PurpleConnection *gc = (PurpleConnection *)data;
	gfire_data *gfire = NULL;

	purple_debug(PURPLE_DEBUG_MISC, "gfire", "(game_watch_cb): Child has exited, reaping pid.\n");
 	g_spawn_close_pid(pid);
	/* we could have been disconnected */
	if ( PURPLE_CONNECTION_IS_VALID(gc) && PURPLE_CONNECTION_IS_CONNECTED(gc) ) {
		if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return;

		len = gfire_join_game_create(gc, 0, 0, NULL);
		if (len) gfire_send(gc, gfire->buff_out, len);
		gfire->gameid = 0;
	}
}


/**
 *	g_timeout_add callback function to watch for $HOME/.purple/ingame.tmp
 *	file.  If this file is found we send join_game data to xfire network
 *	if we are in game and then the file is no longer present then take
 *	client out of game.  Only do this when in game status was met using
 *	ingame.tmp file.  The ingame.tmp file is a copied LaunchInfo.txt file
 *	from XQF.  The user must create a script to copy this file and then
 *	remove it once the game is quit.  We match the contents of the XQF
 *	file against our launch.xml.  Find the game and then send the network
 *	message.  This function is not called directy. It is called by glib.
 *
 *	@param		gc		pointer to purple connection struct for this connection
 *
 *	@return				returns TRUE always, except if gc connection state
 *						is set to disconnected.
**/
int gfire_check_xqf_cb(PurpleConnection *gc)
{

 	static char *filename = NULL;
 	static gboolean found = FALSE;
 	gfire_xqf_linfo *xqfs = NULL;
	int len = 0;
 	int game = 0;
	gfire_data *gfire = NULL;
	char *ipbin = NULL;
	char *game_name = NULL;

	if (!gc || !(gfire = (gfire_data *)gc->proto_data)) return FALSE;

 	if (purple_connection_get_state(gc) != PURPLE_DISCONNECTED) {
 		if ((0 != gfire->gameid) && !found) return TRUE;
 		if (!filename) {
			filename = g_build_filename(purple_user_dir(), GFIRE_XQF_FILENAME, NULL);
 		}
 		if (g_file_test(filename, G_FILE_TEST_EXISTS)){
 			if (found) return TRUE;
 			/* file was found, but not previously */
 			found = TRUE;
 			xqfs = gfire_linfo_parse_xqf(filename);
 			if (!xqfs) return TRUE;
 			game = gfire_xqf_search(gc, xqfs);
 			if (!game) {
 				purple_debug(PURPLE_DEBUG_WARNING, "gfire", "(XQF cb): parsed ingame.tmp. No game match found.\n");
 				gfire_xqf_linfo_free(xqfs);
 				return TRUE;
 			}

 			game_name = gfire_game_name(gc, game);

 			gboolean norm = purple_account_get_bool(purple_connection_get_account(gc), "ingamenotificationnorm", FALSE);
			if (norm) purple_notify_message(NULL, PURPLE_NOTIFY_MSG_INFO, "Ingame status",
			game_name, "Your status has been changed.", NULL, NULL);

			/* turn ip string into ip binary string in xfire format */
			ipbin = gfire_ipstr_to_bin(xqfs->ip);
 			len = gfire_join_game_create(gc, game, xqfs->port, ipbin);
 			if (len) gfire_send(gc, gfire->buff_out, len);
			g_free(ipbin);
 			gfire->gameid = game;
 			purple_debug(PURPLE_DEBUG_MISC, "gfire", "(XQF cb): Detected game join (%d) at (%s:%d)\n", game,
						NN(xqfs->ip), xqfs->port );
 			gfire_xqf_linfo_free(xqfs);
 		} else {
 			if (!found) return TRUE;
 			if (gfire->gameid) {
 				purple_debug(PURPLE_DEBUG_MISC, "gfire", "(XQF cb): Status file removed, sending out of game msg\n");
 				gfire->gameid = 0;
 				len = gfire_join_game_create(gc, 0, 0, NULL);
 				if (len) gfire_send(gc, gfire->buff_out, len);
 			}
 			found = FALSE;
 		}
 		return TRUE;
 	}
 	purple_debug(PURPLE_DEBUG_ERROR, "gfire", "(XQF cb): Still running but GC says not connected!\n");
 	return FALSE;
}



char *gfire_escape_html(const char *html)
{
	char *escaped = NULL;

	if (html != NULL) {
		const char *c = html;
		GString *ret = g_string_new("");
		while (*c) {
			if (!strncmp(c, "&", 1)) {
				ret = g_string_append(ret, "&amp;");
				c += 1;
			} else if (!strncmp(c, "<", 1)) {
				ret = g_string_append(ret, "&lt;");
				c += 1;
			} else if (!strncmp(c, ">", 1)) {
				ret = g_string_append(ret, "&gt;");
				c += 1;
			} else if (!strncmp(c, "\"", 1)) {
				ret = g_string_append(ret, "&quot;");
				c += 1;
			} else if (!strncmp(c, "'", 1)) {
				ret = g_string_append(ret, "&apos;");
				c += 1;
			} else if (!strncmp(c, "\n", 1)) {
				ret = g_string_append(ret, "<br>");
				c += 1;
			} else {
				ret = g_string_append_c(ret, c[0]);
				c++;
			}
		}

		escaped = ret->str;
		g_string_free(ret, FALSE);
	}
	return escaped;
}



/*
 * Plugin Initialization section
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
	NULL
};


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,		/**< type           */
	NULL,						/**< ui_requirement */
	0,							/**< flags          */
	NULL,						/**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,	/**< priority       */
	"prpl-xfire",				/**< id             */
	"Xfire",					/**< name           */
	GFIRE_VERSION,				/**< version        */
	"Xfire Protocol Plugin",	/**  summary        */
	"Xfire Protocol Plugin",	/**  description    */
	NULL,						/**< author         */
	GFIRE_WEBSITE,				/**< homepage       */
	NULL,						/**< load           */
	NULL,						/**< unload         */
	NULL,						/**< destroy        */
	NULL,						/**< ui_info        */
	&prpl_info,					/**< extra_info     */
	NULL,						/**< prefs_info     */
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

	option = purple_account_option_string_new("Server", "server",XFIRE_SERVER);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_int_new("Port", "port", XFIRE_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_int_new("Version", "version", XFIRE_PROTO_VERSION);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new("Don't delete buddies from server",
						"buddynorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new("Buddies can see if I'm typing",
						"typenorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new("Auto detect for ingame status",
						"ingamedetectionnorm", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);

	option = purple_account_option_bool_new("Notifiy me when my status is ingame",
						"ingamenotificationnorm", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,option);


	_gfire_plugin = plugin;
}

PURPLE_INIT_PLUGIN(gfire, _init_plugin, info);



