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

#ifndef _GF_GROUPS_PROTO_H
#define _GF_GROUPS_PROTO_H

#include "gf_base.h"
#include "gfire.h"

// Packet creation
guint16 gfire_group_proto_create_create_group(const gchar *p_name);
guint16 gfire_group_proto_create_remove_group(guint32 p_groupid);
guint16 gfire_group_proto_create_rename_group(guint32 p_groupid, const gchar *p_name);
guint16 gfire_group_proto_create_add_buddy_to_group(guint32 p_groupid, guint32 p_userid);
guint16 gfire_group_proto_create_remove_buddy_from_group(guint32 p_groupid, guint32 p_userid);

// Packet parsing
void gfire_group_proto_groups(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_group_proto_buddies_in_groups(gfire_data *p_gfire, guint16 p_packet_len);
void gfire_group_proto_group_added(gfire_data *p_gfire, guint16 p_packet_len);

#endif // _GF_GROUPS_PROTO_H
