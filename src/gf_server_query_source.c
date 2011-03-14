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

#include "gf_server_query.h"

typedef struct _gfire_sq_source_player
{
	gchar *name;
	gint32 score;
	gfloat time;
} gfire_sq_source_player;

#define GFSQ_SOURCE_STAGE_CHALLENGE	1
#define GFSQ_SOURCE_STAGE_PLAYERS	2
#define GFSQ_SOURCE_STAGE_RULES		3

typedef struct _gfire_sq_source_data
{
	// Query handling
	guint8 query_stage;
	guint32 challenge;

	// General data
	guint8 proto_ver;
	gchar *name;
	gchar *map;
	gchar *game_dir;
	gchar *game_desc;
	guint16 steam_id;
	guint8 num_players;
	guint8 max_players;
	guint8 bots;
	guint8 type;
	guint8 os;
	guint8 password;
	guint8 secure;
	gchar *version;

	// Extended data
	GData *rules;
	GSList *players;
} gfire_sq_source_data;

// Prototypes
static void gfire_sq_source_query(gfire_game_server *p_server, gboolean p_full, int p_socket);
static gboolean gfire_sq_source_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len);
static gchar *gfire_sq_source_details(gfire_game_server *p_server);
static void gfire_sq_source_free_server(gfire_game_server *p_server);

gfire_server_query_driver gf_sq_source_driver =
{
	gfire_sq_source_query,
	gfire_sq_source_parse,
	gfire_sq_source_details,
	gfire_sq_source_free_server,
	5
};

static void gfire_sq_source_query(gfire_game_server *p_server, gboolean p_full, int p_socket)
{
	static const guint8 challengeQuery[] =	{ 0xff, 0xff, 0xff, 0xff, 0x55,
											  0xff, 0xff, 0xff, 0xff };
	static const guint8 detailsQuery[] =	{ 0xff, 0xff, 0xff, 0xff, 0x54,
											  'S', 'o', 'u', 'r', 'c', 'e', ' ',
											  'E', 'n', 'g', 'i', 'n', 'e', ' ',
											  'Q', 'u', 'e', 'r', 'y', 0x00 };
	static guint8 rulesQuery[] =			{ 0xff, 0xff, 0xff, 0xff, 0x56,
											  0x00, 0x00, 0x00, 0x00 };
	static guint8 playersQuery[] =			{ 0xff, 0xff, 0xff, 0xff, 0x55,
											  0x00, 0x00, 0x00, 0x00 };

	const guint8 *query = NULL;
	guint8 len;
	if(!p_server->data)
	{
		len = sizeof(detailsQuery);
		query = detailsQuery;
	}
	else if(p_full)
	{
		gfire_sq_source_data *data = (gfire_sq_source_data*)p_server->data->proto_data;
		switch(data->query_stage)
		{
		case GFSQ_SOURCE_STAGE_CHALLENGE:
			len = sizeof(challengeQuery);
			query = challengeQuery;
			break;
		case GFSQ_SOURCE_STAGE_PLAYERS:
			len = sizeof(playersQuery);
			query = playersQuery;
			memcpy(playersQuery + 5, &data->challenge, 4);
			break;
		case GFSQ_SOURCE_STAGE_RULES:
			len = sizeof(rulesQuery);
			query = rulesQuery;
			memcpy(rulesQuery + 5, &data->challenge, 4);
			break;
		default:
			return;
		}
	}
	else
		return;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = g_htonl(p_server->ip);
	addr.sin_port = g_htons(p_server->query_port);

	sendto(p_socket, query, len, 0, (struct sockaddr*)&addr, sizeof(addr));
}

static gchar *gfire_sq_source_get_string(const unsigned char *p_data, guint p_len, guint16 *p_offset)
{
	GString *str = g_string_new(NULL);
	while(p_data[*p_offset] && *p_offset < p_len)
	{
		g_string_append_c(str, p_data[*p_offset]);
		(*p_offset)++;
	}
	(*p_offset)++;
	return g_string_free(str, FALSE);
}

