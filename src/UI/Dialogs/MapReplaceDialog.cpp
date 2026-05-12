
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapReplaceDialog.cpp
// Description: Dialog for 'Replace in Maps' functionality, allows to replace
//              all instances of a certain line special / thing type / etc in
//              all maps in an archive
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
#include "MapReplaceDialog.h"
#include "MainEditor/ArchiveOperations.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// ThingTypeReplacePanel Class
//
// -----------------------------------------------------------------------------
namespace slade
{
class ThingTypeReplacePanel : public wxPanel
{
public:
	ThingTypeReplacePanel(wxWindow* parent);
	~ThingTypeReplacePanel() override = default;

	void doReplace(Archive* archive) const;

private:
	wxSpinCtrl* spin_from_ = nullptr;
	wxSpinCtrl* spin_to_   = nullptr;
};
} // namespace slade

// -----------------------------------------------------------------------------
// ThingTypeReplacePanel class constructor
// -----------------------------------------------------------------------------
ThingTypeReplacePanel::ThingTypeReplacePanel(wxWindow* parent) : wxPanel(parent, -1)
{
	auto lh = ui::LayoutHelper(this);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->AddStretchSpacer();
	sizer->Add(gbsizer, lh.sfWithLargeBorder().Center());
	sizer->AddStretchSpacer();

	// From type
	gbsizer->Add(
		new wxStaticText(this, -1, wxS("Replace Type:")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
	spin_from_ = new wxSpinCtrl(
		this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 0, 999999);
	gbsizer->Add(spin_from_, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// To type
	gbsizer->Add(
		new wxStaticText(this, -1, wxS("With Type:")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
	spin_to_ = new wxSpinCtrl(
		this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 0, 999999);
	gbsizer->Add(spin_to_, { 1, 1 }, { 1, 1 }, wxEXPAND);
}

// -----------------------------------------------------------------------------
// Performs replace using settings from the panel controls for [archive]
// -----------------------------------------------------------------------------
void ThingTypeReplacePanel::doReplace(Archive* archive) const
{
	size_t count = archiveoperations::replaceThings(archive, spin_from_->GetValue(), spin_to_->GetValue());
	wxMessageBox(
		WX_FMT("Replaced {} occurrences. See console log for more detailed information.", count),
		wxS("Replace Things"));
}


// -----------------------------------------------------------------------------
//
// SpecialReplacePanel Class
//
// -----------------------------------------------------------------------------
namespace slade
{
class SpecialReplacePanel : public wxPanel
{
public:
	SpecialReplacePanel(wxWindow* parent);
	~SpecialReplacePanel() override = default;

	void doReplace(Archive* archive) const;

private:
	wxSpinCtrl* spin_from_         = nullptr;
	wxSpinCtrl* spin_to_           = nullptr;
	wxCheckBox* cb_line_specials_  = nullptr;
	wxCheckBox* cb_thing_specials_ = nullptr;
	wxSpinCtrl* spin_args_from_[5] = {};
	wxSpinCtrl* spin_args_to_[5]   = {};
	wxCheckBox* cb_args_[5]        = {};
};
} // namespace slade

// -----------------------------------------------------------------------------
// SpecialReplacePanel class constructor
// -----------------------------------------------------------------------------
SpecialReplacePanel::SpecialReplacePanel(wxWindow* parent) : wxPanel(parent, -1)
{
	auto lh = ui::LayoutHelper(this);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->AddStretchSpacer();
	sizer->Add(gbsizer, lh.sfWithLargeBorder().Center());

	// From special
	gbsizer->Add(
		new wxStaticText(this, -1, wxS("Replace Special:")),
		{ 0, 0 },
		{ 1, 1 },
		wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
	spin_from_ = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999);
	gbsizer->Add(spin_from_, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// To special
	gbsizer->Add(
		new wxStaticText(this, -1, wxS("With Special:")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
	spin_to_ = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999);
	gbsizer->Add(spin_to_, { 1, 1 }, { 1, 1 }, wxEXPAND);

	// Replace line specials
	cb_line_specials_ = new wxCheckBox(this, -1, wxS("Replace Line Specials"));
	gbsizer->Add(cb_line_specials_, { 0, 2 }, { 1, 1 }, wxEXPAND);

	// Replace thing specials
	cb_thing_specials_ = new wxCheckBox(this, -1, wxS("Replace Thing Specials"));
	gbsizer->Add(cb_thing_specials_, { 1, 2 }, { 1, 1 }, wxEXPAND);

	sizer->Add(
		new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL),
		lh.sfWithBorder(0, wxLEFT | wxRIGHT).Expand());

	// Args
	gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->Add(gbsizer, lh.sfWithLargeBorder().Center());
	for (int a = 0; a < 5; a++)
	{
		// Create controls
		cb_args_[a]        = new wxCheckBox(this, -1, WX_FMT("Arg {}", a));
		spin_args_from_[a] = new wxSpinCtrl(
			this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 0, 255);
		spin_args_to_[a] = new wxSpinCtrl(
			this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER, 0, 255);

		// Add to grid
		gbsizer->Add(cb_args_[a], { a, 0 }, { 1, 1 }, wxEXPAND);
		gbsizer->Add(
			new wxStaticText(this, -1, wxS("Replace:")), { a, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
		gbsizer->Add(spin_args_from_[a], { a, 2 }, { 1, 1 }, wxEXPAND);
		gbsizer->Add(
			new wxStaticText(this, -1, wxS("With:")), { a, 3 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
		gbsizer->Add(spin_args_to_[a], { a, 4 }, { 1, 1 }, wxEXPAND);
	}

	sizer->AddStretchSpacer();

	cb_line_specials_->SetValue(true);
}

// -----------------------------------------------------------------------------
// Performs replace using settings from the panel controls for [archive]
// -----------------------------------------------------------------------------
void SpecialReplacePanel::doReplace(Archive* archive) const
{
	size_t count = archiveoperations::replaceSpecials(
		archive,
		spin_from_->GetValue(),
		spin_to_->GetValue(),
		cb_line_specials_->GetValue(),
		cb_thing_specials_->GetValue(),
		cb_args_[0]->GetValue(),
		spin_args_from_[0]->GetValue(),
		spin_args_to_[0]->GetValue(),
		cb_args_[1]->GetValue(),
		spin_args_from_[1]->GetValue(),
		spin_args_to_[1]->GetValue(),
		cb_args_[2]->GetValue(),
		spin_args_from_[2]->GetValue(),
		spin_args_to_[2]->GetValue(),
		cb_args_[3]->GetValue(),
		spin_args_from_[3]->GetValue(),
		spin_args_to_[3]->GetValue(),
		cb_args_[4]->GetValue(),
		spin_args_from_[4]->GetValue(),
		spin_args_to_[4]->GetValue());

	wxMessageBox(
		WX_FMT("Replaced {} occurrences. See console log for more detailed information.", count),
		wxS("Replace Specials"));
}


// -----------------------------------------------------------------------------
//
// TextureReplacePanel Class
//
// -----------------------------------------------------------------------------
namespace slade
{
class TextureReplacePanel : public wxPanel
{
public:
	TextureReplacePanel(wxWindow* parent);
	~TextureReplacePanel() override = default;

	void doReplace(Archive* archive) const;

private:
	wxTextCtrl* text_from_  = nullptr;
	wxTextCtrl* text_to_    = nullptr;
	wxCheckBox* cb_floor_   = nullptr;
	wxCheckBox* cb_ceiling_ = nullptr;
	wxCheckBox* cb_lower_   = nullptr;
	wxCheckBox* cb_middle_  = nullptr;
	wxCheckBox* cb_upper_   = nullptr;
};
} // namespace slade

// -----------------------------------------------------------------------------
// TextureReplacePanel class constructor
// -----------------------------------------------------------------------------
TextureReplacePanel::TextureReplacePanel(wxWindow* parent) : wxPanel(parent, -1)
{
	auto lh = ui::LayoutHelper(this);

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->AddStretchSpacer();
	sizer->Add(gbsizer, lh.sfWithBorder().Center());

	// From texture
	gbsizer->Add(
		new wxStaticText(this, -1, wxS("Replace Texture:")),
		{ 0, 0 },
		{ 1, 1 },
		wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
	text_from_ = new wxTextCtrl(this, -1);
	gbsizer->Add(text_from_, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// To texture
	gbsizer->Add(
		new wxStaticText(this, -1, wxS("With Texture:")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
	text_to_ = new wxTextCtrl(this, -1);
	gbsizer->Add(text_to_, { 1, 1 }, { 1, 1 }, wxEXPAND);

	sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL), lh.sfWithBorder().Expand());

	gbsizer = new wxGridBagSizer(lh.pad(), lh.pad());
	sizer->Add(gbsizer, lh.sfWithBorder().Center());

	// Upper
	cb_upper_ = new wxCheckBox(this, -1, wxS("Upper Textures"));
	gbsizer->Add(cb_upper_, { 0, 0 }, { 1, 1 }, wxEXPAND);

	// Middle
	cb_middle_ = new wxCheckBox(this, -1, wxS("Middle Textures"));
	gbsizer->Add(cb_middle_, { 1, 0 }, { 1, 1 }, wxEXPAND);

	// Lower
	cb_lower_ = new wxCheckBox(this, -1, wxS("Lower Textures"));
	gbsizer->Add(cb_lower_, { 2, 0 }, { 1, 1 }, wxEXPAND);

	// Floors
	cb_floor_ = new wxCheckBox(this, -1, wxS("Floor Textures"));
	gbsizer->Add(cb_floor_, { 0, 1 }, { 1, 1 }, wxEXPAND);

	// Ceilings
	cb_ceiling_ = new wxCheckBox(this, -1, wxS("Ceiling ") wxS("Textures"));
	gbsizer->Add(cb_ceiling_, { 1, 1 }, { 1, 1 }, wxEXPAND);

	sizer->AddStretchSpacer();
}

// -----------------------------------------------------------------------------
// Performs replace using settings from the panel controls for [archive]
// -----------------------------------------------------------------------------
void TextureReplacePanel::doReplace(Archive* archive) const
{
	size_t count = archiveoperations::replaceTextures(
		archive,
		text_from_->GetValue().utf8_string(),
		text_to_->GetValue().utf8_string(),
		cb_floor_->GetValue(),
		cb_ceiling_->GetValue(),
		cb_lower_->GetValue(),
		cb_middle_->GetValue(),
		cb_upper_->GetValue());

	wxMessageBox(
		WX_FMT("Replaced {} occurrences. See console log for more detailed information.", count),
		wxS("Replace Textures"));
}


// -----------------------------------------------------------------------------
//
// MapReplaceDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapReplaceDialog class constructor
// -----------------------------------------------------------------------------
MapReplaceDialog::MapReplaceDialog(wxWindow* parent, Archive* archive) :
	wxDialog(
		parent,
		-1,
		wxS("Replace In Maps"),
		wxDefaultPosition,
		wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	archive_{ archive }
{
	auto lh = ui::LayoutHelper(this);

	// Init variables
	archive_ = archive;

	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add tabs
	stc_tabs_ = STabCtrl::createControl(this);
	sizer->Add(stc_tabs_, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Thing type tab
	panel_thing_ = new ThingTypeReplacePanel(stc_tabs_);
	stc_tabs_->AddPage(panel_thing_, wxS("Thing Types"));

	// Specials tab
	panel_special_ = new SpecialReplacePanel(stc_tabs_);
	stc_tabs_->AddPage(panel_special_, wxS("Specials"));

	// Textures tab
	panel_texture_ = new TextureReplacePanel(stc_tabs_);
	stc_tabs_->AddPage(panel_texture_, wxS("Textures"));

	// Dialog buttons
	btn_replace_ = new wxButton(this, wxID_OK, wxS("Replace"));
	btn_done_    = new wxButton(this, wxID_CANCEL, wxS("Close"));
	auto hbox    = new wxBoxSizer(wxHORIZONTAL);
	hbox->AddStretchSpacer();
	hbox->Add(btn_replace_, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->Add(btn_done_, wxSizerFlags().Expand());
	sizer->AddSpacer(lh.pad());
	sizer->Add(hbox, lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

	// Setup dialog layout
	SetInitialSize(wxSize(-1, -1));
	wxTopLevelWindowBase::Layout();
	wxWindowBase::Fit();
	wxTopLevelWindowBase::SetMinSize(GetBestSize());
	CenterOnParent();

	// Bind events
	btn_done_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { EndModal(wxID_OK); });
	btn_replace_->Bind(wxEVT_BUTTON, &MapReplaceDialog::onBtnReplace, this);
}


// -----------------------------------------------------------------------------
//
// MapReplaceDialog Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the 'Replace' button is clicked
// -----------------------------------------------------------------------------
void MapReplaceDialog::onBtnReplace(wxCommandEvent& e)
{
	// Get current tab
	int current = stc_tabs_->GetSelection();

	// Thing types
	if (current == 0)
		panel_thing_->doReplace(archive_);

	// Specials
	else if (current == 1)
		panel_special_->doReplace(archive_);

	// Textures
	else if (current == 2)
		panel_texture_->doReplace(archive_);
}
