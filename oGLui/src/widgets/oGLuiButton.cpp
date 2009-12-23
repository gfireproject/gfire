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

#include <widgets/oGLuiButton.h>

oGLuiButton::oGLuiButton(const string &p_caption, oGLuiWidget *p_parent) :
		oGLuiWidget(p_parent)
{
	setDrawBackground(true);
	setBorderStyle(oGLui::BorderRaised);
	setBackgroundRole(oGLui::ButtonRole);
	setForegroundRole(oGLui::ButtonTextRole);

	m_renderer.setFont(style()->font());
	m_renderer.setUseMarkup(true);
	m_renderer.setUseWordWrap(true);
	m_renderer.setAlign(oGLui::Center);

	m_caption = p_caption;
}

void oGLuiButton::setCaption(const string &p_caption)
{
	if(m_caption != p_caption)
	{
		m_caption = p_caption;

		invalidate();
	}
}

const string &oGLuiButton::caption() const
{
	return m_caption;
}

bool oGLuiButton::paintEvent(oGLuiPaintEvent *p_event)
{
	oGLuiWidget::paintEvent(p_event);

	m_renderer.setTarget(&m_pixmap);
	m_renderer.setText(m_caption);

	m_renderer.setWidth(clientWidth());
	m_renderer.setHeight(0);

	oGLuiSize tsize = m_renderer.textExtents();
	oGLuiPoint pos(clientRect().left(), (clientHeight() - tsize.height()) / 2 + clientRect().top());

	m_pixmap.clipRect(clientRect());
	m_renderer.draw(pos, style()->palette().color(foregroundRole()));
	m_pixmap.removeClipping();

	m_renderer.setTarget(0);

	return true;
}
