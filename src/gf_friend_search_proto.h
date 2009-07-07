/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009,	    Laurent De Marez <laurentdemarez@gmail.com>,
 *						    Warren Dumortier <nwarrenfl@gmail.com>,
 *						    Oliver Ney <oliver@dryder.de>
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

#ifndef _GF_FRIEND_SEARCH_PROTO_H
#define _GF_FRIEND_SEARCH_PROTO_H

#include "gf_base.h"
#include "gf_network.h"
#include "gf_protocol.h"
#include "gfire.h"

guint16 gfire_friend_search_proto_create_request(const gchar *p_search);
void gfire_friend_search_proto_result(gfire_data *p_gfire, guint16 p_packet_len);

#endif // _GF_FRIEND_SEARCH_PROTO_H
