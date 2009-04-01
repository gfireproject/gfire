/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
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
 * along with Gfire.  If not, see <http://www.gnu.org/licenses/>.A
*/

#ifndef _GF_NETWORK_H
#define _GF_NETWORK_H

#include "gfire.h"

#define GFIRE_BUFFOUT_SIZE 65535
#define GFIRE_BUFFIN_SIZE 65535

void gfire_send(PurpleConnection *gc, const guint8 *packet, int size);
int gfire_initialize_connection(guint8 *packet, int version);
void gfire_input_cb(gpointer data, gint source, PurpleInputCondition condition);
void gfire_send_away(PurpleConnection *gc, const char *msg);
void gfire_process_invitation(PurpleConnection *gc, GList *invites);

/* Function is in gfire.c, but we put this here for the timeout. */
gboolean gfire_detect_running_games_cb(PurpleConnection *gc);

#endif /* _GF_NETWORK_H */
