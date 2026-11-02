
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ExtMessageDialog.cpp
// Description: A simple message dialog that displays a short message and a
//              scrollable extended text area, used to present potentially
//              lengthy text (error logs, etc)
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
#include "ExtMessageDialog.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ExtMessageDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ExtMessageDialog class constructor
// -----------------------------------------------------------------------------
ExtMessageDialog::ExtMessageDialog(wxWindow* parent, string_view caption) :
	wxDialog(
		parent,
		-1,
		wxutil::strFromView(caption),
		wxDefaultPosition,
		wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	auto lh = ui::LayoutHelper(this);

	// Create and set sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add message label
	label_message_ = new wxStaticText(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	sizer->Add(label_message_, lh.sfWithBorder().Expand());

	// Add extended text box
	text_ext_ = new wxTextCtrl(
		this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_ext_->SetFont(wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	sizer->Add(text_ext_, lh.sfWithBorder(1, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Add OK button
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());
	hbox->AddStretchSpacer(1);
	auto btn_ok = new wxButton(this, wxID_OK, wxS("OK"));
	btn_ok->SetDefault();
	hbox->Add(btn_ok);

	SetInitialSize(lh.size(500, 500));

	// Bind events
	Bind(wxEVT_SIZE, &ExtMessageDialog::onSize, this);
}

// -----------------------------------------------------------------------------
// Sets the dialog short message
// -----------------------------------------------------------------------------
void ExtMessageDialog::setMessage(const string& message) const
{
	label_message_->SetLabel(wxString::FromUTF8(message));
}

// -----------------------------------------------------------------------------
// Sets the dialog extended text
// -----------------------------------------------------------------------------
void ExtMessageDialog::setExt(const string& text) const
{
	text_ext_->SetValue(wxString::FromUTF8(text));
}


// -----------------------------------------------------------------------------
//
// ExtMessageDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void ExtMessageDialog::onSize(wxSizeEvent& e)
{
	Layout();
	label_message_->Wrap(label_message_->GetSize().GetWidth());
	Layout();
}
