
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextEditorPrefsPanel.cpp
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
#include "TextEditorPrefsPanel.h"
#include "General/UI.h"


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


// -----------------------------------------------------------------------------
//
// TextEditorPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextEditorPrefsPanel class constructor
// -----------------------------------------------------------------------------
TextEditorPrefsPanel::TextEditorPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create main sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	auto gb_sizer = new wxGridBagSizer(UI::pad(), UI::padLarge());
	auto row      = 0;
	sizer->Add(gb_sizer, 0, wxEXPAND);

	// --- Whitespace/indentation ---

	// Tab width
	spin_tab_width_ = new wxSpinCtrl(
		this,
		-1,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize,
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		1,
		100,
		txed_tab_width);
	gb_sizer->Add(new wxStaticText(this, -1, "Indentation Width: "), { row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_tab_width_, { row, 1 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);

	// Tabs as spaces
	cb_tab_spaces_ = new wxCheckBox(this, -1, "Indent With Spaces");
	gb_sizer->Add(cb_tab_spaces_, { row, 2 }, { 1, 2 }, wxEXPAND);

	// Auto indent
	cb_auto_indent_ = new wxCheckBox(this, -1, "Enable Auto-Indent");
	gb_sizer->Add(cb_auto_indent_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Trim whitespace
	cb_trim_whitespace_ = new wxCheckBox(this, -1, "Trim Whitespace on Save");
	gb_sizer->Add(cb_trim_whitespace_, { row, 2 }, { 1, 2 }, wxEXPAND);

	// Separator
	gb_sizer->Add(
		new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL),
		{ ++row, 0 },
		{ 1, 4 },
		wxEXPAND | wxTOP | wxBOTTOM,
		8);

	// --- Display ---

	// Right margin
	spin_right_margin_ = new wxSpinCtrl(
		this,
		-1,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize,
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		0,
		1000,
		txed_edge_column);
	gb_sizer->Add(
		new wxStaticText(this, -1, "Right Margin at Column: "), { ++row, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_right_margin_, { row, 1 }, { 1, 1 }, wxEXPAND);

	// Indentation guides
	cb_indent_guides_ = new wxCheckBox(this, -1, "Show Indentation Guides");
	gb_sizer->Add(cb_indent_guides_, { row, 2 }, { 1, 2 }, wxEXPAND);

	// Syntax Hilighting
	cb_syntax_hilight_ = new wxCheckBox(this, -1, "Show Syntax Hilighting");
	gb_sizer->Add(cb_syntax_hilight_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Hilight current line
	string hl_line_choices[] = { "Off", "Background", "Background+Underline" };
	choice_line_hilight_     = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, hl_line_choices);
	gb_sizer->Add(new wxStaticText(this, -1, "Current Line Hilight: "), { row, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_line_hilight_, { row, 3 }, { 1, 1 }, wxEXPAND);

	// Brace matching
	cb_brace_match_ = new wxCheckBox(this, -1, "Hilight Matching Braces");
	gb_sizer->Add(cb_brace_match_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Line extra spacing
	spin_line_spacing_ = new wxSpinCtrl(
		this,
		-1,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize,
		wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER,
		0,
		10,
		txed_edge_column);
	gb_sizer->Add(new wxStaticText(this, -1, "Extra Line Spacing: "), { row, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(spin_line_spacing_, { row, 3 }, { 1, 1 }, wxEXPAND);

	// Word matching
	cb_match_cursor_word_ = new wxCheckBox(this, -1, "Hilight Matching Words");
	cb_match_cursor_word_->SetToolTip(
		"When enabled, any words matching the word at the current cursor position or selection will be hilighted");
	gb_sizer->Add(cb_match_cursor_word_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Show whitespace
	string show_ws_choices[] = { "Off", "After Indentation Only", "Always" };
	choice_show_whitespace_  = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, show_ws_choices);
	gb_sizer->Add(new wxStaticText(this, -1, "Show Whitespace: "), { row, 2 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(choice_show_whitespace_, { row, 3 }, { 1, 1 }, wxEXPAND);

	// Separator
	gb_sizer->Add(
		new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL),
		{ ++row, 0 },
		{ 1, 4 },
		wxEXPAND | wxTOP | wxBOTTOM,
		8);

	// --- Calltips ---

	// Calltips on mouse hover
	cb_calltips_mouse_ = new wxCheckBox(this, -1, "Show calltips on mouse hover");
	gb_sizer->Add(cb_calltips_mouse_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Colourise calltips
	cb_calltips_colourise_ = new wxCheckBox(this, -1, "Colourise calltip text");
	gb_sizer->Add(cb_calltips_colourise_, { row, 2 }, { 1, 2 }, wxEXPAND);

	// Calltips on parenthesis
	cb_calltips_parenthesis_ = new wxCheckBox(this, -1, "Show calltips on opening parenthesis");
	gb_sizer->Add(cb_calltips_parenthesis_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Dim optional calltip parameters
	cb_calltips_dim_optional_ = new wxCheckBox(this, -1, "Dim optional function parameters");
	cb_calltips_dim_optional_->SetToolTip("If disabled, optional parameters will be shown between [] brackets");
	gb_sizer->Add(cb_calltips_dim_optional_, { row, 2 }, { 1, 2 }, wxEXPAND);

	// Use text editor font in calltips
	cb_calltips_use_font_ = new wxCheckBox(this, -1, "Use the text editor font in calltips");
	gb_sizer->Add(cb_calltips_use_font_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Separator
	gb_sizer->Add(
		new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL),
		{ ++row, 0 },
		{ 1, 4 },
		wxEXPAND | wxTOP | wxBOTTOM,
		8);

	// --- Folding ---

	// Enable Code Folding
	cb_fold_enable_ = new wxCheckBox(this, -1, "Enable Code Folding");
	gb_sizer->Add(cb_fold_enable_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Fold Comments
	cb_fold_comments_ = new wxCheckBox(this, -1, "Fold comment blocks");
	gb_sizer->Add(cb_fold_comments_, { row, 2 }, { 1, 2 }, wxEXPAND);

	// Fold Lines
	cb_fold_lines_ = new wxCheckBox(this, -1, "Show lines at contracted code folding regions");
	gb_sizer->Add(cb_fold_lines_, { ++row, 0 }, { 1, 2 }, wxEXPAND);

	// Fold Preprcessor
	cb_fold_preprocessor_ = new wxCheckBox(this, -1, "Fold preprocessor regions");
	cb_fold_preprocessor_->SetToolTip("Enable folding for preprocessor regions, eg. #if/#endif, #region/#endregion");
	gb_sizer->Add(cb_fold_preprocessor_, { row, 2 }, { 1, 2 }, wxEXPAND);

	gb_sizer->AddGrowableCol(0, 1);
	gb_sizer->AddGrowableCol(2, 1);
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void TextEditorPrefsPanel::init()
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
	spin_right_margin_->SetValue(txed_edge_column);
	spin_tab_width_->SetValue(txed_tab_width);
	cb_fold_enable_->SetValue(txed_fold_enable);
	cb_fold_comments_->SetValue(txed_fold_comments);
	cb_fold_preprocessor_->SetValue(txed_fold_preprocessor);
	cb_fold_lines_->SetValue(txed_fold_lines);
	choice_line_hilight_->SetSelection(txed_hilight_current_line);
	spin_line_spacing_->SetValue(txed_line_extra_height);
	cb_tab_spaces_->SetValue(txed_tab_spaces);
	choice_show_whitespace_->SetSelection(txed_show_whitespace);
}

// -----------------------------------------------------------------------------
// Applies preference values from the controls to CVARs
// -----------------------------------------------------------------------------
void TextEditorPrefsPanel::applyPreferences()
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
	txed_fold_enable           = cb_fold_enable_->GetValue();
	txed_fold_comments         = cb_fold_comments_->GetValue();
	txed_fold_preprocessor     = cb_fold_preprocessor_->GetValue();
	txed_fold_lines            = cb_fold_lines_->GetValue();
	txed_hilight_current_line  = choice_line_hilight_->GetSelection();
	txed_line_extra_height     = spin_line_spacing_->GetValue();
	txed_tab_spaces            = cb_tab_spaces_->GetValue();
	txed_show_whitespace       = choice_show_whitespace_->GetSelection();
}
