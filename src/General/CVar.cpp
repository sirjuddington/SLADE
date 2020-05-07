
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
#include <fmt/format.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
CVar**   cvars;
uint16_t n_cvars;
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Adds a CVar to the CVar list
// -----------------------------------------------------------------------------
void addCVarList(CVar* cvar)
{
	cvars          = (CVar**)realloc(cvars, (n_cvars + 1) * sizeof(CVar*));
	cvars[n_cvars] = cvar;
	n_cvars++;
}
} // namespace


// -----------------------------------------------------------------------------
//
// CVar (and subclasses) Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CIntCVar class constructor
// -----------------------------------------------------------------------------
CIntCVar::CIntCVar(const string& NAME, int defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Integer;
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// CBoolCVar class constructor
// -----------------------------------------------------------------------------
CBoolCVar::CBoolCVar(const string& NAME, bool defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Boolean;
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// CFloatCVar class constructor
// -----------------------------------------------------------------------------
CFloatCVar::CFloatCVar(const string& NAME, double defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Float;
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// CStringCVar class constructor
// -----------------------------------------------------------------------------
CStringCVar::CStringCVar(const string& NAME, const string& defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::String;
	addCVarList(this);
}


// -----------------------------------------------------------------------------
//
// CVar Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Finds a CVar by name
// -----------------------------------------------------------------------------
CVar* CVar::get(const string& name)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (cvars[i]->name == name)
			return cvars[i];
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds all cvar names to a vector of strings
// -----------------------------------------------------------------------------
void CVar::putList(vector<string>& list)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (!(cvars[i]->flags & Flag::Secret))
			list.push_back(cvars[i]->name);
	}
}

// -----------------------------------------------------------------------------
// Saves cvars to a config file
// -----------------------------------------------------------------------------
string CVar::writeAll()
{
	uint32_t max_size = 0;
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (cvars[i]->name.size() > max_size)
			max_size = cvars[i]->name.size();
	}

	fmt::memory_buffer buf;
	format_to(buf, "cvars\n{{\n");

	for (unsigned i = 0; i < n_cvars; ++i)
	{
		auto cvar = cvars[i];
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
				auto value = strutil::escapedString(dynamic_cast<CStringCVar*>(cvar)->value, true);
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
void CVar::set(const string& name, const string& value)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		auto cvar = cvars[i];
		if (name == cvar->name)
		{
			if (cvar->type == Type::Integer)
				*dynamic_cast<CIntCVar*>(cvar) = strutil::asInt(value);

			if (cvar->type == Type::Boolean)
				*dynamic_cast<CBoolCVar*>(cvar) = strutil::asBoolean(value);

			if (cvar->type == Type::Float)
				*dynamic_cast<CFloatCVar*>(cvar) = strutil::asFloat(value);

			if (cvar->type == Type::String)
				*dynamic_cast<CStringCVar*>(cvar) = value;
		}
	}
}
