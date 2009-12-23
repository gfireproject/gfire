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

#include <core/oGLuiLayoutWidgetElement.h>

oGLuiLayoutWidgetElement::oGLuiLayoutWidgetElement(oGLuiWidget *p_widget)
{
	m_widget = p_widget;
}

oGLuiSize oGLuiLayoutWidgetElement::minimumSize() const
{
	if(m_widget)
		return m_widget->minimumSize();

	return oGLuiSize(1, 1);
}

oGLuiSize oGLuiLayoutWidgetElement::maximumSize() const
{
	if(m_widget)
		return m_widget->maximumSize();

	return oGLuiSize(UINT_MAX, UINT_MAX);
}

void oGLuiLayoutWidgetElement::setGeometry(int p_x, int p_y, unsigned int p_width, unsigned int p_height)
{
	if(m_widget)
		m_widget->setGeometry(p_y, p_x, p_width, p_height);
}
