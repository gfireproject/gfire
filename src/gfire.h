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

#ifndef _GFIRE_H

#define _GFIRE_H

#ifdef HAVE_CONFIG_H
	#include <gfire_config.h>
#endif

#define PURPLE_PLUGINS

#include <stddef.h>
#include <stdlib.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <netdb.h>
#include <ifaddrs.h>

#include <libintl.h>
#include <locale.h>

#define _(string) gettext (string)
#define gettext_noop(string) string
#define N_(string) gettext (string)

#ifdef _WIN32
	#define IS_WINDOWS
#else
	#define IS_NOT_WINDOWS
	#include <gtk/gtk.h>
	#include <gio/gio.h>
	#include <errno.h>
	// Required for teamspeak server detection
	#include <sys/socket.h>
	#include <sys/un.h>
#endif /* _WIN32 */

#ifndef G_GNUC_NULL_TERMINATED
	#if __GNUC__ >= 4
		#define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
	#else
		#define G_GNUC_NULL_TERMINATED
	#endif /* __GNUC__ >= 4 */
#endif /* G_GNUC_NULL_TERMINATED */

#include "util.h"
#include "server.h"
#include "notify.h"
#include "plugin.h"
#include "account.h"
#include "accountopt.h"
#include "blist.h"
#include "conversation.h"
#include "debug.h"
#include "prpl.h" 
#include "proxy.h"
#include "util.h"  
#include "version.h" 
#include "request.h"

#include "xmlnode.h"
#include "gf_debug.h"

#define GFIRE_WEBSITE "http://gfireproject.org"
#define GFIRE_WIKI "http://my-trac.assembla.com/gfire/wiki"
#define GFIRE_XQF_FILENAME "ingame.tmp"
#define GFIRE_DEFAULT_GROUP_NAME "Xfire"
#define GFIRE_CLAN_GROUP_NAME "Clan"
#define GFIRE_CLAN_GROUP_FORMATTING "%s [%s]" // long name, short name
#define GFIRE_VERSION "0.9.0"
#define GFIRE_GAMES_XML_URL "http://gfireproject.org/files/gfire_games.xml"
#define XFIRE_HEADER_LEN 5
#define XFIRE_USERID_LEN 4
#define XFIRE_CLANID_LEN 4
#define XFIRE_SID_LEN 16
#define XFIRE_GAMEID_LEN 4
#define XFIRE_GAMEPORT_LEN 4
#define XFIRE_GAMEIP_LEN 4
#define XFIRE_CHATID_LEN 21
#define XFIRE_SERVER "cs.xfire.com"
#define XFIRE_PORT 25999
#define XFIRE_PROTO_VERSION 109
#define XFIRE_CONNECT_STEPS 3
#define XFIRE_SID_OFFLINE_STR "00000000000000000000000000000000"
#define XFIRE_KEEPALIVE_TIME 300  // see gfire_keep_alive for more info
#define XFIRE_PROFILE_URL "http://www.xfire.com/profile/"
#define XFIRE_XML_INFO_URL "http://www.xfire.com/xml/%s/%s/" // username, info-type
#define XFIRE_AVATAR_URL "http://screenshot.xfire.com/avatar/%s.jpg?%u" // username, revision number
#define XFIRE_GALLERY_AVATAR_URL "http://media.xfire.com/xfire/xf/images/avatars/gallery/default/%03u.gif" // avatar id
#define XFIRE_SEND_TYPING_TIMEOUT 10

typedef struct _gfire_data	gfire_data;
typedef struct _gfire_buddy	gfire_buddy;
typedef struct _gfire_im	gfire_im;
typedef struct _gfire_clan	gfire_clan;
typedef struct _gfire_c_msg	gfire_chat_msg;
typedef struct _manage_games_callback_args manage_games_callback_args;
typedef struct _get_info_callback_args get_info_callback_args;

struct _gfire_data { 
	int fd; 
	int chat;
	gchar *email;
	guint16 bytes_read;
	guint8 *buff_out;
	guint8 *buff_in;
	GList *buddies;
	GList *chats;				/* glist of _gfire_chat structs */
	gfire_im *im;				/* im struct, filled when we have an im to proccess */
	gboolean away;
	xmlnode *xml_games_list;
	xmlnode *xml_launch_info;
	gulong last_packet;			/* time (in seconds) of our last packet */
	guint8 *userid;				/* our userid on the xfire network */
	guint8 *sid;				/* our session id for this connection */
	gchar *alias;				/* our current server alias */
	guint32 gameid;				/* our current game id */
	guint32 voipid;				/* our current voip id */
	guint32	voipport;			/* our current voip port */
	guint8 *voipip;				/* our current voip ip (char[4], each byte is an octet) */
	guint xqf_source;			/* g_timeout_add source number for xqf callback */
	guint det_source;			/* g_timeout_add source number for game detection callback */
	gboolean game_running;		/* bool to know if a game has already been detected */
	gboolean blist_loaded;		/* TRUE == buddy list is loaded, we need this for the clan list */
	GtkBuilder *server_browser;
	GList *server_list;
	GMutex *server_mutex;		/* mutex for writing the found server */
	gchar *server_ip;			/* server ip */
	int server_port;			/* server port */
	GList *clans;
};

