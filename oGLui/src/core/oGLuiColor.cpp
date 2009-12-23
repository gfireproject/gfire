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

#include <core/oGLuiColor.h>

#include <core/oGLuiUtils.h>
#include <sstream>
#include <cmath>

oGLuiColor::oGLuiColor(unsigned char p_r, unsigned char p_g, unsigned char p_b, unsigned char p_a)
{
	m_r = p_r;
	m_g = p_g;
	m_b = p_b;
	m_a = p_a;
}

oGLuiColor::oGLuiColor(const oGLuiColor &p_color)
{
	oGLuiColor::operator=(p_color);
}

unsigned char oGLuiColor::red() const
{
	return m_r;
}

float oGLuiColor::redf() const
{
	return static_cast<float>(m_r) / 255.0f;
}

void oGLuiColor::setRed(unsigned char p_red)
{
	m_r = p_red;
}

void oGLuiColor::setRedf(float p_red)
{
	if(p_red > 1.0f)
		p_red = 1.0f;
	else if(p_red < 0.0f)
		p_red = 0.0f;

	m_r = static_cast<unsigned char>(floor(p_red * 255.0f));
}

unsigned char oGLuiColor::green() const
{
	return m_g;
}

float oGLuiColor::greenf() const
{
	return static_cast<float>(m_g) / 255.0f;
}

void oGLuiColor::setGreen(unsigned char p_green)
{
	m_g = p_green;
}

void oGLuiColor::setGreenf(float p_green)
{
	if(p_green > 1.0f)
		p_green = 1.0f;
	else if(p_green < 0.0f)
		p_green = 0.0f;

	m_g = static_cast<unsigned char>(floor(p_green * 255.0f));
}

unsigned char oGLuiColor::blue() const
{
	return m_b;
}

float oGLuiColor::bluef() const
{
	return static_cast<float>(m_b) / 255.0f;
}

void oGLuiColor::setBlue(unsigned char p_blue)
{
	m_b = p_blue;
}

void oGLuiColor::setBluef(float p_blue)
{
	if(p_blue > 1.0f)
		p_blue = 1.0f;
	else if(p_blue < 0.0f)
		p_blue = 0.0f;

	m_b = static_cast<unsigned char>(floor(p_blue * 255.0f));
}

unsigned char oGLuiColor::alpha() const
{
	return m_a;
}

float oGLuiColor::alphaf() const
{
	return static_cast<float>(m_a) / 255.0f;
}

void oGLuiColor::setAlpha(unsigned char p_alpha)
{
	m_a = p_alpha;
}

void oGLuiColor::setAlphaf(float p_alpha)
{
	if(p_alpha > 1.0f)
		p_alpha = 1.0f;
	else if(p_alpha < 0.0f)
		p_alpha = 0.0f;

	m_b = static_cast<unsigned char>(floor(p_alpha * 255.0f));
}

void oGLuiColor::setColor(unsigned char p_r, unsigned char p_g, unsigned char p_b, unsigned char p_a)
{
	setRed(p_r);
	setGreen(p_g);
	setBlue(p_b);
	setAlpha(p_a);
}

void oGLuiColor::setColorf(float p_r, float p_g, float p_b, float p_a)
{
	setRedf(p_r);
	setGreenf(p_g);
	setBluef(p_b);
	setAlphaf(p_a);
}

unsigned int oGLuiColor::toUInt() const
{
	unsigned int ret = 0;
	ret += m_a << 24;
	ret += m_r << 16;
	ret += m_g << 8;
	ret += m_b;

	return ret;
}

void oGLuiColor::fromUInt(unsigned int p_int)
{
	setAlpha((p_int & (0xFF << 24)) >> 24);
	setRed((p_int & (0xFF << 16)) >> 16);
	setGreen((p_int & (0xFF << 8)) >> 8);
	setBlue(p_int & 0xFF);
}

std::string oGLuiColor::toHexString() const
{
	std::string ret;
	ret = "#" + oGLui::toHexString(m_r) + oGLui::toHexString(m_g) + oGLui::toHexString(m_b) + oGLui::toHexString(m_a);
	return ret;
}

void oGLuiColor::fromHexString(const std::string &p_string)
{
	if(p_string.empty())
		return;

	unsigned int pos = 0;
	if(p_string.at(pos) == '#')
	{
		if(p_string.length() < 7)
			return;

		pos++;
	}
	else if(p_string.length() < 6)
		return;

	setRed(oGLui::fromHexString(p_string.substr(pos, 2)));
	pos += 2;

	setGreen(oGLui::fromHexString(p_string.substr(pos, 2)));
	pos += 2;

	setBlue(oGLui::fromHexString(p_string.substr(pos, 2)));
	pos += 2;

	if(p_string.length() >= (pos + 2))
		setAlpha(oGLui::fromHexString(p_string.substr(pos, 2)));
	else
		setAlpha(255);
}

oGLuiColor &oGLuiColor::operator=(const oGLuiColor &p_color)
{
	if(this != &p_color)
	{
		m_r = p_color.m_r;
		m_g = p_color.m_g;
		m_b = p_color.m_b;
		m_a = p_color.m_a;
	}

	return *this;
}

bool oGLuiColor::operator==(const oGLuiColor &p_color)
{
	if(m_r == p_color.m_r &&
	   m_g == p_color.m_g &&
	   m_b == p_color.m_b &&
	   m_a == p_color.m_a)
		return true;

	return false;
}

bool oGLuiColor::operator!=(const oGLuiColor &p_color)
{
	return !(*this == p_color);
}
