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

#ifndef _OGLUI_H
#define _OGLUI_H

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif // HAVE_CONFIG_H

// X libraries
#include <X11/Xlib.h>

// OpenGL
#define GLX_GLXEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

// Cairo
#include <cairo/cairo.h>

#include <climits>

namespace oGLui
{
	/**
	 * \enum PixelFormat
	 * Possible pixel formats for Pixmaps
	**/
	enum PixelFormat
	{
		ARGB = CAIRO_FORMAT_ARGB32, // RGB + Alpha channel
		RGB = CAIRO_FORMAT_RGB24 // RGB
	};

	/**
	 * \enum MouseButton
	 * Possible mouse buttons for button events
	 * @see oGLuiMouseButtonEvent
	**/
	enum MouseButton
	{
		LeftButton,
		RightButton
	};

	enum HAlignment
	{
		Left,
		Center,
		Right
	};

	enum VAlignment
	{
		Top,
		Middle,
		Bottom
	};

	enum Direction
	{
		Horizontal,
		Vertical
	};

	enum BorderStyle
	{
		BorderNone,
		BorderSolid,
		BorderSunk,
		BorderRaised
	};

	enum ColorRole
	{
		BackgroundRole = 0,
		TextRole = 1,
		ButtonRole = 2,
		ButtonTextRole = 3,
		InputRole = 4,
		InputTextRole = 5,
		InputHighlightRole = 6,
		InputHighlightTextRole = 7,
		// 100 - 999 Reserved for styles
		StyleRole = 100,
		// 1000 - ... Reserved for users
		UserRole = 1000
	};
}

#endif // _OGLUI_H
