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

#ifndef _OGLUISOLIDSTYLE_H
#define _OGLUISOLIDSTYLE_H

#include <core/oGLuiStyle.h>

class oGLuiSolidStyle : public oGLuiStyle
{
	public:
		oGLuiSolidStyle();

		void drawBorder(oGLuiPainter *p_painter, oGLui::BorderStyle p_style) const;
		oGLuiRect borderExtents(oGLui::BorderStyle p_style) const;

		unsigned int borderWidth() const;
		void setBorderWidth(unsigned int p_width);

		enum ColorRole
		{
			BorderRole = oGLui::UserRole
		};

	private:
		unsigned int m_border_width;
};

#endif // _OGLUISOLIDSTYLE_H
