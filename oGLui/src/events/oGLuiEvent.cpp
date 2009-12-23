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

#include <events/oGLuiEvent.h>

oGLuiEvent::oGLuiEvent(oGLuiEvent::Type p_type, oGLuiEvent::Direction p_direction)
{
	m_type = p_type;
	m_direction = p_direction;
	m_accepted = false;
}

void oGLuiEvent::accept()
{
	m_accepted = true;
}

void oGLuiEvent::ignore()
{
	m_accepted = false;
}

bool oGLuiEvent::isAccepted() const
{
	return m_accepted;
}

oGLuiEvent::Type oGLuiEvent::type() const
{
	return m_type;
}

oGLuiEvent::Direction oGLuiEvent::direction() const
{
	return m_direction;
}
