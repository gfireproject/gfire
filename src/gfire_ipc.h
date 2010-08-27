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

#ifndef _GFIRE_IPC_H
#define _GFIRE_IPC_H

#define GFIRE_IPC_VERSION 0
#define GFIRE_IPC_PORT 52634
#define GFIRE_IPC_CHAT_PORT (GFIRE_IPC_PORT + 1)
#define GFIRE_IPC_BUFFER_LEN 65535
#define GFIRE_IPC_KEEP_ALIVE_INTERVAL 15
#define GFIRE_IPC_TIMEOUT 20

#define GFIRE_IPC_HEADER_LEN 8
/*
 * Layout of an IPC packet
 *
 * Header:
 *   2 bytes - length of whole packet
 *   4 bytes - PID of the client
 *   2 bytes - ID (type) of packet
 * Data:
 *   * bytes - ID dependend data
 *
 */

// IPC Packet Types ////////////////////////////////////
//	Client Handshake (sent by client)
//		Packet layout:
//		8 bytes - Header
//		2 bytes - IPC version (see GFIRE_IPC_VERSION)
#define GFIRE_IPC_CLIENT_HS		0x0001

//	Server Handshake (sent by server)
//		Packet layout:
//		8 bytes - Header
//		1 byte  - version okay (1 or 0)
#define GFIRE_IPC_SERVER_HS		0x0002

// Shutdown packet (sent by the closing side (client & server))
//		Packet layout
//		8 bytes - Header
#define GFIRE_IPC_SHUTDOWN		0x0003

// Keep-alive packet (sent by client & server in a 15s interval)
//		Packet layout
//		8 bytes - Header
#define GFIRE_IPC_KEEP_ALIVE	0x0004

// Xfire SDK data (sent by client)
//		Packet layout
//		8 bytes - Header
//		Unknown
#define GFIRE_IPC_ID_SDK		0x005F

#endif // _GFIRE_IPC_H
