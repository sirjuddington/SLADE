
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextEditorSettingsPanel.cpp
// Description: Panel containing text editor settings controls
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
#include "UI/Controls/SettingsTable.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Constants
//
// -----------------------------------------------------------------------------
constexpr int MINIMAP_SMALL  = 80;
constexpr int MINIMAP_MEDIUM = 110;
constexpr int MINIMAP_LARGE  = 140;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, txed_minimap_width)


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
	tabs->AddPage(createSettingsPanel(tabs), wxS("General"));
	tabs->AddPage(createCodePanel(tabs), wxS("Code && Language"));
	tabs->AddPage(wxutil::createPadPanel(tabs, style_panel_, padLarge()), wxS("Fonts && Colours"));
	sizer->Add(tabs, wxSizerFlags(1).Expand());
}

// -----------------------------------------------------------------------------
// Creates the main text editor settings panel
// -----------------------------------------------------------------------------
wxPanel* TextEditorSettingsPanel::createSettingsPanel(wxWindow* parent)
{
	settings_general_ = new SettingsTable(parent, true, "Whitespace && Indentation");

	// Whitespace/Indentation
	settings_general_->addCheckBox("Enable auto-indent", "txed_auto_indent");
	settings_general_->addCheckBox("Trim whitespace on save", "txed_trim_whitespace");
	settings_general_->addCheckBox("Indent with spaces", "txed_tab_spaces");
	settings_general_->addSpinControl("Indentation width", "txed_tab_width", 1, 100);

	// Display
	settings_general_->addSectionSeparator("Display");
	settings_general_->addCheckBox("Show indentation guides", "txed_indent_guides");
	settings_general_->addCheckBox("Hilight matching braces", "txed_brace_match");
	settings_general_->addCheckBox(
		"Hilight matching words|"
		"When enabled, any words matching the word at the current cursor position or selection will be hilighted",
		"txed_match_cursor_word");
	settings_general_->addSpinControl("Right margin at column", "txed_edge_column", 0, 1000);
	settings_general_->addSpinControl("Extra line spacing", "txed_line_extra_height", 0, 10);
	settings_general_->addRadioButtons(
		"Current line hilight", "txed_hilight_current_line", { "Off", "Background", "Background+Underline" });
	settings_general_->addRadioButtons(
		"Show whitespace", "txed_show_whitespace", { "Off", "After indentation only", "Always" });
#if wxCHECK_VERSION(3, 3, 2)
	rbp_minimap_ = new RadioButtonPanel(settings_general_, { "Off", "Small", "Medium", "Large" });
	settings_general_->addCustomControl("Minimap", rbp_minimap_);
#endif

	return settings_general_;
}

// -----------------------------------------------------------------------------
// Creates the code settings panel
// -----------------------------------------------------------------------------
wxPanel* TextEditorSettingsPanel::createCodePanel(wxWindow* parent)
{
	settings_code_ = new SettingsTable(parent);

	settings_code_->addCheckBox("Enable syntax hilighting", "txed_syntax_hilight");

	// Calltips
	settings_code_->addSectionSeparator("Calltips");
	settings_code_->addCheckBox("Show calltips on mouse hover", "txed_calltips_mouse");
	settings_code_->addCheckBox("Show calltips on opening parenthesis", "txed_calltips_parenthesis");
	settings_code_->addCheckBox("Colourise calltip text", "txed_calltips_colourise");
	settings_code_->addCheckBox(
		"Dim optional function parameters|"
		"If disabled, optional parameters will be shown between [] brackets",
		"txed_calltips_dim_optional");
	settings_code_->addCheckBox("Use the text editor font in calltips", "txed_calltips_use_font");
	settings_code_->addCheckBox("Use up/down keys to cycle function signatures", "txed_calltips_argset_kb");

	// Code Folding
	settings_code_->addSectionSeparator("Code Folding");
	settings_code_->addCheckBox("Enable code folding", "txed_fold_enable");
	settings_code_->addCheckBox("Fold comment blocks", "txed_fold_comments");
	settings_code_->addCheckBox(
		"Fold preprocessor regions|"
		"Enable folding for preprocessor regions, eg. #if/#endif, #region/#endregion",
		"txed_fold_preprocessor");
	settings_code_->addCheckBox("Show lines at contracted code folding regions", "txed_fold_lines");

	return settings_code_;
}

// -----------------------------------------------------------------------------
// Loads settings from cvars into the controls
// -----------------------------------------------------------------------------
void TextEditorSettingsPanel::loadSettings()
{
#if wxCHECK_VERSION(3, 3, 2)
	if (txed_minimap_width <= 0)
		rbp_minimap_->setSelection(0);
	else if (txed_minimap_width <= MINIMAP_SMALL)
		rbp_minimap_->setSelection(1);
	else if (txed_minimap_width <= MINIMAP_MEDIUM)
		rbp_minimap_->setSelection(2);
	else
		rbp_minimap_->setSelection(3);
#endif

	settings_general_->loadSettings();
	settings_code_->loadSettings();
	style_panel_->loadSettings();
}

// -----------------------------------------------------------------------------
// Applies settings from the panel controls to cvars
// -----------------------------------------------------------------------------
void TextEditorSettingsPanel::applySettings()
{
#if wxCHECK_VERSION(3, 3, 2)
	// Apply minimap width (allowing for custom width values)
	if (rbp_minimap_->getSelection() == 0)
		txed_minimap_width = 0;
	else if (rbp_minimap_->getSelection() == 1 && (txed_minimap_width <= 0 || txed_minimap_width > MINIMAP_SMALL))
		txed_minimap_width = MINIMAP_SMALL;
	else if (
		rbp_minimap_->getSelection() == 2
		&& (txed_minimap_width <= MINIMAP_SMALL || txed_minimap_width > MINIMAP_MEDIUM))
		txed_minimap_width = MINIMAP_MEDIUM;
	else if (rbp_minimap_->getSelection() == 3 && txed_minimap_width <= MINIMAP_MEDIUM)
		txed_minimap_width = MINIMAP_LARGE;
#endif

	settings_general_->applySettings();
	settings_code_->applySettings();
	style_panel_->applySettings();
}
