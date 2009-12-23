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

#ifndef _OGLUIEVENT_H
#define _OGLUIEVENT_H

class oGLuiEvent
{
	public:
		enum Type
		{
			ETPaint,
			ETMouseEnter,
			ETMouseLeave,
			ETMouseMove,
			ETKeyPress,
			ETKeyRelease,
			ETButtonDown,
			ETButtonUp,
			ETUser = 1000
		};

		enum Direction
		{
			ToParent,
			ToChildren
		};

		oGLuiEvent(Type p_type, Direction p_direction = ToParent);

		void accept();
		void ignore();
		bool isAccepted() const;

		Type type() const;
		Direction direction() const;

	private:
		bool m_accepted;
		Type m_type;
		Direction m_direction;
};

#endif // _OGLUIEVENT_H