struct _gfire_buddy {
	gboolean away;			/* TRUE == buddy is away */
	gchar *away_msg;		/* pointer to away message, null if not away */
	guint32	im;				/* im index ++'d on each im reception and and send */
	gchar *name;			/* name (xfire login id) */
	gchar *alias;			/* nick (xfire alias) */
	guint8 *userid;			/* xfire user id binary */
	gchar *uid_str;			/* xfire user id (string format hex) 8 chars + 1 0x00 */
	guint8 *sid;			/* sid binary form, length = XFIRE_SID_LEN */
	gchar *sid_str;			/* sid string representation */
	guint32 gameid;			/* int game id */
	guint32	gameport;		/* int game port */
	guint8 *gameip;			/* char[4] game port, each byte is an octet */
	guint32 voipid;			/* int voip id */
	guint32	voipport;		/* int voip port */
	guint8 *voipip;			/* char[4] voip ip, each byte is an octet */
	int chatperm;			/* group chat permissions (only used for group chat members)*/
	guint32 clanid;			/* clanid (only used for group chat members)*/
	gboolean friend;		/* TRUE == buddy is in friendslist */
	gboolean clan;			/* TRUE == buddy is in clanlist */
	gboolean groupchat;		/* TRUE == buddy is in groupchat */
	guint32 avatarnumber;	/* xfire avatar revision number */
	guint32 avatartype;		/* xfire avatar type */
};

struct _gfire_im {
	guint32	type;		/* msgtype 0 = im, 1 = ack packet, 2 = p2p info, 3 = typing info */
	guint8 peer;		/* peermsg from packet */
	guint32 index;		/* im index for this conversation */
	gchar *sid_str;		/* sid (string format) of buddy that this im belongs to */
	gchar *im_str;		/* im text */
};

struct _gfire_c_msg {
	guint8 *chat_id;	/* xfire chat ID of group chat */
	guint8 *uid;		/* userid of user posting the message */
	gchar *im_str;		/* im text */
	gfire_buddy *b;		/* for users joining the chat */
};

struct _gfire_clan {
	guint32 clanid;
	gchar *clanLongName;
	gchar *clanShortName;
	PurpleGroup *group;
};


struct _manage_games_callback_args {
	PurpleConnection *gc;
	GtkBuilder *builder;
};

struct _get_info_callback_args {
	PurpleConnection *gc;
	PurpleNotifyUserInfo *user_info;
	gfire_buddy *gf_buddy;
};

/* gfire_find_buddy_in_list MODES */
#define GFFB_NAME		0	/* by name, pass pointer to string */
#define GFFB_ALIAS		1	/* by alias, pass pointer to string */
#define GFFB_USERID		2	/* by userid, pass pointer to string ver of uid */
#define GFFB_UIDBIN		4	/* by userid, pass binary string of userid */
#define GFFB_SIDS		8	/* by sid (as string) pass pointer of string */
#define GFFB_SIDBIN 	16	/* by sid (binary data) pass pointer */

/* gfire_update_buddy_status TYPES */
#define GFIRE_STATUS_ONLINE		0	/* set buddies online / offline */
#define	GFIRE_STATUS_GAME		1	/* update game information */
#define	GFIRE_STATUS_AWAY		2	/* update away status */

void gfire_close(PurpleConnection *gc);
GList *gfire_find_buddy_in_list( GList *blist, gpointer *data, int mode );
void gfire_new_buddy(PurpleConnection *gc, gchar *alias, gchar *name, gboolean friend, guint32 clanid);
void gfire_new_buddies(PurpleConnection *gc);
void gfire_check_for_left_clan_members(PurpleConnection *gc, guint32 clanid);
void gfire_leave_clan(PurpleConnection *gc, guint32 clanid);
void gfire_add_clan(PurpleConnection *gc, gfire_clan *newClan);
void gfire_handle_im(PurpleConnection *gc);
void gfire_update_buddy_status(PurpleConnection *gc, GList *buddies, int status);
void gfire_buddy_add_authorize_cb(void *data);
void gfire_buddy_add_deny_cb(void *data);
int gfire_check_xqf_cb(PurpleConnection *gc);
void gfire_avatar_download_cb( PurpleUtilFetchUrlData *url_data, gpointer data, const char *buf, gsize len, const gchar *error_message);
char *gfire_escape_color_codes(char *string);

static void gfire_detect_game_server(PurpleConnection *gc);
#endif /* _GFIRE_H */
