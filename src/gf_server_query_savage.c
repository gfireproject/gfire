/*
 * purple - Xfire Protocol Plugin
 *
 * Copyright (C) 2005-2006, Beat Wolf <asraniel@fryx.ch>
 * Copyright (C) 2006,      Keith Geffert <keith@penguingurus.com>
 * Copyright (C) 2008-2009	Laurent De Marez <laurentdemarez@gmail.com>
 * Copyright (C) 2009-2010  Warren Dumortier <nwarrenfl@gmail.com>
 * Copyright (C) 2009-2011  Oliver Ney <oliver@dryder.de>
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

#include "gf_server_query.h"

#ifdef HAVE_GTK

typedef struct _gfire_sq_savage_data
{
	GData *info;
	gchar **players;
} gfire_sq_savage_data;

// Savage character based color code
static const struct { gchar ch; const gchar *hex; } savagecodes[] =
{
	// w would actually be FFFFFF, but that's not readable and commonly used by Savage
	{ 'w', "CCCCCC" }, { 'r', "FF4C4C" }, { 'y', "FFFF19" }, { 'g', "008000" },
	{ 'k', "000000" }, { 'c', "00FFFF" }, { 'b', "4C4CFF" }, { 'm', "FF00FF" },
	{ 0, NULL }
};

static const struct { gchar ch; const gchar *hex; } savagenumcodes[] =
{
	{ '0', "00" }, { '1', "1C" }, { '2', "38" }, { '3', "54" },
	{ '4', "70" }, { '5', "8C" }, { '6', "A8" }, { '7', "C4" },
	{ '8', "E0" }, { '9', "FF" }, { 0, NULL }
};

// Prototypes
static void gfire_sq_savage_query(gfire_game_server *p_server, gboolean p_full, int p_socket);
static gboolean gfire_sq_savage_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len);
static gchar *gfire_sq_savage_details(gfire_game_server *p_server);
static void gfire_sq_savage_free_server(gfire_game_server *p_server);

gfire_server_query_driver gf_sq_savage_driver =
{
	gfire_sq_savage_query,
	gfire_sq_savage_parse,
	gfire_sq_savage_details,
	gfire_sq_savage_free_server,
	3
};

static void gfire_sq_savage_query(gfire_game_server *p_server, gboolean p_full, int p_socket)
{
	static guint8 query[] = { 0x9e, 0x4c, 0x23, 0x00, 0x00, 0xce, 'G', 'F', 'S', 'Q' };

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = g_htonl(p_server->ip);
	addr.sin_port = g_htons(p_server->query_port);

	sendto(p_socket, query, 10, 0, (struct sockaddr*)&addr, sizeof(addr));
}

static gchar *gfire_sq_savage_strip_color_codes(const gchar *p_string)
{
	GString *ret = g_string_new(NULL);

	int pos = 0;
	while(p_string[pos])
	{
		if(p_string[pos] == '^')
		{
			if(p_string[pos + 1])
			{
				switch(p_string[pos + 1])
				{
				case 'w':
				case 'r':
				case 'y':
				case 'g':
				case 'k':
				case 'c':
				case 'b':
				case 'm':
					pos += 2;
					continue;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if(p_string[pos + 2] && g_ascii_isdigit(p_string[pos + 2]) &&
							p_string[pos + 3] && g_ascii_isdigit(p_string[pos + 3]))
					{
						pos += 4;
						continue;
					}
				default:
					g_string_append_c(ret, '^');
					break;
				}

				pos++;
				continue;
			}
		}

		g_string_append_c(ret, p_string[pos]);
		pos++;
	}

	return g_string_free(ret, FALSE);
}

static gboolean gfire_sq_savage_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len)
{
	static guint8 check[] = { 0x9e, 0x4c, 0x23, 0x00, 0x00, 0xcf };

	if(p_len < 12 || memcmp(p_data, check, 6) != 0 || memcmp(p_data + 7, "GFSQ", 4) != 0)
		return FALSE;

	p_server->data = g_new0(gfire_game_server_data, 1);
	p_server->data->driver = &gf_sq_savage_driver;
	p_server->data->ping = p_ping;

	gfire_sq_savage_data *gsdata = p_server->data->proto_data = g_new0(gfire_sq_savage_data, 1);
	g_datalist_init(&gsdata->info);

	// Parse
	const gchar *section = (const gchar*)p_data + 12;
	guint offset = 0;

	while(offset < p_len)
	{
		gchar **chunks = g_strsplit(section, "\xff", -1);
		gchar **pos = chunks;
		while(*pos)
		{
			char **pieces = g_strsplit(*pos, "\xfe", -1);
			if(g_strv_length(pieces) != 2)
			{
				g_strfreev(pieces);
				pos++;
				continue;
			}

			if(g_strcmp0(pieces[0], "players") == 0)
				gsdata->players = g_strsplit(pieces[1], "\n", -1);
			else
				g_datalist_set_data_full(&gsdata->info, pieces[0], g_strdup(pieces[1]), g_free);

			g_strfreev(pieces);
			pos++;
		}
		g_strfreev(chunks);

		offset += strlen(section) + 1;
		section = (const gchar*)p_data + offset + 2; // skip 0x00 and 0xff
	}

	if(g_datalist_get_data(&gsdata->info, "name"))
		p_server->data->name = gfire_sq_savage_strip_color_codes(g_datalist_get_data(&gsdata->info, "name"));
	if(g_datalist_get_data(&gsdata->info, "world"))
		p_server->data->map = g_strdup(g_datalist_get_data(&gsdata->info, "world"));
	if(g_datalist_get_data(&gsdata->info, "cnum"))
		sscanf(g_datalist_get_data(&gsdata->info, "cnum"), "%u", &p_server->data->players);
	if(g_datalist_get_data(&gsdata->info, "cmax"))
		sscanf(g_datalist_get_data(&gsdata->info, "cmax"), "%u", &p_server->data->max_players);

	return FALSE;
}

static const gchar *gfire_sq_savage_color_hex_for_digit(gchar p_ch)
{
	int i = 0;
	while(savagenumcodes[i].ch)
	{
		if(savagenumcodes[i].ch == p_ch)
			return savagenumcodes[i].hex;
		i++;
	}
	return NULL;
}

static gchar *gfire_sq_savage_color_codes_to_html(const gchar *p_str)
{
	if(!p_str)
		return NULL;

	gchar **chunks = g_strsplit(p_str, "^", -1);

	GString *str = g_string_new(NULL);
	int i = 0;
	for(; i < g_strv_length(chunks); i++)
	{
		if(i > 0)
		{
			int j = 0;
			while(savagecodes[j].ch)
			{
				if(chunks[i][0] == savagecodes[j].ch)
				{
					g_string_append_printf(str, "<font color=\"#%s\">%s</font>", savagecodes[j].hex, chunks[i] + 1);
					break;
				}
				j++;
			}
			if(!savagecodes[j].ch)
			{
				if(g_ascii_isdigit(chunks[i][0]) &&
						chunks[i][1] && g_ascii_isdigit(chunks[i][1]) &&
						chunks[i][2] && g_ascii_isdigit(chunks[i][2]))
				{
					g_string_append_printf(str, "<font color=\"#%s%s%s\">%s</font>",
										   gfire_sq_savage_color_hex_for_digit(chunks[i][0]),
										   gfire_sq_savage_color_hex_for_digit(chunks[i][1]),
										   gfire_sq_savage_color_hex_for_digit(chunks[i][2]),
										   chunks[i] + 3);
				}
				else
					g_string_append(g_string_append_c(str, '^'), chunks[i]);
			}
		}
		else
			g_string_append(str, chunks[i]);
	}

	g_strfreev(chunks);
	return g_string_free(str, FALSE);
}

static void gfire_sq_savage_details_vars(GQuark p_key, gpointer p_data, gpointer p_udata)
{
	if(g_strcmp0(g_quark_to_string(p_key), "players") != 0)
	{
		gchar *escaped = gfire_escape_html((gchar*)p_data);
		gchar *html = gfire_sq_savage_color_codes_to_html(escaped);
		g_free(escaped);
		g_string_append_printf((GString*)p_udata, "<b>%s:</b> %s<br>", g_quark_to_string(p_key), html);
		g_free(html);
	}
}

static gchar *gfire_sq_savage_details(gfire_game_server *p_server)
{
	GString *str = g_string_new(NULL);
	gchar *escaped;
	gchar *html;

	gfire_sq_savage_data *data = (gfire_sq_savage_data*)p_server->data->proto_data;

	// General server infos
	g_string_append(str, _("<b><font size=\"5\">General Server Details:</font></b><br>"));

	//	Server Name
	if(g_datalist_get_data(&data->info, "name"))
	{
		escaped = gfire_escape_html(g_datalist_get_data(&data->info, "name"));
		html = gfire_sq_savage_color_codes_to_html(escaped);
		g_free(escaped);
		g_string_append_printf(str, _("<b>Server Name:</b> %s<br>"), html);
		g_free(html);
	}
	else
		g_string_append_printf(str, _("<b>Server Name:</b> %s<br>"), _("N/A"));
	//	Players
	g_string_append_printf(str, _("<b>Players:</b> %u/%u<br>"), p_server->data->players, p_server->data->max_players);
	//	Map
	escaped = gfire_escape_html(p_server->data->map);
	g_string_append_printf(str, _("<b>Map:</b> %s<br>"), escaped);
	g_free(escaped);
	//	Password secured
	g_string_append_printf(str, _("<b>Password secured:</b> %s<br>"), (g_strcmp0("1", g_datalist_get_data(&data->info, "pass")) == 0)
						   ? _("Yes") : _("No"));
	// Game Type
	if(g_datalist_get_data(&data->info, "gametype"))
	{
		escaped = gfire_escape_html(g_datalist_get_data(&data->info, "gametype"));
		html = gfire_sq_savage_color_codes_to_html(escaped);
		g_free(escaped);
		g_string_append_printf(str, _("<b>Game Type:</b> %s<br>"), html);
		g_free(html);
	}
	else
		g_string_append_printf(str, _("<b>Game Type:</b> %s<br>"), _("N/A"));
	// Version
	if(g_datalist_get_data(&data->info, "ver"))
	{
		escaped = gfire_escape_html(g_datalist_get_data(&data->info, "ver"));
		html = gfire_sq_savage_color_codes_to_html(escaped);
		g_free(escaped);
		g_string_append_printf(str, _("<b>Version:</b> %s"), html);
		g_free(html);
	}
	else
		g_string_append_printf(str, _("<b>Version:</b> %s"), _("N/A"));

	// Players
	g_string_append(str, _("<br><br><b><font size=\"5\">Players:</font></b><br>"));
	gchar **cur = data->players;
	while(cur && *cur)
	{
		escaped = gfire_escape_html(*cur);
		html = gfire_sq_savage_color_codes_to_html(escaped);
		g_free(escaped);

		g_string_append_printf(str, "%s<br>", html);
		g_free(html);

		cur++;
	}

	// Other server rules
	g_string_append(str, _("<br></font><b><font size=\"5\">All Server Info:</font></b><br>"));
	g_datalist_foreach(&data->info, gfire_sq_savage_details_vars, str);

	return g_string_free(str, FALSE);
}

static void gfire_sq_savage_free_server(gfire_game_server *p_server)
{
	if(p_server->data && p_server->data->proto_data)
	{
		gfire_sq_savage_data *data = (gfire_sq_savage_data*)p_server->data->proto_data;

		g_datalist_clear(&data->info);
		g_strfreev(data->players);

		g_free(data);
	}
}

#endif // HAVE_GTK
