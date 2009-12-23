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

#include <core/oGLuiPixmap.h>

#include <core/oGLuiApplication.h>
#include <core/oGLuiPoint.h>
#include <core/oGLuiColor.h>
#include <core/oGLuiRect.h>
#include <core/oGLuiFont.h>
#include <core/oGLuiSize.h>

#include <cstring>
#include <sstream>

#define CAIRO_CHECK(context, surface, do_destroy, do_return, val) \
if(cairo_status(context) != CAIRO_STATUS_SUCCESS) \
{ \
	std::stringstream err_ss; \
	err_ss << __FILE__ << "(" << __LINE__ << "): Cairo Context Error: " << cairo_status_to_string(cairo_status(context)); \
	oGLuiApp->log().writeLine(oGLuiLog::Error, err_ss.str()); \
	if(do_destroy) { cairo_destroy(context); context = 0; cairo_surface_destroy(surface); surface = 0; } \
	if(do_return) return val; \
}

#define CAIRO_SURFACE_CHECK(surface, do_destroy, do_return, do_const, val) \
if(cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) \
{ \
	std::stringstream err_ss; \
	err_ss << __FILE__ << "(" << __LINE__ << "): Cairo Surface Error: " << cairo_status_to_string(cairo_surface_status(surface)); \
	oGLuiApp->log().writeLine(oGLuiLog::Error, err_ss.str()); \
	if(do_destroy) { cairo_surface_destroy(surface); surface = 0; } \
	if(do_return) return val; \
	else if(do_const) { oGLuiPixmap(1, 1); return val; } \
}

#define HAS_SURFACE(val) if(!m_surface) return val

oGLuiPixmap::oGLuiPixmap(unsigned int p_width, unsigned int p_height, oGLui::PixelFormat p_format)
{
	if(p_width < 1)
		p_width = 1;

	if(p_height < 1)
		p_height = 1;

	m_size.setWidth(p_width);
	m_size.setHeight(p_height);

	m_format = p_format;
	m_clipped = false;

	m_cr = 0;

	m_surface = cairo_image_surface_create(static_cast<cairo_format_t>(m_format), m_size.width(), m_size.height());
	CAIRO_SURFACE_CHECK(m_surface, true, true, false,);

	m_cr = cairo_create(m_surface);
	CAIRO_CHECK(m_cr, m_surface, true, false,);
}

oGLuiPixmap::oGLuiPixmap(const oGLuiPixmap &p_pixmap)
{
	m_size = p_pixmap.m_size;
	m_format = p_pixmap.m_format;
	m_clipped = false;

	m_surface = cairo_image_surface_create(static_cast<cairo_format_t>(m_format), m_size.width(), m_size.height());
	CAIRO_SURFACE_CHECK(m_surface, true, false, true,);

	m_cr = cairo_create(m_surface);
	CAIRO_CHECK(m_cr, m_surface, true, true,);

	// Copy content
	if(p_pixmap.m_surface)
	{
		cairo_set_source_surface(m_cr, p_pixmap.m_surface, 0, 0);
		cairo_paint(m_cr);
	}
}

oGLuiPixmap::oGLuiPixmap(const std::string &p_filename)
{
	if(p_filename.empty())
	{
		oGLuiPixmap(1, 1);
		return;
	}

	m_surface = cairo_image_surface_create_from_png(p_filename.c_str());
	CAIRO_SURFACE_CHECK(m_surface, true, false, true,);

	m_cr = cairo_create(m_surface);
	CAIRO_CHECK(m_cr, m_surface, true, true,);

	m_size.setWidth(cairo_image_surface_get_width(m_surface));
	m_size.setHeight(cairo_image_surface_get_height(m_surface));

	m_clipped = false;
	m_format = oGLui::ARGB;
}

oGLuiPixmap::~oGLuiPixmap()
{
	if(m_cr)
		cairo_destroy(m_cr);

	if(m_surface)
		cairo_surface_destroy(m_surface);
}

const oGLuiSize &oGLuiPixmap::size() const
{
	return m_size;
}

void oGLuiPixmap::resize(const oGLuiSize &p_size)
{
	HAS_SURFACE();

	cairo_surface_t *new_surface = cairo_image_surface_create(static_cast<cairo_format_t>(m_format), p_size.width(), p_size.height());
	CAIRO_SURFACE_CHECK(new_surface, true, true, false,);

	cairo_t *new_cr = cairo_create(new_surface);
	CAIRO_CHECK(new_cr, new_surface, true, true,);

	cairo_set_source_surface(new_cr, m_surface, 0, 0);
	cairo_paint(new_cr);

	cairo_destroy(m_cr);
	cairo_surface_destroy(m_surface);

	m_surface = new_surface;
	m_cr = new_cr;

	m_size = p_size;
}

