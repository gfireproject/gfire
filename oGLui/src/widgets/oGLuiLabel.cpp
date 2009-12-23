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

#include <widgets/oGLuiLabel.h>

#include <core/oGLuiPainter.h>
#include <core/oGLuiSize.h>

oGLuiLabel::oGLuiLabel(const string &p_caption, oGLuiWidget *p_parent) :
		oGLuiWidget(p_parent)
{
	m_caption = p_caption;
	m_multiline = false;

	m_renderer.setFont(style()->font());
	m_renderer.setUseMarkup(true);
	m_renderer.setUseWordWrap(true);
}

void oGLuiLabel::setCaption(const string &p_caption)
{
	if(m_caption != p_caption)
	{
		m_caption = p_caption;

		invalidate();
	}
}

const string &oGLuiLabel::caption() const
{
	return m_caption;
}

void oGLuiLabel::setTextAlign(oGLui::HAlignment p_align)
{
	if(m_renderer.align() != p_align)
	{
		m_renderer.setAlign(p_align);

		invalidate();
	}
}

oGLui::HAlignment oGLuiLabel::textAlign() const
{
	return m_renderer.align();
}

void oGLuiLabel::setMultiLine(bool p_multiline)
{
	if(m_multiline != p_multiline)
	{
		m_multiline = p_multiline;

		invalidate();
	}
}

bool oGLuiLabel::multiLine() const
{
	return m_multiline;
}

bool oGLuiLabel::paintEvent(oGLuiPaintEvent *p_event)
{
	oGLuiWidget::paintEvent(p_event);

	if(m_caption.length() > 0)
	{
		//oGLuiPainter painter(&m_pixmap);

		m_renderer.setTarget(&m_pixmap);
		m_renderer.setText(m_caption);

		m_renderer.setWidth(clientWidth());
		if(m_multiline)
			m_renderer.setHeight(clientHeight());
		else
			m_renderer.setHeight(0);

		oGLuiSize tsize = m_renderer.textExtents();
		oGLuiPoint pos(clientRect().left(), (clientHeight() - tsize.height()) / 2 + clientRect().top());

		m_pixmap.clipRect(clientRect());
		m_renderer.draw(pos, style()->palette().color(foregroundRole()));
		m_pixmap.removeClipping();

		m_renderer.setTarget(0);

		return true;
	}

	return false;
}
