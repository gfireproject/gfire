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

#ifndef _GF_PREFERENCES_PROTO_H
#define _GF_PREFERENCES_PROTO_H

#include "gf_base.h"
#include "gfire.h"

// Packet creation
guint16 gfire_pref_proto_create_changed_preferences(const GList *p_prefs);

// Packet parsing
void gfire_pref_proto_client_preferences(gfire_data *p_gfire, guint16 p_packet_len);

#endif // _GF_PREFERENCES_PROTO_H
