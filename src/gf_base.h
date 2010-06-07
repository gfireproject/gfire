/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
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

#ifndef _GF_BASE_H
#define _GF_BASE_H

// Globally required headers ////////////////////////////////////////
#ifdef _WIN32
#	include <w32api.h>
#	define _WIN32_WINNT WindowsXP
#	define WINVER WindowsXP
#	include "gfire_config_win.h"
#	include "internal.h"
#	undef _
#else
#	ifdef HAVE_CONFIG_H
#		include "gfire_config.h"
#	endif // HAVE_CONFIG_H
#	include <sys/socket.h>
#	include <sys/types.h>
#	include <unistd.h>
#	include <arpa/inet.h>
#	include <netinet/in.h>
#endif // _WIN32

// Compareable Gfire version
#define GFIRE_VERSION ((GFIRE_VERSION_MAJOR << 16) | (GFIRE_VERSION_MINOR << 8) | GFIRE_VERSION_PATCH)

// Standard libraries
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Glib
#include <glib-object.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gthread.h>

#ifdef HAVE_GTK
	#include <gtk/gtk.h>
	#include <gio/gio.h>
#endif // HAVE_GTK

// Libpurple
#include "core.h"
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
#include "cmds.h"
#include "savedstatuses.h"

// Gfire debugging macros
#include "gf_debug.h"

#include "gf_util.h"

// Internationalization /////////////////////////////////////////////
#ifdef ENABLE_NLS
	#include <glib/gi18n-lib.h>
#else
#	define _(string) (const char*)(string)
#	define N_(string) _(string)
#endif // ENABLE_NLS

#ifndef G_GNUC_NULL_TERMINATED
#	if __GNUC__ >= 4
#		define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#	else
#		define G_GNUC_NULL_TERMINATED
#	endif /* __GNUC__ >= 4 */
#endif // G_GNUC_NULL_TERMINATED

// Global defintions ////////////////////////////////////////////////
#define GFIRE_PRPL_ID "prpl-xfire"
#define GFIRE_WEBSITE "http://gfireproject.org"
#define GFIRE_WIKI "http://my-trac.assembla.com/gfire/wiki"
#define GFIRE_DEFAULT_GROUP_NAME "Xfire"
#define GFIRE_FRIENDS_OF_FRIENDS_GROUP_NAME _("Xfire - Friends of Friends playing games")
#define GFIRE_CLAN_GROUP_FORMATTING "%s [%s]" // long name, short name
#define GFIRE_GAMES_XML_URL "http://gfireproject.org/files/gfire_games_v2.xml"
#define GFIRE_CURRENT_VERSION_XML_URL "http://gfireproject.org/files/gfire_version.xml"
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
#define XFIRE_PROTO_VERSION 112
#define XFIRE_CONNECT_STEPS 3
#define XFIRE_TIMEOUT_TIME 240  // See gfire_keep_alive for more info
#define XFIRE_PROFILE_URL "http://www.xfire.com/profile/"
#define XFIRE_COMMUNITY_URL "http://www.xfire.com/communities/%s/" // community tag
#define XFIRE_XML_INFO_URL "http://www.xfire.com/xml/%s/%s/" // username, info-type
#define XFIRE_AVATAR_URL "http://screenshot.xfire.com/avatar/%s.jpg?%u" // username, revision number
#define XFIRE_GALLERY_AVATAR_URL "http://media.xfire.com/xfire/xf/images/avatars/gallery/default/%03u.gif" // avatar id
#define XFIRE_COMMUNITY_AVATAR_URL "http://screenshot.xfire.com/clan_logo/160/%s.jpg?v=10" // long community name
#define XFIRE_SEND_TYPING_TIMEOUT 10
#define XFIRE_SEND_ACK_TIMEOUT 15
#define XFIRE_SEND_ACK_P2P_TIMEOUT 2

#endif // _GF_BASE_H
