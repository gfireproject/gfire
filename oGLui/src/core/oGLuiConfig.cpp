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

#include <core/oGLuiConfig.h>

#include <core/oGLuiApplication.h>
#include <fstream>
#include <sstream>

oGLuiConfig::oGLuiConfig(const std::string &p_filename)
{
	load(p_filename);
}

oGLuiConfig::oGLuiConfig(const oGLuiConfig &p_config)
{
	m_pairs = p_config.m_pairs;
}

oGLuiConfig::oGLuiConfig()
{
}

bool oGLuiConfig::load(const std::string &p_filename)
{
	if(p_filename.empty())
		return false;

	std::ifstream file(p_filename.c_str());
	if(!file.is_open())
	{
		std::stringstream ss;
		ss << "File \"" << p_filename << "\" couldn't be opened!";
		oGLuiApp->log().writeLine(oGLuiLog::Warning, ss.str());
		return false;
	}

	m_pairs.clear();

	char buffer[4096];
	unsigned int line_count = 0;
	while(file.good())
	{
		line_count++;
		file.getline(buffer, 4096);
		// Skip empty lines
		if(buffer[0] == 0)
			continue;

		std::string bufstr(buffer);

		// Find beginning of text
		size_t start = bufstr.find_first_not_of(" \t");
		// Skip empty lines
		if(start == std::string::npos)
			continue;

		// Skip comments
		if(bufstr.at(start) == '#')
			continue;

		// Find '=' sign
		size_t eq_pos = bufstr.find_first_of('=', start + 1);
		// Skip invalid lines
		if(eq_pos == std::string::npos)
		{
			std::stringstream ss;
			ss << p_filename << ": Line " << line_count << " is missing the '=' character or has an empty key name!";
			oGLuiApp->log().writeLine(oGLuiLog::Warning, ss.str());
			continue;
		}

		std::string key = bufstr.substr(start, eq_pos - 1);
		// Trim
		if(key.find_last_not_of(" \t") != std::string::npos)
			key.erase(key.find_last_not_of(" \t") + 1);

		std::string value = (bufstr.find_first_not_of(" \t", eq_pos + 1) != std::string::npos) ?
							bufstr.substr(bufstr.find_first_not_of(" \t", eq_pos + 1)) : "";
		// Trim
		if(value.find_last_not_of(" \t") != std::string::npos)
			value.erase(value.find_last_not_of(" \t") + 1);

		m_pairs[key] = value;
	}

	file.close();

	return true;
}

bool oGLuiConfig::save(const std::string &p_filename) const
{
	if(p_filename.empty())
		return false;

	std::ofstream file(p_filename.c_str());
	if(!file.is_open())
	{
		std::stringstream ss;
		ss << "File \"" << p_filename << "\" couldn't be opened!";
		oGLuiApp->log().writeLine(oGLuiLog::Warning, ss.str());
		return false;
	}

	std::map<std::string, std::string>::const_iterator iter;
	for(iter = m_pairs.begin(); iter != m_pairs.end(); iter++)
	{
		file << (*iter).first << '=' << (*iter).second << std::endl;
	}

	file.close();
	return true;
}

void oGLuiConfig::setPair(const std::string &p_key, const std::string &p_value)
{
	if(!p_key.empty())
		m_pairs[p_key] = p_value;
}

std::string oGLuiConfig::getValue(const std::string &p_key) const
{
	if(!p_key.empty() && hasKey(p_key))
		return m_pairs.at(p_key);
	else
		return std::string();
}

bool oGLuiConfig::hasKey(const std::string &p_key) const
{
	return (m_pairs.find(p_key) != m_pairs.end());
}

void oGLuiConfig::clear()
{
	m_pairs.clear();
}
