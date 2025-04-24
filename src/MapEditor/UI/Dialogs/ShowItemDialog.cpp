
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    ShowItemDialog.cpp
// Description: A dialog allowing the user to select a map object type
//              (line / thing / etc) and enter an index.Used for the
//              'Show Item...' menu item in the map editor
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
#include "ShowItemDialog.h"
#include "SLADEMap/Types.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
vector obj_types{ map::ObjectType::Vertex,
				  map::ObjectType::Line,
				  map::ObjectType::Side,
				  map::ObjectType::Sector,
				  map::ObjectType::Thing };
}


// -----------------------------------------------------------------------------
//
// ShowItemDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ShowItemDialog class constructor
// -----------------------------------------------------------------------------
ShowItemDialog::ShowItemDialog(wxWindow* parent) : wxDialog(parent, -1, wxS("Show Item"))
{
	auto lh = ui::LayoutHelper(this);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	auto gb_sizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->Add(gb_sizer, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Object type
	vector<string> types = { "Vertex", "Line", "Side", "Sector", "Thing" };
	gb_sizer->Add(new wxStaticText(this, -1, wxS("Type:")), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	choice_type_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, wxutil::arrayStringStd(types));
	gb_sizer->Add(choice_type_, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Index
	gb_sizer->Add(
		new wxStaticText(this, -1, wxS("Index:")), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	text_index_ = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	gb_sizer->Add(text_index_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

	// Dialog buttons
	sizer->AddSpacer(lh.pad());
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Init layout
	gb_sizer->AddGrowableCol(1, 1);
	SetInitialSize(lh.size(300, -1));
	CenterOnParent();
	wxTopLevelWindowBase::Layout();
	text_index_->SetFocus();
	text_index_->SetFocusFromKbd();
}

// -----------------------------------------------------------------------------
// Returns the selected object type
// -----------------------------------------------------------------------------
map::ObjectType ShowItemDialog::type() const
{
	return obj_types[choice_type_->GetSelection()];
}

// -----------------------------------------------------------------------------
// Returns the entered index, or -1 if invalid
// -----------------------------------------------------------------------------
int ShowItemDialog::index() const
{
	long value;
	if (text_index_->GetValue().ToLong(&value))
		return value;
	else
		return -1;
}

// -----------------------------------------------------------------------------
// Sets the object type dropdown to [type]
// -----------------------------------------------------------------------------
void ShowItemDialog::setType(map::ObjectType type) const
{
	for (unsigned a = 0; a < obj_types.size(); ++a)
		if (obj_types[a] == type)
			choice_type_->Select(a);
}
