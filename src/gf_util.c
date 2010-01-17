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

#include "gf_util.h"

gchar *gfire_remove_quake3_color_codes(const gchar *p_string)
{
	if(!p_string)
		return NULL;

	const gchar color_codes[] =
	{
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		'U', 'V', 'W', 'X', 'Y', 'Z',
		'?', '+', '@', '-', '_', '/', '&', '(', '>', '.'
	};

	gchar *escaped = g_strdup(p_string);

	int i;
	gchar code[3];
	gchar *tmp = NULL;

	for (i = 0; i < sizeof(color_codes); i++)
	{
		g_snprintf(code, 3, "^%c", color_codes[i]);
		tmp = purple_strcasereplace(escaped, code, "");
		g_free(escaped);
		escaped = tmp;
	}

	return escaped;
}

gchar *gfire_escape_html(const gchar *p_html)
{
	if(p_html != NULL)
	{
		gchar *tmp = NULL;
		gchar *tmp2 = NULL;

		tmp = purple_strreplace(p_html, "&", "&amp;");

		tmp2 = purple_strreplace(tmp, "<", "&lt;");
		if(tmp) g_free(tmp);

		tmp = purple_strreplace(tmp2, ">", "&gt;");
		if(tmp2) g_free(tmp2);

		tmp2 = purple_strreplace(tmp, "\"", "&quot;");
		if(tmp) g_free(tmp);

		tmp = purple_strreplace(tmp2, "\n", "<br />");
		if(tmp2) g_free(tmp2);

		return tmp;
	}
	else
		return NULL;
}

void gfire_list_clear(GList *p_list)
{
	if(!p_list)
		return;

	GList *cur = g_list_first(p_list);
	for(; cur; cur = g_list_next(cur))
		if(cur->data) g_free(cur->data);

	g_list_free(p_list);
}

gchar *gfire_strip_character_range(gchar *p_string, gchar p_start, gchar p_end)
{
	if(!p_string)
		return NULL;

	int i;
	int len = strlen(p_string);

	for(i = 0; i < len; i++)
	{
		if((p_string[i] >= p_start) && (p_string[i] <= p_end))
		{
			memcpy(&p_string[i], &p_string[i + 1], len - i);
			i--;
			len--;
		}
	}

	return p_string;
}

void hashSha1(const gchar *p_input, gchar *p_digest)
//based on code from purple_util_get_image_filename in the pidgin 2.2.0 source
{
	if(!p_digest)
		return;

	PurpleCipherContext *context;

	context = purple_cipher_context_new_by_name("sha1", NULL);
	if (context == NULL)
	{
		purple_debug_error("gfire", "Could not find sha1 cipher\n");
		return;
	}

	purple_cipher_context_append(context, (guchar*)p_input, strlen(p_input));
	if (!purple_cipher_context_digest_to_str(context, 41, p_digest, NULL))
	{
		purple_debug_error("gfire", "Failed to get SHA-1 digest.\n");
		return;
	}
	purple_cipher_context_destroy(context);
	p_digest[40]=0;
}

void hashSha1_bin(const guchar *p_input, int p_len, guchar *p_digest)
{
	if(!p_digest)
		return;

	PurpleCipherContext *context;

	context = purple_cipher_context_new_by_name("sha1", NULL);
	if (context == NULL)
	{
		purple_debug_error("gfire", "Could not find sha1 cipher\n");
		return;
	}

	purple_cipher_context_append(context, (guchar*)p_input, p_len);
	if(!purple_cipher_context_digest(context, 20, p_digest, NULL))
	{
		purple_debug_error("gfire", "Failed to get SHA-1 digest.\n");
		return;
	}
	purple_cipher_context_destroy(context);
}

void hashSha1_bin_to_str(const guchar *p_input, int p_len, gchar *p_digest)
{
	if(!p_digest)
		return;

	PurpleCipherContext *context;

	context = purple_cipher_context_new_by_name("sha1", NULL);
	if (context == NULL)
	{
		purple_debug_error("gfire", "Could not find sha1 cipher\n");
		return;
	}

	purple_cipher_context_append(context, (guchar*)p_input, p_len);
	if(!purple_cipher_context_digest_to_str(context, 41, p_digest, NULL))
	{
		purple_debug_error("gfire", "Failed to get SHA-1 digest.\n");
		return;
	}
	purple_cipher_context_destroy(context);
}

void hashSha1_file_to_str(FILE *p_file, gchar *p_digest)
{
	if(!p_file || !p_digest)
		return;

	PurpleCipherContext *context;

	context = purple_cipher_context_new_by_name("sha1", NULL);
	if (context == NULL)
	{
		purple_debug_error("gfire", "Could not find sha1 cipher\n");
		return;
	}

	guchar *buf = g_malloc0(256 * 1024); // 256KB
	fseek(p_file, 0, SEEK_SET);
	while(!feof(p_file))
	{
		size_t bytes_read = fread(buf, 1, 256 * 1024, p_file);
		purple_cipher_context_append(context, buf, bytes_read);
	}
	g_free(buf);

	if(!purple_cipher_context_digest_to_str(context, 41, p_digest, NULL))
	{
		purple_debug_error("gfire", "Failed to get SHA-1 digest of file.\n");
		return;
	}
	purple_cipher_context_destroy(context);
}