static gboolean gfire_sq_source_parse_details(gfire_sq_source_data *p_sdata, const unsigned char *p_data, guint p_len)
{
	if(p_len < 20)
		return FALSE;

	static const guint8 detailsReply[] =	{ 0xff, 0xff, 0xff, 0xff, 0x49 };
	if(memcmp(detailsReply, p_data, 5) != 0)
		return FALSE;

	p_sdata->proto_ver = p_data[5];
	guint16 offset = 6;

	// Name
	p_sdata->name = gfire_sq_source_get_string(p_data, p_len, &offset);
	if(offset == p_len)
		goto parse_error;

	// Map
	p_sdata->map = gfire_sq_source_get_string(p_data, p_len, &offset);
	if(offset == p_len)
		goto parse_error;

	// Game directory
	p_sdata->game_dir = gfire_sq_source_get_string(p_data, p_len, &offset);
	if(offset == p_len)
		goto parse_error;

	// Game description
	p_sdata->game_desc = gfire_sq_source_get_string(p_data, p_len, &offset);
	if(offset == p_len || (p_len - offset) < 10)
		goto parse_error;

	// Steam Application ID
	p_sdata->steam_id = GUINT16_FROM_LE(*(guint16*)(p_data + offset));
	offset += 2;

	// Current players
	p_sdata->num_players = *(p_data + offset);
	offset++;

	// Maximum players
	p_sdata->max_players = *(p_data + offset);
	offset++;

	// Bots
	p_sdata->bots = *(p_data + offset);
	offset++;

	// Dedicated server?
	p_sdata->type = *(p_data + offset);
	offset++;

	// OS
	p_sdata->os = *(p_data + offset);
	offset++;

	// Password secured
	p_sdata->password = *(p_data + offset);
	offset++;

	// Secure (VAC)
	p_sdata->secure = *(p_data + offset);
	offset++;

	// Game version
	p_sdata->version = gfire_sq_source_get_string(p_data, p_len, &offset);

	return TRUE;

parse_error:
	g_free(p_sdata->name);
	g_free(p_sdata->map);
	g_free(p_sdata->game_dir);
	g_free(p_sdata->game_desc);
	g_free(p_sdata->version);

	return FALSE;
}

static gboolean gfire_sq_source_parse_challenge(gfire_sq_source_data *p_sdata, const unsigned char *p_data, guint p_len)
{
	if(p_len != 9)
		return FALSE;

	static const guint8 challengeReply[] =	{ 0xff, 0xff, 0xff, 0xff, 0x41 };
	if(memcmp(challengeReply, p_data, 5) != 0)
		return FALSE;

	// No byte order conversation since we only use it for sending back
	p_sdata->challenge = *(guint32*)(p_data + 5);
	return TRUE;
}

static gboolean gfire_sq_source_parse_players(gfire_sq_source_data *p_sdata, const unsigned char *p_data, guint p_len)
{
	if(p_len < 6)
		return FALSE;

	static const guint8 playersReply[] =	{ 0xff, 0xff, 0xff, 0xff, 0x44 };
	if(memcmp(playersReply, p_data, 5) != 0)
		return FALSE;

	guint8 numPlayers = *(p_data + 5);

	guint16 offset = 6;
	while(offset < (p_len - 10) && g_slist_length(p_sdata->players) < numPlayers)
	{
		// Not used
		//guint8 index = *(p_data + offset);
		offset++;

		gchar *name = gfire_sq_source_get_string(p_data, p_len, &offset);
		if(offset >= (p_len - 8)) // Error
		{
			g_free(name);
			return FALSE;
		}

		gfire_sq_source_player *player = g_malloc0(sizeof(gfire_sq_source_player));
		player->name = name;

		player->score = GINT32_FROM_LE(*(gint32*)(p_data + offset));
		offset += 4;

		player->time = *(gfloat*)(p_data + offset);
		offset += 4;

		p_sdata->players = g_slist_append(p_sdata->players, player);
	}

	return TRUE;
}

