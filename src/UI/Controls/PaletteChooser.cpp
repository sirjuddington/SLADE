
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PaletteChooser.cpp
// Description: PaletteChooser class. A wxComboBox that contains a list of
//              palettes available (from the PaletteManager)
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
#include "PaletteChooser.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "General/Misc.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/Palette/PaletteManager.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PaletteChooser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PaletteChooser class constructor
// -----------------------------------------------------------------------------
PaletteChooser::PaletteChooser(wxWindow* parent, int id) : wxChoice(parent, id), pal_global_{ new Palette }
{
	// Init variables
	pal_global_->copyPalette(app::paletteManager()->globalPalette());

	// Add first 'existing' item
	Append(wxS("Existing/Global"));

	// Add palette names from palette manager
	for (int a = 0; a < app::paletteManager()->numPalettes(); a++)
		Append(wxString::FromUTF8(app::paletteManager()->palName(a)));

	// Add greyscale palette
	Append(wxS("Greyscale"));

	// Select first item
	wxChoice::SetSelection(0);

	// Bind events
	Bind(wxEVT_CHOICE, &PaletteChooser::onPaletteChanged, this);
}

// -----------------------------------------------------------------------------
// PaletteChooser class destructor
// -----------------------------------------------------------------------------
PaletteChooser::~PaletteChooser() = default;

// -----------------------------------------------------------------------------
// Called when the current image palette chooser is changed
// -----------------------------------------------------------------------------
void PaletteChooser::onPaletteChanged(wxCommandEvent& e)
{
	signals_.palette_changed();
}

// -----------------------------------------------------------------------------
// Sets the chooser's 'global' palette to the palette contained in [archive], or
// if it doesn't exist, the PaletteManager's global palette
// -----------------------------------------------------------------------------
void PaletteChooser::setGlobalFromArchive(Archive* archive, int lump)
{
	if (!archive)
		pal_global_->copyPalette(app::paletteManager()->globalPalette());

	else if (!misc::loadPaletteFromArchive(pal_global_.get(), archive, lump))
		setGlobalFromArchive(archive->parentArchive(), lump);
}

// -----------------------------------------------------------------------------
// Returns the selected palette (from the PaletteManager)
// -----------------------------------------------------------------------------
Palette* PaletteChooser::selectedPalette(const ArchiveEntry* entry) const
{
	if (GetSelection() > 0)
		return app::paletteManager()->palette(GetSelection() - 1);
	else if (entry)
		misc::loadPaletteFromArchive(pal_global_.get(), entry->parent(), misc::detectPaletteHack(entry));
	return pal_global_.get();
}

// -----------------------------------------------------------------------------
// Returns true if the 'Archive/Global Palette' entry is selected
// -----------------------------------------------------------------------------
bool PaletteChooser::globalSelected() const
{
	return (GetSelection() == 0);
}

// -----------------------------------------------------------------------------
// Selects the palette matching [name], or the default palette if no match was
// found
// -----------------------------------------------------------------------------
void PaletteChooser::selectPalette(const string& name)
{
	// Go through palettes list
	for (unsigned a = 0; a < GetCount(); a++)
	{
		if (strutil::equalCI(GetString(a).utf8_string(), name))
		{
			SetSelection(a);
			return;
		}
	}

	// No match found, set to default
	SetSelection(0);
}

// -----------------------------------------------------------------------------
// Appends another palette choice to the list.
// This is needed for the "Add Custom Palette" action, since the MainWindow's
// palette chooser is only initialized at startup and the new custom palette,
// without this function, requires exiting and restarting the app to appear in
// the list.
// -----------------------------------------------------------------------------
void PaletteChooser::addPalette(const string& name)
{
	// We want it to be just before the "Greyscale" choice
	if (GetCount() > 2)
		Insert(wxString::FromUTF8(name), GetCount() - 1);
}
