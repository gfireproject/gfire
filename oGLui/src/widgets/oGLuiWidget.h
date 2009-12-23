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

#ifndef _OGLUIWIDGET_H
#define _OGLUIWIDGET_H

#include <vector>

#include <core/oGLuiApplication.h>
#include <core/oGLuiRect.h>
#include <core/oGLuiColor.h>
#include <core/oGLuiObject.h>
#include <core/oGLuiPixmap.h>
#include <core/oGLuiSize.h>
#include <core/oGLuiStyle.h>

using std::vector;

class oGLuiPainter;
class oGLuiPoint;
class oGLuiLayout;
class oGLuiEvent;
class oGLuiPaintEvent;

class oGLuiWidget : public oGLuiObject
{
	OGLUI_OBJECT(oGLuiWidget)

	private:
		// Geometry
		oGLuiRect m_geometry;
		oGLuiSize m_size_min;
		oGLuiSize m_size_max;
		oGLuiRect m_client_margin;
		oGLuiLayout *m_layout;

		void updateSize();

		// Style
		oGLuiStyle *m_style;
		bool m_draw_background;
		oGLui::BorderStyle m_border;
		unsigned int m_background_role;
		unsigned int m_foreground_role;

		// State
		bool m_focus;
		bool m_hidden;

		// Painting
		void updateTexture(bool p_redraw = true);
		void freeTexture();

		GLuint m_texture;
		GLfloat m_check_color[4];

		// Event handling
		virtual bool eventHandler(oGLuiEvent *p_event);

	protected:
		oGLuiPixmap m_pixmap;

		virtual bool paintEvent(oGLuiPaintEvent *p_event);

	public:
		oGLuiWidget(oGLuiWidget *p_parent = 0);
		virtual ~oGLuiWidget();

		void setParent(oGLuiObject *p_parent);

		const oGLuiPixmap &pixmap();

		// Geometry
		const oGLuiRect &geometry() const;
		void setGeometry(const oGLuiRect &p_geometry);
		void setGeometry(unsigned int p_top, unsigned int p_left, unsigned int p_width, unsigned int p_height);

		const oGLuiSize &minimumSize() const;
		void setMinimumSize(const oGLuiSize &p_size);
		void setMinimumSize(unsigned int p_width, unsigned int p_height);

		const oGLuiSize &maximumSize() const;
		void setMaximumSize(const oGLuiSize &p_size);
		void setMaximumSize(unsigned int p_width, unsigned int p_height);

		void setClientMargin(const oGLuiRect &p_margin);
		void setClientMargin(unsigned int p_top, unsigned int p_left, unsigned int p_right, unsigned int p_bottom);
		const oGLuiRect &clientMargin() const;
		oGLuiRect clientRect() const;
		int clientWidth() const;
		int clientHeight() const;

		oGLuiPoint pos() const;
		void move(const oGLuiPoint &p_point);
		void move(unsigned int p_x, unsigned int p_y);

		int width() const;
		int height() const;
		void resize(const oGLuiSize &p_size);
		void resize(unsigned int p_width, unsigned int p_height);

		void setLayout(oGLuiLayout *p_layout);
		oGLuiLayout *layout() const;

		// Style
		void setStyle(oGLuiStyle *p_style);
		const oGLuiStyle *style() const;

		bool drawBackground() const;
		void setDrawBackground(bool p_drawit);
		oGLui::BorderStyle borderStyle() const;
		void setBorderStyle(oGLui::BorderStyle p_style);

		void setBackgroundRole(unsigned int p_role);
		unsigned int backgroundRole() const;

		void setForegroundRole(unsigned int p_role);
		unsigned int foregroundRole() const;

		// Painting
		void invalidate();
		void paintTexture();

		// State
		bool hasFocus() const;
		void setFocus(bool p_focus);
		bool canHaveFocus() const;

		inline void show() { showSlot(this, 0); }
		inline void hide() { hideSlot(this, 0); }
		bool hidden() const;

	private slots:
		static void showSlot(oGLuiObject *p_this, void *);
		static void hideSlot(oGLuiObject *p_this, void *);

		friend class oGLuiLayout;
};

#endif // _OGLUIWIDGET_H
