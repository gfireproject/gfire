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

#ifndef _GFIRE_H
#define _GFIRE_H

// Required prototypes
typedef struct _gfire_data gfire_data;

#include "gf_base.h"

#include "gf_buddies.h"
#include "gf_chat.h"
#include "gf_games.h"

/* we only include this on win32 builds */
#  ifdef _WIN32
#    include "internal.h"
#  endif /* _WIN32 */

// gfire_find_buddy modes
typedef enum _gfire_find_buddy_mode
{
	GFFB_NAME = 0,	// by name, pass pointer to string
	GFFB_ALIAS,		// by alias, pass pointer to string
	GFFB_USERID,	// by userid, pass pointer to uid
	GFFB_SID		// by sid, pass pointer to sid array
} gfire_find_buddy_mode;

typedef enum _gfire_find_chat_mode
{
	GFFC_CID = 0,	// by chat ID
	GFFC_PURPLEID	// by purple ID
} gfire_find_chat_mode;

typedef struct _gfire_chat_msg
{
	guint8 *chat_id;	/* xfire chat ID of group chat */
	guint32 uid;		/* userid of user posting the message */
	gchar *im_str;		/* im text */
	gfire_buddy *b;		/* for users joining the chat */
} gfire_chat_msg;

struct _gfire_data
{
	// Networking
	int fd;
	guint8 *buff_in;
	guint16 bytes_read;
	gulong last_packet;			/* time (in seconds) of our last packet */
	gulong last_response;		// time (in seconds) of the last packet from server
	PurpleConnection *gc;

	// Xfire session
	guint32 userid;				/* our userid on the xfire network */
	guint8 *sid;				/* our session id for this connection */
	gchar *alias;				/* our current server alias */

	// Buddies
	GList *buddies;
	GList *clans;

	guint32 chat;
	gchar *email;

	GList *chats;				/* glist of _gfire_chat structs */


	// Detected programs
	gfire_game_data game_data;
	gfire_game_data voip_data;

	// Timer callbacks
	guint xqf_source;			/* g_timeout_add source number for xqf callback */
	guint det_source;			/* g_timeout_add source number for game detection callback */

#ifdef HAVE_GTK
	// Server Browser
	GtkBuilder *server_browser;
#endif // HAVE_GTK

	// Server Detection
	GList *server_list;
	gboolean server_changed;	
	GMutex *server_mutex;		/* mutex for writing found server */
};

typedef struct _invitation_callback_args
{
	gfire_data *gfire;
	gchar *name;
} invitation_callback_args;

typedef struct _get_info_callback_args
{
	gfire_data *gfire;
	PurpleNotifyUserInfo *user_info;
	gfire_buddy *gf_buddy;
} get_info_callback_args;


// Creation and freeing
gfire_data *gfire_create(PurpleConnection *p_gc);
void gfire_free(gfire_data *p_gfire);

// Connection
PurpleConnection *gfire_get_connection(const gfire_data *p_gfire);
void gfire_login(gfire_data *p_gfire);
void gfire_close(gfire_data *p_gfire);
void gfire_authenticate(gfire_data *p_gfire, const gchar *p_salt);
void gfire_keep_alive(gfire_data *p_gfire);

// Session
void gfire_set_userid(gfire_data *p_gfire, guint32 p_userid);
void gfire_set_sid(gfire_data *p_gfire, guint8 *p_sid);

// Buddy handling
gfire_buddy *gfire_find_buddy(gfire_data *p_gfire, const void *p_data, gfire_find_buddy_mode p_mode);
void gfire_add_buddy(gfire_data *p_gfire, gfire_buddy *p_buddy, PurpleGroup *p_group);
void gfire_remove_buddy(gfire_data *p_gfire, gfire_buddy *p_buddy, gboolean p_fromServer, gboolean p_force);
void gfire_got_invitation(gfire_data *p_gfire, const gchar *p_name, const gchar *p_alias, const gchar *p_msg);
void gfire_show_buddy_info(gfire_data *p_gfire, const gchar *p_name);

// Clan handling
gfire_clan *gfire_find_clan(gfire_data *p_gfire, guint32 p_clanid);
void gfire_add_clan(gfire_data *p_gfire, gfire_clan *p_clan);
void gfire_remove_clan(gfire_data *p_gfire, gfire_clan *p_clan);
void gfire_leave_clan(gfire_data *p_gfire, guint32 p_clanid);
void gfire_remove_buddy_from_clan(gfire_data *p_gfire, gfire_buddy *p_buddy, guint32 p_clanid);

// Chat handling
gfire_chat *gfire_find_chat(gfire_data *p_gfire, const void *p_data, gfire_find_chat_mode p_mode);
void gfire_add_chat(gfire_data *p_gfire, gfire_chat *p_chat);
void gfire_leave_chat(gfire_data *p_gfire, gfire_chat *p_chat);

// Gaming status
gboolean gfire_is_playing(const gfire_data *p_gfire);
gboolean gfire_is_talking(const gfire_data *p_gfire);
const gfire_game_data *gfire_get_game_data(gfire_data *p_gfire);
const gfire_game_data *gfire_get_voip_data(gfire_data *p_gfire);
void gfire_join_game(gfire_data *p_gfire, const gfire_game_data *p_game_data);

// Appearance
const gchar *gfire_get_name(const gfire_data *p_gfire);
void gfire_set_alias(gfire_data *p_gfire, const gchar* p_alias); // Local
void gfire_set_nick(gfire_data *p_gfire, const gchar *p_nick); // Remote

// Status handling
void gfire_set_status(gfire_data *p_gfire, const PurpleStatus *p_status);

// Identification
gboolean gfire_is_self(const gfire_data *p_gfire, guint32 p_userid);

// Detection
gboolean gfire_detect_running_processes_cb(gfire_data *p_gfire);

#endif // _GFIRE_H
