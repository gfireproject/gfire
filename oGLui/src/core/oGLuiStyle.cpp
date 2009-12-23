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

#include <core/oGLuiStyle.h>

oGLuiStyle::oGLuiStyle()
{
	m_refcount = 0;
}

void oGLuiStyle::ref()
{
	m_refcount++;
}

void oGLuiStyle::unref()
{
	m_refcount--;

	if(m_refcount <= 0)
		delete this;
}

const oGLuiFont &oGLuiStyle::font() const
{
	return m_font;
}

void oGLuiStyle::setPalette(const oGLuiColorPalette &p_palette)
{
	m_palette = p_palette;
}

const oGLuiColorPalette &oGLuiStyle::palette() const
{
	return m_palette;
}
