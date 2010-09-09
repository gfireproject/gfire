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

#ifndef _GF_SERVER_BROWSER_PROTO_H
#define _GF_SERVER_BROWSER_PROTO_H

#include "gf_base.h"
#include "gf_server_browser.h"

#ifdef HAVE_GTK
#include "gf_network.h"
#include "gf_protocol.h"
#include "gfire.h"

// Packet creation
guint16 gfire_server_browser_proto_create_friends_fav_serverlist_request(guint32 p_gameid);
guint16 gfire_server_browser_proto_create_serverlist_request(guint32 p_gameid);
guint16 gfire_server_browser_proto_create_add_fav_server(guint32 p_gameid, guint32 p_ip, guint32 p_port);
guint16 gfire_server_browser_proto_create_remove_fav_server(guint32 p_gameid, guint32 p_ip, guint32 p_port);

// Packet parsing
void gfire_server_browser_proto_friends_fav_serverlist(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_server_browser_proto_fav_serverlist(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_server_browser_proto_serverlist(gfire_data *p_gfire, guint16 p_packet_len);

#endif // HAVE_GTK

#endif // _GF_SERVER_BROWSER_PROTO_H