void oGLuiPixmap::resize(unsigned int p_width, unsigned int p_height)
{
	resize(oGLuiSize(p_width, p_height));
}

void oGLuiPixmap::clear()
{
	HAS_SURFACE();

	cairo_set_operator(m_cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(m_cr);
	cairo_set_operator(m_cr, CAIRO_OPERATOR_OVER);
}

void oGLuiPixmap::drawPixel(const oGLuiPoint &p_pixel, const oGLuiColor &p_color)
{
	HAS_SURFACE();

	if(p_pixel.x() < 0 || p_pixel.x() > m_size.width() || p_pixel.y() < 0 || p_pixel.y() > m_size.height())
		return;

	cairo_set_source_rgba(m_cr, p_color.redf(), p_color.greenf(), p_color.bluef(), p_color.alphaf());
	cairo_rectangle(m_cr, p_pixel.x(), p_pixel.y(), 1, 1);
	cairo_fill(m_cr);
}

void oGLuiPixmap::drawLine(const oGLuiPoint &p_start, const oGLuiPoint &p_end, const oGLuiColor &p_color, unsigned int p_line_width)
{
	HAS_SURFACE();

	cairo_set_source_rgba(m_cr, p_color.redf(), p_color.greenf(), p_color.bluef(), p_color.alphaf());
	cairo_move_to(m_cr, p_start.x(), p_start.y());
	cairo_line_to(m_cr, p_end.x(), p_end.y());
	cairo_set_line_width(m_cr, p_line_width);
	cairo_stroke(m_cr);
}

void oGLuiPixmap::drawRectangle(const oGLuiRect &p_rect, const oGLuiColor &p_color, unsigned int p_line_width)
{
	HAS_SURFACE();

	cairo_set_source_rgba(m_cr, p_color.redf(), p_color.greenf(), p_color.bluef(), p_color.alphaf());
	cairo_rectangle(m_cr, p_rect.left(), p_rect.top(), p_rect.width(), p_rect.height());
	cairo_set_line_width(m_cr, p_line_width);
	cairo_stroke(m_cr);
}

void oGLuiPixmap::fillRectangle(const oGLuiRect &p_rect, const oGLuiColor &p_color)
{
	HAS_SURFACE();

	cairo_set_source_rgba(m_cr, p_color.redf(), p_color.greenf(), p_color.bluef(), p_color.alphaf());
	cairo_rectangle(m_cr, p_rect.left(), p_rect.top(), p_rect.width(), p_rect.height());
	cairo_fill(m_cr);
}

bool oGLuiPixmap::hasClipping() const
{
	return m_clipped;
}

void oGLuiPixmap::clipRect(const oGLuiRect &p_rect)
{
	HAS_SURFACE();

	cairo_rectangle(m_cr, p_rect.left(), p_rect.top(), p_rect.width(), p_rect.height());
	cairo_clip(m_cr);

	m_clipped = true;
}

void oGLuiPixmap::clipRectInverted(const oGLuiRect &p_rect)
{
	HAS_SURFACE();

	// [************]
	// **** CLIP ****
	// **************
	cairo_rectangle(m_cr, 0, 0, size().width(), p_rect.top());
	cairo_clip(m_cr);

	// **************
	// [**] CLIP ****
	// **************
	cairo_rectangle(m_cr, 0, p_rect.top(), p_rect.left(), p_rect.height());
	cairo_clip(m_cr);

	// **************
	// **** CLIP [**]
	// **************
	cairo_rectangle(m_cr, p_rect.top() + p_rect.width(), p_rect.top(), size().width() - (p_rect.top() + p_rect.width()), p_rect.height());
	cairo_clip(m_cr);

	// **************
	// **** CLIP ****
	// [************]
	cairo_rectangle(m_cr, 0, p_rect.top() + p_rect.height(), size().width(), size().height() - (p_rect.top() + p_rect.height()));
	cairo_clip(m_cr);

	m_clipped = true;
}

void oGLuiPixmap::removeClipping()
{
	HAS_SURFACE();

	cairo_reset_clip(m_cr);
	m_clipped = false;
}

bool oGLuiPixmap::fromPNGFile(const std::string &p_filename)
{
	if(p_filename.empty())
		return false;

	cairo_surface_t *new_surface = cairo_image_surface_create_from_png(p_filename.c_str());
	if(!new_surface || cairo_surface_status(new_surface) != CAIRO_STATUS_SUCCESS)
	{
		if(new_surface)
			cairo_surface_destroy(new_surface);

		return false;
	}

	if(m_cr)
		cairo_destroy(m_cr);

	if(m_surface)
		cairo_surface_destroy(m_surface);

	m_surface = new_surface;
	m_cr = cairo_create(m_surface);
	CAIRO_CHECK(m_cr, m_surface, true, false, false);

	m_size.setWidth(cairo_image_surface_get_width(m_surface));
	m_size.setHeight(cairo_image_surface_get_height(m_surface));

	return true;
}

oGLuiPixmap *oGLuiPixmap::createFromPNGFile(const std::string &p_filename)
{
	if(p_filename.empty())
		return 0;

	oGLuiPixmap *ret = new oGLuiPixmap(1, 1);
	if(ret->fromPNGFile(p_filename))
		return ret;
	else
	{
		delete ret;
		return 0;
	}
}

bool oGLuiPixmap::toPNGFile(const std::string &p_filename) const
{
	HAS_SURFACE(false);

	if(p_filename.empty())
		return false;

	if(cairo_surface_write_to_png(m_surface, p_filename.c_str()) != CAIRO_STATUS_SUCCESS)
		return false;

	return true;
}

void oGLuiPixmap::toOpenGLTexture() const
{
	HAS_SURFACE();

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_size.width(), m_size.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, cairo_image_surface_get_data(m_surface));
}

