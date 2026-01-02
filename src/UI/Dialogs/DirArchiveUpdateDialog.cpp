
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DirArchiveUpdateDialog.cpp
// Description: A dialog that shows a list of changes to files in a directory,
//              with checkboxes to apply them.Used when checking if an open
//              directory archive's entries have been modified on disk outside
//              of SLADE
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
#include "DirArchiveUpdateDialog.h"
#include "General/UI.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// DirArchiveUpdateDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DirArchiveUpdateDialog class constructor
// -----------------------------------------------------------------------------
DirArchiveUpdateDialog::DirArchiveUpdateDialog(wxWindow* parent, DirArchive* archive, vector<DirEntryChange>& changes) :
	SDialog(parent, "Directory Content Changed", "dir_archive_update"),
	archive_{ archive },
	changes_{ changes }
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Message
	wxString message = WX_FMT(
		"Contents of the directory \"{}\" have been modified outside of SLADE,\n", archive->filename());
	message += wxS("please tick the changes below that you wish to apply.");
	sizer->Add(new wxStaticText(this, -1, message), 0, wxEXPAND | wxALL, ui::padLarge());
	message = wxS("Note that any unticked changes will be overwritten on disk when the directory is saved.");
	sizer->Add(new wxStaticText(this, -1, message), 0, wxEXPAND | wxALL, ui::padLarge());

	// Changes list
	list_changes_ = new wxDataViewListCtrl(this, -1);
	list_changes_->AppendToggleColumn(
		wxEmptyString, wxDATAVIEW_CELL_ACTIVATABLE, wxDVC_DEFAULT_MINWIDTH, wxALIGN_CENTER);
	list_changes_->AppendTextColumn(wxS("Change"));
	list_changes_->AppendTextColumn(wxS("Filename"), wxDATAVIEW_CELL_INERT, -2);
	list_changes_->SetMinSize(wxSize(0, ui::scalePx(200)));
	sizer->Add(list_changes_, 1, wxEXPAND | wxLEFT | wxRIGHT, ui::padLarge());

	// OK button
	auto btn_ok = new wxButton(this, wxID_OK, wxS("Apply Selected Changes"));
	btn_ok->SetDefault();
	sizer->AddSpacer(ui::pad());
	sizer->Add(btn_ok, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, ui::padLarge());
	btn_ok->Bind(wxEVT_BUTTON, &DirArchiveUpdateDialog::onBtnOKClicked, this);

	populateChangeList();

	wxWindowBase::Layout();
	wxWindowBase::Fit();
	const wxSize size = GetSize() * GetContentScaleFactor();
	SetInitialSize(size);
}

// -----------------------------------------------------------------------------
// Populates the changes list
// -----------------------------------------------------------------------------
void DirArchiveUpdateDialog::populateChangeList()
{
	wxVector<wxVariant> row;
	for (unsigned a = 0; a < changes_.size(); a++)
	{
		row.push_back(wxVariant(true));
		switch (changes_[a].action)
		{
		case DirEntryChange::Action::AddedFile: row.push_back(wxVariant("Added")); break;
		case DirEntryChange::Action::AddedDir: row.push_back(wxVariant("Added")); break;
		case DirEntryChange::Action::DeletedFile: row.push_back(wxVariant("Deleted")); break;
		case DirEntryChange::Action::DeletedDir: row.push_back(wxVariant("Deleted")); break;
		case DirEntryChange::Action::Updated: row.push_back(wxVariant("Modified")); break;
		default: break;
		}
		row.push_back(wxVariant(changes_[a].file_path));
		list_changes_->AppendItem(row);
		row.clear();
	}
}


// -----------------------------------------------------------------------------
//
// DirArchiveUpdateDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the 'Apply Selected Changes' button is clicked
// -----------------------------------------------------------------------------
void DirArchiveUpdateDialog::onBtnOKClicked(wxCommandEvent& e)
{
	// Get selected changes to apply
	vector<DirEntryChange> apply_changes;
	vector<DirEntryChange> ignore_changes;
	for (unsigned a = 0; a < changes_.size(); a++)
		if (list_changes_->GetToggleValue(a, 0))
			apply_changes.push_back(changes_[a]);
		else
			ignore_changes.push_back(changes_[a]);

	archive_->ignoreChangedEntries(ignore_changes);
	archive_->updateChangedEntries(apply_changes);

	EndModal(wxID_OK);
}
