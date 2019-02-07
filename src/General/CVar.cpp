
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


// -----------------------------------------------------------------------------
// Adds a CVar to the CVar list
// -----------------------------------------------------------------------------
void addCVarList(CVar* cvar)
{
	cvars          = (CVar**)realloc(cvars, (n_cvars + 1) * sizeof(CVar*));
	cvars[n_cvars] = cvar;
	n_cvars++;
}

// -----------------------------------------------------------------------------
// Finds a CVar by name
// -----------------------------------------------------------------------------
CVar* CVar::get(const wxString& name)
{
	for (uint16_t c = 0; c < n_cvars; c++)
	{
		if (cvars[c]->name == name)
			return cvars[c];
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds all cvar names to a vector of strings
// -----------------------------------------------------------------------------
void CVar::putList(vector<wxString>& list)
{
	for (uint16_t c = 0; c < n_cvars; c++)
	{
		if (!(cvars[c]->flags & Flag::Secret))
			list.push_back(cvars[c]->name);
	}
}

// -----------------------------------------------------------------------------
// Saves cvars to a config file
// -----------------------------------------------------------------------------
void CVar::saveToFile(wxFile& file)
{
	uint32_t max_size = 0;
	for (uint32_t c = 0; c < n_cvars; c++)
	{
		if (cvars[c]->name.size() > max_size)
			max_size = cvars[c]->name.size();
	}

	file.Write("cvars\n{\n");

	for (uint32_t c = 0; c < n_cvars; c++)
	{
		if (cvars[c]->flags & Flag::Save)
		{
			file.Write(wxString::Format("\t%s ", cvars[c]->name));

			int spaces = max_size - cvars[c]->name.size();
			for (int a = 0; a < spaces; a++)
				file.Write(" ");

			if (cvars[c]->type == Type::Integer)
				file.Write(wxString::Format("%d\n", cvars[c]->getValue().Int));

			if (cvars[c]->type == Type::Boolean)
				file.Write(wxString::Format("%d\n", cvars[c]->getValue().Bool));

			if (cvars[c]->type == Type::Float)
				file.Write(wxString::Format("%1.5f\n", cvars[c]->getValue().Float));

			if (cvars[c]->type == Type::String)
			{
				wxString value = ((CStringCVar*)cvars[c])->value;
				value.Replace("\\", "/");
				value.Replace("\"", "\\\"");
				file.Write(wxString::Format("\"%s\"\n", value), wxConvUTF8);
			}
		}
	}

	file.Write("}\n\n");
}

// -----------------------------------------------------------------------------
// Reads [value] into the CVar with matching [name],
// or does nothing if no CVar [name] exists
// -----------------------------------------------------------------------------
void CVar::set(const wxString& name, const wxString& value)
{
	for (uint16_t c = 0; c < n_cvars; c++)
	{
		if (name == cvars[c]->name)
		{
			if (cvars[c]->type == Type::Integer)
				*((CIntCVar*)cvars[c]) = wxStringUtils::toInt(value);

			if (cvars[c]->type == Type::Boolean)
				*((CBoolCVar*)cvars[c]) = !!(wxStringUtils::toInt(value));

			if (cvars[c]->type == Type::Float)
				*((CFloatCVar*)cvars[c]) = wxStringUtils::toFloat(value);

			if (cvars[c]->type == Type::String)
				*((CStringCVar*)cvars[c]) = wxString::FromUTF8(CHR(value));
		}
	}
}


// -----------------------------------------------------------------------------
//
// CVar (and subclasses) Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CIntCVar class constructor
// -----------------------------------------------------------------------------
CIntCVar::CIntCVar(const wxString& NAME, int defval, uint16_t FLAGS)
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
CBoolCVar::CBoolCVar(const wxString& NAME, bool defval, uint16_t FLAGS)
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
CFloatCVar::CFloatCVar(const wxString& NAME, double defval, uint16_t FLAGS)
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
CStringCVar::CStringCVar(const wxString& NAME, const wxString& defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::String;
	addCVarList(this);
}
