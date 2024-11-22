
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextEditorSettingsPanel.cpp
// Description: Panel containing text editor preference controls
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
#include "TextEditorSettingsPanel.h"
#include "TextEditorStyleSettingsPanel.h"
#include "UI/Controls/RadioButtonPanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Layout.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, txed_tab_width)
EXTERN_CVAR(Bool, txed_auto_indent)
EXTERN_CVAR(Bool, txed_syntax_hilight)
EXTERN_CVAR(Bool, txed_brace_match)
EXTERN_CVAR(Int, txed_edge_column)
EXTERN_CVAR(Bool, txed_indent_guides)
EXTERN_CVAR(String, txed_style_set)
EXTERN_CVAR(Bool, txed_trim_whitespace)
EXTERN_CVAR(Bool, txed_calltips_mouse)
EXTERN_CVAR(Bool, txed_calltips_parenthesis)
EXTERN_CVAR(Bool, txed_calltips_colourise)
EXTERN_CVAR(Bool, txed_calltips_dim_optional)
EXTERN_CVAR(Bool, txed_calltips_use_font)
EXTERN_CVAR(Bool, txed_fold_enable)
EXTERN_CVAR(Bool, txed_fold_comments)
EXTERN_CVAR(Bool, txed_fold_preprocessor)
EXTERN_CVAR(Bool, txed_fold_lines)
EXTERN_CVAR(Bool, txed_match_cursor_word)
EXTERN_CVAR(Int, txed_hilight_current_line)
EXTERN_CVAR(Int, txed_line_extra_height)
EXTERN_CVAR(Bool, txed_tab_spaces)
EXTERN_CVAR(Int, txed_show_whitespace)
EXTERN_CVAR(Bool, txed_calltips_argset_kb)


// -----------------------------------------------------------------------------
//
// TextEditorSettingsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextEditorSettingsPanel class constructor
// -----------------------------------------------------------------------------
TextEditorSettingsPanel::TextEditorSettingsPanel(wxWindow* parent) : SettingsPanel(parent)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	style_panel_ = new TextEditorStyleSettingsPanel(this);

	auto tabs = STabCtrl::createControl(this);
	tabs->AddPage(createSettingsPanel(tabs), "General");
	tabs->AddPage(createCodePanel(tabs), "Code && Language");
	tabs->AddPage(wxutil::createPadPanel(tabs, style_panel_, padLarge()), "Fonts && Colours");
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

