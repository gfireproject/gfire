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

#ifndef _GF_P2P_DL_HANDLER_H
#define _GF_P2P_DL_HANDLER_H

#include "gf_base.h"
#include "gf_p2p_session.h"

// GFIRE P2P DL DATA HANDLER
gboolean gfire_p2p_dl_handler_handle(gfire_p2p_session *p_session, const guint8 *p_data, guint32 p_len);

#endif // _GF_P2P_DL_HANDLER_H
