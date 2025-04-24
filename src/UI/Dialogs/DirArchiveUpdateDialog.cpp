
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Archive/Archive.h"
#include "Archive/Formats/DirArchiveHandler.h"
#include "UI/Layout.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// DirArchiveUpdateDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DirArchiveUpdateDialog class constructor
// -----------------------------------------------------------------------------
DirArchiveUpdateDialog::DirArchiveUpdateDialog(
	wxWindow*                     parent,
	Archive*                      archive,
	const vector<DirEntryChange>& changes) :
	SDialog(parent, "Directory Content Changed", "dir_archive_update"),
	archive_{ archive },
	changes_{ changes }
{
	auto lh = ui::LayoutHelper(this);

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Message
	wxString message = WX_FMT(
		"Contents of the directory \"{}\" have been modified outside of SLADE,\n", archive->filename());
	message += wxS("please tick the changes below that you wish to apply.");
	sizer->Add(new wxStaticText(this, -1, message), lh.sfWithLargeBorder().Expand());
	message = wxS("Note that any unticked changes will be overwritten on disk when the directory is saved.");
	sizer->Add(new wxStaticText(this, -1, message), lh.sfWithLargeBorder().Expand());

	// Changes list
	list_changes_ = new wxDataViewListCtrl(this, -1);
	list_changes_->AppendToggleColumn(
		wxEmptyString, wxDATAVIEW_CELL_ACTIVATABLE, wxDVC_DEFAULT_MINWIDTH, wxALIGN_CENTER);
	list_changes_->AppendTextColumn(wxS("Change"));
	list_changes_->AppendTextColumn(wxS("Filename"), wxDATAVIEW_CELL_INERT, -2);
	list_changes_->SetMinSize(lh.size(0, 200));
	sizer->Add(list_changes_, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT).Expand());

	// OK button
	auto btn_ok = new wxButton(this, wxID_OK, wxS("Apply Selected Changes"));
	btn_ok->SetDefault();
	sizer->AddSpacer(lh.pad());
	sizer->Add(btn_ok, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Right());
	btn_ok->Bind(wxEVT_BUTTON, &DirArchiveUpdateDialog::onBtnOKClicked, this);

	populateChangeList();

	wxTopLevelWindowBase::Layout();
	wxWindowBase::Fit();
	const wxSize size = GetSize() * wxWindowBase::GetContentScaleFactor();
	SetInitialSize(size);
}

// -----------------------------------------------------------------------------
// Populates the changes list
// -----------------------------------------------------------------------------
void DirArchiveUpdateDialog::populateChangeList() const
{
	wxVector<wxVariant> row;
	for (const auto& change : changes_)
	{
		row.push_back(wxVariant(true));
		switch (change.action)
		{
		case DirEntryChange::Action::AddedFile:   row.push_back(wxVariant("Added")); break;
		case DirEntryChange::Action::AddedDir:    row.push_back(wxVariant("Added")); break;
		case DirEntryChange::Action::DeletedFile: row.push_back(wxVariant("Deleted")); break;
		case DirEntryChange::Action::DeletedDir:  row.push_back(wxVariant("Deleted")); break;
		case DirEntryChange::Action::Updated:     row.push_back(wxVariant("Modified")); break;
		default:                                  break;
		}
		row.push_back(wxVariant(change.file_path));
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

	auto format_handler = dynamic_cast<DirArchiveHandler&>(archive_->formatHandler());

	format_handler.ignoreChangedEntries(ignore_changes);
	format_handler.updateChangedEntries(*archive_, apply_changes);

	EndModal(wxID_OK);
}
