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

#ifndef _OGLUIPIXMAP_H
#define _OGLUIPIXMAP_H

#include <core/oGLui.h>

#include <string>
#include <core/oGLuiSize.h>

class oGLuiPoint;
class oGLuiColor;
class oGLuiRect;
class oGLuiFont;
class oGLuiSize;

class oGLuiPixmap
{
	private:
		cairo_surface_t *m_surface;
		cairo_t *m_cr;
		oGLui::PixelFormat m_format;

		oGLuiSize m_size;

		bool m_clipped;

	public:
		oGLuiPixmap(unsigned int p_width, unsigned int p_height, oGLui::PixelFormat p_format = oGLui::ARGB);
		oGLuiPixmap(const oGLuiPixmap &p_pixmap);
		oGLuiPixmap(const std::string &p_filename);
		~oGLuiPixmap();

		const oGLuiSize &size() const;
		void resize(const oGLuiSize &p_size);
		void resize(unsigned int p_width, unsigned int p_height);

		void clear();
		void drawPixel(const oGLuiPoint &p_pixel, const oGLuiColor &p_color);
		void drawLine(const oGLuiPoint &p_start, const oGLuiPoint &p_end, const oGLuiColor &p_color, unsigned int p_line_width);
		void drawRectangle(const oGLuiRect &p_rect, const oGLuiColor &p_color, unsigned int p_line_width);
		void fillRectangle(const oGLuiRect &p_rect, const oGLuiColor &p_color);

		void copy_full(const oGLuiPoint &p_pos, const oGLuiPixmap &p_pixmap);
		void copy_full(unsigned int p_x, unsigned int p_y, const oGLuiPixmap &p_pixmap);
		void copy(const oGLuiPoint &p_pos, const oGLuiSize &p_size, const oGLuiPixmap &p_pixmap);
		void copy(unsigned int p_x, unsigned int p_y, unsigned int p_width, unsigned int p_height, const oGLuiPixmap &p_pixmap);

		oGLuiPixmap scaled(const oGLuiSize &p_size) const;
		oGLuiPixmap scaled(unsigned int p_width, unsigned int p_height) const;

		bool fromPNGFile(const std::string &p_filename);
		static oGLuiPixmap *createFromPNGFile(const std::string &p_filename);
		bool toPNGFile(const std::string &p_filename) const;
		void toOpenGLTexture() const;

		bool hasClipping() const;
		void clipRect(const oGLuiRect &p_rect);
		void clipRectInverted(const oGLuiRect &p_rect);
		void removeClipping();

		static bool oGLFrameBufferToPNGFile(const std::string &p_filename);

		friend class oGLuiFontRenderer;
};

#endif // _OGLUIPIXMAP_H
