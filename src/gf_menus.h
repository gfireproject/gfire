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

#ifndef _GF_MENUS_H
#define _GF_MENUS_H

#include "gf_base.h"
#include "gfire.h"
#include "gf_games.h"
#include "gf_purple.h"

// GFIRE ACCOUNT MENU ///////////////////////////////////////////////
void gfire_menu_action_nick_change_cb(PurplePluginAction *p_action);
void gfire_menu_action_reload_lconfig_cb(PurplePluginAction *p_action);
void gfire_menu_action_about_cb(PurplePluginAction *p_action);
void gfire_menu_action_profile_page_cb(PurplePluginAction *p_action);
void gfire_menu_action_launch_game_cb(PurplePluginAction *p_action);

// GFIRE BUDDY MENU /////////////////////////////////////////////////
void gfire_buddy_menu_profile_cb(PurpleBlistNode *p_node, gpointer *p_data);
void gfire_buddy_menu_add_as_friend_cb(PurpleBlistNode *p_node, gpointer *p_data);
void gfire_buddy_menu_joingame_cb(PurpleBlistNode *p_node, gpointer *p_data);
#ifdef HAVE_GTK
void gfire_buddy_menu_server_details_cb(PurpleBlistNode *p_node, gpointer *p_data);
#endif // HAVE_GTK
void gfire_buddy_menu_joinvoip_cb(PurpleBlistNode *p_node, gpointer *p_data);

// GFIRE CLAN MENU //////////////////////////////////////////////////
void gfire_clan_menu_site_cb(PurpleBlistNode *p_node, gpointer *p_data);

#endif // _GF_MENUS
