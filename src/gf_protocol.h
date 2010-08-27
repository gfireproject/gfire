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

#ifndef _GF_PROTOCOL_H
#define _GF_PROTOCOL_H

#include "gf_base.h"

typedef union _list_type_pointers
{
	gchar *pchar;
	guint8 *puint8;
	guint32 *puint32;
	guint64 *puint64;
	gboolean *pboolean;
	GList *plist;
} list_type_pointers;

// String style attributes
guint32 gfire_proto_check_attribute_ss(const guint8 *p_buff, const gchar *p_name, guint8 p_type, guint32 p_offset);
guint32 gfire_proto_read_attr_string_ss(const guint8 *p_buff, gchar **p_dest, const gchar *p_name, guint32 p_offset);
guint32 gfire_proto_read_attr_int32_ss(const guint8 *p_buff, guint32 *p_dest, const gchar *p_name, guint32 p_offset);
guint32 gfire_proto_read_attr_sid_ss(const guint8 *p_buff, guint8 **p_dest, const gchar *p_name, guint32 p_offset);
guint32 gfire_proto_read_attr_chatid_ss(const guint8 *p_buff, guint8 **p_dest, const gchar *p_name, guint32 p_offset);
guint32 gfire_proto_read_attr_int64_ss(const guint8 *p_buff, guint64 *p_dest, const gchar *p_name, guint32 p_offset);
guint32 gfire_proto_read_attr_boolean_ss(const guint8 *p_buff, gboolean *p_dest, const gchar *p_name, guint32 p_offset);
guint32 gfire_proto_read_attr_list_ss(const guint8 *p_buff, GList **p_dest, const gchar *p_name, guint32 p_offset);

// Byte style attributes
guint32 gfire_proto_check_attribute_bs(const guint8 *p_buff, guint8 p_id, guint8 p_type, guint32 p_offset);
guint32 gfire_proto_read_attr_string_bs(const guint8 *p_buff, gchar **p_dest, guint8 p_id, guint32 p_offset);
guint32 gfire_proto_read_attr_int32_bs(const guint8 *p_buff, guint32 *p_dest, guint8 p_id, guint32 p_offset);
guint32 gfire_proto_read_attr_sid_bs(const guint8 *p_buff, guint8 **p_dest, guint8 p_id, guint32 p_offset);
guint32 gfire_proto_read_attr_chatid_bs(const guint8 *p_buff, guint8 **p_dest, guint8 p_id, guint32 p_offset);
guint32 gfire_proto_read_attr_int64_bs(const guint8 *p_buff, guint64 *p_dest, guint8 p_id, guint32 p_offset);
guint32 gfire_proto_read_attr_boolean_bs(const guint8 *p_buff, gboolean *p_dest, guint8 p_id, guint32 p_offset);
guint32 gfire_proto_read_attr_list_bs(const guint8 *p_buff, GList **p_dest, guint8 p_id, guint32 p_offset);

// Child-attributes attributes
guint32 gfire_proto_read_attr_children_count_ss(const guint8 *p_buff, guint8 *p_dest, const gchar *p_name, guint32 p_offset);
guint32 gfire_proto_read_attr_children_count_bs(const guint8 *p_buff, guint8 *p_dest, guint8 p_id, guint32 p_offset);

// Writing attributes
guint32 gfire_proto_write_attr_ss(const gchar *p_name, guint8 p_type, const void *p_data, guint16 p_size, guint32 p_offset);
guint32 gfire_proto_write_attr_bs(guint8 p_id, guint8 p_type, const void *p_data, guint16 p_size, guint32 p_offset);
guint32 gfire_proto_write_attr_list_ss(const gchar *p_name, GList *p_list, guint8 p_type, guint16 p_typelen, guint32 p_offset);
guint32 gfire_proto_write_attr_list_bs(guint8 p_id, GList *p_list, guint8 p_type, guint16 p_typelen, guint32 p_offset);
guint32 gfire_proto_write_header(guint16 p_length, guint16 p_type, guint8 p_atts, guint32 p_offset);
guint32 gfire_proto_write_header32(guint32 p_length, guint16 p_type, guint8 p_atts, guint32 p_offset);

#endif // _GF_PROTOCOL_H
