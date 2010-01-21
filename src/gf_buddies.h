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

#ifndef _GF_BUDDIES_H
#define _GF_BUDDIES_H

typedef struct _gfire_buddy gfire_buddy;
typedef struct _gfire_clan gfire_clan;

#include "gf_base.h"
#include "gf_games.h"
#include "gf_p2p_session.h"
#include "gf_groups.h"

// Stores information about all clans this gfire session has to handle with
struct _gfire_clan
{
	guint32 id;					// Clan ID
	gchar *long_name;			// Long Clan Name, e.g. "Gfire - Linux and Co Users"
	gchar *short_name;			// Short Clan Name, e.g. "gfire"
	PurpleGroup *prpl_group;	// Purple Buddy List Group
};

// Enum with all possible buddy types
typedef enum _gfire_buddy_type
{
	GFBT_FRIEND = 0,
	GFBT_CLAN,
	GFBT_GROUPCHAT,
	GFBT_FRIEND_OF_FRIEND
} gfire_buddy_type;

// Stores a buddy's clan information
typedef struct _gfire_buddy_clan_data
{
	gfire_clan *clan;
	gchar *clan_alias;
	gboolean is_default;
} gfire_buddy_clan_data;

// Stores game client info (belonging to Xfire SDK)
typedef struct _game_client_data
{
	gchar *key;
	gchar *value;
} game_client_data;

// Stores a FoF buddies game data
typedef struct _fof_game_data
{
	guint8 *sid;
	gfire_game_data game;
	GList *gcd;
} fof_game_data;

// Stores sent messages, which we haven't received an ack for
typedef struct _im_sent
{
	guint32 imindex;
	gchar *msg;
	glong time;
} im_sent;

typedef enum _can_handle_p2p
{
	GFP2P_UNKNOWN,
	GFP2P_YES,
	GFP2P_NO
} can_handle_p2p;

// Information about a gfire buddy
struct _gfire_buddy
{
	// Connection for sending
	PurpleConnection *gc;

	// Xfire user data
	guint32 userid;			// user id
	guint8 *sid;			// session id, length = XFIRE_SID_LEN
	gchar *name;			// name (xfire login id)
	gchar *alias;			// nick (xfire alias)

	// Status data
	PurpleStatusPrimitive status;
	gchar *status_msg;		// away message

	// Chat state
	guint32	im;				// im index ++'d on each im reception and send
	GList *pending_ims;		// List of im_sent structs
	GList *pending_p2p_ims;	// List of im_sent structs for P2P messages
	guint lost_ims_timer;	// Timer handle to check for lost IMs
	guint lost_p2p_ims_timer; // Timer handle to check for lost P2P IMs
	guint32 chatperm;		// group chat permissions (only used for group chat members)
	guint32 highest_im;		// Highest received imindex
	GList *missing_ims;		// List of missing received imindex's

	// P2P
	can_handle_p2p hasP2P;
	gboolean p2p_requested;
	gboolean p2p_notify;
	gfire_p2p_session *p2p;

	// Game data
	gfire_game_data game_data;
	GList *game_client_data;

	// VoIP data
	gfire_game_data voip_data;

	// FoF common buddies
	GList *common_buddies;

	// Got advanced info
	glong get_info_block;
	gboolean got_info;

	// Buddy type
	GList *clan_data;		// list of type gfire_buddy_clan_data
	gfire_buddy_type type;	// buddy type

	// Avatar data
	guint32 avatarnumber;	// xfire avatar reference number
	guint32 avatartype;		// xfire avatar type

	// Purple Buddy
	PurpleBuddy *prpl_buddy;
};

// GFIRE BUDDIES ////////////////////////////////////////////////////
// Creation and freeing
gfire_buddy *gfire_buddy_create(const guint32 p_userid, const gchar *p_name, const gchar *p_alias, gfire_buddy_type p_type);
void gfire_buddy_free(gfire_buddy *p_buddy);

// Type checks
gboolean gfire_buddy_is_friend(const gfire_buddy *p_buddy);
gboolean gfire_buddy_is_clan_member(const gfire_buddy *p_buddy);
gboolean gfire_buddy_is_friend_of_friend(const gfire_buddy *p_buddy);

// Identifying
gboolean gfire_buddy_is_by_userid(const gfire_buddy *p_buddy, guint32 p_userid);
gboolean gfire_buddy_is_by_sid(const gfire_buddy *p_buddy, const guint8 *p_sid);

// IM handling
void gfire_buddy_send(gfire_buddy *p_buddy, const gchar *p_msg);
void gfire_buddy_send_nop2p(gfire_buddy *p_buddy, const gchar *p_msg, guint32 p_imindex);
void gfire_buddy_got_im(gfire_buddy *p_buddy, guint32 p_imindex, const gchar *p_msg, gboolean p_p2p);
void gfire_buddy_send_typing(gfire_buddy *p_buddy, gboolean p_typing);
void gfire_buddy_got_typing(const gfire_buddy *p_buddy, gboolean p_typing);
void gfire_buddy_got_im_ack(gfire_buddy *p_buddy, guint32 p_imindex);
gboolean gfire_buddy_check_pending_ims_cb(gfire_buddy *p_buddy);
gboolean gfire_buddy_check_pending_p2p_ims_cb(gfire_buddy *p_buddy);

// PurpleBuddy creation/deletion
void gfire_buddy_prpl_add(gfire_buddy *p_buddy, gfire_group *p_group);
void gfire_buddy_prpl_remove(gfire_buddy *p_buddy);
void gfire_buddy_prpl_move(gfire_buddy *p_buddy, PurpleGroup *p_group);

