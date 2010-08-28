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

#include "gf_preferences.h"
#include "gf_preferences_proto.h"

static gboolean gfire_preferences_get_default(guint8 p_id)
{
	static const gf_pref defaultPrefs[] =
	{
		{ 0x01, TRUE },		// Show game status to buddies
		{ 0x02, TRUE },		// Show game server data to buddies
		{ 0x03, TRUE },		// Show game status on the profile
		{ 0x04, TRUE },		// Play sound on new messages
		{ 0x05, TRUE },		// Play sound on new messages while in a game
		{ 0x06, FALSE },	// Show timestamp in chat windows
		{ 0x07, TRUE },		// Play sound on logon/off
		{ 0x08, TRUE },		// Show (game status to) FoFs
		{ 0x09, TRUE },		// Show offline buddies
		{ 0x0a, TRUE },		// Show nickname, if available
		{ 0x0b, TRUE },		// Show voice server to buddies
		{ 0x0c, TRUE },		// Show typing notification to buddies
		{ 0x10, TRUE },		// Show tooltip on logon
		{ 0x11, TRUE },		// Show tooltip on download
		{ 0x12, TRUE },		// Play sound on join/leave in chat rooms
		{ 0x13, TRUE },		// Play sound on voice call
		{ 0x14, TRUE },		// Play sound on screenshot
		{ 0xff, FALSE }		// Always the last element
	};

	const gf_pref *cur = defaultPrefs;
	while(cur->id != 0xff)
	{
		if(cur->id == p_id)
			return cur->set;
		cur++;
	}

	return FALSE;
}

gfire_preferences *gfire_preferences_create()
{
	return g_malloc0(sizeof(gfire_preferences));
}

void gfire_preferences_free(gfire_preferences *p_prefs)
{
	if(!p_prefs)
		return;

	gfire_list_clear(p_prefs->prefs);
	g_free(p_prefs);
}

void gfire_preferences_set(gfire_preferences *p_prefs, guint8 p_id, gboolean p_set)
{
	if(!p_prefs)
		return;

	// Change an existing setting
	GList *cur = p_prefs->prefs;
	while(cur)
	{
		gf_pref *pref = (gf_pref*)cur->data;
		if(pref->id == p_id)
		{
			if(p_set == gfire_preferences_get_default(p_id))
			{
				g_free(pref);
				p_prefs->prefs = g_list_delete_link(p_prefs->prefs, cur);
			}
			else
				pref->set = p_set;
			return;
		}
		cur = g_list_next(cur);
	}

	// Add a new one
	gf_pref *pref = g_malloc(sizeof(gf_pref));
	pref->id = p_id;
	pref->set = p_set;
	p_prefs->prefs = g_list_append(p_prefs->prefs, pref);
}

gboolean gfire_preferences_get(const gfire_preferences *p_prefs, guint8 p_id)
{
	if(!p_prefs)
		return FALSE;

	const GList *cur = p_prefs->prefs;
	while(cur)
	{
		const gf_pref *pref = (gf_pref*)cur->data;
		if(pref->id == p_id)
			return pref->set;
		cur = g_list_next(cur);
	}

	// Return default setting otherwise
	return gfire_preferences_get_default(p_id);
}

void gfire_preferences_send(const gfire_preferences *p_prefs, PurpleConnection *p_gc)
{
	if(!p_prefs || !p_gc)
		return;

	purple_debug_info("gfire", "sending client preferences...\n");
	guint16 len = gfire_pref_proto_create_changed_preferences(p_prefs->prefs);
	gfire_send(p_gc, len);
}
