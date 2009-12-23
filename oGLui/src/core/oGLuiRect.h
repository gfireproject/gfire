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

#ifndef _OGLUIRECT_H
#define _OGLUIRECT_H

class oGLuiPoint;
class oGLuiSize;

class oGLuiRect
{
	private:
		unsigned int m_top, m_left, m_bottom, m_right;

	public:
		oGLuiRect(unsigned int p_left = 0, unsigned int p_top = 0, unsigned int p_right = 0, unsigned int p_bottom = 0);
		oGLuiRect(const oGLuiPoint &p_topleft, const oGLuiPoint &p_bottomright);
		oGLuiRect(const oGLuiRect &p_rect);

		unsigned int left() const;
		void setLeft(unsigned int p_left);

		unsigned int top() const;
		void setTop(unsigned int p_top);

		unsigned int right() const;
		void setRight(unsigned int p_right);

		unsigned int bottom() const;
		void setBottom(unsigned int p_bottom);

		unsigned int height() const;
		unsigned int width() const;

		oGLuiPoint topLeft() const;
		void setTopLeft(const oGLuiPoint &p_topleft);

		oGLuiPoint bottomRight() const;
		void setBottomRight(const oGLuiPoint &p_bottomright);

		oGLuiSize size() const;
		void resize(const oGLuiSize &p_size);
		void resize(unsigned int p_width, unsigned int p_height);

		void move(int p_x, int p_y);

		bool contains(const oGLuiPoint &p_point) const;
		void shrinkInto(oGLuiRect &p_rect) const;

		oGLuiRect &operator=(const oGLuiRect &p_rect);
		bool operator==(const oGLuiRect &p_rect);
		bool operator!=(const oGLuiRect &p_rect);
		bool operator>(const oGLuiRect &p_rect);
		bool operator<(const oGLuiRect &p_rect);
		bool operator>=(const oGLuiRect &p_rect);
		bool operator<=(const oGLuiRect &p_rect);
};

#endif // _OGLUIRECT_H
