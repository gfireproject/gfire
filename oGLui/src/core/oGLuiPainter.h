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

#ifndef _OGLUIPAINTER_H
#define _OGLUIPAINTER_H

#include <core/oGLuiColor.h>
#include <core/oGLuiRect.h>

#include <string>

using std::string;

// Forward declaration
class oGLuiPixmap;
class oGLuiFont;

class oGLuiPainter
{
	private:
		oGLuiPixmap *m_pixmap;
		oGLuiColor m_color;
		unsigned int m_pen_size;

		oGLuiRect m_region;

	public:
		oGLuiPainter(oGLuiPixmap *p_pixmap = 0);

		oGLuiPixmap *pixmap() const;
		void setPixmap(oGLuiPixmap *p_pixmap);

		const oGLuiColor &color() const;
		void setColor(const oGLuiColor &p_color);

		unsigned int penSize() const;
		void setPenSize(unsigned int p_size);

		void narrowRegion(const oGLuiRect &p_rect);

		void drawPixel(const oGLuiPoint &p_pixel);
		void drawLine(const oGLuiPoint &p_start, const oGLuiPoint &p_end);
		void drawRectangle(const oGLuiRect &p_rect);
		void fillRectangle(const oGLuiRect &p_rect);

		void copy_full(const oGLuiPoint &p_pos, const oGLuiPixmap &p_pixmap);
		void copy_full(unsigned int p_x, unsigned int p_y, const oGLuiPixmap &p_pixmap);

		void copy(const oGLuiPoint &p_pos, const oGLuiSize &p_size, const oGLuiPixmap &p_pixmap);
		void copy(unsigned int p_x, unsigned int p_y, unsigned int p_width, unsigned int p_height, const oGLuiPixmap &p_pixmap);

		void copyScaled(const oGLuiPoint &p_pos, const oGLuiSize &p_size, const oGLuiPixmap &p_pixmap);
		void copyScaled(unsigned int p_x, unsigned int p_y, unsigned int p_width, unsigned int p_height, const oGLuiPixmap &p_pixmap);
};

#endif // _OGLUIPAINTER_H
