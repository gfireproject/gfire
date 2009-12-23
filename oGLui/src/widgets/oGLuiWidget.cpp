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

#include <widgets/oGLuiWidget.h>

#include <core/oGLuiPoint.h>
#include <core/oGLuiPainter.h>
#include <core/oGLuiLayout.h>
#include <events/oGLuiEvent.h>
#include <events/oGLuiPaintEvent.h>

#include <cstdlib>
#include <cstring>

oGLuiWidget::oGLuiWidget(oGLuiWidget *p_parent) :
		oGLuiObject(p_parent), m_pixmap(100, 100)
{
	// Set Widget status for oGLuiObject
	m_is_widget = true;

	if(!oGLuiApp)
		throw("No oGLuiApp instance was present when using oGLuiWidget!");

	m_style = 0;
	m_draw_background = false;
	m_border = oGLui::BorderNone;
	m_background_role = oGLui::BackgroundRole;
	m_foreground_role = oGLui::TextRole;
	m_hidden = false;
	m_layout = 0;

	// OpenGL Texture
	m_texture = 0;
	m_check_color[0] = static_cast<GLfloat>(rand() % 255) / 255.0f;
	m_check_color[1] = static_cast<GLfloat>(rand() % 255) / 255.0f;
	m_check_color[2] = static_cast<GLfloat>(rand() % 255) / 255.0f;
	m_check_color[3] = static_cast<GLfloat>(rand() % 255) / 255.0f;

	m_size_min = oGLuiSize(1, 1);
	m_size_max = oGLuiSize(UINT_MAX, UINT_MAX);
	setGeometry(oGLuiRect(0, 0, 100, 100));
	setClientMargin(oGLuiRect(5, 5, 5, 5));

	if(!p_parent)
		oGLuiApp->registerWidget(this);

	// Define signals and slots
	OGLUI_SIGNAL("hidden");

	OGLUI_SLOT(showSlot);
	OGLUI_SLOT(hideSlot);

	invalidate();
}

oGLuiWidget::~oGLuiWidget()
{
	freeTexture();

	if(!parent())
		oGLuiApp->removeWidget(this);
}

void oGLuiWidget::setParent(oGLuiObject *p_parent)
{
	if(parent() != p_parent)
	{
		if(parent() && !p_parent)
			oGLuiApp->registerWidget(this);
		else if(p_parent && !parent())
			oGLuiApp->removeWidget(this);

		oGLuiObject::setParent(p_parent);
	}
}

const oGLuiPixmap &oGLuiWidget::pixmap()
{
	return m_pixmap;
}

const oGLuiRect &oGLuiWidget::geometry() const
{
	return m_geometry;
}

void oGLuiWidget::setGeometry(const oGLuiRect &p_geometry)
{
	m_geometry = p_geometry;

	if(m_geometry.width() == 0)
		m_geometry.setRight(m_geometry.right() + 1);

	if(m_geometry.height() == 0)
		m_geometry.setBottom(m_geometry.bottom() + 1);

	updateSize();
}

void oGLuiWidget::setGeometry(unsigned int p_top, unsigned int p_left, unsigned int p_width, unsigned int p_height)
{
	setGeometry(oGLuiRect(p_left, p_top, p_left + p_width, p_top + p_height));
}

const oGLuiSize &oGLuiWidget::minimumSize() const
{
	return m_size_min;
}

void oGLuiWidget::setMinimumSize(const oGLuiSize &p_size)
{
	m_size_min = p_size;
	if(m_size_min.width() < 1)
		m_size_min.setWidth(1);
	if(m_size_min.height() < 1)
		m_size_min.setHeight(1);

	if(parent() && parent()->inherits("oGLuiLayout"))
	{
		reinterpret_cast<oGLuiLayout*>(parent())->update();
		return;
	}

	updateSize();
}

void oGLuiWidget::setMinimumSize(unsigned int p_width, unsigned int p_height)
{
	setMinimumSize(oGLuiSize(p_width, p_height));
}

const oGLuiSize &oGLuiWidget::maximumSize() const
{
	return m_size_max;
}

void oGLuiWidget::setMaximumSize(const oGLuiSize &p_size)
{
	m_size_max = p_size;

	if(parent() && parent()->isWidget() && static_cast<oGLuiWidget*>(parent())->layout())
	{
		static_cast<oGLuiWidget*>(parent())->layout()->update();
		return;
	}

	updateSize();
}

void oGLuiWidget::setMaximumSize(unsigned int p_width, unsigned int p_height)
{
	setMaximumSize(oGLuiSize(p_width, p_height));
}

