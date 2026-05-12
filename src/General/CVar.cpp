
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
	cvars          = static_cast<CVar**>(realloc(cvars, (n_cvars + 1) * sizeof(CVar*)));
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
	CVar{ Type::Boolean, flags, name },
	value{ defval }
{
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// CFloatCVar class constructor
// -----------------------------------------------------------------------------
CFloatCVar::CFloatCVar(string_view name, double defval, uint16_t flags) :
	CVar{ Type::Float, flags, name },
	value{ defval }
{
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// CStringCVar class constructor
// -----------------------------------------------------------------------------
CStringCVar::CStringCVar(string_view name, string_view defval, uint16_t flags) :
	CVar{ Type::String, flags, name },
	value{ defval }
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
// Gets the value of the boolean CVar with matching [name]
// -----------------------------------------------------------------------------
bool CVar::getBool(const string& cvar_name)
{
	if (auto* cvar = get(cvar_name); cvar && cvar->type == Type::Boolean)
		return *dynamic_cast<CBoolCVar*>(cvar);

	return false;
}

// -----------------------------------------------------------------------------
// Gets the value of the integer CVar with matching [name]
// -----------------------------------------------------------------------------
int CVar::getInt(const string& cvar_name)
{
	if (auto* cvar = get(cvar_name); cvar && cvar->type == Type::Integer)
		return *dynamic_cast<CIntCVar*>(cvar);

	return 0;
}

// -----------------------------------------------------------------------------
// Gets the value of the float CVar with matching [name]
// -----------------------------------------------------------------------------
double CVar::getFloat(const string& cvar_name)
{
	if (auto* cvar = get(cvar_name); cvar && cvar->type == Type::Float)
		return *dynamic_cast<CFloatCVar*>(cvar);

	return 0.0;
}

// -----------------------------------------------------------------------------
// Gets the value of the string CVar with matching [name]
// -----------------------------------------------------------------------------
string CVar::getString(const string& cvar_name)
{
	if (auto* cvar = get(cvar_name); cvar && cvar->type == Type::String)
		return *dynamic_cast<CStringCVar*>(cvar);

	return "";
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
// Returns a vector of all CVars, optionally [sorted] by name
// -----------------------------------------------------------------------------
vector<CVar*> CVar::allCvars(bool sorted)
{
	vector<CVar*> cvarlist;

	for (unsigned i = 0; i < n_cvars; ++i)
		cvarlist.push_back(cvars[i]);

	if (sorted)
		std::ranges::sort(cvarlist, [](const CVar* a, const CVar* b) { return a->name < b->name; });

	return cvarlist;
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

// -----------------------------------------------------------------------------
// Reads [value] into the boolean CVar with matching [name],
// or does nothing if no boolean CVar [name] exists
// -----------------------------------------------------------------------------
void CVar::setBool(const string& cvar_name, bool value)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (auto* cvar = cvars[i]; cvar_name == cvar->name && cvar->type == Type::Boolean)
		{
			*dynamic_cast<CBoolCVar*>(cvar) = value;
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Reads [value] into the integer CVar with matching [name],
// or does nothing if no integer CVar [name] exists
// -----------------------------------------------------------------------------
void CVar::setInt(const string& cvar_name, int value)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (auto* cvar = cvars[i]; cvar_name == cvar->name && cvar->type == Type::Integer)
		{
			*dynamic_cast<CIntCVar*>(cvar) = value;
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Reads [value] into the float CVar with matching [name],
// or does nothing if no float CVar [name] exists
// -----------------------------------------------------------------------------
void CVar::setFloat(const string& cvar_name, double value)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (auto* cvar = cvars[i]; cvar_name == cvar->name && cvar->type == Type::Float)
		{
			*dynamic_cast<CFloatCVar*>(cvar) = value;
			return;
		}
	}
}
