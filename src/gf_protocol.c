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

#include "gf_network.h"
#include "gf_protocol.h"

guint32 gfire_proto_check_attribute_ss(const guint8 *p_buff, const gchar *p_name, guint8 p_type, guint32 p_offset)
{
	if(!p_buff || !p_name)
		return -1;

	guint8 attr_name_len = *((guint8*)(p_buff + p_offset));
	p_offset += sizeof(attr_name_len);

	gchar attr_name[attr_name_len + 1];
	memcpy(attr_name, p_buff + p_offset, attr_name_len);
	attr_name[attr_name_len] = 0;

	p_offset += attr_name_len;

	if(g_ascii_strcasecmp(p_name, attr_name) != 0)
		return -1;

	guint8 attr_type = *((guint8*)(p_buff + p_offset));
	p_offset += sizeof(attr_type);

	if(attr_type != p_type)
		return -1;

	return p_offset;
}

guint32 gfire_proto_check_attribute_bs(const guint8 *p_buff, guint8 p_id, guint8 p_type, guint32 p_offset)
{
	if(!p_buff)
		return -1;

	guint8 attr_id = *((guint8*)(p_buff + p_offset));
	p_offset += sizeof(attr_id);

	if(attr_id != p_id)
		return -1;

	guint8 attr_type = *((guint8*)(p_buff + p_offset));
	p_offset += sizeof(attr_type);

	if(attr_type != p_type)
		return -1;

	return p_offset;
}

static guint32 gfire_proto_read_string_value(const guint8 *p_buff, gchar **p_dest, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	guint16 value_len = GUINT16_FROM_LE(*(guint16*)(p_buff + p_offset));
	p_offset += sizeof(value_len);

	*p_dest = g_malloc0(value_len + 1);
	if(!*p_dest)
		return -1;

	if(value_len > 0)
		memcpy(*p_dest, p_buff + p_offset, value_len);

	(*p_dest)[value_len] = 0;

	p_offset += value_len;

	return p_offset;
}

