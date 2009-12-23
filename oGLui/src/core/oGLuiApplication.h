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

#ifndef _OGLUIAPPLICATION_H
#define _OGLUIAPPLICATION_H

#include <list>
#include <sstream>
#include <core/oGLui.h>
#include <core/oGLuiObject.h>
#include <core/oGLuiPoint.h>
#include <core/oGLuiLog.h>
#include <core/oGLuiSolidStyle.h>
#include <core/oGLuiPointer.h>

class oGLuiEvent;
class oGLuiWidget;
class oGLuiSize;

#define oGLuiApp oGLuiApplication::instance()

class oGLuiApplication : public oGLuiObject
{
	OGLUI_OBJECT(oGLuiApplication)

	private:
		static oGLuiApplication *m_self;

		oGLuiLog m_log;

		// Events
		struct event_container
		{
			oGLuiEvent *event;
			oGLuiObject *recipient;
		};

		std::list<event_container> m_events;
		std::list<oGLuiWidget*> m_widgets;

		// Mouse Pos
		oGLuiPoint m_last_mouse_pos;

		// Pointer
		oGLuiPointer m_pointer;

		// Environment
		Display *m_display;
		GLXDrawable m_drawable;
		GLXContext m_context;

		bool m_ogl_setup;
		GLint m_old_program;
		GLint m_old_viewport[4];
		GLdouble m_old_projection[16];
		GLdouble m_old_model[16];
		GLdouble m_old_texture[16];
		void setupOpenGL();
		bool isOpenGLSetUp() const;
		void restoreOpenGL();

		// Application Default Style
		oGLuiStyle *m_style;

		// Event handling
		void removeEventsFor(oGLuiObject *p_recipient);

	public:
		oGLuiApplication();
		~oGLuiApplication();

		static oGLuiApplication *instance() { return m_self; }

		oGLuiLog &log();

		// Environment
		void updateEnvironment(Display *p_display, GLXDrawable p_drawable);
		bool hasEnvironment() const;
		GLXContext oGLContext() const;

		unsigned int screenWidth() const;
		unsigned int screenHeight() const;
		oGLuiSize screenSize() const;

		void registerWidget(oGLuiWidget *p_widget);
		void removeWidget(const oGLuiWidget *p_widget);

		// Event handling
		void postEvent(oGLuiObject *p_recipient, oGLuiEvent *p_event);
		void processEvents();

		void keyboardInput(const XKeyEvent *p_event);
		void mouseMove(const XMotionEvent *p_event);
		void mouseInput(const XButtonEvent *p_event);

		// Pointer
		oGLuiPointer &pointer();

		// Painting
		void paintAll();

		// Style
		void setStyle(oGLuiStyle *p_style);
		const oGLuiStyle *style() const;

		friend class oGLuiObject;
};

#ifdef DEBUG
#define OPENGL_CHECK \
{ \
	GLenum gl_error = glGetError(); \
	if(gl_error != GL_NO_ERROR) \
	{ \
		std::stringstream err_ss; \
		err_ss << __FILE__ << "(" << __LINE__ << "):OpenGL Error: " << gluErrorString(gl_error); \
		oGLuiApp->log().writeLine(oGLuiLog::Error, err_ss.str()); \
	} \
}
#else
#define OPENGL_CHECK
#endif // DEBUG

#endif // _OGLUIAPPLICATION_H
