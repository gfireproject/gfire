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

#include <events/oGLuiPaintEvent.h>

#include <core/oGLuiPainter.h>

oGLuiPaintEvent::oGLuiPaintEvent(oGLuiPainter *p_painter) :
		oGLuiEvent(oGLuiEvent::ETPaint, oGLuiEvent::ToChildren)
{
	m_painter = p_painter;
}

oGLuiPaintEvent::~oGLuiPaintEvent()
{
	if(m_painter)
		delete m_painter;
}

oGLuiPainter *oGLuiPaintEvent::painter()
{
	return m_painter;
}
