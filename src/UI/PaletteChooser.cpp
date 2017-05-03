
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    PaletteChooser.cpp
 * Description: PaletteChooser class. A wxComboBox that contains a
 *              list of palettes available (from the PaletteManager)
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
#include "App.h"
#include "PaletteChooser.h"
#include "Archive/Archive.h"
#include "General/Misc.h"
#include "Graphics/Palette/PaletteManager.h"


/*******************************************************************
 * PALETTECHOOSER CLASS FUNCTIONS
 *******************************************************************/

/* PaletteChooser::PaletteChooser
 * PaletteChooser class constructor
 *******************************************************************/
PaletteChooser::PaletteChooser(wxWindow* parent, int id)
	: wxChoice(parent, id)
{
	// Init variables
	pal_global = new Palette8bit();
	pal_global->copyPalette(App::paletteManager()->globalPalette());

	// Add first 'existing' item
	Append("Existing/Global");

	// Add palette names from palette manager
	for (int a = 0; a < App::paletteManager()->numPalettes(); a++)
		Append(App::paletteManager()->getPalName(a));

	// Add greyscale palette
	Append("Greyscale");

	// Select first item
	SetSelection(0);

	// Bind events
	Bind(wxEVT_CHOICE, &PaletteChooser::onPaletteChanged, this);
}

/* PaletteChooser::~PaletteChooser
 * PaletteChooser class destructor
 *******************************************************************/
PaletteChooser::~PaletteChooser()
{
	delete pal_global;
}

/* PaletteChooser::onPaletteChanged
 * Called when the current image palette chooser is changed
 *******************************************************************/
void PaletteChooser::onPaletteChanged(wxCommandEvent& e)
{
	announce("main_palette_changed");
}

/* PaletteChooser::setGlobalFromArchive
 * Sets the chooser's 'global' palette to the palette contained in
 * [archive], or if it doesn't exist, the PaletteManager's global
 * palette
 *******************************************************************/
void PaletteChooser::setGlobalFromArchive(Archive* archive, int lump)
{
	if (!archive)
		pal_global->copyPalette(App::paletteManager()->globalPalette());

	else if (!Misc::loadPaletteFromArchive(pal_global, archive, lump))
		setGlobalFromArchive(archive->getParentArchive(), lump);
}

/* PaletteChooser::getSelectedPalette
 * Returns the selected palette (from the PaletteManager)
 *******************************************************************/
Palette8bit* PaletteChooser::getSelectedPalette(ArchiveEntry* entry)
{
	if (GetSelection() > 0)
		return App::paletteManager()->getPalette(GetSelection() - 1);
	else if (entry)
		Misc::loadPaletteFromArchive(pal_global, entry->getParent(), Misc::detectPaletteHack(entry));
	return pal_global;
}

/* PaletteChooser::globalSelected
 * Returns true if the 'Archive/Global Palette' entry is selected
 *******************************************************************/
bool PaletteChooser::globalSelected()
{
	return (GetSelection() == 0);
}

/* PaletteChooser::selectPalette
 * Selects the palette matching [name], or the default palette if
 * no match was found
 *******************************************************************/
void PaletteChooser::selectPalette(string name)
{
	// Go through palettes list
	for (unsigned a = 0; a < GetCount(); a++)
	{
		if (S_CMPNOCASE(GetString(a), name))
		{
			SetSelection(a);
			return;
		}
	}

	// No match found, set to default
	SetSelection(0);
}

/* PaletteChooser::addPalette
 * Appends another palette choice to the list. This is needed
 * for the "Add Custom Palette" action, since the MainWindow's
 * palette chooser is only initialized at startup and the new
 * custom palette, without this function, requires exiting and
 * restarting the app to appear in the list.
 *******************************************************************/
void PaletteChooser::addPalette(string name)
{
	// We want it to be just before the "Greyscale" choice
	if (GetCount() > 2)
		Insert(name, GetCount() - 1);
}