void hashSha1_to_bin(const gchar *p_input, guint8 *p_digest)
{
	if(!p_digest)
		return;

	PurpleCipherContext *context;

	context = purple_cipher_context_new_by_name("sha1", NULL);
	if (context == NULL)
	{
		purple_debug_error("gfire", "Could not find sha1 cipher\n");
		return;
	}

	purple_cipher_context_append(context, (guchar*)p_input, strlen(p_input));
	if (!purple_cipher_context_digest(context, 20, p_digest, NULL))
	{
		purple_debug_error("gfire", "Failed to get SHA-1 digest.\n");
		return;
	}
	purple_cipher_context_destroy(context);
}


gchar *gfire_hex_bin_to_str(guint8 *p_data, guint32 p_len)
{
	if(!p_data || !p_len)
		return NULL;

	gchar *ret = g_malloc0(p_len * 2 + 1);
	guint32 i;
	for(i = 0; i < p_len; i++)
		g_sprintf(ret + (i * 2), "%02x", p_data[i]);

	return ret;
}

guint8 *gfire_hex_str_to_bin(const gchar *p_str)
{
	if(!p_str)
		return NULL;

	guint8 *ret = g_malloc0(strlen(p_str) / 2);
	if(!ret)
		return NULL;

	int bin_pos = 0;
	int i = 0;
	for(; i < strlen(p_str); i++)
	{
		int value = 0;
		switch(p_str[i])
		{
		case '0':
			value += 0x00;
			break;
		case '1':
			value += 0x01;
			break;
		case '2':
			value += 0x02;
			break;
		case '3':
			value += 0x03;
			break;
		case '4':
			value += 0x04;
			break;
		case '5':
			value += 0x05;
			break;
		case '6':
			value += 0x06;
			break;
		case '7':
			value += 0x07;
			break;
		case '8':
			value += 0x08;
			break;
		case '9':
			value += 0x09;
			break;
		case 'a':
			value += 0x0A;
			break;
		case 'b':
			value += 0x0B;
			break;
		case 'c':
			value += 0x0C;
			break;
		case 'd':
			value += 0x0D;
			break;
		case 'e':
			value += 0x0E;
			break;
		case 'f':
			value += 0x0F;
			break;
		}
		if((i % 2) == 0)
		{
			ret[bin_pos] = value << 4;
		}
		else
		{
			ret[bin_pos] += value;
			bin_pos++;
		}
	}

	return ret;
}

static const guint32 _gfire_crc32_table[256] =
{
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,

	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,

	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,

	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

guint32 gfire_crc32(const void *p_data, guint32 p_len)
{
	if(!p_data || !p_len)
		return 0;

	guint32 crc32 = 0xFFFFFFFF;

	guint32 i;
	for(i = 0; i < p_len; i++)
	{
		crc32 = (crc32 >> 8) ^ _gfire_crc32_table[((guint8*)p_data)[i] ^ (crc32 & 0x000000FF)];
	}

	return ~crc32;
}

gfire_bitlist *gfire_bitlist_new()
{
	gfire_bitlist *ret = g_malloc0(sizeof(gfire_bitlist));
	ret->data = g_malloc0(10);
	ret->size = 10;

	return ret;
}

void gfire_bitlist_free(gfire_bitlist *p_list)
{
	if(!p_list)
		return;

	g_free(p_list->data);
	g_free(p_list);
}

gboolean gfire_bitlist_get(const gfire_bitlist *p_list, guint32 p_index)
{
	if(!p_list)
		return FALSE;

	guint32 byte_pos = p_index / 8;
	if(byte_pos >= p_list->size)
		return FALSE;

	return (p_list->data[byte_pos] & (0x01 << (p_index % 8)));
}

void gfire_bitlist_set(gfire_bitlist *p_list, guint32 p_index, gboolean p_isset)
{
	if(!p_list)
		return;

	guint32 byte_pos = p_index / 8;
	if(byte_pos >= p_list->size)
	{
		guint32 oldsize = p_list->size;
		p_list->size = byte_pos + 10;
		p_list->data = g_realloc(p_list->data, p_list->size);
		memset(p_list->data + oldsize, 0, p_list->size - oldsize);
	}

	if(p_isset)
	{
		if(!(p_list->data[byte_pos] & (0x01 << (p_index % 8))))
			p_list->bits_set++;

		p_list->data[byte_pos] |= (guint8)(0x01 << (p_index % 8));
	}
	else
	{
		if(p_list->data[byte_pos] & (0x01 << (p_index % 8)))
		{
			p_list->bits_set--;
			p_list->data[byte_pos] ^= (guint8)(0x01 << (p_index % 8));
		}
	}
}

guint32 gfire_bitlist_bits_set(const gfire_bitlist *p_list)
{
	if(!p_list)
		return 0;

	return p_list->bits_set;
}

guint32 gfire_bitlist_bits_unset(const gfire_bitlist *p_list)
{
	if(!p_list)
		return 0;

	return ((p_list->size * 8) - p_list->bits_set);
}

void gfire_bitlist_clear(gfire_bitlist *p_list)
{
	if(!p_list)
		return;

	p_list->data = g_realloc(p_list->data, 10);
	p_list->size = 0;
	memset(p_list->data, 0, 10);

	p_list->bits_set = 0;
}
