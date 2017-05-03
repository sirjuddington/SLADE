
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TextEditorPrefsPanel.cpp
 * Description: Panel containing text editor preference controls
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "TextEditorPrefsPanel.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
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


/*******************************************************************
 * TEXTEDITORPREFSPANEL CLASS FUNCTIONS
 *******************************************************************/

/* TextEditorPrefsPanel::TextEditorPrefsPanel
 * TextEditorPrefsPanel class constructor
 *******************************************************************/
TextEditorPrefsPanel::TextEditorPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create main sizer
	wxBoxSizer* psizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(psizer);

	// Create frame+sizer
	wxStaticBox* frame = new wxStaticBox(this, -1, "Text Editor Preferences");
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	psizer->Add(sizer, 1, wxEXPAND|wxALL, 4);

	// Tab width
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	spin_tab_width = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, 1, 100, txed_tab_width);
	hbox->Add(new wxStaticText(this, -1, "Tab Indentation Width: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox->Add(spin_tab_width, 1, wxEXPAND);
	sizer->Add(hbox, 0, wxALL|wxEXPAND, 4);

	// Auto indent
	cb_auto_indent = new wxCheckBox(this, -1, "Enable Auto-Indent");
	sizer->Add(cb_auto_indent, 0, wxEXPAND|wxALL, 4);

	// Trim whitespace
	cb_trim_whitespace = new wxCheckBox(this, -1, "Trim Whitespace on Save");
	sizer->Add(cb_trim_whitespace, 0, wxEXPAND|wxALL, 4);

	// Separator
	sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND|wxALL, 4);

	// Right margin
	hbox = new wxBoxSizer(wxHORIZONTAL);
	spin_right_margin = new wxSpinCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, 0, 1000, txed_edge_column);
	hbox->Add(new wxStaticText(this, -1, "Right Margin at Column: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox->Add(spin_right_margin, 1, wxEXPAND);
	sizer->Add(hbox, 0, wxALL|wxEXPAND, 4);

	// Syntax Hilighting
	cb_syntax_hilight = new wxCheckBox(this, -1, "Show Syntax Hilighting");
	sizer->Add(cb_syntax_hilight, 0, wxEXPAND|wxALL, 4);

	// Indentation guides
	cb_indent_guides = new wxCheckBox(this, -1, "Show Indentation Guides");
	sizer->Add(cb_indent_guides, 0, wxEXPAND|wxALL, 4);

	// Hilight current line
	string choices[] = { "Off", "Background", "Background + Underline" };
	hbox = new wxBoxSizer(wxHORIZONTAL);
	choice_line_hilight = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 3, choices);
	hbox->Add(new wxStaticText(this, -1, "Current Line Hilight: "), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 2);
	hbox->Add(choice_line_hilight, 1, wxEXPAND);
	sizer->Add(hbox, 0, wxALL, 4);

	// Brace matching
	cb_brace_match = new wxCheckBox(this, -1, "Hilight Matching Braces");
	sizer->Add(cb_brace_match, 0, wxEXPAND|wxALL, 4);

	// Word matching
	cb_match_cursor_word = new wxCheckBox(this, -1, "Hilight Matching Words");
	cb_match_cursor_word->SetToolTip(
		"When enabled, any words matching the word at the current cursor position will be hilighted"
	);
	sizer->Add(cb_match_cursor_word, 0, wxEXPAND|wxALL, 4);

	// Separator
	sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxALL, 4);

	// Calltips on mouse hover
	cb_calltips_mouse = new wxCheckBox(this, -1, "Show calltips on mouse hover");
	sizer->Add(cb_calltips_mouse, 0, wxEXPAND|wxALL, 4);

	// Calltips on parenthesis
	cb_calltips_parenthesis = new wxCheckBox(this, -1, "Show calltips on opening parenthesis");
	sizer->Add(cb_calltips_parenthesis, 0, wxEXPAND|wxALL, 4);

	// Colourise calltips
	cb_calltips_colourise = new wxCheckBox(this, -1, "Colourise calltip function name and parameter types");
	sizer->Add(cb_calltips_colourise, 0, wxEXPAND | wxALL, 4);

	// Dim optional calltip parameters
	cb_calltips_dim_optional = new wxCheckBox(this, -1, "Dim optional function parameters in calltips");
	cb_calltips_dim_optional->SetToolTip("If disabled, optional parameters will be shown between [] brackets");
	sizer->Add(cb_calltips_dim_optional, 0, wxEXPAND | wxALL, 4);

	// Use text editor font in calltips
	cb_calltips_use_font = new wxCheckBox(this, -1, "Use the text editor font in calltips");
	sizer->Add(cb_calltips_use_font, 0, wxEXPAND | wxALL, 4);

	// Separator
	sizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxALL, 4);

	// Enable Code Folding
	cb_fold_enable = new wxCheckBox(this, -1, "Enable Code Folding");
	sizer->Add(cb_fold_enable, 0, wxEXPAND | wxALL, 4);

	// Fold Comments
	cb_fold_comments = new wxCheckBox(this, -1, "Fold comment blocks");
	sizer->Add(cb_fold_comments, 0, wxEXPAND | wxALL, 4);

	// Fold Preprcessor
	cb_fold_preprocessor = new wxCheckBox(this, -1, "Fold preprocessor regions");
	cb_fold_preprocessor->SetToolTip("Enable folding for preprocessor regions, eg. #if/#endif, #region/#endregion");
	sizer->Add(cb_fold_preprocessor, 0, wxEXPAND | wxALL, 4);

	// Fold Lines
	cb_fold_lines = new wxCheckBox(this, -1, "Show lines at contracted code folding regions");
	sizer->Add(cb_fold_lines, 0, wxEXPAND | wxALL, 4);
}

