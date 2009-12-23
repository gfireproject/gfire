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

#include <core/oGLuiLayout.h>

#include <core/oGLuiPainter.h>
#include <core/oGLuiLayoutWidgetElement.h>
#include <core/oGLuiUtils.h>

oGLuiLayout::oGLuiLayout(oGLuiObject *p_parent) :
		oGLuiObject(p_parent)
{
	m_spacing = 5;
	m_halign = oGLui::Center;
	m_valign = oGLui::Middle;

	// Parent is set by oGLuiObject(oGLuiObject*)
}

void oGLuiLayout::setParent(oGLuiObject *p_parent)
{
	bool skip_check = (parent() == 0);
	oGLuiObject::setParent(p_parent);

	if(p_parent->isWidget())
	{
		if(!skip_check && m_parent_widget && static_cast<oGLuiWidget*>(m_parent_widget)->layout() == this)
			static_cast<oGLuiWidget*>(m_parent_widget)->setLayout(0);

		static_cast<oGLuiWidget*>(p_parent)->setLayout(this);
	}

	getParentWidget();
}

void oGLuiLayout::addWidget(oGLuiWidget *p_widget)
{
	if(p_widget)
	{
		addElement(new oGLuiLayoutWidgetElement(p_widget));
	}
}

void oGLuiLayout::setGeometry(int p_x, int p_y, unsigned int p_width, unsigned int p_height)
{
	m_geometry = oGLuiRect(p_x, p_y, p_x + p_width, p_y + p_height);

	update();
}

void oGLuiLayout::setSpacing(unsigned int p_spacing)
{
	if(p_spacing != m_spacing)
	{
		m_spacing = p_spacing;
		update();
	}
}

unsigned int oGLuiLayout::spacing() const
{
	return m_spacing;
}

void oGLuiLayout::setHorizontalAlign(oGLui::HAlignment p_align)
{
	if(p_align != m_halign)
	{
		m_halign = p_align;
		update();
	}
}

oGLui::HAlignment oGLuiLayout::horizontalAlign() const
{
	return m_halign;
}

void oGLuiLayout::setVerticalAlign(oGLui::VAlignment p_align)
{
	if(p_align != m_valign)
	{
		m_valign = p_align;
		update();
	}
}

oGLui::VAlignment oGLuiLayout::verticalAlign() const
{
	return m_valign;
}

void oGLuiLayout::update()
{
	realign();
}

void oGLuiLayout::getParentWidget()
{
	if(!parent())
		m_parent_widget = 0;
	else
	{
		oGLuiObject *cur = parent();

		while(cur)
		{
			if(cur->isWidget())
			{
				m_parent_widget = cur;
				break;
			}

			cur = cur->parent();
		}

		if(!cur)
			m_parent_widget = 0;
	}

	reparentElements();
}

void oGLuiLayout::reparentElements()
{
	for(unsigned int i = 0; i < count(); i++)
	{
		if(elementAt(i)->widget())
			elementAt(i)->widget()->setParent(m_parent_widget ? m_parent_widget : this);
		else if(elementAt(i)->layout())
			elementAt(i)->layout()->setParent(this);
	}
}
