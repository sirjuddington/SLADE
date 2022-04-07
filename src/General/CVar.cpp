
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "thirdparty/fmt/include/fmt/format.h"

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
CIntCVar::CIntCVar(string_view name, int defval, uint16_t flags) : CVar{ Type::Integer, flags, name }, value{ defval }
{
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// CBoolCVar class constructor
// -----------------------------------------------------------------------------
CBoolCVar::CBoolCVar(string_view name, bool defval, uint16_t flags) :
	CVar{ Type::Boolean, flags, name }, value{ defval }
{
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// CFloatCVar class constructor
// -----------------------------------------------------------------------------
CFloatCVar::CFloatCVar(string_view name, double defval, uint16_t flags) :
	CVar{ Type::Float, flags, name }, value{ defval }
{
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// CStringCVar class constructor
// -----------------------------------------------------------------------------
CStringCVar::CStringCVar(string_view name, string_view defval, uint16_t flags) :
	CVar{ Type::String, flags, name }, value{ defval }
{
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
	vector<CVar*> all_cvars;
	for (unsigned i = 0; i < n_cvars; ++i)
		all_cvars.push_back(cvars[i]);

	std::sort(
		all_cvars.begin(),
		all_cvars.end(),
		[](const CVar* left, const CVar* right) { return left->name < right->name; });

	uint32_t max_size = 0;
	for (auto* cvar : all_cvars)
	{
		if (cvar->name.size() > max_size)
			max_size = cvar->name.size();
	}

	fmt::memory_buffer mem_buf;
	auto buf = fmt::appender(mem_buf);
	format_to(buf, "cvars\n{{\n");

	for (auto* cvar : all_cvars)
	{
		if (cvar->flags & Flag::Save)
		{
			format_to(buf, "\t{} ", cvar->name);

			int spaces = max_size - cvar->name.size();
			for (int a = 0; a < spaces; a++)
				mem_buf.push_back(' ');

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

	return to_string(mem_buf);
}

// -----------------------------------------------------------------------------
// Reads [value] into the CVar with matching [name],
// or does nothing if no CVar [name] exists
// -----------------------------------------------------------------------------
void CVar::set(const string& name, const string& value)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (auto* cvar = cvars[i]; name == cvar->name)
		{
			if (cvar->type == Type::Integer)
				*dynamic_cast<CIntCVar*>(cvar) = strutil::asInt(value);

			if (cvar->type == Type::Boolean)
				*dynamic_cast<CBoolCVar*>(cvar) = strutil::asBoolean(value);

			if (cvar->type == Type::Float)
				*dynamic_cast<CFloatCVar*>(cvar) = strutil::asDouble(value);

			if (cvar->type == Type::String)
				*dynamic_cast<CStringCVar*>(cvar) = value;
		}
	}
}
