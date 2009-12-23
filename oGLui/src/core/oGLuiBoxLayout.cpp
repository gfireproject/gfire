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

#include <core/oGLuiBoxLayout.h>
#include <core/oGLuiUtils.h>
#include <widgets/oGLuiWidget.h>

oGLuiBoxLayout::oGLuiBoxLayout(oGLui::Direction p_direction, oGLuiObject *p_parent) :
		oGLuiLayout(p_parent)
{
	m_direction = p_direction;
}

oGLuiBoxLayout::~oGLuiBoxLayout()
{
	// Delete all items
	list<oGLuiLayoutElement*>::iterator iter;
	for(iter = m_elements.begin(); iter != m_elements.end(); iter++)
		delete *iter;
}

void oGLuiBoxLayout::addElement(oGLuiLayoutElement *p_element)
{
	if(p_element)
	{
		if(p_element->widget())
			p_element->widget()->setParent(m_parent_widget ? m_parent_widget : this);
		else if(p_element->layout())
			p_element->layout()->setParent(this);

		m_elements.push_back(p_element);
		update();
	}
}

unsigned int oGLuiBoxLayout::count() const
{
	return m_elements.size();
}

oGLuiLayoutElement *oGLuiBoxLayout::elementAt(unsigned int p_index) const
{
	if(p_index > count() - 1)
		return 0;

	list<oGLuiLayoutElement*>::const_iterator iter = m_elements.begin();
	for(unsigned int i = 0; i < p_index; i++)
		iter++;

	return (*iter);
}

oGLuiSize oGLuiBoxLayout::minimumSize() const
{
	oGLuiSize minsize(1,1);

	list<oGLuiLayoutElement*>::const_iterator iter;
	for(iter = m_elements.begin(); iter != m_elements.end(); iter++)
	{
		minsize.setWidth(oGLui::max((*iter)->minimumSize().width(), minsize.width()));
		minsize.setHeight(oGLui::max((*iter)->minimumSize().height(), minsize.height()));
	}

	return minsize;
}

oGLuiSize oGLuiBoxLayout::maximumSize() const
{
	return oGLuiSize();
}

void oGLuiBoxLayout::realign()
{
	if(count() == 0)
		return;

	unsigned int dynamic_size = ((m_direction == oGLui::Horizontal) ? m_geometry.width() : m_geometry.height())
								- ((count() - 1) * m_spacing);

	unsigned int pos = ((m_direction == oGLui::Horizontal) ? m_geometry.left() : m_geometry.top());

#define DYN_SIZE_PE (dynamic_size / count())

	// Check for elements that need to be smaller than dynamic size is
	list<oGLuiLayoutElement*>::iterator iter;
	for(iter = m_elements.begin(); iter != m_elements.end(); iter++)
	{
		if(m_direction == oGLui::Horizontal)
		{
			if((*iter)->maximumSize().width() < DYN_SIZE_PE)
				dynamic_size += DYN_SIZE_PE - (*iter)->maximumSize().width();
		}
		else
		{
			if((*iter)->maximumSize().height() < DYN_SIZE_PE)
				dynamic_size +=  DYN_SIZE_PE - (*iter)->maximumSize().height();
		}
	}

	// Check for elements that need to be bigger than the dynamic size allows
	for(iter = m_elements.begin(); iter != m_elements.end(); iter++)
	{
		if(m_direction == oGLui::Horizontal)
		{
			if((*iter)->minimumSize().width() > DYN_SIZE_PE)
				dynamic_size -= (*iter)->minimumSize().width() - DYN_SIZE_PE;
		}
		else
		{
			if((*iter)->minimumSize().height() > DYN_SIZE_PE)
				dynamic_size -= (*iter)->minimumSize().height() - DYN_SIZE_PE;
		}
	}

	// Realign
	for(iter = m_elements.begin(); iter != m_elements.end(); iter++)
	{
		if(m_direction == oGLui::Horizontal)
		{
			unsigned int pos_y = m_geometry.top();

			if((*iter)->maximumSize().height() < m_geometry.height())
			{
				// Top doesn't change anything
				if(m_valign == oGLui::Middle)
					pos_y += (m_geometry.height() - (*iter)->maximumSize().height()) / 2;
				else if(m_valign == oGLui::Bottom)
					pos_y += (m_geometry.height() - (*iter)->maximumSize().height());
			}

			if((*iter)->minimumSize().width() > DYN_SIZE_PE)
			{
				(*iter)->setGeometry(pos, pos_y, (*iter)->minimumSize().width(), oGLui::min((*iter)->maximumSize().height(), m_geometry.height()));
				pos += (*iter)->minimumSize().width() + m_spacing;
			}
			else if((*iter)->maximumSize().width() < DYN_SIZE_PE)
			{
				(*iter)->setGeometry(pos, pos_y, (*iter)->maximumSize().width(), oGLui::min((*iter)->maximumSize().height(), m_geometry.height()));
				pos += (*iter)->maximumSize().width() + m_spacing;
			}
			else
			{
				(*iter)->setGeometry(pos, pos_y, DYN_SIZE_PE, oGLui::min((*iter)->maximumSize().height(), m_geometry.height()));
				pos += DYN_SIZE_PE + m_spacing;
			}
		}
		else
		{
			unsigned int pos_x = m_geometry.left();

			if((*iter)->maximumSize().width() < m_geometry.width())
			{
				// Left doesn't change anything
				if(m_halign == oGLui::Center)
					pos_x += (m_geometry.width() - (*iter)->maximumSize().width()) / 2;
				else if(m_halign == oGLui::Right)
					pos_x += (m_geometry.width() - (*iter)->maximumSize().width());
			}

			if((*iter)->minimumSize().height() > DYN_SIZE_PE)
			{
				(*iter)->setGeometry(pos_x, pos, oGLui::min((*iter)->maximumSize().width(), m_geometry.width()), (*iter)->minimumSize().height());
				pos += (*iter)->minimumSize().width() + m_spacing;
			}
			else if((*iter)->maximumSize().height() < DYN_SIZE_PE)
			{
				(*iter)->setGeometry(pos_x, pos, oGLui::min((*iter)->maximumSize().width(), m_geometry.width()), (*iter)->maximumSize().height());
				pos += (*iter)->maximumSize().width() + m_spacing;
			}
			else
			{
				(*iter)->setGeometry(pos_x, pos, oGLui::min((*iter)->maximumSize().width(), m_geometry.width()), DYN_SIZE_PE);
				pos += DYN_SIZE_PE + m_spacing;
			}
		}
	}

#undef DYN_SIZE_PE
}