static gboolean gfire_sq_source_parse_rules(gfire_sq_source_data *p_sdata, const unsigned char *p_data, guint p_len)
{
	if(p_len < 7)
		return FALSE;

	static const guint8 rulesReply[] =	{ 0xff, 0xff, 0xff, 0xff, 0x45 };
	if(memcmp(rulesReply, p_data, 5) != 0)
		return FALSE;

	guint16 numRules = GUINT16_FROM_LE(*(guint16*)(p_data + 5));

	g_datalist_init(&p_sdata->rules);

	guint16 offset = 7;
	guint16 rules = 0;
	gchar *key = NULL;
	while(offset != p_len && rules < numRules)
	{
		if(!key)
			key = gfire_sq_source_get_string(p_data, p_len, &offset);
		else
		{
			g_datalist_set_data_full(&p_sdata->rules, key, gfire_sq_source_get_string(p_data, p_len, &offset), g_free);
			g_free(key);
			key = NULL;
			rules++;
		}
	}
	if(key)
		g_free(key);

	return TRUE;
}

static gboolean gfire_sq_source_parse(gfire_game_server *p_server, guint16 p_ping, gboolean p_full,
									 const unsigned char *p_data, guint p_len)
{
	if(!p_server->data)
	{
		gfire_sq_source_data *data = g_malloc0(sizeof(gfire_sq_source_data));

		// Parse the details data
		if(!gfire_sq_source_parse_details(data, p_data, p_len))
		{
			g_free(data);
			return FALSE;
		}

		// Store it for the server
		p_server->data = g_malloc0(sizeof(gfire_game_server_data));
		p_server->data->proto_data = data;

		// Set the default values
		p_server->data->driver = &gf_sq_source_driver;
		p_server->data->name = g_strdup(data->name);
		p_server->data->map = g_strdup(data->map);
		p_server->data->players = data->num_players;
		p_server->data->max_players = data->max_players;
		p_server->data->ping = p_ping;


		// Next stage
		data->query_stage = GFSQ_SOURCE_STAGE_CHALLENGE;
	}
	else
	{
		gfire_sq_source_data *data = (gfire_sq_source_data*)p_server->data->proto_data;
		if(data->query_stage == GFSQ_SOURCE_STAGE_CHALLENGE && gfire_sq_source_parse_challenge(data, p_data, p_len))
			data->query_stage = GFSQ_SOURCE_STAGE_PLAYERS;
		else if(data->query_stage == GFSQ_SOURCE_STAGE_CHALLENGE && gfire_sq_source_parse_players(data, p_data, p_len))
			data->query_stage = GFSQ_SOURCE_STAGE_RULES;
		else if(data->query_stage == GFSQ_SOURCE_STAGE_PLAYERS && gfire_sq_source_parse_players(data, p_data, p_len))
			data->query_stage = GFSQ_SOURCE_STAGE_RULES;
		else if(data->query_stage == GFSQ_SOURCE_STAGE_RULES && gfire_sq_source_parse_rules(data, p_data, p_len))
			return FALSE;
		else
		{
			// Querying failed, remove all data and abort
			gfire_sq_source_free_server(p_server);
			g_free(p_server->data->name);
			g_free(p_server->data->map);
			g_free(p_server->data);
			p_server->data = NULL;
			return FALSE;
		}
	}

	// Go to the next stage, if requested
	return p_full;
}

static gchar *gfire_sq_source_fixed_len_string(const gchar *p_str, guint p_len)
{
	if(!p_str)
		return NULL;

	// Pad the string with spaces if necessary
	if(g_utf8_strlen(p_str, -1) < p_len)
	{
		gchar *str = g_strnfill(p_len + strlen(p_str) - g_utf8_strlen(p_str, -1), ' ');
		memcpy(str, p_str, strlen(p_str));
		return str;
	}
	// Copy max p_len bytes
	else if(g_utf8_strlen(p_str, -1) > p_len)
	{
		const gchar *end = g_utf8_offset_to_pointer(p_str, p_len - 3);
		gchar *str = g_strnfill(end - p_str + 3, '.');
		memcpy(str, p_str, end - p_str);
		return str;
	}
	else
		return g_strdup(p_str);
}

