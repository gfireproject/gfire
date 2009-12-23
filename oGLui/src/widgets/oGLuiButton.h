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

#ifndef _OGLUIBUTTON_H
#define _OGLUIBUTTON_H

#include <widgets/oGLuiWidget.h>
#include <core/oGLuiFontRenderer.h>
#include <string>
using std::string;

class oGLuiButton : public oGLuiWidget
{
	OGLUI_OBJECT1(oGLuiButton, oGLuiWidget)

	public:
		oGLuiButton(const string &p_caption = "", oGLuiWidget *p_parent = 0);

		void setCaption(const string &p_caption);
		const string &caption() const;

	private:
		virtual bool paintEvent(oGLuiPaintEvent *p_event);

		bool m_down;
		string m_caption;
		oGLuiFontRenderer m_renderer;
};

#endif // _OGLUIBUTTON_H
