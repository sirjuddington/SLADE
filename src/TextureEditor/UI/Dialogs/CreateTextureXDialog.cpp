
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CreateTextureXDialog.cpp
// Description: A simple dialog that provides options to create new (empty)
//              texture definitions or import them from the base resource
//              archive. Also has options for the desired texture format (doom,
//              strife or TEXTURES)
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
#include "CreateTextureXDialog.h"
#include "UI/Layout.h"

using namespace slade;
using namespace texeditor;


// -----------------------------------------------------------------------------
//
// CreateTextureXDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CreateTextureXDialog class constructor
// -----------------------------------------------------------------------------
CreateTextureXDialog::CreateTextureXDialog(wxWindow* parent) : wxDialog(parent, -1, wxS("Create Texture Definitions"))
{
	auto lh = ui::LayoutHelper(this);

	// Setup layout
	auto m_vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_vbox);


	// --- Source options ---
	auto frame      = new wxStaticBox(this, -1, wxS("Source"));
	auto framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	m_vbox->Add(framesizer, lh.sfWithLargeBorder(wxLEFT | wxRIGHT).Expand());

	// New list
	rb_new_ = new wxRadioButton(frame, -1, wxS("Create New (Empty)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	framesizer->Add(rb_new_, lh.sfWithBorder().Expand());

	// Import from Base Resource Archive
	rb_import_bra_ = new wxRadioButton(frame, -1, wxS("Import from Base Resource Archive:"));
	framesizer->Add(rb_import_bra_, lh.sfWithBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());


	// --- Format options ---
	frame      = new wxStaticBox(this, -1, wxS("Format"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	m_vbox->Add(framesizer, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT).Expand());

	// Doom format
	rb_format_doom_ = new wxRadioButton(
		frame, -1, wxS("Doom (TEXTURE1 + PNAMES)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	rb_format_strife_   = new wxRadioButton(frame, -1, wxS("Strife (TEXTURE1 + PNAMES)"));
	rb_format_textures_ = new wxRadioButton(frame, -1, wxS("ZDoom (TEXTURES)"));
	lh.layoutVertically(
		framesizer, { rb_format_doom_, rb_format_strife_, rb_format_textures_ }, lh.sfWithBorder(1).Expand());

	// Add buttons
	m_vbox->AddSpacer(lh.pad());
	m_vbox->Add(CreateButtonSizer(wxOK | wxCANCEL), lh.sfWithLargeBorder().Expand());

	// Bind events
	rb_new_->Bind(wxEVT_RADIOBUTTON, &CreateTextureXDialog::onRadioNewSelected, this);
	rb_import_bra_->Bind(wxEVT_RADIOBUTTON, &CreateTextureXDialog::onRadioNewSelected, this);

	SetInitialSize(wxSize(-1, -1));
	wxTopLevelWindowBase::Layout();
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// CreateTextureXDialog class destructor
// -----------------------------------------------------------------------------
CreateTextureXDialog::~CreateTextureXDialog() = default;

// -----------------------------------------------------------------------------
// Returns the selected texture format
// -----------------------------------------------------------------------------
TextureXFormat CreateTextureXDialog::getSelectedFormat() const
{
	if (rb_format_doom_->GetValue())
		return TextureXFormat::Normal;
	else if (rb_format_strife_->GetValue())
		return TextureXFormat::Strife11;
	else if (rb_format_textures_->GetValue())
		return TextureXFormat::Textures;
	else
		return TextureXFormat::Normal;
}

// -----------------------------------------------------------------------------
// Called when the "New" radio button selection is changed
// -----------------------------------------------------------------------------
void CreateTextureXDialog::onRadioNewSelected(wxCommandEvent& e)
{
	// Enable/Disable format selection depending on rb_new state
	if (rb_new_->GetValue())
	{
		rb_format_doom_->Enable(true);
		rb_format_strife_->Enable(true);
		rb_format_textures_->Enable(true);
	}
	else
	{
		rb_format_doom_->Enable(false);
		rb_format_strife_->Enable(false);
		rb_format_textures_->Enable(false);
	}
}
