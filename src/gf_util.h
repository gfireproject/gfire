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

#ifndef _GF_UTIL_H
#define _GF_UTIL_H

typedef struct _gfire_bitlist gfire_bitlist;

#include "gf_base.h"

// Utils for the server browser
gchar *gfire_remove_quake3_color_codes(const gchar *p_string);

// Utils for filetype detection
gboolean gfire_filetype_use_wine(const gchar *p_path);

// Utils for handling HTML
gchar *gfire_escape_html(const gchar *p_html);

// Utils for handling lists
void gfire_list_clear(GList *p_list);

// Utils for handling strings
gchar *gfire_strip_character_range(gchar *p_string, gchar p_start, gchar p_end);
void gfire_strip_invalid_utf8(gchar *p_utf8);

// Utils for SHA-1 hashing
void hashSha1(const gchar *p_input, gchar *p_digest);
void hashSha1_to_bin(const gchar *p_input, guint8 *p_digest);
void hashSha1_bin(const guchar *p_input, int p_len, guchar *p_digest);
void hashSha1_bin_to_str(const guchar *p_input, int p_len, gchar *p_digest);
void hashSha1_file_to_str(FILE *p_file, gchar *p_digest);

// Utils for Hex<->String conversion
gchar *gfire_hex_bin_to_str(guint8 *p_data, guint32 p_len);
guint8 *gfire_hex_str_to_bin(const gchar *p_str);

// Utils for checksums
guint32 gfire_crc32(const void *p_data, guint32 p_len);

// Dynamic Bitlist
struct _gfire_bitlist
{
	guint8 *data;
	guint32 size;
	guint32 bits_set;
};

gfire_bitlist *gfire_bitlist_new();
void gfire_bitlist_free(gfire_bitlist *p_list);
gboolean gfire_bitlist_get(const gfire_bitlist *p_list, guint32 p_index);
void gfire_bitlist_set(gfire_bitlist *p_list, guint32 p_index, gboolean p_isset);
guint32 gfire_bitlist_bits_set(const gfire_bitlist *p_list);
guint32 gfire_bitlist_bits_unset(const gfire_bitlist *p_list);
void gfire_bitlist_clear(gfire_bitlist *p_list);

// Notification system
#if defined(HAVE_GTK) && defined(HAVE_LIBNOTIFY)
#	define USE_NOTIFICATIONS 1
void gfire_notify_uninit();
void gfire_notify_system(const gchar *p_title, const gchar *p_msg);
void gfire_notify_buddy(PurpleBuddy *p_buddy, const gchar *p_title, const gchar *p_msg);
#else
#undef USE_NOTIFICATIONS
#endif // HAVE_GTK && HAVE_LIBNOTIFY

#endif // _GF_UTIL_H