bool oGLuiPixmap::oGLFrameBufferToPNGFile(const std::string &p_filename)
{
	if(!oGLuiApp)
		return false;

	oGLuiSize size = oGLuiApp->screenSize();

	// Save without Alpha (screen doesn't have any transparent areas)
	int stride = cairo_format_stride_for_width(static_cast<cairo_format_t>(oGLui::RGB), size.width());

	unsigned char *frame = new unsigned char[stride * size.height()];
	// Request with Alpha Channel, even if cairo handles it as without alpha channel it reserves space for it
	glReadPixels(0, 0, size.width(), size.height(), GL_BGRA, GL_UNSIGNED_BYTE, frame);

	// Rotate picture for 180°
	unsigned char *temp = new unsigned char[stride];
	for(unsigned int i = 0; i < (size.height() / 2); i++)
	{
		memcpy(temp, frame + (stride * i), stride);
		memcpy(frame + (stride * i), frame + (stride * (size.height() - i - 1)), stride);
		memcpy(frame + (stride * (size.height() - i - 1)), temp, stride);
	}
	delete[] temp;

	cairo_surface_t *surface = cairo_image_surface_create_for_data(frame, static_cast<cairo_format_t>(oGLui::RGB), size.width(), size.height(), stride);
	CAIRO_SURFACE_CHECK(surface, true, false, false, false);
	if(!surface)
	{
		delete[] frame;
		return false;
	}

	cairo_surface_write_to_png(surface, p_filename.c_str());
	CAIRO_SURFACE_CHECK(surface, true, false, false, false);
	if(!surface)
	{
		delete[] frame;
		return false;
	}

	cairo_surface_destroy(surface);
	delete[] frame;

	return true;
}

void oGLuiPixmap::copy_full(const oGLuiPoint &p_pos, const oGLuiPixmap &p_pixmap)
{
	HAS_SURFACE();

	cairo_set_source_surface(m_cr, p_pixmap.m_surface, p_pos.x(), p_pos.y());
	cairo_paint(m_cr);
}

void oGLuiPixmap::copy_full(unsigned int p_x, unsigned int p_y, const oGLuiPixmap &p_pixmap)
{
	copy_full(oGLuiPoint(p_x, p_y), p_pixmap);
}

void oGLuiPixmap::copy(const oGLuiPoint &p_pos, const oGLuiSize &p_size, const oGLuiPixmap &p_pixmap)
{
	HAS_SURFACE();

	cairo_set_source_surface(m_cr, p_pixmap.m_surface, p_pos.x(), p_pos.y());
	cairo_rectangle(m_cr, p_pos.x(), p_pos.y(), p_size.width(), p_size.height());
	cairo_fill(m_cr);
}

void oGLuiPixmap::copy(unsigned int p_x, unsigned int p_y, unsigned int p_width, unsigned int p_height, const oGLuiPixmap &p_pixmap)
{
	copy(oGLuiPoint(p_x, p_y), oGLuiSize(p_width, p_height), p_pixmap);
}

oGLuiPixmap oGLuiPixmap::scaled(const oGLuiSize &p_size) const
{
	HAS_SURFACE(oGLuiPixmap(1,1));

	oGLuiPixmap ret(p_size.width(), p_size.height(), m_format);

	cairo_scale(ret.m_cr, static_cast<double>(p_size.width())/static_cast<double>(m_size.width()),
							static_cast<double>(p_size.height())/static_cast<double>(m_size.height()));
	cairo_set_source_surface(ret.m_cr, m_surface, 0, 0);
	cairo_paint(ret.m_cr);

	return ret;
}

oGLuiPixmap oGLuiPixmap::scaled(unsigned int p_width, unsigned int p_height) const
{
	return scaled(oGLuiSize(p_width, p_height));
}
