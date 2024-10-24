
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    NewArchiveDialog.cpp
// Description: A simple dialog that lists the available archive formats to
//              create, and creates an archive of that type if the user chooses.
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
#include "App.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveManager.h"
#include "NewArchiveDiaog.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, archive_last_created_format, "wad", CVar::Save)


// -----------------------------------------------------------------------------
//
// NewArchiveDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// NewArchiveDialog class constructor
// -----------------------------------------------------------------------------
NewArchiveDialog::NewArchiveDialog(wxWindow* parent) : wxDialog(parent, -1, "Create New Archive")
{
	auto lh = LayoutHelper(this);

	// Set dialog icon
	wxutil::setWindowIcon(this, "newarchive");

	// Create controls
	auto* choice_type = new wxChoice(this, -1);
	auto* btn_create  = new wxButton(this, wxID_OK, "Create");
	auto* btn_cancel  = new wxButton(this, wxID_CANCEL, "Cancel");

	// Fill formats list
	long selected_index = 0;
	for (const auto& format : archive::allFormatsInfo())
		if (format.create)
		{
			if (format.id == archive_last_created_format.value)
				selected_index = choice_type->GetCount();

			choice_type->AppendString(format.name + " Archive");
		}

	// Setup controls
	choice_type->SetSelection(selected_index);
	btn_create->SetDefault();

	// Layout
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	sizer->Add(wxutil::createLabelHBox(this, "Type:", choice_type), lh.sfWithLargeBorder().Expand());
	auto* hbox = wxutil::createDialogButtonBox(btn_create, btn_cancel);
	sizer->Add(hbox, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Create button click
	btn_create->Bind(
		wxEVT_BUTTON,
		[this, choice_type](wxCommandEvent&)
		{
			for (const auto& format : archive::allFormatsInfo())
				if (choice_type->GetString(choice_type->GetSelection()) == (format.name + " Archive"))
				{
					archive_created_            = app::archiveManager().newArchive(format.id).get();
					archive_last_created_format = format.id;
					EndModal(wxID_OK);
				}
		});

	// Cancel button click
	btn_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });

	SetInitialSize(lh.size(250, -1));
	wxWindowBase::Layout();
	wxWindowBase::Fit();
	wxTopLevelWindowBase::SetMinSize(GetBestSize());
	CenterOnParent();
}


// -----------------------------------------------------------------------------
// Returns the archive that was created (or nullptr if cancelled)
// -----------------------------------------------------------------------------
Archive* NewArchiveDialog::createdArchive() const
{
	return archive_created_;
}
