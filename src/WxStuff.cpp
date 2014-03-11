
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    WxStuff.cpp
 * Description: Some miscellaneous wxWidgets-related functions for
 *              general/global use in SLADE
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
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "WxStuff.h"
#include "Icons.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(String, font_monospace, "Consolas,Lucida Console", CVAR_SAVE)


/* createMenuItem
 * Creates a wxMenuItem from the given parameters, including giving
 * it an icon from slade.pk3 if specified
 *******************************************************************/
wxMenuItem* createMenuItem(wxMenu* menu, int id, string label, string help, string icon)
{
	wxMenuItem* item = new wxMenuItem(menu, id, label, help);

	if (!icon.IsEmpty())
		item->SetBitmap(getIcon(icon));

	return item;
}

/* getMonospaceFont
 * Returns a copy of the font [base] with a monospace face/family
 *******************************************************************/
wxFont getMonospaceFont(wxFont base)
{
	wxFont font(base.GetPointSize(), wxFONTFAMILY_MODERN, base.GetStyle(), base.GetWeight());

	wxArrayString split = wxSplit(font_monospace, ',');
	for (unsigned a = 0; a < split.size(); a++)
	{
		if (base.SetFaceName(split[a]))
		{
			font.SetFaceName(split[a]);
			break;
		}
	}

	return font;
}
