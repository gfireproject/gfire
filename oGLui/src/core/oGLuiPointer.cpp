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

#include <core/oGLuiPointer.h>

#include <core/oGLuiRect.h>
#include <core/oGLuiApplication.h>
#include <cstdlib>
#include <cstring>

oGLuiPointer::oGLuiPointer() :
		m_pixmap(10, 10)
{
	m_hidden = false;
	m_pixmap.fillRectangle(oGLuiRect(0, 0, 10, 10), oGLuiColor());

	// OpenGL Texture
	m_texture = 0;
	m_check_color[0] = static_cast<GLfloat>(rand() % 255) / 255.0;
	m_check_color[1] = static_cast<GLfloat>(rand() % 255) / 255.0;
	m_check_color[2] = static_cast<GLfloat>(rand() % 255) / 255.0;
	m_check_color[3] = static_cast<GLfloat>(rand() % 255) / 255.0;
}

oGLuiPointer::~oGLuiPointer()
{
	freeTexture();
}

bool oGLuiPointer::loadFromPNG(const std::string &p_filename)
{
	if(!m_pixmap.fromPNGFile(p_filename))
		return false;
	else
	{
		updateTexture();
		return true;
	}
}

const oGLuiPoint &oGLuiPointer::pos() const
{
	return m_pos;
}

void oGLuiPointer::move(const oGLuiPoint &p_pos)
{
	m_pos = p_pos;
}

void oGLuiPointer::move(unsigned int p_x, unsigned int p_y)
{
	m_pos.setX(p_x);
	m_pos.setY(p_y);
}

void oGLuiPointer::updateTexture(bool p_redraw)
{
	if(!oGLuiApp->hasEnvironment())
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
				oGLuiApp->log().writeLine(oGLuiLog::Warning, "oGLuiPointer: Texture has been hijacked!");
				regenerate = true;
			}
		}

		OPENGL_CHECK;
	}

	if(regenerate)
	{
		// Regenerate texture
		m_texture = 0;
		oGLuiApp->log().writeLine(oGLuiLog::Info, "oGLuiPointer: Regenerating texture...");
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

void oGLuiPointer::paintTexture()
{
	if(hidden() || !oGLuiApp->hasEnvironment())
		return;

	updateTexture(false);

	OPENGL_CHECK;

	GLdouble matrix[16];
	//glPushMatrix();
	glGetDoublev(GL_MODELVIEW_MATRIX, matrix);

	glTranslatef(m_pos.x(), m_pos.y(), 0.0);

	glBindTexture(GL_TEXTURE_2D, m_texture);

	OPENGL_CHECK;

	GLint vertex[] = { 0, 0, 0, m_pixmap.size().height(), m_pixmap.size().width(), m_pixmap.size().height(), m_pixmap.size().width(), 0 };
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

void oGLuiPointer::freeTexture()
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

void oGLuiPointer::show()
{
	m_hidden = false;
}

void oGLuiPointer::hide()
{
	m_hidden = true;
}

bool oGLuiPointer::hidden() const
{
	return m_hidden;
}