// Game/VoIP status
void gfire_buddy_set_game_status(gfire_buddy *p_buddy, guint32 p_id, guint32 p_port, guint32 p_ip);
void gfire_buddy_set_voip_status(gfire_buddy *p_buddy, guint32 p_id, guint32 p_port, guint32 p_ip);
void gfire_buddy_set_game_client_data(gfire_buddy *p_buddy, GList *p_data);
gboolean gfire_buddy_is_playing(const gfire_buddy *p_buddy);
gboolean gfire_buddy_is_talking(const gfire_buddy *p_buddy);
const gfire_game_data *gfire_buddy_get_game_data(const gfire_buddy *p_buddy);
const gfire_game_data *gfire_buddy_get_voip_data(const gfire_buddy *p_buddy);
const GList *gfire_buddy_get_game_client_data(const gfire_buddy *p_buddy);

// Status handling
void gfire_buddy_set_session_id(gfire_buddy *p_buddy, const guint8 *p_sessionid);
void gfire_buddy_set_status(gfire_buddy *p_buddy, const gchar *p_status_msg);
gchar *gfire_buddy_get_status_text(const gfire_buddy *p_buddy, gboolean p_nogame);
const gchar *gfire_buddy_get_status_name(const gfire_buddy *p_buddy);
gboolean gfire_buddy_is_available(const gfire_buddy *p_buddy);
gboolean gfire_buddy_is_away(const gfire_buddy *p_buddy);
gboolean gfire_buddy_is_busy(const gfire_buddy *p_buddy);
gboolean gfire_buddy_is_online(const gfire_buddy *p_buddy);

// Extended info
void gfire_buddy_request_info(gfire_buddy *p_buddy);
gboolean gfire_buddy_got_info(const gfire_buddy *p_buddy);

// Clan membership handling
gboolean gfire_buddy_is_clan_member_of(const gfire_buddy *p_buddy, guint32 p_clanid);
void gfire_buddy_add_to_clan(gfire_buddy *p_buddy, gfire_clan *p_clan, const gchar *p_clanalias, gboolean p_default);
void gfire_buddy_remove_clan(gfire_buddy *p_buddy, guint32 p_clanid, guint32 p_newdefault);
void gfire_buddy_set_clan_alias(gfire_buddy *p_buddy, guint32 p_clanid, const gchar *p_alias);
guint32 gfire_buddy_get_default_clan(gfire_buddy *p_buddy);
GList *gfire_buddy_get_clans_info(const gfire_buddy *p_buddy);
void gfire_buddy_make_friend(gfire_buddy *p_buddy, gfire_group *p_group);

// FoF handling
void gfire_buddy_set_common_buddies(gfire_buddy *p_buddy, GList *p_buddies);
gchar *gfire_buddy_get_common_buddies_str(const gfire_buddy *p_buddy);

// Appearance
void gfire_buddy_set_alias(gfire_buddy *p_buddy, const gchar *p_alias);
const gchar *gfire_buddy_get_alias(gfire_buddy *p_buddy);
const gchar *gfire_buddy_get_name(const gfire_buddy *p_buddy);
void gfire_buddy_set_avatar(gfire_buddy *p_buddy, guchar *p_data, guint32 p_len);
void gfire_buddy_download_avatar(gfire_buddy *p_buddy, guint32 p_type, guint32 p_avatarNum);

// P2P
gboolean gfire_buddy_has_p2p(const gfire_buddy *p_buddy);
gboolean gfire_buddy_uses_p2p(const gfire_buddy *p_buddy);
void gfire_buddy_request_p2p(gfire_buddy *p_buddy, gboolean p_notify);
void gfire_buddy_got_p2p_data(gfire_buddy *p_buddy, guint32 p_ip, guint16 p_port, const gchar *p_salt);
void gfire_buddy_p2p_timedout(gfire_buddy *p_buddy);
void gfire_buddy_p2p_uncapable(gfire_buddy *p_buddy);
void gfire_buddy_p2p_ft_init(PurpleXfer *p_xfer);


// GFIRE CLANS //////////////////////////////////////////////////////
// Creation and freeing
gfire_clan *gfire_clan_create(guint32 p_clanid, const gchar *p_longName, const gchar *p_shortName, gboolean p_createGroup);
void gfire_clan_free(gfire_clan *p_clan);

// Identifying
gboolean gfire_clan_is(const gfire_clan *p_clan, guint32 p_clanid);

// External changes
void gfire_clan_set_prpl_group(gfire_clan *p_clan, PurpleGroup *p_group);
PurpleGroup *gfire_clan_get_prpl_group(gfire_clan *p_clan);
void gfire_clan_set_names(gfire_clan *p_clan, const gchar *p_longName, const gchar *p_shortName);
const gchar *gfire_clan_get_long_name(const gfire_clan *p_clan);
const gchar *gfire_clan_get_short_name(const gfire_clan *p_clan);
gchar *gfire_clan_get_name(const gfire_clan *p_clan);

// PurpleGroup deletion
void gfire_clan_prpl_remove(gfire_clan *p_clan);

// Initializing from Buddy List
GList *gfire_clan_get_existing();

// FOF GAME DATA
fof_game_data *gfire_fof_game_data_create(const guint8 *p_sid, guint32 p_game, guint32 p_ip, guint16 p_port);
void gfire_fof_game_data_free(fof_game_data *p_data);

// GAME CLIENT DATA
GList *gfire_game_client_data_parse(const gchar *p_datastring);

#endif // _GF_BUDDIES_H