guint32 gfire_proto_read_attr_string_ss(const guint8 *p_buff, gchar **p_dest, const gchar *p_name, guint32 p_offset)
{
	if(!p_dest || !p_name || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_ss(p_buff, p_name, 0x01, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_string_value(p_buff, p_dest, p_offset);

	return p_offset;
}

guint32 gfire_proto_read_attr_string_bs(const guint8 *p_buff, gchar **p_dest, guint8 p_id, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_bs(p_buff, p_id, 0x01, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_string_value(p_buff, p_dest, p_offset);

	return p_offset;
}

static guint32 gfire_proto_read_int32_value(const guint8 *p_buff, guint32 *p_dest, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	memcpy(p_dest, p_buff + p_offset, sizeof(*p_dest));
	*p_dest = GUINT32_FROM_LE(*(guint32*)(p_buff + p_offset));
	p_offset += sizeof(*p_dest);

	return p_offset;
}

guint32 gfire_proto_read_attr_int32_ss(const guint8 *p_buff, guint32 *p_dest, const gchar *p_name, guint32 p_offset)
{
	if(!p_dest || !p_name || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_ss(p_buff, p_name, 0x02, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_int32_value(p_buff, p_dest, p_offset);

	return p_offset;
}

guint32 gfire_proto_read_attr_int32_bs(const guint8 *p_buff, guint32 *p_dest, guint8 p_id, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_bs(p_buff, p_id, 0x02, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_int32_value(p_buff, p_dest, p_offset);

	return p_offset;
}

static guint32 gfire_proto_read_sid_value(const guint8 *p_buff, guint8 **p_dest, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	*p_dest = g_malloc0(XFIRE_SID_LEN);
	if(!*p_dest)
		return -1;

	memcpy(*p_dest, p_buff + p_offset, XFIRE_SID_LEN);
	p_offset += XFIRE_SID_LEN;

	return p_offset;
}

guint32 gfire_proto_read_attr_sid_ss(const guint8 *p_buff, guint8 **p_dest, const gchar *p_name, guint32 p_offset)
{
	if(!p_dest || !p_name || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_ss(p_buff, p_name, 0x03, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_sid_value(p_buff, p_dest, p_offset);

	return p_offset;
}

guint32 gfire_proto_read_attr_sid_bs(const guint8 *p_buff, guint8 **p_dest, guint8 p_id, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_bs(p_buff, p_id, 0x03, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_sid_value(p_buff, p_dest, p_offset);

	return p_offset;
}

static guint32 gfire_proto_read_chatid_value(const guint8 *p_buff, guint8 **p_dest, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	*p_dest = g_malloc0(XFIRE_CHATID_LEN);
	if(!*p_dest)
		return -1;

	memcpy(*p_dest, p_buff + p_offset, XFIRE_CHATID_LEN);
	p_offset += XFIRE_CHATID_LEN;

	return p_offset;
}

guint32 gfire_proto_read_attr_chatid_ss(const guint8 *p_buff, guint8 **p_dest, const gchar *p_name, guint32 p_offset)
{
	if(!p_dest || !p_name || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_ss(p_buff, p_name, 0x06, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_chatid_value(p_buff, p_dest, p_offset);

	return p_offset;
}

guint32 gfire_proto_read_attr_chatid_bs(const guint8 *p_buff, guint8 **p_dest, guint8 p_id, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_bs(p_buff, p_id, 0x06, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_chatid_value(p_buff, p_dest, p_offset);

	return p_offset;
}

static guint32 gfire_proto_read_int64_value(const guint8 *p_buff, guint64 *p_dest, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	*p_dest = GUINT64_FROM_LE(*(guint64*)(p_buff + p_offset));
	p_offset += sizeof(*p_dest);

	return p_offset;
}

guint32 gfire_proto_read_attr_int64_ss(const guint8 *p_buff, guint64 *p_dest, const gchar *p_name, guint32 p_offset)
{
	if(!p_dest || !p_name || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_ss(p_buff, p_name, 0x07, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_int64_value(p_buff, p_dest, p_offset);

	return p_offset;
}

guint32 gfire_proto_read_attr_int64_bs(const guint8 *p_buff, guint64 *p_dest, guint8 p_id, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_bs(p_buff, p_id, 0x07, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_int64_value(p_buff, p_dest, p_offset);

	return p_offset;
}

static guint32 gfire_proto_read_boolean_value(const guint8 *p_buff, gboolean *p_dest, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	guint8 value = *(guint8*)(p_buff + p_offset);
	if(value != 0)
		*p_dest = TRUE;
	else
		*p_dest = FALSE;

	p_offset += sizeof(*p_dest);

	return p_offset;
}

guint32 gfire_proto_read_attr_boolean_ss(const guint8 *p_buff, gboolean *p_dest, const gchar *p_name, guint32 p_offset)
{
	if(!p_dest || !p_name || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_ss(p_buff, p_name, 0x08, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_boolean_value(p_buff, p_dest, p_offset);

	return p_offset;
}

guint32 gfire_proto_read_attr_boolean_bs(const guint8 *p_buff, gboolean *p_dest, guint8 p_id, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_bs(p_buff, p_id, 0x08, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_boolean_value(p_buff, p_dest, p_offset);

	return p_offset;
}

static guint32 gfire_proto_read_list_value(const guint8 *p_buff, GList **p_dest, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	GList *cur = NULL;

	guint8 element_type = *((guint8*)(p_buff + p_offset));
	p_offset += sizeof(element_type);

	guint16 element_count = *((guint16*)(p_buff + p_offset));
	p_offset += sizeof(element_count);

	guint16 i = 0;
	for(; i < element_count; i++)
	{
		list_type_pointers data;
		switch(element_type)
		{
			case 0x01:
				p_offset = gfire_proto_read_string_value(p_buff, &data.pchar, p_offset);
				if(p_offset == -1)
					goto error;
			break;
			case 0x02:
				data.puint32 = (guint32*)g_malloc0(sizeof(guint32));
				p_offset = gfire_proto_read_int32_value(p_buff, data.puint32, p_offset);
				if(p_offset == -1)
				{
					g_free(data.puint8);
					goto error;
				}
			break;
			case 0x03:
				p_offset = gfire_proto_read_sid_value(p_buff, &data.puint8, p_offset);
				if(p_offset == -1)
					goto error;
			break;
			case 0x04:
				p_offset = gfire_proto_read_list_value(p_buff, &data.plist, p_offset);
				if(p_offset == -1)
					goto error;
			break;
			case 0x06:
				p_offset = gfire_proto_read_chatid_value(p_buff, &data.puint8, p_offset);
				if(p_offset == -1)
					goto error;
			break;
			case 0x07:
				data.puint64 = (guint64*)g_malloc0(sizeof(guint64));
				p_offset = gfire_proto_read_int64_value(p_buff, data.puint64, p_offset);
				if(p_offset == -1)
				{
					g_free(data.puint8);
					goto error;
				}
			break;
			case 0x08:
				data.pboolean = (gboolean*)g_malloc0(sizeof(gboolean));
				p_offset = gfire_proto_read_boolean_value(p_buff, data.pboolean, p_offset);
				if(p_offset == -1)
				{
					g_free(data.puint8);
					goto error;
				}
			break;
		}
		*p_dest = g_list_append(*p_dest, data.puint8);
	}

	return p_offset;

error:
	cur = *p_dest;
	for(; cur; cur = g_list_next(cur))
		if(cur->data) g_free(cur->data);
	g_list_free(*p_dest);
	*p_dest = NULL;
	return -1;
}

guint32 gfire_proto_read_attr_list_ss(const guint8 *p_buff, GList **p_dest, const gchar *p_name, guint32 p_offset)
{
	if(!p_dest || !p_name || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_ss(p_buff, p_name, 0x04, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_list_value(p_buff, p_dest, p_offset);

	return p_offset;
}

guint32 gfire_proto_read_attr_list_bs(const guint8 *p_buff, GList **p_dest, guint8 p_id, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_bs(p_buff, p_id, 0x04, p_offset);
	if(p_offset == -1)
		return p_offset;

	p_offset = gfire_proto_read_list_value(p_buff, p_dest, p_offset);

	return p_offset;
}

guint32 gfire_proto_read_attr_children_count_ss(const guint8 *p_buff, guint8 *p_dest, const gchar *p_name, guint32 p_offset)
{
	if(!p_dest || !p_name || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_ss(p_buff, p_name, 0x05, p_offset);
	if(p_offset == -1)
		return p_offset;

	*p_dest = *((guint8*)(p_buff + p_offset));
	p_offset += sizeof(*p_dest);

	return p_offset;
}

guint32 gfire_proto_read_attr_children_count_bs(const guint8 *p_buff, guint8 *p_dest, guint8 p_id, guint32 p_offset)
{
	if(!p_dest || !p_buff)
		return -1;

	p_offset = gfire_proto_check_attribute_bs(p_buff, p_id, 0x09, p_offset);
	if(p_offset == -1)
		return p_offset;

	*p_dest = *((guint8*)(p_buff + p_offset));
	p_offset += sizeof(*p_dest);

	return p_offset;
}

guint32 gfire_proto_write_attr_ss(const gchar *p_name, guint8 p_type, const void *p_data, guint16 p_size, guint32 p_offset)
{
	if(!p_name)
		return -1;

	// attributeLength
	guint8 attr_len = strlen(p_name);
	gfire_network_buffout_write(&attr_len, sizeof(attr_len), p_offset);
	p_offset += sizeof(attr_len);

	// attributeName
	gfire_network_buffout_write(p_name, attr_len, p_offset);
	p_offset += attr_len;

	// attributeType
	gfire_network_buffout_write(&p_type, sizeof(p_type), p_offset);
	p_offset += sizeof(p_type);

	// Additional length
	if(p_type == 0x01)
	{
		p_size = GUINT16_TO_LE(p_size);
		gfire_network_buffout_write(&p_size, sizeof(p_size), p_offset);
		p_offset += sizeof(p_size);
	}
	else if(p_type == 0x05 || p_type == 0x09)
	{
		guint8 size = p_size;
		gfire_network_buffout_write(&size, sizeof(size), p_offset);
		p_offset += sizeof(size);
	}

	// Content
	if(p_size > 0 && p_data)
	{
		gfire_network_buffout_write(p_data, p_size, p_offset);
		p_offset += p_size;
	}

	return p_offset;
}

guint32 gfire_proto_write_attr_bs(guint8 p_id, guint8 p_type, const void *p_data, guint16 p_size, guint32 p_offset)
{
	// attributeID
	gfire_network_buffout_write(&p_id, sizeof(p_id), p_offset);
	p_offset += sizeof(p_id);

	// attributeType
	gfire_network_buffout_write(&p_type, sizeof(p_type), p_offset);
	p_offset += sizeof(p_type);

	// Additional length
	if(p_type == 0x01)
	{
		p_size = GUINT16_TO_LE(p_size);
		gfire_network_buffout_write(&p_size, sizeof(p_size), p_offset);
		p_offset += sizeof(p_size);
	}
	else if(p_type == 0x05 || p_type == 0x09)
	{
		guint8 size = p_size;
		gfire_network_buffout_write(&size, sizeof(size), p_offset);
		p_offset += sizeof(size);
	}

	// Content
	if(p_size > 0 && p_data)
	{
		gfire_network_buffout_write(p_data, p_size, p_offset);
		p_offset += p_size;
	}

	return p_offset;
}

static guint32 gfire_proto_write_attr_list(GList *p_list, guint8 p_type, guint16 p_len, guint32 p_offset)
{
	gfire_network_buffout_write(&p_type, sizeof(p_type), p_offset);
	p_offset += sizeof(p_type);

	guint16 amount = GUINT16_TO_LE(g_list_length(p_list));
	gfire_network_buffout_write(&amount, sizeof(amount), p_offset);
	p_offset += sizeof(amount);

	GList *cur = p_list;
	for(; cur; cur = g_list_next(cur))
	{
		if(p_type == 0x01)
		{
			guint16 str_len = GUINT16_TO_LE(strlen((gchar*)cur->data));
			gfire_network_buffout_write(&str_len, sizeof(str_len), p_offset);
			p_offset += sizeof(str_len);

			gfire_network_buffout_write(cur->data, GUINT16_FROM_LE(str_len), p_offset);
			p_offset += GUINT16_FROM_LE(str_len);

			continue;
		}

		gfire_network_buffout_write(cur->data, p_len, p_offset);
		p_offset += p_len;
	}

	return p_offset;
}

guint32 gfire_proto_write_attr_list_ss(const gchar *p_name, GList *p_list, guint8 p_type, guint16 p_typelen, guint32 p_offset)
{
	if(!p_name)
		return -1;

	// attributeLength
	guint8 attr_len = strlen(p_name);
	gfire_network_buffout_write(&attr_len, sizeof(attr_len), p_offset);
	p_offset += sizeof(attr_len);

	// attributeName
	gfire_network_buffout_write(p_name, attr_len, p_offset);
	p_offset += attr_len;

	// attributeType
	guint8 type = 0x04;
	gfire_network_buffout_write(&type, sizeof(type), p_offset);
	p_offset += sizeof(type);

	p_offset = gfire_proto_write_attr_list(p_list, p_type, p_typelen, p_offset);

	return p_offset;
}

guint32 gfire_proto_write_attr_list_bs(guint8 p_id, GList *p_list, guint8 p_type, guint16 p_typelen, guint32 p_offset)
{
	// attributeID
	gfire_network_buffout_write(&p_id, sizeof(p_id), p_offset);
	p_offset += sizeof(p_id);

	// attributeType
	guint8 type = 0x04;
	gfire_network_buffout_write(&type, sizeof(type), p_offset);
	p_offset += sizeof(type);

	p_offset = gfire_proto_write_attr_list(p_list, p_type, p_typelen, p_offset);

	return p_offset;
}

guint32 gfire_proto_write_header(guint16 p_length, guint16 p_type, guint8 p_atts, guint32 p_offset)
{
	p_length = GUINT16_TO_LE(p_length);
	gfire_network_buffout_write(&p_length, sizeof(p_length), p_offset);
	p_offset += sizeof(p_length);

	p_type = GUINT16_TO_LE(p_type);
	gfire_network_buffout_write(&p_type, sizeof(p_type), p_offset);
	p_offset += sizeof(p_type);

	gfire_network_buffout_write(&p_atts, sizeof(p_atts), p_offset);
	p_offset += sizeof(p_atts);

	return p_offset;
}