void oGLuiWidget::updateSize()
{
	if(m_geometry.width() < m_size_min.width())
		m_geometry.resize(m_size_min.width(), m_geometry.height());

	if(m_geometry.width() > m_size_max.width())
		m_geometry.resize(m_size_max.width(), m_geometry.height());

	if(m_geometry.height() < m_size_min.height())
		m_geometry.resize(m_geometry.width(), m_size_min.height());

	if(m_geometry.height() > m_size_max.height())
		m_geometry.resize(m_geometry.width(), m_size_max.height());

	m_pixmap.resize(m_geometry.size());

	if(m_layout)
		m_layout->setGeometry(clientRect());

	invalidate();
}

void oGLuiWidget::setClientMargin(const oGLuiRect &p_margin)
{
	m_client_margin = p_margin;
}

void oGLuiWidget::setClientMargin(unsigned int p_top, unsigned int p_left, unsigned int p_right, unsigned int p_bottom)
{
	setClientMargin(oGLuiRect(p_left, p_top, p_right, p_bottom));
}

const oGLuiRect &oGLuiWidget::clientMargin() const
{
	return m_client_margin;
}

oGLuiRect oGLuiWidget::clientRect() const
{
	return oGLuiRect(style()->borderExtents(m_border).left() + m_client_margin.left(),
						 style()->borderExtents(m_border).top() + m_client_margin.top(),
						 m_geometry.width() - m_client_margin.right() - style()->borderExtents(m_border).right(),
						 m_geometry.height() - m_client_margin.bottom() - style()->borderExtents(m_border).bottom());
}

int oGLuiWidget::clientWidth() const
{
	return (width() -
			style()->borderExtents(m_border).left() - m_client_margin.left() -
			m_client_margin.right() - style()->borderExtents(m_border).right());
}

int oGLuiWidget::clientHeight() const
{
	return (height() -
			style()->borderExtents(m_border).top() - m_client_margin.top() -
			m_client_margin.bottom() - style()->borderExtents(m_border).bottom());
}

oGLuiPoint oGLuiWidget::pos() const
{
	return m_geometry.topLeft();
}

void oGLuiWidget::move(const oGLuiPoint &p_point)
{
	oGLuiSize size = m_geometry.size();
	m_geometry.setTopLeft(p_point);
	m_geometry.resize(size);
}

void oGLuiWidget::move(unsigned int p_x, unsigned int p_y)
{
	move(oGLuiPoint(p_x, p_y));
}

int oGLuiWidget::width() const
{
	return m_geometry.width();
}

int oGLuiWidget::height() const
{
	return m_geometry.height();
}

void oGLuiWidget::resize(const oGLuiSize &p_size)
{
	m_geometry.resize(p_size);

	updateSize();
}

void oGLuiWidget::resize(unsigned int p_width, unsigned int p_height)
{
	resize(oGLuiSize(p_width, p_height));
}

void oGLuiWidget::setLayout(oGLuiLayout *p_layout)
{
	if(m_layout != p_layout)
	{
		if(m_layout)
			delete m_layout;

		m_layout = p_layout;

		if(m_layout)
		{
			m_layout->setParent(this);
			m_layout->setGeometry(clientRect());
		}
	}
}

oGLuiLayout *oGLuiWidget::layout() const
{
	return m_layout;
}

void oGLuiWidget::setStyle(oGLuiStyle *p_style)
{
	if(p_style != m_style)
	{
		if(m_style)
			m_style->unref();

		m_style = p_style;

		if(m_style)
			m_style->ref();

		invalidate();
	}
}

const oGLuiStyle *oGLuiWidget::style() const
{
	return (m_style ? m_style : oGLuiApp->style());
}

bool oGLuiWidget::drawBackground() const
{
	return m_draw_background;
}

void oGLuiWidget::setDrawBackground(bool p_drawit)
{
	if(m_draw_background != p_drawit)
	{
		m_draw_background = p_drawit;
		invalidate();
	}
}

oGLui::BorderStyle oGLuiWidget::borderStyle() const
{
	return m_border;
}

void oGLuiWidget::setBorderStyle(oGLui::BorderStyle p_style)
{
	if(m_border != p_style)
	{
		m_border = p_style;

		updateSize();
	}
}

void oGLuiWidget::setBackgroundRole(unsigned int p_role)
{
	if(m_background_role != p_role)
	{
		m_background_role = p_role;

		invalidate();
	}
}

unsigned int oGLuiWidget::backgroundRole() const
{
	return m_background_role;
}

void oGLuiWidget::setForegroundRole(unsigned int p_role)
{
	if(m_foreground_role != p_role)
	{
		m_foreground_role = p_role;

		invalidate();
	}
}

unsigned int oGLuiWidget::foregroundRole() const
{
	return m_foreground_role;
}