/* TextEditorPrefsPanel::~TextEditorPrefsPanel
 * TextEditorPrefsPanel class destructor
 *******************************************************************/
TextEditorPrefsPanel::~TextEditorPrefsPanel()
{
}

/* TextEditorPrefsPanel::init
 * Initialises panel controls
 *******************************************************************/
void TextEditorPrefsPanel::init()
{
	cb_auto_indent->SetValue(txed_auto_indent);
	cb_trim_whitespace->SetValue(txed_trim_whitespace);
	cb_syntax_hilight->SetValue(txed_syntax_hilight);
	cb_indent_guides->SetValue(txed_indent_guides);
	cb_brace_match->SetValue(txed_brace_match);
	cb_match_cursor_word->SetValue(txed_match_cursor_word);
	cb_calltips_mouse->SetValue(txed_calltips_mouse);
	cb_calltips_parenthesis->SetValue(txed_calltips_parenthesis);
	cb_calltips_colourise->SetValue(txed_calltips_colourise);
	cb_calltips_dim_optional->SetValue(txed_calltips_dim_optional);
	cb_calltips_use_font->SetValue(txed_calltips_use_font);
	spin_right_margin->SetValue(txed_edge_column);
	spin_tab_width->SetValue(txed_tab_width);
	cb_fold_enable->SetValue(txed_fold_enable);
	cb_fold_comments->SetValue(txed_fold_comments);
	cb_fold_preprocessor->SetValue(txed_fold_preprocessor);
	cb_fold_lines->SetValue(txed_fold_lines);
	choice_line_hilight->SetSelection(txed_hilight_current_line);
}

/* TextEditorPrefsPanel::applyPreferences
 * Applies preference values from the controls to CVARs
 *******************************************************************/
void TextEditorPrefsPanel::applyPreferences()
{
	txed_auto_indent = cb_auto_indent->GetValue();
	txed_trim_whitespace = cb_trim_whitespace->GetValue();
	txed_syntax_hilight = cb_syntax_hilight->GetValue();
	txed_indent_guides = cb_indent_guides->GetValue();
	txed_brace_match = cb_brace_match->GetValue();
	txed_match_cursor_word = cb_match_cursor_word->GetValue();
	txed_tab_width = spin_tab_width->GetValue();
	txed_edge_column = spin_right_margin->GetValue();
	txed_calltips_mouse = cb_calltips_mouse->GetValue();
	txed_calltips_parenthesis = cb_calltips_parenthesis->GetValue();
	txed_calltips_colourise = cb_calltips_colourise->GetValue();
	txed_calltips_dim_optional = cb_calltips_dim_optional->GetValue();
	txed_calltips_use_font = cb_calltips_use_font->GetValue();
	txed_fold_enable = cb_fold_enable->GetValue();
	txed_fold_comments = cb_fold_comments->GetValue();
	txed_fold_preprocessor = cb_fold_preprocessor->GetValue();
	txed_fold_lines = cb_fold_lines->GetValue();
	txed_hilight_current_line = choice_line_hilight->GetSelection();
}
