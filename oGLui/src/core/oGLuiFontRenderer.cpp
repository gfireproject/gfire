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

#include <core/oGLuiFontRenderer.h>
#include <core/oGLuiPixmap.h>
#include <core/oGLuiPoint.h>
#include <core/oGLuiColor.h>

oGLuiFontRenderer::oGLuiFontRenderer()
{
	m_layout = 0;
	m_target = 0;

	m_width = INT_MAX;
	m_height = INT_MAX;

	m_wordwrap = false;
	m_markup = false;

	m_align = oGLui::Left;
}

oGLuiFontRenderer::oGLuiFontRenderer(oGLuiPixmap *p_target, const oGLuiFont &p_font)
{
	oGLuiFontRenderer();

	setFont(p_font);
	setTarget(p_target);
}

oGLuiFontRenderer::~oGLuiFontRenderer()
{
	if(m_layout)
		g_object_unref(m_layout);
}

void oGLuiFontRenderer::setTarget(oGLuiPixmap *p_target)
{
	if(m_layout)
	{
		g_object_unref(m_layout);
		m_layout = 0;
	}

	m_target = p_target;
	regenLayout();
}

oGLuiPixmap *oGLuiFontRenderer::target() const
{
	return m_target;
}

void oGLuiFontRenderer::setFont(const oGLuiFont &p_font)
{
	m_font = p_font;

	if(m_layout)
		pango_layout_set_font_description(m_layout, m_font.m_font_desc);
}

const oGLuiFont &oGLuiFontRenderer::font() const
{
	return m_font;
}

void oGLuiFontRenderer::setUseMarkup(bool p_markup)
{
	m_markup = p_markup;
}

bool oGLuiFontRenderer::useMarkup() const
{
	return m_markup;
}

void oGLuiFontRenderer::setUseWordWrap(bool p_wordwrap)
{
	m_wordwrap = p_wordwrap;

	if(m_layout)
	{
		if(m_wordwrap)
			pango_layout_set_width(m_layout, m_width * PANGO_SCALE);
		else
			pango_layout_set_width(m_layout, -1);
	}
}

bool oGLuiFontRenderer::useWordWrap() const
{
	return m_wordwrap;
}

void oGLuiFontRenderer::setWidth(int p_width)
{
	m_width = p_width;

	if(m_layout && m_wordwrap)
		pango_layout_set_width(m_layout, m_width * PANGO_SCALE);
}

int oGLuiFontRenderer::width() const
{
	return m_width;
}

void oGLuiFontRenderer::setHeight(int p_height)
{
	m_height = p_height;

	if(m_layout)
		pango_layout_set_height(m_layout, m_height * PANGO_SCALE);
}

int oGLuiFontRenderer::height() const
{
	return m_height;
}

void oGLuiFontRenderer::setAlign(oGLui::HAlignment p_align)
{
	m_align = p_align;

	if(m_layout)
	{
		switch(m_align)
		{
			case oGLui::Left:
				pango_layout_set_alignment(m_layout, PANGO_ALIGN_LEFT);
				break;
			case oGLui::Right:
				pango_layout_set_alignment(m_layout, PANGO_ALIGN_RIGHT);
				break;
			case oGLui::Center:
				pango_layout_set_alignment(m_layout, PANGO_ALIGN_CENTER);
				break;
		}
	}
}

oGLui::HAlignment oGLuiFontRenderer::align() const
{
	return m_align;
}

void oGLuiFontRenderer::setText(const string &p_text)
{
	if(m_layout)
	{
		if(m_markup)
			pango_layout_set_markup(m_layout, p_text.c_str(), -1);
		else
		{
			pango_layout_set_attributes(m_layout, 0);
			pango_layout_set_text(m_layout, p_text.c_str(), -1);
		}
	}
}

oGLuiSize oGLuiFontRenderer::textExtents()
{
	int width = 0, height = 0;
	if(m_layout)
	{
		pango_cairo_update_layout(m_target->m_cr, m_layout);

		pango_layout_get_pixel_size(m_layout, &width, &height);
	}

	return oGLuiSize(width, height);
}

void oGLuiFontRenderer::draw(const oGLuiPoint &p_pos, const oGLuiColor &p_color)
{
	if(m_layout)
	{
		cairo_set_source_rgba(m_target->m_cr, p_color.redf(), p_color.greenf(), p_color.bluef(), p_color.alphaf());
		pango_cairo_update_layout(m_target->m_cr, m_layout);

		cairo_move_to(m_target->m_cr, p_pos.x(), p_pos.y());
		pango_cairo_show_layout(m_target->m_cr, m_layout);
	}
}

void oGLuiFontRenderer::regenLayout()
{
	if(!m_target)
		return;

	if(!m_layout)
	{
		m_layout = pango_cairo_create_layout(m_target->m_cr);
		if(!m_layout)
			return;
	}

	pango_layout_set_wrap(m_layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize(m_layout, PANGO_ELLIPSIZE_NONE);

	if(m_wordwrap)
		pango_layout_set_width(m_layout, m_width * PANGO_SCALE);
	else
		pango_layout_set_width(m_layout, -1);

	pango_layout_set_height(m_layout, m_height * PANGO_SCALE);

	switch(m_align)
	{
		case oGLui::Left:
			pango_layout_set_alignment(m_layout, PANGO_ALIGN_LEFT);
			break;
		case oGLui::Right:
			pango_layout_set_alignment(m_layout, PANGO_ALIGN_RIGHT);
			break;
		case oGLui::Center:
			pango_layout_set_alignment(m_layout, PANGO_ALIGN_CENTER);
			break;
	}

	pango_layout_set_font_description(m_layout, m_font.m_font_desc);
}
