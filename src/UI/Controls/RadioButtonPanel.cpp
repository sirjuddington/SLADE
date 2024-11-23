
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    RadioButtonPanel.cpp
// Description: Panel containing a group of radio buttons and optional label
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
#include "RadioButtonPanel.h"
#include "UI/Layout.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// RadioButtonPanel Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// RadioButtonPanel class constructor
// -----------------------------------------------------------------------------
RadioButtonPanel::RadioButtonPanel(
	wxWindow*               parent,
	const vector<wxString>& choices,
	const wxString&         label,
	int                     selected,
	int                     orientation) :
	wxPanel(parent)
{
	auto lh    = LayoutHelper(this);
	auto sizer = new wxBoxSizer(orientation);

	// Check if we need to add a label
	if (!label.empty())
	{
		auto text       = new wxStaticText(this, wxID_ANY, label);
		auto main_sizer = new wxBoxSizer(wxVERTICAL);
		main_sizer->Add(text, lh.sfWithSmallBorder(0, wxBOTTOM));
		main_sizer->Add(sizer, lh.sfWithXLargeBorder(1, wxLEFT).Expand());
		SetSizer(main_sizer);
	}
	else
		SetSizer(sizer);

	// Create radio buttons
	for (size_t i = 0; i < choices.size(); i++)
	{
		if (i > 0)
			sizer->AddSpacer(lh.pad());

		auto rb = new wxRadioButton(this, wxID_ANY, choices[i]);
		sizer->Add(rb, wxSizerFlags());
		radio_buttons_.push_back(rb);

		if (i == selected)
			rb->SetValue(true);
	}
}

// -----------------------------------------------------------------------------
// Returns the index of the selected radio button
// -----------------------------------------------------------------------------
int RadioButtonPanel::getSelection() const
{
	for (size_t i = 0; i < radio_buttons_.size(); i++)
		if (radio_buttons_[i]->GetValue())
			return i;

	return -1;
}

// -----------------------------------------------------------------------------
// Sets the selected radio button by index
// -----------------------------------------------------------------------------
void RadioButtonPanel::setSelection(int index) const
{
	if (index >= 0 && index < static_cast<int>(radio_buttons_.size()))
		radio_buttons_[index]->SetValue(true);
}
