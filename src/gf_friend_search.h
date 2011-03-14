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

#ifndef _GF_FRIEND_SEARCH_H
#define _GF_FRIEND_SEARCH_H

#include "gfire.h"
#include "gf_friend_search_proto.h"

void gfire_show_friend_search_cb(PurplePluginAction *p_action);
void gfire_friend_search_results(gfire_data *p_gfire, GList *p_usernames, GList *p_firstnames, GList *p_lastnames);

#endif // _GF_FRIEND_SEARCH_H
