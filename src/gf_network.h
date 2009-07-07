/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
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

#ifndef _GF_NETWORK_H
#define _GF_NETWORK_H

#include "gf_base.h"
#include "gfire.h"

#define GFIRE_BUFFOUT_SIZE 65535
#define GFIRE_BUFFIN_SIZE 65535

// Network system
void gfire_network_init();
void gfire_network_cleanup();
void gfire_network_buffout_write(const void *p_data, guint16 p_len, guint16 p_offset);

// Traffic handling
void gfire_send(PurpleConnection *p_gc, guint16 p_size);
void gfire_input_cb(gpointer p_data, gint p_source, PurpleInputCondition p_condition);
void gfire_parse_packet(gfire_data *p_gfire, guint16 p_packet_len, guint16 p_packet_id);

#endif // _GF_NETWORK_H
