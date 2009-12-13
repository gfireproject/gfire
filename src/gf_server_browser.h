/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009  Laurent De Marez <laurentdemarez@gmail.com>
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

#include "gf_server_browser_proto.h"

#ifndef _GF_SERVER_BROWSER_H
#define _GF_SERVER_BROWSER_H

void gfire_server_browser_show(PurplePluginAction *p_action);
static void gfire_server_browser_request_list_cb(server_browser_callback_args *p_args, GtkWidget *p_button);
static void gfire_server_browser_server_information_cb(server_browser_callback_args *p_args, GtkWidget *p_sender);
static void gfire_server_browser_connect_cb(server_browser_callback_args *p_args, GtkWidget *p_sender);

#endif // _GF_SERVER_BROWSER_H
