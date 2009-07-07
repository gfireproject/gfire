/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2000-2001, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008,	    Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009,	    Laurent De Marez <laurentdemarez@gmail.com>,
 *						    Warren Dumortier <nwarrenfl@gmail.com>,
 *						    Oliver Ney <oliver@dryder.de>
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

#include "gf_friend_search.h"
#include "gf_friend_search_proto.h"

guint16 gfire_friend_search_proto_create_request(const gchar *p_search)
{
	if(!p_search)
		return -1;

	guint32 offset = XFIRE_HEADER_LEN;

	offset = gfire_proto_write_attr_ss("name", 0x01, p_search, strlen(p_search), offset);
	offset = gfire_proto_write_attr_ss("fname", 0x01, "", 0, offset);
	offset = gfire_proto_write_attr_ss("lname", 0x01, "", 0, offset);
	offset = gfire_proto_write_attr_ss("email", 0x01, "", 0, offset);

	gfire_proto_write_header(offset, 0x0C, 4, 0);

	return offset;
}

void gfire_friend_search_proto_result(gfire_data *p_gfire, guint16 p_packet_len)
{
	if(!p_gfire)
		return;

	guint32 offset = XFIRE_HEADER_LEN;

	GList *usernames = NULL;
	GList *firstnames = NULL;
	GList *lastnames = NULL;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &usernames, "name", offset);
	if(offset == -1 || !usernames)
		return;

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &firstnames, "fname", offset);
	if(offset == -1 || !firstnames)
	{
		//mem cleanup code
		if (usernames) g_list_free(usernames);
		return;
	}

	offset = gfire_proto_read_attr_list_ss(p_gfire->buff_in, &lastnames, "lname", offset);
	if(offset == -1 || !lastnames)
	{
		//mem cleanup code
		if (usernames) g_list_free(usernames);
		if (firstnames) g_list_free(firstnames);
		return;
	}

	gfire_friend_search_results(usernames, firstnames, lastnames);
}
