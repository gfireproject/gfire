/*
 * Copyright (C) 2009 Oliver Ney <oliver@dryder.de>
 *
 * This file is part of the OpenGL Overlay GUI Library (oGLui).
 *
 * oGLui is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with oGLui.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <core/oGLuiColorPalette.h>

oGLuiColor oGLuiColorPalette::m_white(255, 255, 255);

void oGLuiColorPalette::setColor(unsigned int p_role, const oGLuiColor &p_color)
{
	m_colors[p_role] = p_color;
}

const oGLuiColor &oGLuiColorPalette::color(unsigned int p_role) const
{
	map<unsigned int, oGLuiColor>::const_iterator iter = m_colors.find(p_role);
	if(iter == m_colors.end())
		return m_white;
	else
		return (*iter).second;
}
