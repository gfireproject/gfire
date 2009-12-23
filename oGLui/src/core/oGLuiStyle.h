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

#ifndef _OGLUISTYLE_H
#define _OGLUISTYLE_H

#include <core/oGLui.h>
#include <core/oGLuiFont.h>
#include <core/oGLuiColorPalette.h>
#include <string>

class oGLuiConfig;
class oGLuiRect;
class oGLuiPainter;

class oGLuiStyle
{
	public:
		oGLuiStyle();
		virtual ~oGLuiStyle() {}

		void ref();
		void unref();

		virtual void drawBorder(oGLuiPainter *p_painter, oGLui::BorderStyle p_style) const = 0;
		virtual oGLuiRect borderExtents(oGLui::BorderStyle p_style) const = 0;

		void setPalette(const oGLuiColorPalette &p_palette);
		const oGLuiColorPalette &palette() const;
		virtual const oGLuiFont &font() const;

	protected:
		oGLuiColorPalette m_palette;

	private:
		unsigned int m_refcount;

		oGLuiFont m_font;
};

#endif // _OGLUISTYLE_H
