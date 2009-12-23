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

#include <core/oGLuiPoint.h>

oGLuiPoint::oGLuiPoint(unsigned int p_x, unsigned int p_y)
{
	m_x = p_x;
	m_y = p_y;
}

oGLuiPoint::oGLuiPoint(const oGLuiPoint &p_point)
{
	oGLuiPoint::operator=(p_point);
}

unsigned int oGLuiPoint::x() const
{
	return m_x;
}

void oGLuiPoint::setX(unsigned int p_x)
{
	m_x = p_x;
}

unsigned int oGLuiPoint::y() const
{
	return m_y;
}

void oGLuiPoint::setY(unsigned int p_y)
{
	m_y = p_y;
}

oGLuiPoint &oGLuiPoint::operator=(const oGLuiPoint &p_point)
{
	if(this != &p_point)
	{
		m_x = p_point.m_x;
		m_y = p_point.m_y;
	}

	return *this;
}

bool oGLuiPoint::operator==(const oGLuiPoint &p_point)
{
	return ((m_x == p_point.m_x) && (m_y == p_point.m_y));
}
