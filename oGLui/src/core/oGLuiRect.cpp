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

#include <core/oGLuiRect.h>

#include <core/oGLuiPoint.h>
#include <core/oGLuiSize.h>

oGLuiRect::oGLuiRect(unsigned int p_left, unsigned int p_top, unsigned int p_right, unsigned int p_bottom)
{
	m_left = p_left;
	m_top = p_top;
	m_right = p_right;
	m_bottom = p_bottom;
}

oGLuiRect::oGLuiRect(const oGLuiPoint &p_topleft, const oGLuiPoint &p_bottomright)
{
	m_left = p_topleft.x();
	m_top = p_topleft.y();

	m_right = p_bottomright.x();
	m_bottom = p_bottomright.y();
}

oGLuiRect::oGLuiRect(const oGLuiRect &p_rect)
{
	oGLuiRect::operator=(p_rect);
}

unsigned int oGLuiRect::left() const
{
	return m_left;
}

void oGLuiRect::setLeft(unsigned int p_left)
{
	m_left = p_left;

	if(m_left > m_right)
		m_right = m_left;
}

unsigned int oGLuiRect::top() const
{
	return m_top;
}

void oGLuiRect::setTop(unsigned int p_top)
{
	m_top = p_top;

	if(m_top > m_bottom)
		m_bottom = m_top;
}

unsigned int oGLuiRect::right() const
{
	return m_right;
}

void oGLuiRect::setRight(unsigned int p_right)
{
	m_right = p_right;

	if(m_right < m_left)
		m_right = m_left;
}

unsigned int oGLuiRect::bottom() const
{
	return m_bottom;
}

void oGLuiRect::setBottom(unsigned int p_bottom)
{
	m_bottom = p_bottom;

	if(m_bottom < m_top)
		m_bottom = m_top;
}

unsigned int oGLuiRect::width() const
{
	return m_right - m_left;
}


unsigned int oGLuiRect::height() const
{
	return m_bottom - m_top;
}

oGLuiPoint oGLuiRect::topLeft() const
{
	return oGLuiPoint(m_left, m_top);
}

void oGLuiRect::setTopLeft(const oGLuiPoint &p_topleft)
{
	setLeft(p_topleft.x());
	setTop(p_topleft.y());
}

oGLuiPoint oGLuiRect::bottomRight() const
{
	return oGLuiPoint(m_right, m_bottom);
}

void oGLuiRect::setBottomRight(const oGLuiPoint &p_bottomright)
{
	setRight(p_bottomright.x());
	setBottom(p_bottomright.y());
}

oGLuiSize oGLuiRect::size() const
{
	return oGLuiSize(width(), height());
}

void oGLuiRect::resize(const oGLuiSize &p_size)
{
	setRight(left() + p_size.width());
	setBottom(top() + p_size.height());
}

void oGLuiRect::resize(unsigned int p_width, unsigned int p_height)
{
	resize(oGLuiSize(p_width, p_height));
}

void oGLuiRect::move(int p_x, int p_y)
{
	setLeft(left() + p_x);
	setRight(right() + p_x);
	setTop(top() + p_y);
	setBottom(bottom() + p_y);
}

bool oGLuiRect::contains(const oGLuiPoint &p_point) const
{
	if(p_point.x() < left())
		return false;
	else if(p_point.y() < top())
		return false;
	else if(p_point.x() > right())
		return false;
	else if(p_point.y() > bottom())
		return false;

	return true;
}

void oGLuiRect::shrinkInto(oGLuiRect &p_rect) const
{
	// Check if it fits at all
	if(p_rect.left() > width())
	{
		p_rect.setLeft(width());
		p_rect.setRight(width());

		return;
	}
	if(p_rect.top() > height())
	{
		p_rect.setTop(height());
		p_rect.setBottom(height());

		return;
	}

	if(p_rect.left() < 0)
		p_rect.setLeft(0);
	if(p_rect.top() < 0)
		p_rect.setTop(0);
	if(p_rect.right() > width())
		p_rect.setRight(width());
	if(p_rect.bottom() > height())
		p_rect.setBottom(height());
}

oGLuiRect &oGLuiRect::operator=(const oGLuiRect &p_rect)
{
	if(this != &p_rect)
	{
		m_left = p_rect.m_left;
		m_top = p_rect.m_top;
		m_right = p_rect.m_right;
		m_bottom = p_rect.m_bottom;
	}

	return *this;
}

bool oGLuiRect::operator==(const oGLuiRect &p_rect)
{
	if(m_left == p_rect.m_left && m_top == p_rect.m_top && m_right == p_rect.m_right && m_bottom == p_rect.m_bottom)
		return true;
	else
		return false;
}

bool oGLuiRect::operator!=(const oGLuiRect &p_rect)
{
	return !(*this == p_rect);
}

bool oGLuiRect::operator>(const oGLuiRect &p_rect)
{
	return ((width() * height()) > (p_rect.width() * p_rect.height()));
}

bool oGLuiRect::operator<(const oGLuiRect &p_rect)
{
	return ((width() * height()) < (p_rect.width() * p_rect.height()));
}

bool oGLuiRect::operator>=(const oGLuiRect &p_rect)
{
	return ((*this > p_rect) || (*this == p_rect));
}

bool oGLuiRect::operator<=(const oGLuiRect &p_rect)
{
	return ((*this < p_rect) || (*this == p_rect));
}
