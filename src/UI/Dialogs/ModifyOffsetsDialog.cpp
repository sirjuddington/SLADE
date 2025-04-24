
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ModifyOffsetsDialog.cpp
// Description: A dialog UI containing options for modifying gfx entry offsets
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
#include "ModifyOffsetsDialog.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryType.h"
#include "Graphics/GameFormats.h"
#include "Graphics/Graphics.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ModifyOffsetsDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ModifyOffsetsDialog class constructor
// -----------------------------------------------------------------------------
ModifyOffsetsDialog::ModifyOffsetsDialog() :
	wxDialog(nullptr, -1, wxS("Modify Gfx Offset(s)"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	auto           lh       = ui::LayoutHelper(this);
	vector<string> offtypes = {
		"Monster",           "Monster (GL-friendly)", "Projectile",         "Hud/Weapon",
		"Hud/Weapon (Doom)", "Hud/Weapon (Heretic)",  "Hud/Weapon (Hexen)",
	};

	// Set dialog icon
	wxutil::setWindowIcon(this, "offset");

	// Create controls
	opt_auto_ = new wxRadioButton(this, -1, wxS("Automatic Offsets"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	combo_aligntype_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(offtypes));
	opt_set_         = new wxRadioButton(this, -1, wxS("Set Offsets"));
	entry_xoff_      = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, lh.size(40, -1));
	entry_yoff_      = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, lh.size(40, -1));
	cbox_relative_   = new wxCheckBox(this, wxID_ANY, wxS("Relative"));

	// Setup controls
	combo_aligntype_->Select(0);
	entry_xoff_->Enable(false);
	entry_yoff_->Enable(false);
	cbox_relative_->Enable(false);

	// Setup layout
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	auto* gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->Add(gbsizer, lh.sfWithLargeBorder(1).Expand());
	gbsizer->Add(opt_auto_, { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gbsizer->Add(combo_aligntype_, { 0, 1 }, { 1, 3 }, wxEXPAND);
	gbsizer->Add(opt_set_, { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gbsizer->Add(entry_xoff_, { 1, 1 }, { 1, 1 }, wxEXPAND);
	gbsizer->Add(entry_yoff_, { 1, 2 }, { 1, 1 }, wxEXPAND);
	gbsizer->Add(cbox_relative_, { 1, 3 }, { 1, 1 }, wxEXPAND);

	// Add default dialog buttons
	sizer->Add(
		wxutil::createDialogButtonBox(this, "OK", "Cancel"),
		lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());


	// Bind events
	opt_auto_->Bind(wxEVT_RADIOBUTTON, &ModifyOffsetsDialog::onOptAuto, this);
	opt_set_->Bind(wxEVT_RADIOBUTTON, &ModifyOffsetsDialog::onOptSet, this);


	// Apply layout and size
	wxTopLevelWindowBase::Layout();
	SetInitialSize(wxDefaultSize);
}

// -----------------------------------------------------------------------------
// Returns the offsets that have been entered
// -----------------------------------------------------------------------------
Vec2i ModifyOffsetsDialog::offset() const
{
	long x = 0;
	long y = 0;
	entry_xoff_->GetValue().ToLong(&x);
	entry_yoff_->GetValue().ToLong(&y);

	return { static_cast<int>(x), static_cast<int>(y) };
}

// -----------------------------------------------------------------------------
// Returns the selected alignment type
// -----------------------------------------------------------------------------
int ModifyOffsetsDialog::alignType() const
{
	if (opt_auto_->GetValue())
		return combo_aligntype_->GetSelection();
	else
		return -1;
}

// -----------------------------------------------------------------------------
// Returns true if 'automatic offsets' is selected
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::autoOffset() const
{
	if (opt_auto_->GetValue())
		return true;
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns true if the 'relative' offset checkbox is checked
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::relativeOffset() const
{
	return cbox_relative_->GetValue();
}

// -----------------------------------------------------------------------------
// Returns true if the user has entered an x-offset
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::xOffChange() const
{
	if (entry_xoff_->GetValue().IsEmpty())
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Returns true if the user has entered a y-offset
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::yOffChange() const
{
	if (entry_yoff_->GetValue().IsEmpty())
		return false;
	else
		return true;
}

// -----------------------------------------------------------------------------
// Calculates the modified offsets for a graphic with existing offsets
// [xoff,yoff] and size [width,height], based on the currently selected options
// in the dialog
// -----------------------------------------------------------------------------
Vec2i ModifyOffsetsDialog::calculateOffsets(int xoff, int yoff, int width, int height) const
{
	int  type = alignType();
	auto ofs  = offset();
	int  x    = xoff;
	int  y    = yoff;

	if (type >= 0)
	{
		// Monster
		if (type == 0)
		{
			x = width * 0.5;
			y = height - 4;
		}

		// Monster (GL-friendly)
		else if (type == 1)
		{
			x = width * 0.5;
			y = height;
		}

		// Projectile
		else if (type == 2)
		{
			x = width * 0.5;
			y = height * 0.5;
		}

		// Weapon (Fullscreen)
		else if (type == 3)
		{
			x = -160 + (width * 0.5);
			y = -200 + height;
		}

		// Weapon (Doom status bar)
		else if (type == 4)
		{
			x = -160 + (width * 0.5);
			y = -200 + 32 + height;
		}

		// Weapon (Heretic status bar)
		else if (type == 5)
		{
			x = -160 + (width * 0.5);
			y = -200 + 42 + height;
		}

		// Weapon (Hexen status bar)
		else if (type == 6)
		{
			x = -160 + (width * 0.5);
			y = -200 + 38 + height;
		}
	}
	else
	{
		// Relative offset
		if (relativeOffset())
		{
			if (xOffChange())
				x = xoff + ofs.x;
			if (yOffChange())
				y = yoff + ofs.y;
		}

		// Set offset
		else
		{
			if (xOffChange())
				x = ofs.x;
			if (yOffChange())
				y = ofs.y;
		}
	}

	return { x, y };
}

// -----------------------------------------------------------------------------
// Changes the offsets of the given gfx entry, based on settings selected in
// the dialog.
// Returns false if the entry is not an offset-supported format
// -----------------------------------------------------------------------------
bool ModifyOffsetsDialog::apply(ArchiveEntry& entry) const
{
	// Check entry type
	if (!gfx::supportsOffsets(entry))
	{
		log::error("Entry \"{}\" is of type \"{}\" which does not support offsets", entry.name(), entry.type()->name());
		return false;
	}

	const auto& entryformat = entry.type()->formatId();

	// Doom gfx format, normal and beta version.
	// Also arah format from alpha 0.2 because it uses the same header format.
	if (entryformat == "img_doom" || entryformat == "img_doom_beta" || entryformat == "image_doom_arah")
	{
		// Get patch header
		gfx::PatchHeader header;
		entry.seek(0, SEEK_SET);
		entry.read(&header, 8);

		// Calculate new offsets
		Vec2i offsets = calculateOffsets(header.left, header.top, header.width, header.height);

		// Apply new offsets
		header.left = wxINT16_SWAP_ON_BE(static_cast<int16_t>(offsets.x));
		header.top  = wxINT16_SWAP_ON_BE(static_cast<int16_t>(offsets.y));

		// Write new header to entry
		entry.seek(0, SEEK_SET);
		entry.write(&header, 8);
	}

	// Doom alpha gfx format
	else if (entryformat == "img_doom_alpha")
	{
		// Get patch header
		entry.seek(0, SEEK_SET);
		gfx::OldPatchHeader header;
		entry.read(&header, 4);

		// Calculate new offsets
		Vec2i offsets = calculateOffsets(header.left, header.top, header.width, header.height);

		// Apply new offsets
		header.left = static_cast<int8_t>(offsets.x);
		header.top  = static_cast<int8_t>(offsets.y);

		// Write new header to entry
		entry.seek(0, SEEK_SET);
		entry.write(&header, 4);
	}

	// PNG format
	else if (entryformat == "img_png")
	{
		// Get PNG size
		auto size = gfx::pngGetSize(entry.data());

		// Get PNG offsets (grAb chunk)
		auto ofs  = gfx::pngGetgrAb(entry.data());
		auto xoff = ofs ? ofs->x : 0;
		auto yoff = ofs ? ofs->y : 0;

		// Calculate new offsets
		Vec2i offsets = calculateOffsets(xoff, yoff, size.x, size.y);

		// Write offsets to PNG data
		MemChunk data(entry.data());
		if (gfx::pngSetgrAb(data, offsets.x, offsets.y))
			entry.importMemChunk(data);
	}
	else
		return false;

	return true;
}


// -----------------------------------------------------------------------------
//
// ModifyOffsetsDialog Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the 'Set Offsets' radio button is selected
// -----------------------------------------------------------------------------
void ModifyOffsetsDialog::onOptSet(wxCommandEvent& e)
{
	entry_xoff_->Enable(true);
	entry_yoff_->Enable(true);
	cbox_relative_->Enable(true);
	combo_aligntype_->Enable(false);
}

// -----------------------------------------------------------------------------
// Called when the 'Automatic Offsets' radio button is selected
// -----------------------------------------------------------------------------
void ModifyOffsetsDialog::onOptAuto(wxCommandEvent& e)
{
	entry_xoff_->Enable(false);
	entry_yoff_->Enable(false);
	cbox_relative_->Enable(false);
	combo_aligntype_->Enable(true);
}
