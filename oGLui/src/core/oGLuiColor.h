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

#ifndef _OGLUICOLOR_H
#define _OGLUICOLOR_H

#include <string>

class oGLuiColor
{
	private:
		unsigned char m_r, m_g, m_b, m_a;

	public:
		oGLuiColor(unsigned char p_r = 0, unsigned char p_g = 0, unsigned char p_b = 0, unsigned char p_a = 255);
		oGLuiColor(const oGLuiColor &p_color);

		unsigned char red() const;
		float redf() const;
		void setRed(unsigned char p_red);
		void setRedf(float p_red);

		unsigned char green() const;
		float greenf() const;
		void setGreen(unsigned char p_green);
		void setGreenf(float p_green);

		unsigned char blue() const;
		float bluef() const;
		void setBlue(unsigned char p_blue);
		void setBluef(float p_blue);

		unsigned char alpha() const;
		float alphaf() const;
		void setAlpha(unsigned char p_alpha);
		void setAlphaf(float p_alpha);

		void setColor(unsigned char p_r, unsigned char p_g, unsigned char p_b, unsigned char p_a = 255);
		void setColorf(float p_r, float p_g, float p_b, float p_a = 1.0f);

		unsigned int toUInt() const;
		void fromUInt(unsigned int p_int);

		std::string toHexString() const;
		void fromHexString(const std::string &p_string);

		oGLuiColor &operator=(const oGLuiColor &p_color);
		bool operator==(const oGLuiColor &p_color);
		bool operator!=(const oGLuiColor &p_color);
};

#endif // _OGLUICOLOR_H
