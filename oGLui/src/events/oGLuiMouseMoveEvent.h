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

#ifndef _OGLUIMOUSEMOVEEVENT_H
#define _OGLUIMOUSEMOVEEVENT_H

#include <events/oGLuiEvent.h>
#include <core/oGLuiPoint.h>

/**
 * \class oGLuiMouseMoveEvent
 * \brief Event for mouse motion
 *
 * This class is used to handle mouse motion.
**/
class oGLuiMouseMoveEvent :  public oGLuiEvent
{
	private:
		/** Mouse Position */
		oGLuiPoint m_pos;

	public:
		/**
		 * Constructor
		 * @param p_pos the new mouse position
		**/
		oGLuiMouseMoveEvent(const oGLuiPoint &p_pos);

		/**
		 * Gives access to the new mouse position stored in the event.
		 * @return new mouse position
		**/
		const oGLuiPoint &pos() const;
};

#endif // _OGLUIMOUSEMOVEEVENT_H
