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

#ifndef _OGLUILAYOUT_H
#define _OGLUILAYOUT_H

#include <core/oGLui.h>
#include <core/oGLuiObject.h>
#include <core/oGLuiLayoutElement.h>
#include <core/oGLuiRect.h>

#include <list>

using std::list;

class oGLuiWidget;
class oGLuiPainter;
class oGLuiSize;

class oGLuiLayout : public oGLuiObject, public oGLuiLayoutElement
{
	OGLUI_OBJECT(oGLuiLayout)

	public:
		oGLuiLayout(oGLuiObject *p_parent);

		void setParent(oGLuiObject *p_parent);

		void addWidget(oGLuiWidget *p_widget);
		virtual void addElement(oGLuiLayoutElement *p_element) = 0;

		void setSpacing(unsigned int p_spacing);
		unsigned int spacing() const;

		void setHorizontalAlign(oGLui::HAlignment p_align);
		oGLui::HAlignment horizontalAlign() const;

		void setVerticalAlign(oGLui::VAlignment p_align);
		oGLui::VAlignment verticalAlign() const;

		void update();

		virtual unsigned int count() const = 0;
		virtual oGLuiLayoutElement *elementAt(unsigned int p_index) const = 0;

		// oGLuiLayoutElement functions
		virtual void setGeometry(int p_x, int p_y, unsigned int p_width, unsigned int p_height);
		inline void setGeometry(const oGLuiRect &p_geometry)
		{
			setGeometry(p_geometry.left(), p_geometry.top(), p_geometry.width(), p_geometry.height());
		}

		oGLuiLayout *layout() const { return const_cast<oGLuiLayout*>(this); }

	private:
		virtual void realign() = 0;
		void getParentWidget();
		void reparentElements();

	protected:
		oGLuiRect m_geometry;
		unsigned int m_spacing;
		oGLuiObject *m_parent_widget;
		oGLui::HAlignment m_halign;
		oGLui::VAlignment m_valign;
};

#endif // _OGLUILAYOUT_H
