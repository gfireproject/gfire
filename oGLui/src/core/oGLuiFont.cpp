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

#include <core/oGLuiFont.h>

oGLuiFont::oGLuiFont(const std::string &p_family, unsigned int p_size)
{
	m_font_desc = pango_font_description_new();

	if(p_family.length() > 0)
		setFamily(p_family);
	else
		setFamily(defaultFamily());

	setSize(p_size);

	m_bold = false;
	m_italic = false;
	m_underlined = false;
	m_allcapital = false;
}

oGLuiFont::oGLuiFont(const oGLuiFont &p_font)
{
	operator=(p_font);
}

oGLuiFont::~oGLuiFont()
{
	pango_font_description_free(m_font_desc);
}

void oGLuiFont::setFamily(const std::string &p_family)
{
	m_family = p_family;
	pango_font_description_set_family(m_font_desc, m_family.c_str());
}

const std::string &oGLuiFont::family() const
{
	return m_family;
}

void oGLuiFont::setSize(unsigned int p_size)
{
	m_size = p_size;
	pango_font_description_set_size(m_font_desc, m_size * PANGO_SCALE);
}

unsigned int oGLuiFont::size() const
{
	return m_size;
}

void oGLuiFont::setBold(bool p_bold)
{
	m_bold = p_bold;
	pango_font_description_set_weight(m_font_desc, m_bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
}

bool oGLuiFont::bold() const
{
	return m_bold;
}

void oGLuiFont::setItalic(bool p_italic)
{
	m_italic = p_italic;
	pango_font_description_set_style(m_font_desc, m_italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
}

bool oGLuiFont::italic() const
{
	return m_italic;
}

void oGLuiFont::setUnderlined(bool p_underlined)
{
	m_underlined = p_underlined;
}

bool oGLuiFont::underlined() const
{
	return m_underlined;
}

void oGLuiFont::setAllCapital(bool p_capital)
{
	m_allcapital = p_capital;
}

bool oGLuiFont::allCapital() const
{
	return m_allcapital;
}

oGLuiFont &oGLuiFont::operator=(const oGLuiFont &p_font)
{
	if(this != &p_font)
	{
		m_family = p_font.m_family;
		m_size = p_font.m_size;
		m_bold = p_font.m_bold;
		m_italic = p_font.m_italic;
		m_underlined = p_font.m_underlined;
		m_allcapital = p_font.m_allcapital;

		m_font_desc = pango_font_description_copy(p_font.m_font_desc);
	}

	return *this;
}
