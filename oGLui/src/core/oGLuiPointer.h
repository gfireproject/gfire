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

#ifndef _OGLUIPOINTER_H
#define _OGLUIPOINTER_H

#include <core/oGLui.h>
#include <core/oGLuiPixmap.h>
#include <core/oGLuiPoint.h>
#include <string>

class oGLuiPointer
{
	public:
		oGLuiPointer();
		~oGLuiPointer();

		bool loadFromPNG(const std::string &p_filename);

		const oGLuiPoint &pos() const;
		void move(const oGLuiPoint &p_pos);
		void move(unsigned int p_x, unsigned int p_y);

		void paintTexture();

		void show();
		void hide();
		bool hidden() const;

	private:
		oGLuiPoint m_pos;
		oGLuiPixmap m_pixmap;

		GLuint m_texture;
		GLfloat m_check_color[4];

		bool m_hidden;

		void updateTexture(bool p_redraw = true);
		void freeTexture();
};

#endif // _OGLUIPOINTER_H
