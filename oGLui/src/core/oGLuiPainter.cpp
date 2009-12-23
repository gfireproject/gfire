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

#include <core/oGLuiPainter.h>

#include <core/oGLuiPixmap.h>
#include <core/oGLuiPoint.h>

oGLuiPainter::oGLuiPainter(oGLuiPixmap *p_pixmap)
{
	m_pixmap = p_pixmap;

	m_pen_size = 2;

	if(m_pixmap)
	{
		m_region.setTopLeft(oGLuiPoint(0, 0));
		m_region.resize(m_pixmap->size());
	}
}

oGLuiPixmap *oGLuiPainter::pixmap() const
{
	return m_pixmap;
}

void oGLuiPainter::setPixmap(oGLuiPixmap *p_pixmap)
{
	if(!p_pixmap)
		return;

	m_pixmap = p_pixmap;
	m_region.setTopLeft(oGLuiPoint(0, 0));
	m_region.resize(m_pixmap->size());
}

const oGLuiColor &oGLuiPainter::color() const
{
	return m_color;
}

void oGLuiPainter::setColor(const oGLuiColor &p_color)
{
	m_color = p_color;
}

unsigned int oGLuiPainter::penSize() const
{
	return m_pen_size;
}

void oGLuiPainter::setPenSize(unsigned int p_size)
{
	m_pen_size = p_size;
}

void oGLuiPainter::narrowRegion(const oGLuiRect &p_rect)
{
	m_region.setLeft(m_region.left() + p_rect.left());

	if((m_region.left() + p_rect.width()) < m_region.right())
		m_region.setRight(m_region.left() + p_rect.width());

	m_region.setTop(m_region.top() + p_rect.top());

	if((m_region.top() + p_rect.height()) < m_region.bottom())
		m_region.setBottom(m_region.top() + p_rect.height());
}

void oGLuiPainter::drawPixel(const oGLuiPoint &p_pixel)
{
	if(!m_pixmap)
		return;

	oGLuiPoint pixel(p_pixel.x() + m_region.left(), p_pixel.y() + m_region.top());

	if(m_region.contains(pixel))
		m_pixmap->drawPixel(pixel, m_color);
}

void oGLuiPainter::drawLine(const oGLuiPoint &p_start, const oGLuiPoint &p_end)
{
	if(!m_pixmap)
		return;

	oGLuiPoint start(p_start.x() + m_region.left(), p_start.y() + m_region.top());
	oGLuiPoint end(p_end.x() + m_region.left(), p_end.y() + m_region.top());

	m_pixmap->clipRect(m_region);
	m_pixmap->drawLine(start, end, m_color, m_pen_size);
	m_pixmap->removeClipping();
}

void oGLuiPainter::drawRectangle(const oGLuiRect &p_rect)
{
	if(!m_pixmap)
		return;

	oGLuiRect rect = p_rect;
	rect.move(m_region.left(), m_region.top());

	m_pixmap->clipRect(m_region);
	m_pixmap->drawRectangle(rect, m_color, m_pen_size);
	m_pixmap->removeClipping();
}

void oGLuiPainter::fillRectangle(const oGLuiRect &p_rect)
{
	if(!m_pixmap)
		return;

	oGLuiRect rect = p_rect;
	rect.move(m_region.left(), m_region.top());

	m_pixmap->clipRect(m_region);
	m_pixmap->fillRectangle(rect, m_color);
	m_pixmap->removeClipping();
}

void oGLuiPainter::copy_full(const oGLuiPoint &p_pos, const oGLuiPixmap &p_pixmap)
{
	oGLuiPoint pos(p_pos.x() + m_region.left(), p_pos.y() + m_region.top());
	m_pixmap->clipRect(m_region);
	m_pixmap->copy_full(pos, p_pixmap);
	m_pixmap->removeClipping();
}

void oGLuiPainter::copy_full(unsigned int p_x, unsigned int p_y, const oGLuiPixmap &p_pixmap)
{
	copy_full(oGLuiPoint(p_x, p_y), p_pixmap);
}

void oGLuiPainter::copy(const oGLuiPoint &p_pos, const oGLuiSize &p_size, const oGLuiPixmap &p_pixmap)
{
	oGLuiPoint pos(p_pos.x() + m_region.left(), p_pos.y() + m_region.top());
	m_pixmap->clipRect(m_region);
	m_pixmap->copy(pos, p_size, p_pixmap);
	m_pixmap->removeClipping();
}

void oGLuiPainter::copy(unsigned int p_x, unsigned int p_y, unsigned int p_width, unsigned int p_height, const oGLuiPixmap &p_pixmap)
{
	copy(oGLuiPoint(p_x, p_y), oGLuiSize(p_width, p_height), p_pixmap);
}

void oGLuiPainter::copyScaled(const oGLuiPoint &p_pos, const oGLuiSize &p_size, const oGLuiPixmap &p_pixmap)
{
	oGLuiPoint pos(p_pos.x() + m_region.left(), p_pos.y() + m_region.top());
	m_pixmap->clipRect(m_region);
	m_pixmap->copy_full(pos, p_pixmap.scaled(p_size));
	m_pixmap->removeClipping();
}

void oGLuiPainter::copyScaled(unsigned int p_x, unsigned int p_y, unsigned int p_width, unsigned int p_height, const oGLuiPixmap &p_pixmap)
{
	copyScaled(oGLuiPoint(p_x, p_y), oGLuiSize(p_width, p_height), p_pixmap);
}
