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

#ifndef _GF_BASE_H
#define _GF_BASE_H

// Globally required headers ////////////////////////////////////////
#ifdef HAVE_CONFIG_H
	#include <gfire_config.h>
#endif // HAVE_CONFIG_H

// Standard libraries
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

// Glib
#include <glib-object.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gthread.h>

#ifdef _WIN32
#else
	#include <gtk/gtk.h>
	#include <gio/gio.h>
	#include <errno.h>
#endif /* _WIN32 */

// Libpurple
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
#include "cipher.h"
#include "xmlnode.h"
#include "privacy.h"

// Gfire debugging macros
#include "gf_debug.h"

#include "gf_util.h"


// Internationalization /////////////////////////////////////////////
#include <libintl.h>
#include <locale.h>

#define _(string) gettext (string)
#define gettext_noop(string) string
#define N_(string) gettext (string)

#ifndef G_GNUC_NULL_TERMINATED
	#if __GNUC__ >= 4
		#define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
	#else
		#define G_GNUC_NULL_TERMINATED
	#endif /* __GNUC__ >= 4 */
#endif // G_GNUC_NULL_TERMINATED

// Global defintions ////////////////////////////////////////////////
#define GFIRE_WEBSITE "http://gfireproject.org"
#define GFIRE_WIKI "http://my-trac.assembla.com/gfire/wiki"
#define GFIRE_XQF_FILENAME "ingame.tmp"
#define GFIRE_DEFAULT_GROUP_NAME "Xfire"
#define GFIRE_FRIENDS_OF_FRIENDS_GROUP_NAME "Xfire - Friends of Friends"
#define GFIRE_CLAN_GROUP_NAME "Clan"
#define GFIRE_CLAN_GROUP_FORMATTING "%s [%s]" // long name, short name
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
#define XFIRE_KEEPALIVE_TIME 300  // see gfire_keep_alive for more info
#define XFIRE_PROFILE_URL "http://www.xfire.com/profile/"
#define XFIRE_XML_INFO_URL "http://www.xfire.com/xml/%s/%s/" // username, info-type
#define XFIRE_AVATAR_URL "http://screenshot.xfire.com/avatar/%s.jpg?%u" // username, revision number
#define XFIRE_GALLERY_AVATAR_URL "http://media.xfire.com/xfire/xf/images/avatars/gallery/default/%03u.gif" // avatar id
#define XFIRE_SEND_TYPING_TIMEOUT 10
#define XFIRE_SEND_ACK_TIMEOUT 15

#endif // _GF_BASE_H