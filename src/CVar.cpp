
/* *****************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2009 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    cvar.cpp
 * Description: Cvar system. 'Borrowed' from ZDoom, which is written
 *              by Randy Heit.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * *****************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVar**		cvars;
uint16_t	n_cvars;


/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

/* add_cvar_list
 * Adds a CVar to the CVar list
 *******************************************************************/
void add_cvar_list(CVar* cvar)
{
	cvars = (CVar**)realloc(cvars, (n_cvars + 1) * sizeof(CVar*));
	cvars[n_cvars] = cvar;
	n_cvars++;
}

/* get_cvar
 * Finds a CVar by name
 *******************************************************************/
CVar* get_cvar(string name)
{
	for (uint16_t c = 0; c < n_cvars; c++)
	{
		if (cvars[c]->name == name)
			return cvars[c];
	}

	return NULL;
}

/* dump_cvars
 * Dumps all CVar info to a string
 *******************************************************************/
void dump_cvars()
{
	for (uint16_t c = 0; c < n_cvars; c++)
	{
		if (!(cvars[c]->flags & CVAR_SECRET))
		{
			if (cvars[c]->type == CVAR_INTEGER)
				printf("%s %d\n", CHR(cvars[c]->name), cvars[c]->GetValue().Int);

			if (cvars[c]->type == CVAR_BOOLEAN)
				printf("%s %d\n", CHR(cvars[c]->name), cvars[c]->GetValue().Bool);

			if (cvars[c]->type == CVAR_FLOAT)
				printf("%s %1.5f\n", CHR(cvars[c]->name), cvars[c]->GetValue().Float);

			if (cvars[c]->type == CVAR_STRING)
				printf("%s \"%s\"\n", CHR(cvars[c]->name), CHR(((CStringCVar*)cvars[c])->value));
		}
	}
}

/* get_cvar_list
 * Adds all cvar names to a vector of strings
 *******************************************************************/
void get_cvar_list(vector<string>& list)
{
	for (uint16_t c = 0; c < n_cvars; c++)
	{
		if (!(cvars[c]->flags & CVAR_SECRET))
			list.push_back(cvars[c]->name);
	}
}

/* save_cvars
 * Saves cvars to a config file
 *******************************************************************/
void save_cvars(wxFile& file)
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
		if (cvars[c]->flags & CVAR_SAVE)
		{
			file.Write(S_FMT("\t%s ", cvars[c]->name));

			int spaces = max_size - cvars[c]->name.size();
			for (int a = 0; a < spaces; a++) file.Write(" ");

			if (cvars[c]->type == CVAR_INTEGER)
				file.Write(S_FMT("%d\n", cvars[c]->GetValue().Int));

			if (cvars[c]->type == CVAR_BOOLEAN)
				file.Write(S_FMT("%d\n", cvars[c]->GetValue().Bool));

			if (cvars[c]->type == CVAR_FLOAT)
				file.Write(S_FMT("%1.5f\n", cvars[c]->GetValue().Float));

			if (cvars[c]->type == CVAR_STRING)
				file.Write(S_FMT("\"%s\"\n", ((CStringCVar*)cvars[c])->value), wxConvUTF8);
		}
	}

	file.Write("}\n\n");
}

/* read_cvar
 * Reads [value] into the CVar with matching [name], or does nothing
 * if no CVar [name] exists
 *******************************************************************/
void read_cvar(string name, string value)
{
	for (uint16_t c = 0; c < n_cvars; c++)
	{
		if (name == cvars[c]->name)
		{
			if (cvars[c]->type == CVAR_INTEGER)
				*((CIntCVar*) cvars[c]) = atoi(CHR(value));

			if (cvars[c]->type == CVAR_BOOLEAN)
				*((CBoolCVar*) cvars[c]) = !!(atoi(CHR(value)));

			if (cvars[c]->type == CVAR_FLOAT)
				*((CFloatCVar*) cvars[c]) = atof(CHR(value));

			if (cvars[c]->type == CVAR_STRING)
				*((CStringCVar*) cvars[c]) = wxString::FromUTF8(value);
		}
	}
}


/*******************************************************************
 * C<TYPE>CVAR CLASS FUNCTIONS
 *******************************************************************/

/* CIntCVar::CIntCVar
 * CIntCVar class constructor
 *******************************************************************/
CIntCVar::CIntCVar(string NAME, int defval, uint16_t FLAGS)
{
	name = NAME;
	flags = FLAGS;
	value = defval;
	type = CVAR_INTEGER;
	add_cvar_list(this);
}

/* CBoolCVar::CBoolCVar
 * CBoolCVar class constructor
 *******************************************************************/
CBoolCVar::CBoolCVar(string NAME, bool defval, uint16_t FLAGS)
{
	name = NAME;
	flags = FLAGS;
	value = defval;
	type = CVAR_BOOLEAN;
	add_cvar_list(this);
}

/* CFloatCVar::CFloatCVar
 * CFloatCVar class constructor
 *******************************************************************/
CFloatCVar::CFloatCVar(string NAME, double defval, uint16_t FLAGS)
{
	name = NAME;
	flags = FLAGS;
	value = defval;
	type = CVAR_FLOAT;
	add_cvar_list(this);
}

/* CStringCVar::CStringCVar
 * CStringCVar class constructor
 *******************************************************************/
CStringCVar::CStringCVar(string NAME, string defval, uint16_t FLAGS)
{
	name = NAME;
	flags = FLAGS;
	value = defval;
	type = CVAR_STRING;
	add_cvar_list(this);
}
