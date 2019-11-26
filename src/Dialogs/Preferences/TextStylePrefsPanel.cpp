
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextStylePrefsPanel.cpp
// Description: Panel containing text style controls, to change the fonts and
//              colours used in the text editor
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
#include "TextStylePrefsPanel.h"
#include "App.h"
#include "General/UI.h"
#include "TextEditor/UI/TextEditorCtrl.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, txed_override_font)
EXTERN_CVAR(Int, txed_override_font_size)


// -----------------------------------------------------------------------------
//
// TextStylePrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextStylePrefsPanel class constructor
// -----------------------------------------------------------------------------
TextStylePrefsPanel::TextStylePrefsPanel(wxWindow* parent) : PrefsPanelBase{ parent }, language_preview_{ "preview" }
{
	// Init variables
	ss_current_.copySet(StyleSet::currentSet());
	ts_current_ = ss_current_.style("default");

	auto sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	SetSizer(sizer);

	// Styleset font override
	cb_font_override_ = new wxCheckBox(this, -1, "Override Default Font:");
	cb_font_override_->SetToolTip("Always use the selected font in the text editor, instead of the style's font below");
	fp_font_override_ = new wxFontPickerCtrl(this, -1);
	sizer->Add(WxUtils::layoutHorizontally({ cb_font_override_, fp_font_override_ }, 1), { 0, 0 }, { 1, 2 }, wxEXPAND);

	// Styleset selector
	choice_styleset_ = new wxChoice(this, -1);
	for (unsigned a = 0; a < StyleSet::numSets(); a++)
		choice_styleset_->Append(StyleSet::styleName(a));
	btn_savestyleset_ = new wxButton(this, -1, "Save Set");
	auto hbox         = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(this, -1, "Style Set:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(choice_styleset_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(btn_savestyleset_, 0, wxEXPAND);
	sizer->Add(hbox, { 1, 0 }, { 1, 2 }, wxEXPAND);

	// Style list
	list_styles_ = new wxListBox(this, -1);
	list_styles_->Append("Default");
	list_styles_->Append("Selection");
	for (unsigned a = 0; a < ss_current_.nStyles(); a++)
		list_styles_->Append(ss_current_.style(a)->description());
	sizer->Add(list_styles_, { 2, 0 }, { 2, 1 }, wxEXPAND);

	// Style properties
	auto style_props_panel = createStylePanel();
	sizer->Add(style_props_panel, { 2, 1 }, { 1, 1 }, wxEXPAND);

	// Preview
	te_preview_ = new TextEditorCtrl(this, -1);
	sizer->Add(te_preview_, { 3, 1 }, { 1, 1 }, wxEXPAND);

	sizer->AddGrowableCol(1, 1);
	sizer->AddGrowableRow(3, 1);

	// Bind events
	list_styles_->Bind(wxEVT_LISTBOX, &TextStylePrefsPanel::onStyleSelected, this);
	cb_override_font_face_->Bind(wxEVT_CHECKBOX, &TextStylePrefsPanel::onCBOverrideFontFace, this);
	cb_override_font_size_->Bind(wxEVT_CHECKBOX, &TextStylePrefsPanel::onCBOverrideFontSize, this);
	cb_override_font_bold_->Bind(wxEVT_CHECKBOX, &TextStylePrefsPanel::onCBOverrideFontBold, this);
	cb_override_font_italic_->Bind(wxEVT_CHECKBOX, &TextStylePrefsPanel::onCBOverrideFontItalic, this);
	cb_override_font_underlined_->Bind(wxEVT_CHECKBOX, &TextStylePrefsPanel::onCBOverrideFontUnderlined, this);
	cb_override_foreground_->Bind(wxEVT_CHECKBOX, &TextStylePrefsPanel::onCBOverrideForeground, this);
	cb_override_background_->Bind(wxEVT_CHECKBOX, &TextStylePrefsPanel::onCBOverrideBackground, this);
	fp_font_->Bind(wxEVT_FONTPICKER_CHANGED, &TextStylePrefsPanel::onFontChanged, this);
	cp_foreground_->Bind(wxEVT_COLOURPICKER_CHANGED, &TextStylePrefsPanel::onForegroundChanged, this);
	cp_background_->Bind(wxEVT_COLOURPICKER_CHANGED, &TextStylePrefsPanel::onBackgroundChanged, this);
	btn_savestyleset_->Bind(wxEVT_BUTTON, &TextStylePrefsPanel::onBtnSaveStyleSet, this);
	choice_styleset_->Bind(wxEVT_CHOICE, &TextStylePrefsPanel::onStyleSetSelected, this);
	cb_font_override_->Bind(wxEVT_CHECKBOX, &TextStylePrefsPanel::onCBOverrideFont, this);
	fp_font_override_->Bind(wxEVT_FONTPICKER_CHANGED, &TextStylePrefsPanel::onFontOverrideChanged, this);

	// Init controls
	if (!txed_override_font.value.empty())
	{
		cb_font_override_->SetValue(true);
		fp_font_override_->SetSelectedFont(wxFont(
			txed_override_font_size == 0 ? 10 : txed_override_font_size,
			wxFONTFAMILY_MODERN,
			wxFONTSTYLE_NORMAL,
			wxFONTWEIGHT_NORMAL,
			false,
			txed_override_font));
		fp_font_override_->Enable(true);
	}
	else
	{
		cb_font_override_->SetValue(false);
		fp_font_override_->SetSelectedFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
		fp_font_override_->Enable(false);
	}

	// Select default style
	init_done_ = false;
	list_styles_->SetSelection(0);
	updateStyleControls();
	init_done_ = true;
}

// -----------------------------------------------------------------------------
// Creates and returns a panel containing text style controls
// -----------------------------------------------------------------------------
wxPanel* TextStylePrefsPanel::createStylePanel()
{
	auto panel = new wxPanel(this);
	auto sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	panel->SetSizer(sizer);

	// Font
	sizer->Add(
		WxUtils::createLabelHBox(panel, "Font:", fp_font_ = new wxFontPickerCtrl(panel, -1)),
		{ 0, 0 },
		{ 1, 2 },
		wxEXPAND);

	// Override properties
	auto override_props_sizer = WxUtils::layoutHorizontally(
		vector<wxObject*>{ cb_override_font_face_       = new wxCheckBox(panel, -1, "Face"),
						   cb_override_font_size_       = new wxCheckBox(panel, -1, "Size"),
						   cb_override_font_bold_       = new wxCheckBox(panel, -1, "Bold"),
						   cb_override_font_italic_     = new wxCheckBox(panel, -1, "Italic"),
						   cb_override_font_underlined_ = new wxCheckBox(panel, -1, "Underlined") });
	sizer->Add(
		WxUtils::createLabelVBox(panel, "Override default font properties:", override_props_sizer),
		{ 1, 0 },
		{ 1, 2 },
		wxEXPAND);

	// Foreground colour
	cb_override_foreground_ = new wxCheckBox(panel, -1, "Foreground Colour:");
	cp_foreground_          = new wxColourPickerCtrl(
        panel, -1, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_LABEL | wxCLRP_USE_TEXTCTRL);
	sizer->Add(cb_override_foreground_, { 2, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(cp_foreground_, { 2, 1 }, { 1, 1 }, wxEXPAND);

	// Background colour
	cb_override_background_ = new wxCheckBox(panel, -1, "Background Colour:");
	cp_background_          = new wxColourPickerCtrl(
        panel, -1, *wxBLACK, wxDefaultPosition, wxDefaultSize, wxCLRP_SHOW_LABEL | wxCLRP_USE_TEXTCTRL);
	sizer->Add(cb_override_background_, { 3, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
	sizer->Add(cp_background_, { 3, 1 }, { 1, 1 }, wxEXPAND);

	sizer->AddGrowableCol(1, 1);

	return panel;
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::init()
{
	te_preview_->SetText(
		"#include \"include.txt\"\n"
		"\n"
		"string text = \"A string here\";\n"
		"char c = \'c\';\n"
		"\n"
		"// Comment\n"
		"void function(int x, int y)\n"
		"{\n"
		"\tx = (x + 10);\n"
		"\ty = y - CONSTANT;\n\n"
		"\tif (x > OTHER_CONSTANT)\n"
		"\t{\n"
		"\t\tx = CONSTANT;\n"
		"\t\ty += 50;\n"
		"\t\tobject.x_property = x;\n"
		"\t\tobject.y_property = y;\n"
		"\t}\n"
		"}\n");

	language_preview_.addWord(TextLanguage::WordType::Constant, "CONSTANT");
	language_preview_.addWord(TextLanguage::WordType::Constant, "OTHER_CONSTANT");
	language_preview_.addWord(TextLanguage::WordType::Type, "string");
	language_preview_.addWord(TextLanguage::WordType::Type, "char");
	language_preview_.addWord(TextLanguage::WordType::Keyword, "void");
	language_preview_.addWord(TextLanguage::WordType::Keyword, "return");
	language_preview_.addWord(TextLanguage::WordType::Type, "int");
	language_preview_.addWord(TextLanguage::WordType::Keyword, "if");
	language_preview_.addWord(TextLanguage::WordType::Type, "object");
	language_preview_.addWord(TextLanguage::WordType::Property, "x_property");
	language_preview_.addWord(TextLanguage::WordType::Property, "y_property");
	language_preview_.addFunction("function", "int x, int y");
	te_preview_->setLanguage(&language_preview_);

	te_preview_->SetReadOnly(true);
	te_preview_->SetEdgeColumn(34);
}

// -----------------------------------------------------------------------------
// Updates style-related controls to reflect the currently selected style in
// the list
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updateStyleControls()
{
	if (!ts_current_)
		return;

	// Get default style
	auto style_default = ss_current_.style("default");

	// Reset UI stuff
	cb_override_font_face_->SetValue(true);
	cb_override_font_size_->SetValue(true);
	cb_override_font_bold_->SetValue(true);
	cb_override_font_italic_->SetValue(true);
	cb_override_font_underlined_->SetValue(true);
	cb_override_foreground_->SetValue(true);
	cb_override_background_->SetValue(true);

	// Disable override checkboxes if default style
	bool enable = (ts_current_ != style_default);
	cb_override_font_face_->Enable(enable);
	cb_override_font_size_->Enable(enable);
	cb_override_font_bold_->Enable(enable);
	cb_override_font_italic_->Enable(enable);
	cb_override_font_underlined_->Enable(enable);
	cb_override_foreground_->Enable(enable);
	cb_override_background_->Enable(enable);

	// Update style properties
	auto font = fp_font_->GetSelectedFont();

	// Font face
	wxString font_face = ts_current_->fontFace();
	if (font_face.IsEmpty())
	{
		font_face = style_default->fontFace();
		cb_override_font_face_->SetValue(false);
	}
	font.SetFaceName(font_face);

	// Font size
	int font_size = ts_current_->fontSize();
	if (font_size <= 0)
	{
		font_size = style_default->fontSize();
		cb_override_font_size_->SetValue(false);
	}
	font.SetPointSize(font_size);

	// Bold
	bool bold = false;
	if (ts_current_->bold() > 0)
		bold = true;
	else if (ts_current_->bold() < 0)
	{
		bold = !!style_default->bold();
		cb_override_font_bold_->SetValue(false);
	}
	if (bold)
		font.SetWeight(wxFONTWEIGHT_BOLD);
	else
		font.SetWeight(wxFONTWEIGHT_NORMAL);

	// Italic
	bool italic = false;
	if (ts_current_->italic() > 0)
		italic = true;
	else if (ts_current_->italic() < 0)
	{
		italic = !!style_default->italic();
		cb_override_font_italic_->SetValue(false);
	}
	if (italic)
		font.SetStyle(wxFONTSTYLE_ITALIC);
	else
		font.SetStyle(wxFONTSTYLE_NORMAL);

	// Underlined
	bool underlined = false;
	if (ts_current_->underlined() > 0)
		underlined = true;
	else if (ts_current_->underlined() < 0)
	{
		underlined = !!style_default->underlined();
		cb_override_font_underlined_->SetValue(false);
	}
	font.SetUnderlined(underlined);

	// Foreground
	ColRGBA col_foreground;
	if (ts_current_->hasForeground())
		col_foreground.set(ts_current_->foreground());
	else
	{
		col_foreground.set(style_default->foreground());
		cb_override_foreground_->SetValue(false);
	}
	cp_foreground_->SetColour(WXCOL(col_foreground));

	// Background
	ColRGBA col_background;
	if (ts_current_->hasBackground())
		col_background.set(ts_current_->background());
	else
	{
		col_background.set(style_default->background());
		cb_override_background_->SetValue(false);
	}
	cp_background_->SetColour(WXCOL(col_background));

	// Apply font
	fp_font_->SetSelectedFont(font);
}

// -----------------------------------------------------------------------------
// Updates the font face property of the currently selected style
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updateFontFace() const
{
	// Update current style
	if (cb_override_font_face_->GetValue())
		ts_current_->setFontFace(fp_font_->GetSelectedFont().GetFaceName().ToStdString());
	else
		ts_current_->setFontFace("");
}

// -----------------------------------------------------------------------------
// Updates the font size property of the currently selected style
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updateFontSize() const
{
	if (cb_override_font_size_->GetValue())
		ts_current_->setFontSize(fp_font_->GetSelectedFont().GetPointSize());
	else
		ts_current_->setFontSize(-1);
}

// -----------------------------------------------------------------------------
// Updates the font bold property of the currently selected style
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updateFontBold() const
{
	if (cb_override_font_bold_->GetValue())
	{
		auto font = fp_font_->GetSelectedFont();
		if (font.GetWeight() == wxFONTWEIGHT_BOLD)
			ts_current_->setBold(1);
		else
			ts_current_->setBold(0);
	}
	else
		ts_current_->setBold(-1);
}

// -----------------------------------------------------------------------------
// Updates the font italic property of the currently selected style
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updateFontItalic() const
{
	if (cb_override_font_italic_->GetValue())
	{
		auto font = fp_font_->GetSelectedFont();
		if (font.GetStyle() == wxFONTSTYLE_ITALIC)
			ts_current_->setItalic(1);
		else
			ts_current_->setItalic(0);
	}
	else
		ts_current_->setItalic(-1);
}

// -----------------------------------------------------------------------------
// Updates the font underline property of the currently selected style
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updateFontUnderlined() const
{
	if (cb_override_font_underlined_->GetValue())
	{
		auto font = fp_font_->GetSelectedFont();
		if (font.GetUnderlined())
			ts_current_->setUnderlined(1);
		else
			ts_current_->setUnderlined(0);
	}
	else
		ts_current_->setUnderlined(-1);
}

// -----------------------------------------------------------------------------
// Updates the foreground colour property of the currently selected style
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updateForeground() const
{
	if (cb_override_foreground_->GetValue())
	{
		auto wxc = cp_foreground_->GetColour();
		ts_current_->setForeground(ColRGBA(COLWX(wxc), 255));
	}
	else
	{
		ts_current_->clearForeground();
	}
}

// -----------------------------------------------------------------------------
// Updates the background colour property of the currently selected style
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updateBackground() const
{
	if (cb_override_background_->GetValue())
	{
		auto wxc = cp_background_->GetColour();
		ts_current_->setBackground(ColRGBA(COLWX(wxc), 255));
	}
	else
	{
		ts_current_->clearBackground();
	}
}

// -----------------------------------------------------------------------------
// Updates the preview text editor
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::updatePreview()
{
	// Save current font override options
	string f_override = txed_override_font;
	int    s_override = txed_override_font_size;

	// Apply font override options (temporarily)
	if (cb_font_override_->GetValue())
	{
		txed_override_font      = WxUtils::strToView(fp_font_override_->GetSelectedFont().GetFaceName());
		txed_override_font_size = fp_font_override_->GetSelectedFont().GetPointSize();
	}
	else
	{
		txed_override_font      = "";
		txed_override_font_size = 0;
	}

	// Apply style to preview
	ss_current_.applyTo(te_preview_);

	// Restore font override options
	txed_override_font      = f_override;
	txed_override_font_size = s_override;
}

// -----------------------------------------------------------------------------
// Applies the current style properties to the current set
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::applyPreferences()
{
	if (cb_font_override_->GetValue())
	{
		txed_override_font      = WxUtils::strToView(fp_font_override_->GetSelectedFont().GetFaceName());
		txed_override_font_size = fp_font_override_->GetSelectedFont().GetPointSize();
	}
	else
	{
		txed_override_font      = "";
		txed_override_font_size = 0;
	}

	// Apply styleset to global current
	StyleSet::currentSet()->copySet(&ss_current_);
	StyleSet::applyCurrentToAll();
}


// -----------------------------------------------------------------------------
//
// TextStylePrefsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a style is selected in the style list
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onStyleSelected(wxCommandEvent& e)
{
	int sel = list_styles_->GetSelection();
	if (sel == 0)
		ts_current_ = ss_current_.style("default");
	else if (sel == 1)
		ts_current_ = ss_current_.style("selection");
	else
		ts_current_ = ss_current_.style(sel - 2);

	updateStyleControls();
}

// -----------------------------------------------------------------------------
// Called when the 'Override' font face checkbox is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onCBOverrideFontFace(wxCommandEvent& e)
{
	updateFontFace();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the 'Override' font size checkbox is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onCBOverrideFontSize(wxCommandEvent& e)
{
	updateFontSize();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the 'Override' font bold checkbox is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onCBOverrideFontBold(wxCommandEvent& e)
{
	updateFontBold();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the 'Override' font italic checkbox is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onCBOverrideFontItalic(wxCommandEvent& e)
{
	updateFontItalic();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the 'Override' font underlined checkbox is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onCBOverrideFontUnderlined(wxCommandEvent& e)
{
	updateFontUnderlined();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the 'Override' foreground colour checkbox is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onCBOverrideForeground(wxCommandEvent& e)
{
	updateForeground();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the 'Override' background colour checkbox is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onCBOverrideBackground(wxCommandEvent& e)
{
	updateBackground();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the font chooser font is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onFontChanged(wxFontPickerEvent& e)
{
	// Update relevant style properties
	updateFontFace();
	updateFontSize();
	updateFontBold();
	updateFontItalic();
	updateFontUnderlined();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the foreground colour is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onForegroundChanged(wxColourPickerEvent& e)
{
	updateForeground();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the background colour is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onBackgroundChanged(wxColourPickerEvent& e)
{
	updateBackground();
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the 'Save' style set button is clicked
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onBtnSaveStyleSet(wxCommandEvent& e)
{
	// Get name for set
	auto name = wxGetTextFromUser("Enter Style Set name:", "Save Style Set").ToStdString();
	if (name.empty())
		return;

	// Create temp styleset
	StyleSet ss_temp(name);
	ss_temp.copySet(&ss_current_);

	// Remove spaces from name (for filename)
	std::replace(name.begin(), name.end(), ' ', '_');

	// Write set to file
	auto filename = App::path(fmt::format("text_styles/{}.sss", name), App::Dir::User);
	ss_temp.writeFile(filename);

	// Add new set to list
	StyleSet::addSet(&ss_temp);

	// Refresh styles list
	wxArrayString style_sets;
	for (unsigned a = 0; a < StyleSet::numSets(); a++)
		style_sets.Add(StyleSet::styleName(a));
	choice_styleset_->Set(style_sets);
}

// -----------------------------------------------------------------------------
// Called when the style set selection is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onStyleSetSelected(wxCommandEvent& e)
{
	if (init_done_)
	{
		// Get selected styleset
		auto set = StyleSet::set(choice_styleset_->GetSelection());
		if (set)
		{
			ss_current_.copySet(set);
			updateStyleControls();
			updatePreview();
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Override Default Font' checkbox is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onCBOverrideFont(wxCommandEvent& e)
{
	fp_font_override_->Enable(cb_font_override_->GetValue());
	updatePreview();
}

// -----------------------------------------------------------------------------
// Called when the 'Override Default Font' font is changed
// -----------------------------------------------------------------------------
void TextStylePrefsPanel::onFontOverrideChanged(wxFontPickerEvent& e)
{
	updatePreview();
}