// -----------------------------------------------------------------------------
// Creates the main text editor settings panel
// -----------------------------------------------------------------------------
wxPanel* TextEditorSettingsPanel::createSettingsPanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = LayoutHelper(panel);

	// Create controls
	spin_tab_width_       = wxutil::createSpinCtrl(panel, txed_tab_width, 1, 100);
	cb_tab_spaces_        = new wxCheckBox(panel, -1, "Indent with spaces");
	cb_auto_indent_       = new wxCheckBox(panel, -1, "Enable auto-indent");
	cb_trim_whitespace_   = new wxCheckBox(panel, -1, "Trim whitespace on save");
	cb_brace_match_       = new wxCheckBox(panel, -1, "Hilight matching braces");
	spin_right_margin_    = wxutil::createSpinCtrl(panel, txed_edge_column, 0, 1000);
	spin_line_spacing_    = wxutil::createSpinCtrl(panel, txed_line_extra_height, 0, 10);
	cb_indent_guides_     = new wxCheckBox(panel, -1, "Show indentation guides");
	cb_match_cursor_word_ = new wxCheckBox(panel, -1, "Hilight matching words");
	rbp_line_hilight_     = new RadioButtonPanel(
        panel, { "Off", "Background", "Background+Underline" }, "Current line hilight:");
	rbp_show_whitespace_ = new RadioButtonPanel(
		panel, { "Off", "After indentation only", "Always" }, "Show whitespace:");


	// Create main sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(1).Expand());

	// --- Whitespace/indentation ---
	vbox->Add(
		wxutil::createSectionSeparator(panel, "Whitespace && Indentation"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		vbox,
		{ cb_auto_indent_,
		  cb_trim_whitespace_,
		  cb_tab_spaces_,
		  wxutil::createLabelHBox(panel, "Indentation width: ", spin_tab_width_) },
		lh.sfWithBorder(0, wxLEFT));

	// --- Display ---
	vbox->AddSpacer(lh.padXLarge());
	vbox->Add(wxutil::createSectionSeparator(panel, "Display"), lh.sfWithBorder(0, wxBOTTOM).Expand());

	auto gb_sizer = new wxGridBagSizer(lh.pad(), lh.padLarge());
	vbox->Add(gb_sizer, lh.sfWithBorder(0, wxLEFT));

	int row = 0;
	gb_sizer->Add(cb_indent_guides_, { row++, 0 }, { 1, 3 });
	gb_sizer->Add(cb_brace_match_, { row++, 0 }, { 1, 3 });
	gb_sizer->Add(cb_match_cursor_word_, { row++, 0 }, { 1, 3 });
	gb_sizer->Add(
		new wxStaticText(panel, -1, "Right margin at column:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_right_margin_, { row++, 1 }, { 1, 1 });
	gb_sizer->Add(new wxStaticText(panel, -1, "Extra line spacing:"), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_line_spacing_, { row++, 1 }, { 1, 1 });
	gb_sizer->Add(rbp_line_hilight_, { row++, 0 }, { 1, 3 });
	gb_sizer->Add(rbp_show_whitespace_, { row++, 0 }, { 1, 3 });

	return panel;
}

wxPanel* TextEditorSettingsPanel::createCodePanel(wxWindow* parent)
{
	auto panel = new wxPanel(parent);
	auto lh    = LayoutHelper(panel);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Create controls
	cb_syntax_hilight_        = new wxCheckBox(panel, -1, "Enable syntax hilighting");
	cb_calltips_mouse_        = new wxCheckBox(panel, -1, "Show calltips on mouse hover");
	cb_calltips_parenthesis_  = new wxCheckBox(panel, -1, "Show calltips on opening parenthesis");
	cb_calltips_colourise_    = new wxCheckBox(panel, -1, "Colourise calltip text");
	cb_calltips_dim_optional_ = new wxCheckBox(panel, -1, "Dim optional function parameters");
	cb_calltips_use_font_     = new wxCheckBox(panel, -1, "Use the text editor font in calltips");
	cb_calltips_argset_kb_    = new wxCheckBox(panel, -1, "Use up/down keys to cycle function signatures");
	cb_fold_enable_           = new wxCheckBox(panel, -1, "Enable code folding");
	cb_fold_comments_         = new wxCheckBox(panel, -1, "Fold comment blocks");
	cb_fold_preprocessor_     = new wxCheckBox(panel, -1, "Fold preprocessor regions");
	cb_fold_lines_            = new wxCheckBox(panel, -1, "Show lines at contracted code folding regions");
	cb_calltips_dim_optional_->SetToolTip("If disabled, optional parameters will be shown between [] brackets");

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, lh.sfWithLargeBorder(1).Expand());

	// Syntax Hilighting
	vbox->Add(cb_syntax_hilight_, wxSizerFlags().Expand());

	// Calltips
	vbox->AddSpacer(lh.padXLarge());
	vbox->Add(wxutil::createSectionSeparator(panel, "Calltips"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		vbox,
		{ cb_calltips_mouse_,
		  cb_calltips_colourise_,
		  cb_calltips_parenthesis_,
		  cb_calltips_dim_optional_,
		  cb_calltips_use_font_,
		  cb_calltips_argset_kb_ },
		lh.sfWithBorder(0, wxLEFT));

	// Code Folding
	vbox->AddSpacer(lh.padXLarge());
	vbox->Add(wxutil::createSectionSeparator(panel, "Code Folding"), lh.sfWithBorder(0, wxBOTTOM).Expand());
	lh.layoutVertically(
		vbox,
		{ cb_fold_enable_, cb_fold_comments_, cb_fold_lines_, cb_fold_preprocessor_ },
		lh.sfWithBorder(0, wxLEFT));


	return panel;
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void TextEditorSettingsPanel::loadSettings()
{
	cb_auto_indent_->SetValue(txed_auto_indent);
	cb_trim_whitespace_->SetValue(txed_trim_whitespace);
	cb_syntax_hilight_->SetValue(txed_syntax_hilight);
	cb_indent_guides_->SetValue(txed_indent_guides);
	cb_brace_match_->SetValue(txed_brace_match);
	cb_match_cursor_word_->SetValue(txed_match_cursor_word);
	cb_calltips_mouse_->SetValue(txed_calltips_mouse);
	cb_calltips_parenthesis_->SetValue(txed_calltips_parenthesis);
	cb_calltips_colourise_->SetValue(txed_calltips_colourise);
	cb_calltips_dim_optional_->SetValue(txed_calltips_dim_optional);
	cb_calltips_use_font_->SetValue(txed_calltips_use_font);
	cb_calltips_argset_kb_->SetValue(txed_calltips_argset_kb);
	spin_right_margin_->SetValue(txed_edge_column);
	spin_tab_width_->SetValue(txed_tab_width);
	cb_fold_enable_->SetValue(txed_fold_enable);
	cb_fold_comments_->SetValue(txed_fold_comments);
	cb_fold_preprocessor_->SetValue(txed_fold_preprocessor);
	cb_fold_lines_->SetValue(txed_fold_lines);
	rbp_line_hilight_->setSelection(txed_hilight_current_line);
	spin_line_spacing_->SetValue(txed_line_extra_height);
	cb_tab_spaces_->SetValue(txed_tab_spaces);
	rbp_show_whitespace_->setSelection(txed_show_whitespace);

	style_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void TextEditorSettingsPanel::applySettings()
{
	txed_auto_indent           = cb_auto_indent_->GetValue();
	txed_trim_whitespace       = cb_trim_whitespace_->GetValue();
	txed_syntax_hilight        = cb_syntax_hilight_->GetValue();
	txed_indent_guides         = cb_indent_guides_->GetValue();
	txed_brace_match           = cb_brace_match_->GetValue();
	txed_match_cursor_word     = cb_match_cursor_word_->GetValue();
	txed_tab_width             = spin_tab_width_->GetValue();
	txed_edge_column           = spin_right_margin_->GetValue();
	txed_calltips_mouse        = cb_calltips_mouse_->GetValue();
	txed_calltips_parenthesis  = cb_calltips_parenthesis_->GetValue();
	txed_calltips_colourise    = cb_calltips_colourise_->GetValue();
	txed_calltips_dim_optional = cb_calltips_dim_optional_->GetValue();
	txed_calltips_use_font     = cb_calltips_use_font_->GetValue();
	txed_calltips_argset_kb    = cb_calltips_argset_kb_->GetValue();
	txed_fold_enable           = cb_fold_enable_->GetValue();
	txed_fold_comments         = cb_fold_comments_->GetValue();
	txed_fold_preprocessor     = cb_fold_preprocessor_->GetValue();
	txed_fold_lines            = cb_fold_lines_->GetValue();
	txed_hilight_current_line  = rbp_line_hilight_->getSelection();
	txed_line_extra_height     = spin_line_spacing_->GetValue();
	txed_tab_spaces            = cb_tab_spaces_->GetValue();
	txed_show_whitespace       = rbp_show_whitespace_->getSelection();

	style_panel_->applySettings();
}
