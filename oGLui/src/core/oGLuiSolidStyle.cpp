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

#include <core/oGLuiSolidStyle.h>
#include <core/oGLuiRect.h>
#include <core/oGLuiPainter.h>
#include <core/oGLuiPixmap.h>
#include <core/oGLuiPoint.h>

oGLuiSolidStyle::oGLuiSolidStyle() :
		oGLuiStyle()
{
	m_border_width = 1;

	// Set the styles default colors
	m_palette.setColor(oGLui::BackgroundRole, oGLuiColor(0xCC, 0xCC, 0xCC));
	m_palette.setColor(oGLui::TextRole, oGLuiColor(0x00, 0x00, 0x00));

	m_palette.setColor(oGLui::ButtonRole, oGLuiColor(0x88, 0x88, 0x88));
	m_palette.setColor(oGLui::ButtonTextRole, oGLuiColor(0x00, 0x00, 0x00));

	m_palette.setColor(oGLui::InputRole, oGLuiColor(0xFF, 0xFF, 0xFF));
	m_palette.setColor(oGLui::InputTextRole, oGLuiColor(0x00, 0x00, 0x00));

	m_palette.setColor(oGLui::InputHighlightRole, oGLuiColor(0x55, 0x55, 0xFF));
	m_palette.setColor(oGLui::InputHighlightTextRole, oGLuiColor(0xFF, 0xFF, 0xFF));

	m_palette.setColor(BorderRole, oGLuiColor(0x00, 0x00, 0x00));
}

void oGLuiSolidStyle::drawBorder(oGLuiPainter *p_painter, oGLui::BorderStyle p_style) const
{
	if(!p_painter || p_style == oGLui::BorderNone)
		return;

	p_painter->setColor(oGLuiColor(0, 0, 0));
	p_painter->setPenSize(m_border_width);

	// Top
	oGLuiPoint top_start(0, m_border_width / 2);
	oGLuiPoint top_end(p_painter->pixmap()->size().width(), m_border_width / 2);
	p_painter->drawLine(top_start, top_end);

	// Left
	p_painter->drawLine(oGLuiPoint(m_border_width / 2, m_border_width / 2), oGLuiPoint(m_border_width / 2, p_painter->pixmap()->size().height() - (m_border_width / 2)));

	// Right
	p_painter->drawLine(oGLuiPoint(p_painter->pixmap()->size().width() - (m_border_width / 2), m_border_width / 2), oGLuiPoint(p_painter->pixmap()->size().width() - (m_border_width / 2), p_painter->pixmap()->size().height() - (m_border_width / 2)));


	// Bottom
	oGLuiPoint bottom_start(0, p_painter->pixmap()->size().height() - (m_border_width / 2));
	oGLuiPoint bottom_end(p_painter->pixmap()->size().width(), p_painter->pixmap()->size().height() - (m_border_width / 2));
	p_painter->drawLine(bottom_start, bottom_end);
}

oGLuiRect oGLuiSolidStyle::borderExtents(oGLui::BorderStyle p_style) const
{
	if(p_style == oGLui::BorderNone)
		return oGLuiRect(0, 0, 0, 0);

	oGLuiRect ret;
	ret.setTop(m_border_width);
	ret.setLeft(m_border_width);
	ret.setRight(m_border_width);
	ret.setBottom(m_border_width);

	return ret;
}

unsigned int oGLuiSolidStyle::borderWidth() const
{
	return m_border_width;
}

void oGLuiSolidStyle::setBorderWidth(unsigned int p_width)
{
	if(p_width > 0)
		m_border_width = p_width;
}