void oGLuiWidget::updateTexture(bool p_redraw)
{
	// Textures are only for top-level widgets
	if(parent() || !oGLuiApp->hasEnvironment())
		return;

	bool regenerate = false;
	if(glIsTexture(m_texture) != GL_TRUE)
		regenerate = true;
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_texture);
		if(glGetError() == GL_INVALID_VALUE)
			regenerate = true;
		else
		{
			GLfloat check_color[4];
			glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, check_color);
			if(memcmp(m_check_color, check_color, sizeof(GLfloat) * 4) != 0)
			{
				oGLuiApp->log().writeLine(oGLuiLog::Warning, "oGLuiWidget: Texture has been hijacked!");
				regenerate = true;
			}
		}

		OPENGL_CHECK;
	}

	if(regenerate)
	{
		// Regenerate texture
		m_texture = 0;
		oGLuiApp->log().writeLine(oGLuiLog::Info, "oGLuiWidget: Regenerating texture...");
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, m_check_color);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		OPENGL_CHECK;
	}

	if(p_redraw || regenerate)
		m_pixmap.toOpenGLTexture();
}

void oGLuiWidget::paintTexture()
{
	if(parent() || hidden() || !oGLuiApp->hasEnvironment())
		return;

	updateTexture(false);

	OPENGL_CHECK;

	GLdouble matrix[16];
	//glPushMatrix();
	glGetDoublev(GL_MODELVIEW_MATRIX, matrix);

	glTranslatef(m_geometry.left(), m_geometry.top(), 0.0);

	glBindTexture(GL_TEXTURE_2D, m_texture);

	OPENGL_CHECK;

	GLint vertex[] = { 0, 0, 0, m_geometry.height(), m_geometry.width(), m_geometry.height(), m_geometry.width(), 0 };
	GLfloat tex[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glVertexPointer(2, GL_INT, 0, vertex);
	glTexCoordPointer(2, GL_FLOAT, 0, tex);
	glDrawArrays(GL_QUADS, 0, 4);

	OPENGL_CHECK;

	//glPopMatrix();
	glLoadMatrixd(matrix);

	OPENGL_CHECK;
}

void oGLuiWidget::freeTexture()
{
	if(glIsTexture(m_texture))
	{
		glBindTexture(GL_TEXTURE_2D, m_texture);
		GLfloat check_color[4];
		glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, check_color);
		if(memcmp(m_check_color, check_color, sizeof(GLfloat) * 4) == 0)
		{
			glDeleteTextures(1, &m_texture);
			OPENGL_CHECK;
		}
	}
}

void oGLuiWidget::invalidate()
{
	oGLuiApp->postEvent(this, new oGLuiPaintEvent());
}

bool oGLuiWidget::hasFocus() const
{
	return m_focus;
}

void oGLuiWidget::setFocus(bool p_focus)
{
	m_focus = true;
}

bool oGLuiWidget::canHaveFocus() const
{
	return true;
}

void oGLuiWidget::showSlot(oGLuiObject *p_this, void *)
{
	if(p_this)
	{
		static_cast<oGLuiWidget*>(p_this)->m_hidden = false;
		p_this->emit("hidden", reinterpret_cast<void*>(false));
	}
}

void oGLuiWidget::hideSlot(oGLuiObject *p_this, void *)
{
	if(p_this)
	{
		static_cast<oGLuiWidget*>(p_this)->m_hidden = true;
		p_this->emit("hidden", reinterpret_cast<void*>(true));
	}
}

bool oGLuiWidget::hidden() const
{
	return m_hidden;
}

bool oGLuiWidget::eventHandler(oGLuiEvent *p_event)
{
	if(!p_event)
		return false;

	bool accepted = false;

	switch(p_event->type())
	{
		case oGLuiEvent::ETPaint:
		{
			// PaintEvent itself / Widget content
			accepted = paintEvent(reinterpret_cast<oGLuiPaintEvent*>(p_event));
			if(!parent())
				p_event->accept();

			oGLuiPainter painter(&m_pixmap);

			// Copy child widgets without layout
			list<oGLuiObject*>::const_iterator iter;
			for(iter = children().begin(); iter != children().end(); iter++)
			{
				if((*iter)->isWidget())
				{
					painter.copy_full(reinterpret_cast<oGLuiWidget*>(*iter)->geometry().left(),
									  reinterpret_cast<oGLuiWidget*>(*iter)->geometry().top(),
									  reinterpret_cast<oGLuiWidget*>(*iter)->pixmap());
				}
			}

			// Update the texture
			updateTexture();
			break;
		}
		default:
			return oGLuiObject::eventHandler(p_event);
	}

	return accepted;
}

bool oGLuiWidget::paintEvent(oGLuiPaintEvent *p_event)
{
	if(!p_event)
		return false;

	oGLuiPainter painter(&m_pixmap);

	// Clear the background
	m_pixmap.clear();

	// Draw background
	if(drawBackground())
	{
		painter.setColor(style()->palette().color(backgroundRole()));
		painter.fillRectangle(oGLuiRect(0, 0, width(), height()));
	}

	// Draw border
	style()->drawBorder(&painter, m_border);

	return true;
}
