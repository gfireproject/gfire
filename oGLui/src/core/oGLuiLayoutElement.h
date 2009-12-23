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

#ifndef _OGLUILAYOUTELEMENT_H
#define _OGLUILAYOUTELEMENT_H

#include <core/oGLuiSize.h>
#include <core/oGLuiRect.h>

class oGLuiLayout;
class oGLuiWidget;

class oGLuiLayoutElement
{
	public:
		virtual ~oGLuiLayoutElement() {}

		virtual oGLuiSize minimumSize() const = 0;
		virtual oGLuiSize maximumSize() const = 0;

		virtual void setGeometry(int p_x, int p_y, unsigned int p_width, unsigned int p_height) = 0;
		inline virtual void setGeometry(const oGLuiRect &p_geometry)
		{
			setGeometry(p_geometry.left(), p_geometry.height(), p_geometry.width(), p_geometry.height());
		}

		virtual oGLuiLayout *layout() const { return 0; }
		virtual oGLuiWidget *widget() const { return 0; }
};

#endif // _OGLUILAYOUTELEMENT_H
