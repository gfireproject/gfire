/*
 * purple - Xfire Protocol Plugin
 *
 * This file is part of Gfire.
 *
 * See the AUTHORS file distributed with Gfire for a full list of
 * all contributors and this files copyright holders.
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
