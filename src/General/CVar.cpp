
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CVar.cpp
// Description: CVar system. 'Borrowed' from ZDoom, which is written by Randi
//              Heit
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Utility/StringUtils.h"
#include "thirdparty/fmt/fmt/format.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
vector<CVar*> all_cvars;
} // namespace


// -----------------------------------------------------------------------------
//
// CVar (and subclasses) Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CIntCVar class constructor
// -----------------------------------------------------------------------------
CIntCVar::CIntCVar(const std::string& NAME, int defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Integer;
	all_cvars.push_back(this);
}

// -----------------------------------------------------------------------------
// CBoolCVar class constructor
// -----------------------------------------------------------------------------
CBoolCVar::CBoolCVar(const std::string& NAME, bool defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Boolean;
	all_cvars.push_back(this);
}

// -----------------------------------------------------------------------------
// CFloatCVar class constructor
// -----------------------------------------------------------------------------
CFloatCVar::CFloatCVar(const std::string& NAME, double defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Float;
	all_cvars.push_back(this);
}

// -----------------------------------------------------------------------------
// CStringCVar class constructor
// -----------------------------------------------------------------------------
CStringCVar::CStringCVar(const std::string& NAME, const std::string& defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::String;
	all_cvars.push_back(this);
}


// -----------------------------------------------------------------------------
//
// CVar Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Finds a CVar by name
// -----------------------------------------------------------------------------
CVar* CVar::get(const std::string& name)
{
	for (auto cvar : all_cvars)
	{
		if (cvar->name == name)
			return cvar;
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds all cvar names to a vector of strings
// -----------------------------------------------------------------------------
void CVar::putList(vector<std::string>& list)
{
	for (auto cvar : all_cvars)
	{
		if (!(cvar->flags & Flag::Secret))
			list.push_back(cvar->name);
	}
}

// -----------------------------------------------------------------------------
// Saves cvars to a config file
// -----------------------------------------------------------------------------
std::string CVar::writeAll()
{
	uint32_t max_size = 0;
	for (auto cvar : all_cvars)
	{
		if (cvar->name.size() > max_size)
			max_size = cvar->name.size();
	}

	fmt::memory_buffer buf;
	format_to(buf, "cvars\n{{\n");

	for (auto cvar : all_cvars)
	{
		if (cvar->flags & Flag::Save)
		{
			format_to(buf, "\t{} ", cvar->name);

			int spaces = max_size - cvar->name.size();
			for (int a = 0; a < spaces; a++)
				buf.push_back(' ');

			if (cvar->type == Type::Integer)
				format_to(buf, "{}\n", cvar->getValue().Int);

			if (cvar->type == Type::Boolean)
				format_to(buf, "{}\n", cvar->getValue().Bool);

			if (cvar->type == Type::Float)
				format_to(buf, "{:1.5f}\n", cvar->getValue().Float);

			if (cvar->type == Type::String)
			{
				auto value = StrUtil::escapedString(dynamic_cast<CStringCVar*>(cvar)->value, true);
				format_to(buf, "\"{}\"\n", value);
			}
		}
	}

	format_to(buf, "}}\n\n");

	return to_string(buf);
}

// -----------------------------------------------------------------------------
// Reads [value] into the CVar with matching [name],
// or does nothing if no CVar [name] exists
// -----------------------------------------------------------------------------
void CVar::set(const std::string& name, const std::string& value)
{
	for (auto cvar : all_cvars)
	{
		if (name == cvar->name)
		{
			if (cvar->type == Type::Integer)
				*dynamic_cast<CIntCVar*>(cvar) = StrUtil::toInt(value);

			if (cvar->type == Type::Boolean)
				*dynamic_cast<CBoolCVar*>(cvar) = StrUtil::toBoolean(value);

			if (cvar->type == Type::Float)
				*dynamic_cast<CFloatCVar*>(cvar) = StrUtil::toFloat(value);

			if (cvar->type == Type::String)
				*dynamic_cast<CStringCVar*>(cvar) = value;
		}
	}
}
