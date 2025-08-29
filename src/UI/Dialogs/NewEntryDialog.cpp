
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    NewEntryDialog.cpp
// Description: A simple dialog with controls for creating a new entry,
//              including its name, type and directory for archives that support
//              them.
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
#include "NewEntryDialog.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveFormat.h"
#include "MainEditor/MainEditor.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
vector<string> type_names          = { "Empty (Marker)", "Text", "Palette", "Boom ANIMATED", "Boom SWITCHES" };
int            selected_entry_type = 0;
} // namespace slade::ui


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
// -----------------------------------------------------------------------------
// Adds [dir]'s path and all its subdir paths to [list] recursively
// -----------------------------------------------------------------------------
void allDirs(const ArchiveDir& dir, wxArrayString& list)
{
	list.Add(wxString::FromUTF8(dir.path()));
	for (const auto& subdir : dir.subdirs())
		allDirs(*subdir, list);
}
} // namespace slade::ui


// -----------------------------------------------------------------------------
//
// NewEntryDialog Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// NewEntryDialog Class Constructor
// -----------------------------------------------------------------------------
NewEntryDialog::NewEntryDialog(wxWindow* parent, const Archive& archive, const ArchiveDir* current_dir, bool new_dir) :
	wxDialog(parent, -1, new_dir ? wxS("New Directory") : wxS("New Entry"))
{
	auto lh = LayoutHelper(this);

	wxutil::setWindowIcon(this, new_dir ? "newfolder" : "newentry");

	const auto&   archive_format = archive.formatInfo();
	auto          types          = wxutil::arrayStringStd(type_names);
	wxArrayString all_dirs;
	allDirs(*archive.rootDir(), all_dirs);
	all_dirs.Sort();

	// Create controls
	text_entry_name_   = new wxTextCtrl(this, -1);
	choice_entry_type_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, types);
	combo_parent_dir_  = new wxComboBox(
        this,
        -1,
        current_dir ? wxString::FromUTF8(current_dir->path()) : wxString(wxS("/")),
        wxDefaultPosition,
        wxDefaultSize,
        all_dirs);


	// Setup controls
	combo_parent_dir_->Show(archive_format.supports_dirs);
	choice_entry_type_->Select(selected_entry_type);
	choice_entry_type_->Show(!new_dir);
	text_entry_name_->SetFocusFromKbd();
	if (!new_dir && archive_format.max_name_length > 0)
		text_entry_name_->SetMaxLength(archive_format.max_name_length);


	// --- Layout controls ---
	auto* m_sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_sizer);
	auto* sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	m_sizer->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	// New entry options
	int row = 0;
	sizer->Add(new wxStaticText(this, -1, wxS("Name:")), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(text_entry_name_, { row++, 1 }, { 1, 1 }, wxEXPAND);
	if (!new_dir)
	{
		sizer->Add(new wxStaticText(this, -1, wxS("Type:")), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		sizer->Add(choice_entry_type_, { row++, 1 }, { 1, 1 }, wxEXPAND);
	}
	if (archive_format.supports_dirs)
	{
		sizer->Add(
			new wxStaticText(this, -1, new_dir ? wxS("Parent Directory:") : wxS("Directory:")),
			{ row, 0 },
			{ 1, 1 },
			wxALIGN_CENTER_VERTICAL);
		sizer->Add(combo_parent_dir_, { row++, 1 }, { 1, 1 }, wxEXPAND);
	}
	sizer->AddGrowableCol(1, 1);

	// Dialog buttons
	m_sizer->Add(
		wxutil::createDialogButtonBox(this, "Create", "Cancel"),
		lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());


	// --- Bind events ---

	// Entry type changed
	choice_entry_type_->Bind(
		wxEVT_CHOICE,
		[this](wxCommandEvent& e)
		{
			using namespace maineditor;
			if (e.GetInt() == static_cast<int>(NewEntryType::Animated))
			{
				text_entry_name_->SetValue(wxS("ANIMATED"));
				text_entry_name_->Enable(false);
			}
			else if (e.GetInt() == static_cast<int>(NewEntryType::Switches))
			{
				text_entry_name_->SetValue(wxS("SWITCHES"));
				text_entry_name_->Enable(false);
			}
			else
				text_entry_name_->Enable(true);
		});


	// Init dialog size
	SetInitialSize(lh.size(400, -1));
	wxDialog::Layout();
	wxDialog::Fit();
	wxDialog::SetMinSize(GetBestSize());
	wxDialog::SetMaxSize({ -1, GetBestSize().y });
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Returns the entered entry name
// -----------------------------------------------------------------------------
string NewEntryDialog::entryName() const
{
	return text_entry_name_->GetValue().utf8_string();
}

// -----------------------------------------------------------------------------
// Returns the selected entry type
// -----------------------------------------------------------------------------
int NewEntryDialog::entryType() const
{
	return choice_entry_type_->GetSelection();
}

// -----------------------------------------------------------------------------
// Returns the entered parent directory path for the entry
// -----------------------------------------------------------------------------
string NewEntryDialog::parentDirPath() const
{
	return combo_parent_dir_->GetValue().utf8_string();
}

// -----------------------------------------------------------------------------
// Validate the entered values, returning true if valid
// -----------------------------------------------------------------------------
bool NewEntryDialog::Validate()
{
	// TODO:
	// - Check entry name is valid
	//   - Check dir for duplicate name if unsupported
	//   - Check for invalid characters

	// Remember type choice
	auto type = choice_entry_type_->GetSelection();
	if (type < 3)
		selected_entry_type = choice_entry_type_->GetSelection();

	return wxDialog::Validate();
}
