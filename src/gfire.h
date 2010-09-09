/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2005-2006, Beat Wolf <asraniel@fryx.ch>
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

#ifndef _GFIRE_H
#define _GFIRE_H

// Required prototypes
typedef struct _gfire_data gfire_data;

#include "gf_base.h"

#include "gf_buddies.h"
#include "gf_chat.h"
#include "gf_games.h"
#include "gf_p2p.h"
#include "gf_groups.h"
#include "gf_server_browser_proto.h"
#include "gf_preferences.h"

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
	GFFC_CID = 0,		// by chat ID
	GFFC_TOPIC,			// by topic
	GFFC_PURPLEID,		// by purple ID
	GFFC_PURPLECHAT,	// by PurpleChat
} gfire_find_chat_mode;

typedef enum _gfire_find_group_mode
{
	GFFG_GID = 0,	// by group ID
	GFFG_PURPLE,	// by PurpleGroup
	GFFG_NAME,		// by group name
	GFFG_BUDDY		// by buddy
} gfire_find_group_mode;

//typedef struct _gfire_chat_msg
//{
//	guint8 *chat_id;	/* xfire chat ID of group chat */
//	guint32 uid;		/* userid of user posting the message */
//	gchar *im_str;		/* im text */
//	gfire_buddy *b;		/* for users joining the chat */
//} gfire_chat_msg;

struct _gfire_data
{
	// Networking
	int fd;
	guint8 *buff_in;
	guint16 bytes_read;
	glong last_pong;
	PurpleConnection *gc;

	// Xfire session
	guint32 userid;		/* our userid on the xfire network */
	guint8 *sid;		/* our session id for this connection */
	gchar *alias;		/* our current server alias */

	// P2P Connection
	gfire_p2p_connection *p2p;

	// Buddies
	GList *buddies;
	GList *clans;
	GList *groups;

	guint32 chat;
	gchar *email;

	GList *chats;				/* glist of _gfire_chat structs */

	// Server browser
	gfire_server_browser *server_browser;

	// Client preferences
	gfire_preferences *prefs;
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
void gfire_keep_alive_response(gfire_data *p_gfire);

// Session
void gfire_set_userid(gfire_data *p_gfire, guint32 p_userid);
void gfire_set_sid(gfire_data *p_gfire, guint8 *p_sid);

// Buddy handling
gfire_buddy *gfire_find_buddy(gfire_data *p_gfire, const void *p_data, gfire_find_buddy_mode p_mode);
void gfire_add_buddy(gfire_data *p_gfire, gfire_buddy *p_buddy, gfire_group *p_group);
void gfire_remove_buddy(gfire_data *p_gfire, gfire_buddy *p_buddy, gboolean p_fromServer, gboolean p_force);
void gfire_got_invitation(gfire_data *p_gfire, const gchar *p_name, const gchar *p_alias, const gchar *p_msg);
void gfire_show_buddy_info(gfire_data *p_gfire, const gchar *p_name);

// Clan handling
gfire_clan *gfire_find_clan(gfire_data *p_gfire, guint32 p_clanid);
void gfire_add_clan(gfire_data *p_gfire, gfire_clan *p_clan);
void gfire_remove_clan(gfire_data *p_gfire, gfire_clan *p_clan);
void gfire_leave_clan(gfire_data *p_gfire, guint32 p_clanid);
void gfire_remove_buddy_from_clan(gfire_data *p_gfire, gfire_buddy *p_buddy, guint32 p_clanid);

// Group handling
gfire_group *gfire_find_group(gfire_data *p_gfire, const void *p_data, gfire_find_group_mode p_mode);
void gfire_add_group(gfire_data *p_gfire, gfire_group *p_group);
void gfire_remove_group(gfire_data *p_gfire, gfire_group *p_group, gboolean p_remove);

// Chat handling
gfire_chat *gfire_find_chat(gfire_data *p_gfire, const void *p_data, gfire_find_chat_mode p_mode);
void gfire_add_chat(gfire_data *p_gfire, gfire_chat *p_chat);
void gfire_leave_chat(gfire_data *p_gfire, gfire_chat *p_chat);

// Gaming status
void gfire_set_game_status(gfire_data *p_gfire, const gfire_game_data *p_data);
void gfire_set_voip_status(gfire_data *p_gfire, const gfire_game_data *p_data);

// Appearance
const gchar *gfire_get_name(const gfire_data *p_gfire);
const gchar *gfire_get_nick(const gfire_data *p_gfire);
void gfire_set_alias(gfire_data *p_gfire, const gchar* p_alias); // Local
void gfire_set_nick(gfire_data *p_gfire, const gchar *p_nick); // Remote

// Status handling
void gfire_set_status(gfire_data *p_gfire, const PurpleStatus *p_status);
void gfire_set_current_status(gfire_data *p_gfire);

// Identification
gboolean gfire_is_self(const gfire_data *p_gfire, guint32 p_userid);

// P2P
gboolean gfire_has_p2p(const gfire_data *p_gfire);
gfire_p2p_connection *gfire_get_p2p(const gfire_data *p_gfire);

// Servers
void gfire_show_server_browser(PurplePluginAction *p_action);

// Account settings
void gfire_got_preferences(gfire_data *p_gfire);
gboolean gfire_wants_fofs(const gfire_data *p_gfire);
gboolean gfire_wants_server_detection(const gfire_data *p_gfire);
gboolean gfire_wants_global_status_change(const gfire_data *p_gfire);

// Internal
void gfire_games_update_done();

#endif // _GFIRE_H
