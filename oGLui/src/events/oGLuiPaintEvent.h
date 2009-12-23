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

#ifndef _OGLUIPAINTEVENT_H
#define _OGLUIPAINTEVENT_H

#include <events/oGLuiEvent.h>
#include <core/oGLuiRect.h>

class oGLuiPainter;

/**
 * \class oGLuiPaintEvent
 * \brief Event for repaint requests
 *
 * This class is used to tell widgets that they have to repaint themselves.
**/
class oGLuiPaintEvent :  public oGLuiEvent
{
	private:
		/** the painter that should be used for repainting */
		oGLuiPainter *m_painter;

	public:
		/**
		 * Constructor
		 * @param p_painter the painter for redrawing
		**/
		oGLuiPaintEvent(oGLuiPainter *p_painter = 0);
		~oGLuiPaintEvent();

		/**
		 * Gives access to the redraw-painter
		 * @return the painter for redrawing
		**/
		oGLuiPainter *painter();
};

#endif // _OGLUIPAINTEVENT_H
