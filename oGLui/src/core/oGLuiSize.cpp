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

#include <core/oGLuiSize.h>
#include <core/oGLuiRect.h>

oGLuiSize::oGLuiSize(unsigned int p_width, unsigned int p_height)
{
	m_width = p_width;
	m_height = p_height;
}

oGLuiSize::oGLuiSize(const oGLuiRect &p_rect)
{
	m_width = p_rect.width();
	m_height = p_rect.height();
}

unsigned int oGLuiSize::width() const
{
	return m_width;
}

void oGLuiSize::setWidth(unsigned int p_width)
{
	m_width = p_width;
}

unsigned int oGLuiSize::height() const
{
	return m_height;
}

void oGLuiSize::setHeight(unsigned int p_height)
{
	m_height = p_height;
}
