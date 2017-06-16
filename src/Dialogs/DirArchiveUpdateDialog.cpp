
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "DirArchiveUpdateDialog.h"


// ----------------------------------------------------------------------------
//
// DirArchiveUpdateDialog Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// DirArchiveUpdateDialog::DirArchiveUpdateDialog
//
// DirArchiveUpdateDialog class constructor
// ----------------------------------------------------------------------------
DirArchiveUpdateDialog::DirArchiveUpdateDialog(
	wxWindow* parent,
	DirArchive* archive,
	vector<DirEntryChange>& changes
) :	SDialog(parent, "Directory Content Changed", "dir_archive_update"),
	archive_{ archive },
	changes_{ changes }
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Message
	string message = S_FMT(
		"Contents of the directory \"%s\" have been modified outside of SLADE,\n",
		archive->filename()
	);
	message += "please tick the changes below that you wish to apply.";
	sizer->Add(new wxStaticText(this, -1, message), 0, wxEXPAND | wxALL, 10);
	message = "Note that any unticked changes will be overwritten on disk when the directory is saved.";
	sizer->Add(new wxStaticText(this, -1, message), 0, wxEXPAND | wxALL, 10);

	// Changes list
	list_changes_ = new wxDataViewListCtrl(this, -1);
	list_changes_->AppendToggleColumn("", wxDATAVIEW_CELL_ACTIVATABLE, wxDVC_DEFAULT_MINWIDTH, wxALIGN_CENTER);
	list_changes_->AppendTextColumn("Change");
	list_changes_->AppendTextColumn("Filename", wxDATAVIEW_CELL_INERT, -2);
	list_changes_->SetMinSize(wxSize(0, 200));
	sizer->Add(list_changes_, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

	// OK button
	auto btn_ok = new wxButton(this, wxID_OK, "Apply Selected Changes");
	sizer->AddSpacer(4);
	sizer->Add(btn_ok, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	btn_ok->Bind(wxEVT_BUTTON, &DirArchiveUpdateDialog::onBtnOKClicked, this);

	populateChangeList();

	Layout();
	Fit();
	SetInitialSize(GetSize());
}

// ----------------------------------------------------------------------------
// DirArchiveUpdateDialog::~DirArchiveUpdateDialog
//
// DirArchiveUpdateDialog class destructor
// ----------------------------------------------------------------------------
DirArchiveUpdateDialog::~DirArchiveUpdateDialog()
{
}

// ----------------------------------------------------------------------------
// DirArchiveUpdateDialog::populateChangeList
//
// Populates the changes list
// ----------------------------------------------------------------------------
void DirArchiveUpdateDialog::populateChangeList()
{
	wxVector<wxVariant> row;
	for (unsigned a = 0; a < changes_.size(); a++)
	{
		row.push_back(wxVariant(true));
		switch (changes_[a].action)
		{
		case DirEntryChange::ADDED_FILE: row.push_back(wxVariant("Added")); break;
		case DirEntryChange::ADDED_DIR: row.push_back(wxVariant("Added")); break;
		case DirEntryChange::DELETED_FILE: row.push_back(wxVariant("Deleted")); break;
		case DirEntryChange::DELETED_DIR: row.push_back(wxVariant("Deleted")); break;
		case DirEntryChange::UPDATED: row.push_back(wxVariant("Modified")); break;
		default: break;
		}
		row.push_back(wxVariant(changes_[a].file_path));
		list_changes_->AppendItem(row);
		row.clear();
	}
}


// ----------------------------------------------------------------------------
//
// DirArchiveUpdateDialog Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// DirArchiveUpdateDialog::onBtnOKClicked
//
// Called when the 'Apply Selected Changes' button is clicked
// ----------------------------------------------------------------------------
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
