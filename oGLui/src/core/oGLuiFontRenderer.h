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

#ifndef _OGLUIFONTRENDERER_H
#define _OGLUIFONTRENDERER_H

#include <pango/pangocairo.h>
#include <core/oGLui.h>
#include <core/oGLuiFont.h>
#include <core/oGLuiSize.h>
#include <string>

using std::string;

class oGLuiPixmap;
class oGLuiPoint;
class oGLuiColor;

class oGLuiFontRenderer
{
	public:
		oGLuiFontRenderer();
		oGLuiFontRenderer(oGLuiPixmap *p_target, const oGLuiFont &p_font);
		~oGLuiFontRenderer();

		void setTarget(oGLuiPixmap *p_target);
		oGLuiPixmap *target() const;

		void setFont(const oGLuiFont &p_font);
		const oGLuiFont &font() const;

		void setUseMarkup(bool p_markup);
		bool useMarkup() const;

		void setUseWordWrap(bool p_wordwrap);
		bool useWordWrap() const;

		void setWidth(int p_width);
		int width() const;

		void setHeight(int p_height);
		int height() const;

		void setAlign(oGLui::HAlignment p_align);
		oGLui::HAlignment align() const;

		void setText(const string &p_text);

		oGLuiSize textExtents();
		void draw(const oGLuiPoint &p_pos, const oGLuiColor &p_color);

	private:
		void regenLayout();

		PangoLayout *m_layout;
		oGLuiPixmap *m_target;
		oGLuiFont m_font;

		bool m_markup;
		bool m_wordwrap;
		int m_width;
		int m_height;
		oGLui::HAlignment m_align;
};

#endif // _OGLUIFONTRENDERER_H