static void gfire_sq_source_details_vars(GQuark p_key, gpointer p_data, gpointer p_udata)
{
	g_string_append_printf((GString*)p_udata, "<b>%s:</b> %s<br>", g_quark_to_string(p_key), (gchar*)p_data);
}

static gchar *gfire_sq_source_details(gfire_game_server *p_server)
{
	GString *str = g_string_new(NULL);

	gfire_sq_source_data *data = (gfire_sq_source_data*)p_server->data->proto_data;

	// General server infos
	g_string_append(str, _("<b><font size=\"5\">General Server Details:</font></b><br>"));
	//	Server Name
	if(data->name)
	{
		gchar *escaped = gfire_escape_html(data->name);
		g_string_append_printf(str, _("<b>Server Name:</b> %s<br>"), escaped);
		g_free(escaped);
	}
	//	Players
	g_string_append_printf(str, _("<b>Players:</b> %u/%u<br>"), data->num_players, data->max_players);
	//	Map
	if(data->map)
	{
		gchar *escaped = gfire_escape_html(data->map);
		g_string_append_printf(str, _("<b>Map:</b> %s<br>"), escaped);
		g_free(escaped);
	}
	//	Password secured
	g_string_append_printf(str, _("<b>Password secured:</b> %s<br>"), data->password ? _("Yes") : _("No"));
	// VAC Secured
	g_string_append_printf(str, _("<b>VAC secured:</b> %s<br>"), data->secure ? _("Yes") : _("No"));
	// Server Type
	g_string_append_printf(str, _("<b>Server Type:</b> %s<br>"),
						   data->type == 'l' ? _("Listen") :
						   (data->type == 'd' ? _("Dedicated") :
							(data->type == 'p' ? "SourceTV" : _("Unknown"))));
	// OS
	g_string_append_printf(str, _("<b>Operating system:</b> %s<br>"),
						   data->os == 'l' ? "GNU/Linux" :
						   (data->os == 'w' ? "Windows" : _("Unknown")));
	// Version
	if(data->version)
	{
		gchar *escaped = gfire_escape_html(data->version);
		g_string_append_printf(str, _("<b>Version:</b> %s"), escaped);
		g_free(escaped);
	}

	// Players
	g_string_append(str, _("<br><br><b><font size=\"5\">Players:</font></b><br><font face=\"monospace\"><b>Name             Kills      Playtime</b><br>"));
	GSList *cur = data->players;
	while(cur)
	{
		gfire_sq_source_player *player = (gfire_sq_source_player*)cur->data;

		// Name
		gchar *name = gfire_sq_source_fixed_len_string(player->name, 16);
		gchar *escaped = gfire_escape_html(name);
		g_free(name);

		// Score
		gchar *score = g_strdup_printf("%d", player->score);
		gchar *score_fixed = gfire_sq_source_fixed_len_string(score, 10);
		g_free(score);

		// Time
		guint hours = (guint)player->time / 3600;
		guint min = ((guint)player->time % 3600) / 60;
		guint sec = (guint)player->time % 60;

		g_string_append_printf(str, "%s %s %uh %um %us<br>", escaped, score_fixed, hours, min, sec);

		g_free(score_fixed);
		g_free(escaped);

		cur = g_slist_next(cur);
	}

	// Other server rules
	g_string_append(str, _("<br></font><b><font size=\"5\">All Server Rules:</font></b><br>"));
	g_datalist_foreach(&data->rules, gfire_sq_source_details_vars, str);

	return g_string_free(str, FALSE);
}

static void gfire_sq_source_free_server(gfire_game_server *p_server)
{
	if(p_server->data && p_server->data->proto_data)
	{
		gfire_sq_source_data *data = (gfire_sq_source_data*)p_server->data->proto_data;

		g_free(data->name);
		g_free(data->map);
		g_free(data->game_dir);
		g_free(data->game_desc);
		g_free(data->version);

		g_datalist_clear(&data->rules);

		while(data->players)
		{
			gfire_sq_source_player *player = (gfire_sq_source_player*)data->players->data;
			g_free(player->name);
			g_free(player);
			data->players = g_slist_delete_link(data->players, data->players);
		}
		g_free(data);
	}
}
