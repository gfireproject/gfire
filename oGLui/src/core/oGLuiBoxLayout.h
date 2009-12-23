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

#ifndef _OGLUIBOXLAYOUT_H
#define _OGLUIBOXLAYOUT_H

#include <core/oGLui.h>
#include <core/oGLuiLayout.h>

#include <list>
using std::list;

class oGLuiBoxLayout : public oGLuiLayout
{
	OGLUI_OBJECT1(oGLuiBoxLayout, oGLuiLayout)

	public:
		virtual ~oGLuiBoxLayout();

		// oGLuiLayout functions
		void addElement(oGLuiLayoutElement *p_element);

		unsigned int count() const;
		oGLuiLayoutElement *elementAt(unsigned int p_index) const;

		// oGLuiLayoutElement functions
		virtual oGLuiSize minimumSize() const;
		virtual oGLuiSize maximumSize() const;

	protected:
		oGLuiBoxLayout(oGLui::Direction p_direction, oGLuiObject *p_parent = 0);

	private:
		void realign();

		oGLui::Direction m_direction;
		list<oGLuiLayoutElement*> m_elements;
};

class oGLuiVBoxLayout : public oGLuiBoxLayout
{
	OGLUI_OBJECT1(oGLuiVBoxLayout, oGLuiBoxLayout)

	public:
		oGLuiVBoxLayout(oGLuiObject *p_parent = 0) :
			oGLuiBoxLayout(oGLui::Vertical, p_parent) {}
};

class oGLuiHBoxLayout : public oGLuiBoxLayout
{
	OGLUI_OBJECT1(oGLuiHBoxLayout, oGLuiBoxLayout)

	public:
		oGLuiHBoxLayout(oGLuiObject *p_parent = 0) :
			oGLuiBoxLayout(oGLui::Horizontal, p_parent) {}
};

#endif // _OGLUIBOXLAYOUT_H
