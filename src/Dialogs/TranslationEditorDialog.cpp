
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TranslationEditorDialog.cpp
// Description: A dialog with various controls to setup and preview a palette
//              translation. See Translation.h/cpp
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
#include "TranslationEditorDialog.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SImage.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/Canvas/PaletteCanvas.h"
#include "Utility/MathStuff.h"
#include "Utility/Tokenizer.h"


// -----------------------------------------------------------------------------
//
// GradientBox Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GradientBox class constructor
// -----------------------------------------------------------------------------
GradientBox::GradientBox(wxWindow* parent, int steps) : OGLCanvas(parent, -1)
{
	// Init variables
	col_start_   = COL_BLACK;
	col_end_     = COL_WHITE;
	this->steps_ = steps;

	// Minimum height 16
	SetInitialSize(wxSize(-1, UI::scalePx(16)));
}

// -----------------------------------------------------------------------------
// GradientBox class destructor
// -----------------------------------------------------------------------------
GradientBox::~GradientBox() {}

// -----------------------------------------------------------------------------
// Called when the canvas needs to be redrawn
// -----------------------------------------------------------------------------
void GradientBox::draw()
{
	// Setup the viewport
	glViewport(0, 0, GetSize().x, GetSize().y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, GetSize().x, GetSize().y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw gradient
	if (steps_ < 0)
	{
		// No steps defined, draw smooth gradient
		glBegin(GL_QUADS);
		OpenGL::setColour(col_start_);
		glVertex2d(0, 0);
		glVertex2d(0, GetSize().y);
		OpenGL::setColour(col_end_);
		glVertex2d(GetSize().x, GetSize().y);
		glVertex2d(GetSize().x, 0);
		glEnd();
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


// -----------------------------------------------------------------------------
//
// TranslationEditorDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TranslationEditorDialog class constructor
// -----------------------------------------------------------------------------
TranslationEditorDialog::TranslationEditorDialog(wxWindow* parent, Palette* pal, string title, SImage* preview_image) :
	wxDialog(parent, -1, title)
{
	// Init variables
	palette_ = pal;

	// Setup preview image
	if (preview_image)
	{
		image_preview_.copyImage(preview_image);
		if (preview_image->hasPalette())
			palette_ = preview_image->getPalette();
	}

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "remap"));
	SetIcon(icon);

	// Create sizer
	wxBoxSizer*     mainsizer = new wxBoxSizer(wxVERTICAL);
	wxGridBagSizer* sizer     = new wxGridBagSizer(UI::pad(), UI::pad());
	mainsizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());
	SetSizer(mainsizer);


	// --- Top half (translation origin) ---

	// Translations list
	wxStaticBox*      frame      = new wxStaticBox(this, -1, "Translation Ranges");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	sizer->Add(framesizer, wxGBPosition(0, 0), wxDefaultSpan, wxEXPAND);

	list_translations_ = new wxListBox(this, -1);
	framesizer->Add(list_translations_, 1, wxEXPAND | wxALL, UI::pad());

	// Add translation button
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	framesizer->Add(vbox, 0, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, UI::pad());

	btn_add_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "plus"));
	vbox->Add(btn_add_, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// Remove translation button
	btn_remove_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "minus"));
	vbox->Add(btn_remove_, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// Move up button
	btn_up_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "up"));
	vbox->Add(btn_up_, 0, wxEXPAND | wxBOTTOM, UI::pad());

	// Move down button
	btn_down_ = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "down"));
	vbox->Add(btn_down_, 0, wxEXPAND);


	// Origin range
	frame      = new wxStaticBox(this, -1, "Origin Range");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Origin palette
	pal_canvas_original_ = new PaletteCanvas(this, -1);
	pal_canvas_original_->doubleWidth(true);
	pal_canvas_original_->setPalette(palette_);
	pal_canvas_original_->SetInitialSize(wxSize(UI::scalePx(448), UI::scalePx(112)));
	pal_canvas_original_->allowSelection(2);
	framesizer->Add(pal_canvas_original_->toPanel(this), 1, wxALL | wxEXPAND, UI::pad());


	// --- Bottom half (translation target) ---

	// Target type
	frame      = new wxStaticBox(this, -1, "Target Range Type");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(1, 0), wxDefaultSpan, wxEXPAND);

	// Palette range
	rb_type_palette_ = new wxRadioButton(this, -1, "Palette Range", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	framesizer->Add(rb_type_palette_, 0, wxEXPAND | wxALL, UI::pad());

	// Colour gradient
	rb_type_colour_ = new wxRadioButton(this, -1, "Colour Gradient");
	framesizer->Add(rb_type_colour_, 0, wxEXPAND | wxLEFT | wxRIGHT, UI::pad());

	// Desaturated colour gradient
	rb_type_desaturate_ = new wxRadioButton(this, -1, "Desaturated Colour Gradient");
	framesizer->Add(rb_type_desaturate_, 0, wxEXPAND | wxALL, UI::pad());

	// Colourise range
	rb_type_colourise_ = new wxRadioButton(this, -1, "Colourise Range");
	framesizer->Add(rb_type_colourise_, 0, wxEXPAND | wxLEFT | wxRIGHT, UI::pad());

	// Tint range
	rb_type_tint_ = new wxRadioButton(this, -1, "Tint Range");
	framesizer->Add(rb_type_tint_, 0, wxEXPAND | wxALL, UI::pad());

	// Target range
	frame      = new wxStaticBox(this, -1, "Target Range");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);


	// Target palette range panel
	panel_target_palette_ = new wxPanel(this, -1);
	vbox                  = new wxBoxSizer(wxVERTICAL);
	panel_target_palette_->SetSizer(vbox);

	// Target palette
	pal_canvas_target_ = new PaletteCanvas(panel_target_palette_, -1);
	pal_canvas_target_->doubleWidth(true);
	pal_canvas_target_->setPalette(palette_);
	pal_canvas_target_->SetInitialSize(wxSize(UI::scalePx(448), UI::scalePx(112)));
	pal_canvas_target_->allowSelection(2);
	vbox->Add(pal_canvas_target_->toPanel(panel_target_palette_), 1, wxEXPAND);

	// Reverse origin range
	cb_target_reverse_ = new wxCheckBox(panel_target_palette_, -1, "Reverse Selection");
	vbox->Add(cb_target_reverse_, 0, wxTOP, UI::pad());


	// Target colour gradient panel
	panel_target_gradient_ = new wxPanel(this, -1);
	vbox                   = new wxBoxSizer(wxVERTICAL);
	panel_target_gradient_->SetSizer(vbox);

	// Start colour
	vbox->AddStretchSpacer();
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());

	cb_range_begin_ = new ColourBox(panel_target_gradient_, -1, false, true);
	cb_range_begin_->setColour(COL_BLACK);
	cb_range_begin_->setPalette(pal);
	hbox->Add(cb_range_begin_, 0, wxEXPAND | wxRIGHT, UI::pad());
	hbox->Add(new wxStaticText(panel_target_gradient_, -1, "From"), 0, wxALIGN_CENTER_VERTICAL);

	// End colour
	cb_range_end_ = new ColourBox(panel_target_gradient_, -1, false, true);
	cb_range_end_->setColour(COL_WHITE);
	cb_range_end_->setPalette(pal);
	hbox->AddStretchSpacer();
	hbox->Add(new wxStaticText(panel_target_gradient_, -1, "To"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(cb_range_end_, 0, wxEXPAND);

	// Gradient preview
	gb_gradient_ = new GradientBox(panel_target_gradient_);
	vbox->Add(gb_gradient_->toPanel(panel_target_gradient_), 0, wxEXPAND);
	vbox->AddStretchSpacer();

	// Target colourise/tint panel
	panel_target_tint_ = new wxPanel(this, -1);
	vbox               = new wxBoxSizer(wxVERTICAL);
	panel_target_tint_->SetSizer(vbox);
	vbox->AddStretchSpacer();

	// Add colour chooser
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND | wxALL, UI::pad());

	cb_target_tint_ = new ColourBox(panel_target_tint_, -1, false, true);
	cb_target_tint_->setColour(COL_RED);
	cb_target_tint_->setPalette(pal);
	hbox->Add(cb_target_tint_, 0, wxEXPAND | wxRIGHT, UI::pad());
	hbox->Add(new wxStaticText(panel_target_tint_, -1, "Colour"), 1, wxALIGN_CENTER_VERTICAL);

	// Add 'amount' slider
	slider_tint_  = new wxSlider(panel_target_tint_, -1, 50, 0, 100);
	label_tint_   = new wxStaticText(panel_target_tint_, -1, "50%");
	label_amount_ = new wxStaticText(panel_target_tint_, -1, "Amount");
	hbox->Add(label_amount_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(slider_tint_, 3, wxEXPAND | wxRIGHT, UI::pad());
	hbox->Add(label_tint_, 0, wxALIGN_CENTER_VERTICAL);

	// Show initial target panel (palette)
	framesizer->Add(panel_target_palette_, 1, wxEXPAND | wxALL, UI::pad());
	panel_target_gradient_->Show(false);
	panel_target_tint_->Show(false);

	// --- Preview section ---
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(2, 0), wxGBSpan(1, 2), wxEXPAND);

	// Palette preview
	frame      = new wxStaticBox(this, -1, "Resulting Palette");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 0, wxEXPAND | wxRIGHT, UI::pad());

	pal_canvas_preview_ = new PaletteCanvas(this, -1);
	pal_canvas_preview_->SetInitialSize(wxSize(UI::scalePx(224), UI::scalePx(224)));
	pal_canvas_preview_->setPalette(palette_);
	framesizer->Add(pal_canvas_preview_->toPanel(this), 1, wxEXPAND | wxALL, UI::pad());

	// Image preview
	frame      = new wxStaticBox(this, -1, "Preview");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 1, wxEXPAND);

	gfx_preview_ = new GfxCanvas(this, -1);
	gfx_preview_->setPalette(palette_);
	gfx_preview_->setViewType(GFXVIEW_CENTERED);
	gfx_preview_->getImage()->copyImage(&image_preview_);
	framesizer->Add(gfx_preview_->toPanel(this), 1, wxEXPAND | wxALL, UI::pad());


	// --- Translation string ---
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(3, 0), wxGBSpan(1, 2), wxEXPAND);

	text_string_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	hbox->Add(new wxStaticText(this, -1, "Translation String:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(text_string_, 1, wxEXPAND);


	// --- Dialog buttons ---
	wxSizer* buttonsizer = CreateButtonSizer(wxOK | wxCANCEL);
	sizer->Add(buttonsizer, wxGBPosition(4, 0), wxGBSpan(1, 2), wxEXPAND);

	// Load button
	btn_load_ = new wxButton(this, -1, "Load from File");
	buttonsizer->InsertStretchSpacer(0);
	buttonsizer->Insert(0, btn_load_, 0, wxRIGHT, UI::pad());

	// Save button
	btn_save_ = new wxButton(this, -1, "Save to File");
	buttonsizer->Insert(1, btn_save_, 0, wxRIGHT, UI::pad());

	// Truecolor checkbox
	cb_truecolor_ = new wxCheckBox(this, -1, "Truecolor");
	buttonsizer->Insert(2, cb_truecolor_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());

	// Palette translation only
	cb_paletteonly_ = new wxCheckBox(this, -1, "Palette Translation Only");
	buttonsizer->Insert(3, cb_paletteonly_, 0, wxALIGN_CENTER_VERTICAL);

	// Bind events
	Bind(wxEVT_SIZE, &TranslationEditorDialog::onSize, this);
	list_translations_->Bind(wxEVT_LISTBOX, &TranslationEditorDialog::onTranslationListItemSelected, this);
	rb_type_palette_->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBPaletteSelected, this);
	rb_type_colour_->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBColourSelected, this);
	rb_type_desaturate_->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBDesaturateSelected, this);
	rb_type_colourise_->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBColouriseSelected, this);
	rb_type_tint_->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBTintSelected, this);
	cb_range_begin_->Bind(wxEVT_COLOURBOX_CHANGED, &TranslationEditorDialog::onBeginColourChanged, this);
	cb_range_end_->Bind(wxEVT_COLOURBOX_CHANGED, &TranslationEditorDialog::onEndColourChanged, this);
	cb_target_tint_->Bind(wxEVT_COLOURBOX_CHANGED, &TranslationEditorDialog::onTintColourChanged, this);
	slider_tint_->Bind(wxEVT_SLIDER, &TranslationEditorDialog::onTintAmountChanged, this);
	pal_canvas_original_->Bind(wxEVT_LEFT_UP, &TranslationEditorDialog::onPalOriginLeftUp, this);
	pal_canvas_target_->Bind(wxEVT_LEFT_UP, &TranslationEditorDialog::onPalTargetLeftUp, this);
	btn_add_->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnAdd, this);
	btn_remove_->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnRemove, this);
	btn_up_->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnUp, this);
	btn_down_->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnDown, this);
	btn_load_->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnLoad, this);
	btn_save_->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnSave, this);
	gfx_preview_->Bind(wxEVT_MOTION, &TranslationEditorDialog::onGfxPreviewMouseMotion, this);
	cb_target_reverse_->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBTargetReverse, this);
	cb_truecolor_->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBTruecolor, this);
	cb_paletteonly_->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBPaletteOnly, this);

	// Setup layout
	Layout();
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
	CenterOnParent();
	list_translations_->SetSizeHints(list_translations_->GetSize(), list_translations_->GetSize());
}

// -----------------------------------------------------------------------------
// TranslationEditorDialog class destructor
// -----------------------------------------------------------------------------
TranslationEditorDialog::~TranslationEditorDialog() {}

// -----------------------------------------------------------------------------
// Opens the translation [trans] for preview/modification
// -----------------------------------------------------------------------------
void TranslationEditorDialog::openTranslation(Translation& trans)
{
	// Read translation
	translation_.copy(trans);

	// Update translation list
	list_translations_->Clear();
	for (unsigned a = 0; a < translation_.nRanges(); a++)
		list_translations_->Append(translation_.getRange(a)->asText());

	// Select+open first range if it exists
	if (list_translations_->GetCount() > 0)
	{
		list_translations_->SetSelection(0);
		openRange(0);
	}

	// Update previews
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Opens the translation range [index] from the current translation
// -----------------------------------------------------------------------------
void TranslationEditorDialog::openRange(int index)
{
	// Check index
	if (index < 0 || index >= (int)translation_.nRanges())
		return;

	// Get translation range to open
	TransRange* tr = translation_.getRange(index);

	// Set origin selection
	pal_canvas_original_->setSelection(tr->oStart(), tr->oEnd());
	pal_canvas_original_->Refresh();

	// Check translation range type
	if (tr->getType() == TRANS_PALETTE)
	{
		// Palette range
		TransRangePalette* tpr = (TransRangePalette*)tr;

		// Select palette type radiobox
		rb_type_palette_->SetValue(true);
		showPaletteTarget();

		// Set target range selection
		if (tpr->dStart() <= tpr->dEnd())
		{
			pal_canvas_target_->setSelection(tpr->dStart(), tpr->dEnd());
			cb_target_reverse_->SetValue(false);
		}
		else
		{
			pal_canvas_target_->setSelection(tpr->dEnd(), tpr->dStart());
			cb_target_reverse_->SetValue(true);
		}
		pal_canvas_target_->Refresh();
	}
	else if (tr->getType() == TRANS_COLOUR)
	{
		// Colour gradient
		TransRangeColour* tcr = (TransRangeColour*)tr;

		// Select colour type radiobox
		rb_type_colour_->SetValue(true);
		showGradientTarget();

		// Set beginning colour
		gb_gradient_->setStartCol(tcr->dStart());
		cb_range_begin_->setColour(tcr->dStart());

		// Set ending colour
		gb_gradient_->setEndCol(tcr->dEnd());
		cb_range_end_->setColour(tcr->dEnd());

		// Update UI
		gb_gradient_->Refresh();
	}
	else if (tr->getType() == TRANS_DESAT)
	{
		// Desaturated colour gradient
		TransRangeDesat* tdr = (TransRangeDesat*)tr;

		// Select desaturated colour type radiobox
		rb_type_desaturate_->SetValue(true);
		showGradientTarget();

		// Set beginning colour
		rgba_t col;
		col.r = MathStuff::clamp(tdr->dSr() * 128, 0, 255);
		col.g = MathStuff::clamp(tdr->dSg() * 128, 0, 255);
		col.b = MathStuff::clamp(tdr->dSb() * 128, 0, 255);
		cb_range_begin_->setColour(col);
		gb_gradient_->setStartCol(col);

		// Set ending colour
		col.r = MathStuff::clamp(tdr->dEr() * 128, 0, 255);
		col.g = MathStuff::clamp(tdr->dEg() * 128, 0, 255);
		col.b = MathStuff::clamp(tdr->dEb() * 128, 0, 255);
		cb_range_end_->setColour(col);
		gb_gradient_->setEndCol(col);

		// Update UI
		gb_gradient_->Refresh();
	}
	else if (tr->getType() == TRANS_BLEND)
	{
		// Colourise range
		TransRangeBlend* tcr = (TransRangeBlend*)tr;

		// Select colourise type radiobox
		rb_type_colourise_->SetValue(true);
		showTintTarget(false);

		// Set colour
		cb_target_tint_->setColour(tcr->getColour());
	}
	else if (tr->getType() == TRANS_TINT)
	{
		// Tint range
		TransRangeTint* ttr = (TransRangeTint*)tr;

		// Select colourise type radiobox
		rb_type_tint_->SetValue(true);
		showTintTarget(true);

		// Set colour
		cb_target_tint_->setColour(ttr->getColour());

		// Set amount
		slider_tint_->SetValue(ttr->getAmount());
		label_tint_->SetLabel(S_FMT("%d%% ", ttr->getAmount()));
	}
}

// -----------------------------------------------------------------------------
// Updates the translation range [index] in the list
// -----------------------------------------------------------------------------
void TranslationEditorDialog::updateListItem(int index)
{
	// Check index
	if (index < 0 || index >= (int)list_translations_->GetCount())
		return;

	// Get translation range
	TransRange* tr = translation_.getRange(index);

	// Update list item text
	if (tr)
	{
		list_translations_->Delete(index);
		list_translations_->Insert(tr->asText(), index);
		list_translations_->SetSelection(index);
	}
}

// -----------------------------------------------------------------------------
// Sets the current translation range's destination starting colour to [col]
// -----------------------------------------------------------------------------
void TranslationEditorDialog::setStartColour(rgba_t col)
{
	// Get currently selected translation range
	TransRange* tr = translation_.getRange(list_translations_->GetSelection());

	// Check its type
	if (tr->getType() == TRANS_COLOUR)
	{
		// Colour range
		TransRangeColour* tcr = (TransRangeColour*)tr;

		// Set destination start colour
		tcr->setDStart(col);
	}
	else if (tr->getType() == TRANS_DESAT)
	{
		// Desaturated colour range
		TransRangeDesat* tdr = (TransRangeDesat*)tr;

		// Determine start colour
		float sr = MathStuff::clamp(col.r / 127.0f, 0, 2);
		float sg = MathStuff::clamp(col.g / 127.0f, 0, 2);
		float sb = MathStuff::clamp(col.b / 127.0f, 0, 2);

		// Set destination start colour
		tdr->setDStart(sr, sg, sb);
	}

	// Set gradient box end colour
	gb_gradient_->setStartCol(col);

	// Update UI
	gb_gradient_->Refresh();
	updateListItem(list_translations_->GetSelection());
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Sets the current translation range's destination ending colour to [col]
// -----------------------------------------------------------------------------
void TranslationEditorDialog::setEndColour(rgba_t col)
{
	// Get currently selected translation range
	TransRange* tr = translation_.getRange(list_translations_->GetSelection());

	// Check its type
	if (tr->getType() == TRANS_COLOUR)
	{
		// Colour range
		TransRangeColour* tcr = (TransRangeColour*)tr;

		// Set destination end colour
		tcr->setDEnd(col);
	}
	else if (tr->getType() == TRANS_DESAT)
	{
		// Desaturated colour range
		TransRangeDesat* tdr = (TransRangeDesat*)tr;

		// Determine end colour
		float er = MathStuff::clamp(col.r / 127.0f, 0, 2);
		float eg = MathStuff::clamp(col.g / 127.0f, 0, 2);
		float eb = MathStuff::clamp(col.b / 127.0f, 0, 2);

		// Set destination end colour
		tdr->setDEnd(er, eg, eb);
	}

	// Set gradient box end colour
	gb_gradient_->setEndCol(col);

	// Update UI
	gb_gradient_->Refresh();
	updateListItem(list_translations_->GetSelection());
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Sets the current translation range's tint colour to [col]
// -----------------------------------------------------------------------------
void TranslationEditorDialog::setTintColour(rgba_t col)
{
	// Get currently selected translation range
	TransRange* tr = translation_.getRange(list_translations_->GetSelection());

	// Check its type
	if (tr->getType() == TRANS_BLEND)
	{
		// Colour range
		TransRangeBlend* tcr = (TransRangeBlend*)tr;

		// Set destination end colour
		tcr->setColour(col);
	}
	else if (tr->getType() == TRANS_TINT)
	{
		// Desaturated colour range
		TransRangeTint* ttr = (TransRangeTint*)tr;

		// Set destination end colour
		ttr->setColour(col);
	}

	// Update UI
	updateListItem(list_translations_->GetSelection());
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Sets the current translation range's tint colour to [col]
// -----------------------------------------------------------------------------
void TranslationEditorDialog::setTintAmount(int amount)
{
	// Get currently selected translation range
	TransRange* tr = translation_.getRange(list_translations_->GetSelection());

	// Check its type
	if (tr->getType() == TRANS_TINT)

	{
		// Desaturated colour range
		TransRangeTint* ttr = (TransRangeTint*)tr;

		// Set destination end colour
		ttr->setAmount(amount);
	}

	// Update UI
	updateListItem(list_translations_->GetSelection());
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Shows the palette range translation target controls
// -----------------------------------------------------------------------------
void TranslationEditorDialog::showPaletteTarget()
{
	// Nothing to do
	if (panel_target_palette_->IsShown())
		return;

	// Swap gradient panel for palette panel
	if (panel_target_gradient_->IsShown())
	{
		panel_target_gradient_->Show(false);
		GetSizer()->Replace(panel_target_gradient_, panel_target_palette_, true);
	}
	// Swap tint panel for palette panel
	if (panel_target_tint_->IsShown())
	{
		panel_target_tint_->Show(false);
		GetSizer()->Replace(panel_target_tint_, panel_target_palette_, true);
	}

	// Update UI
	panel_target_palette_->Show(true);
	Layout();
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
}

// -----------------------------------------------------------------------------
// Shows the colour gradient translation target controls
// -----------------------------------------------------------------------------
void TranslationEditorDialog::showGradientTarget()
{
	// Nothing to do
	if (panel_target_gradient_->IsShown())
		return;

	// Swap palette panel for gradient panel
	if (panel_target_palette_->IsShown())
	{
		panel_target_palette_->Show(false);
		GetSizer()->Replace(panel_target_palette_, panel_target_gradient_, true);
	}
	// Swap tint panel for gradient panel
	else if (panel_target_tint_->IsShown())
	{
		panel_target_tint_->Show(false);
		GetSizer()->Replace(panel_target_tint_, panel_target_gradient_, true);
	}

	// Update UI
	panel_target_gradient_->Show(true);
	Layout();
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
}

// -----------------------------------------------------------------------------
//* Shows the colourise/tint translation target controls
// -----------------------------------------------------------------------------
void TranslationEditorDialog::showTintTarget(bool tint)
{
	// Update slider status
	slider_tint_->Enable(tint);
	label_tint_->Enable(tint);
	label_amount_->Enable(tint);

	// Nothing further to do
	if (panel_target_tint_->IsShown())
		return;

	// Swap palette panel for tint panel
	if (panel_target_palette_->IsShown())
	{
		panel_target_palette_->Show(false);
		GetSizer()->Replace(panel_target_palette_, panel_target_tint_, true);
	}
	// Swap gradient panel for tint panel
	else if (panel_target_gradient_->IsShown())
	{
		panel_target_gradient_->Show(false);
		GetSizer()->Replace(panel_target_gradient_, panel_target_tint_, true);
	}

	// Update UI
	panel_target_tint_->Show(true);
	Layout();
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
}

// -----------------------------------------------------------------------------
// Updates the image and resulting palette previews according to the current
// translation
// -----------------------------------------------------------------------------
void TranslationEditorDialog::updatePreviews()
{
	// Update palette preview
	pal_canvas_preview_->setPalette(palette_);
	pal_canvas_preview_->getPalette().applyTranslation(&translation_);
	pal_canvas_preview_->Refresh();

	// Update image preview
	gfx_preview_->getImage()->copyImage(&image_preview_);
	gfx_preview_->getImage()->applyTranslation(&translation_, palette_, cb_truecolor_->GetValue());

	// Update UI
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();

	// Update text string
	if (cb_paletteonly_->GetValue())
	{
		// Create a palette image
		SImage img(PALMASK);
		img.create(256, 1, PALMASK, palette_);
		for (int i = 0; i < 256; ++i)
			img.setPixel(i, 0, i);
		// Apply translation to image
		img.applyTranslation(&translation_, palette_);
		// Create new translation from image data
		Translation newtrans;
		MemChunk    mc;
		if (img.getIndexedData(mc))
		{
			newtrans.read(mc.getData());
			text_string_->SetValue(newtrans.asText());
		}
	}
	else
		text_string_->SetValue(translation_.asText());
}

// -----------------------------------------------------------------------------
// Returns whether the truecolor checkbox is checked
// -----------------------------------------------------------------------------
bool TranslationEditorDialog::getTruecolor()
{
	return cb_truecolor_->GetValue();
}


// -----------------------------------------------------------------------------
//
// TranslationEditorDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onSize(wxSizeEvent& e)
{
	// Update image preview
	gfx_preview_->zoomToFit(true, 0.05f);

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when a translation range list item is selected
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onTranslationListItemSelected(wxCommandEvent& e)
{
	// Open what was selected
	openRange(e.GetInt());
}

// -----------------------------------------------------------------------------
// Called when the 'palette range' translation type radio button is selected
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onRBPaletteSelected(wxCommandEvent& e)
{
	// Swap to target palette panel
	showPaletteTarget();

	// If a range is selected
	int index = list_translations_->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation_.removeRange(index);

		// Recreate it
		translation_.addRange(TRANS_PALETTE, index);
		TransRangePalette* tr = (TransRangePalette*)translation_.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original_->getSelectionStart());
		tr->setOEnd(pal_canvas_original_->getSelectionEnd());
		// Target range
		if (cb_target_reverse_->GetValue())
		{
			tr->setDEnd(pal_canvas_target_->getSelectionStart());
			tr->setDStart(pal_canvas_target_->getSelectionEnd());
		}
		else
		{
			tr->setDStart(pal_canvas_target_->getSelectionStart());
			tr->setDEnd(pal_canvas_target_->getSelectionEnd());
		}

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}

// -----------------------------------------------------------------------------
// Called when the 'colour gradient' translation type radio button is selected
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onRBColourSelected(wxCommandEvent& e)
{
	// Swap to target colour panel
	showGradientTarget();

	// If a range is selected
	int index = list_translations_->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation_.removeRange(index);

		// Recreate it
		translation_.addRange(TRANS_COLOUR, index);
		TransRangeColour* tr = (TransRangeColour*)translation_.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original_->getSelectionStart());
		tr->setOEnd(pal_canvas_original_->getSelectionEnd());
		// Target colour gradient
		tr->setDStart(cb_range_begin_->colour());
		tr->setDEnd(cb_range_end_->colour());

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}

// -----------------------------------------------------------------------------
// Called when the 'desaturated colour gradient' translation type radio button
// is selected
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onRBDesaturateSelected(wxCommandEvent& e)
{
	// Swap to target colour panel
	showGradientTarget();

	// If a range is selected
	int index = list_translations_->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation_.removeRange(index);

		// Recreate it
		translation_.addRange(TRANS_DESAT, index);
		TransRangeDesat* tr = (TransRangeDesat*)translation_.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original_->getSelectionStart());
		tr->setOEnd(pal_canvas_original_->getSelectionEnd());
		// Target colour gradient
		rgba_t sc = cb_range_begin_->colour();
		rgba_t ec = cb_range_end_->colour();
		tr->setDStart(
			MathStuff::clamp((double)sc.r / 127.0f, 0, 2),
			MathStuff::clamp((double)sc.g / 127.0f, 0, 2),
			MathStuff::clamp((double)sc.b / 127.0f, 0, 2));
		tr->setDEnd(
			MathStuff::clamp((double)ec.r / 127.0f, 0, 2),
			MathStuff::clamp((double)ec.g / 127.0f, 0, 2),
			MathStuff::clamp((double)ec.b / 127.0f, 0, 2));

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}

// -----------------------------------------------------------------------------
// Called when the 'colourise' translation type radio button is selected
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onRBColouriseSelected(wxCommandEvent& e)
{
	showTintTarget(false);

	// If a range is selected
	int index = list_translations_->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation_.removeRange(index);

		// Recreate it
		translation_.addRange(TRANS_BLEND, index);
		TransRangeBlend* tr = (TransRangeBlend*)translation_.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original_->getSelectionStart());
		tr->setOEnd(pal_canvas_original_->getSelectionEnd());
		// Target colour
		tr->setColour(cb_target_tint_->colour());

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}


// -----------------------------------------------------------------------------
// Called when the 'tint' translation type radio button is selected
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onRBTintSelected(wxCommandEvent& e)
{
	showTintTarget(true);

	// If a range is selected
	int index = list_translations_->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation_.removeRange(index);

		// Recreate it
		translation_.addRange(TRANS_TINT, index);
		TransRangeTint* tr = (TransRangeTint*)translation_.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original_->getSelectionStart());
		tr->setOEnd(pal_canvas_original_->getSelectionEnd());
		// Target colour
		tr->setColour(cb_target_tint_->colour());
		tr->setAmount(slider_tint_->GetValue());

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}

// -----------------------------------------------------------------------------
// Called when the target gradient start colour is changed
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onBeginColourChanged(wxEvent& e)
{
	// Set start colour to selected colour
	setStartColour(cb_range_begin_->colour());
}

// -----------------------------------------------------------------------------
// Called when the target gradient end colour is changed
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onEndColourChanged(wxEvent& e)
{
	// Set end colour to selected colour
	setEndColour(cb_range_end_->colour());
}

// -----------------------------------------------------------------------------
// Called when the target colourise/tint colour is changed
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onTintColourChanged(wxEvent& e)
{
	setTintColour(cb_target_tint_->colour());
}

// -----------------------------------------------------------------------------
// Called when the tint amount slider is changed
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onTintAmountChanged(wxCommandEvent& e)
{
	setTintAmount(slider_tint_->GetValue());
	label_tint_->SetLabel(S_FMT("%d%% ", slider_tint_->GetValue()));
}

// -----------------------------------------------------------------------------
// Called when the left mouse button is released in the origin palette range
// canvas
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onPalOriginLeftUp(wxMouseEvent& e)
{
	// Get current translation range
	TransRange* tr = translation_.getRange(list_translations_->GetSelection());

	// Update its origin range
	if (tr)
	{
		tr->setOStart(pal_canvas_original_->getSelectionStart());
		tr->setOEnd(pal_canvas_original_->getSelectionEnd());
	}

	// Update UI
	updateListItem(list_translations_->GetSelection());
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Called when the left mouse button is released in the target palette range
// canvas
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onPalTargetLeftUp(wxMouseEvent& e)
{
	// Get current translation range
	TransRange* tr = translation_.getRange(list_translations_->GetSelection());

	// Update its target range if it's a palette translation
	if (tr && tr->getType() == TRANS_PALETTE)
	{
		TransRangePalette* tpr = (TransRangePalette*)tr;
		if (cb_target_reverse_->GetValue())
		{
			tpr->setDEnd(pal_canvas_target_->getSelectionStart());
			tpr->setDStart(pal_canvas_target_->getSelectionEnd());
		}
		else
		{
			tpr->setDStart(pal_canvas_target_->getSelectionStart());
			tpr->setDEnd(pal_canvas_target_->getSelectionEnd());
		}
	}

	// Update UI
	updateListItem(list_translations_->GetSelection());
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Called when the 'Add Translation' button is clicked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onBtnAdd(wxCommandEvent& e)
{
	// Get index to add at
	int index = list_translations_->GetSelection() + 1;

	// Add new range to translation depending on current type selection
	if (rb_type_palette_->GetValue())
		translation_.addRange(TRANS_PALETTE, index);
	else if (rb_type_colour_->GetValue())
		translation_.addRange(TRANS_COLOUR, index);
	else if (rb_type_desaturate_->GetValue())
		translation_.addRange(TRANS_DESAT, index);
	else if (rb_type_colourise_->GetValue())
		translation_.addRange(TRANS_BLEND, index);
	else if (rb_type_tint_->GetValue())
		translation_.addRange(TRANS_TINT, index);

	// Add it to the list
	list_translations_->Insert(translation_.getRange(index)->asText(), index);

	// Update UI
	list_translations_->SetSelection(index);
	openRange(index);
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Called when the 'Remove Translation' button is clicked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onBtnRemove(wxCommandEvent& e)
{
	// Do nothing if there is only one translation
	if (translation_.nRanges() == 1)
		return;

	// Get index of range to remove
	int index = list_translations_->GetSelection();

	// Remove it from the translation
	translation_.removeRange(index);

	// Remove it from the list
	list_translations_->Delete(index);

	// Update UI
	if (index >= (int)list_translations_->GetCount())
		index--;
	if (index >= 0)
	{
		list_translations_->SetSelection(index);
		openRange(index);
	}
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Called when the 'Move Up' button is clicked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onBtnUp(wxCommandEvent& e)
{
	// Get selection
	int index = list_translations_->GetSelection();

	// Check selection is valid
	if (index <= 0)
		return;

	// Swap item with above
	translation_.swapRanges(index, index - 1);
	updateListItem(index);
	updateListItem(index - 1);

	// Update selection
	list_translations_->SetSelection(index - 1);
	openRange(index - 1);

	// Update previews
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Called when the 'Move Down' button is clicked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onBtnDown(wxCommandEvent& e)
{
	// Get selection
	int index = list_translations_->GetSelection();

	// Check selection is valid
	if (index >= (int)translation_.nRanges() - 1)
		return;

	// Swap item with above
	translation_.swapRanges(index, index + 1);
	updateListItem(index);
	updateListItem(index + 1);

	// Update selection
	list_translations_->SetSelection(index + 1);
	openRange(index + 1);

	// Update previews
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Called when the 'Load Translation' button is clicked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onBtnLoad(wxCommandEvent& e)
{
	// Get user directory
	string dir = App::path("translations", App::Dir::User);

	// Create open file dialog
	wxFileDialog dialog_open(
		this,
		"Load Translation from File",
		dir,
		wxEmptyString,
		"Text Files (*.txt)|*.txt",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get the selected filename
		string filename = dialog_open.GetPath();

		// Load file to string
		wxFile file;
		string tstring;
		if (file.Open(dialog_open.GetPath()))
			file.ReadAll(&tstring);

		// Parse as a translation
		Translation trans;
		trans.parse(tstring);

		// Open it if parsed ok
		if (trans.nRanges() > 0)
			openTranslation(trans);
		else
			wxMessageBox("Not a valid translation file", "Error", wxICON_ERROR);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Save Translation' button is clicked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onBtnSave(wxCommandEvent& e)
{
	// If the directory doesn't exist create it
	string dir = App::path("translations", App::Dir::User);
	if (!wxDirExists(dir))
		wxMkdir(dir);

	// Create save file dialog
	wxFileDialog dialog_save(
		this,
		"Save Translation to File",
		dir,
		wxEmptyString,
		"Text File (*.txt)|*.txt",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_save.ShowModal() == wxID_OK)
	{
		// Get translation as text string
		string str = translation_.asText();

		// Open file for writing
		wxFile file(dialog_save.GetPath(), wxFile::write);

		// Write string to file
		file.Write(str);

		// Close file
		file.Close();
	}
}

// -----------------------------------------------------------------------------
// Called when the mouse pointer is moved over the gfx preview canvas
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onGfxPreviewMouseMotion(wxMouseEvent& e)
{
	// Get the image coordinates at the mouse pointer
	point2_t pos = gfx_preview_->imageCoords(e.GetX(), e.GetY() - 2);

	int index = pal_canvas_preview_->getSelectionStart();

	// Get palette index at position
	if (pos.x >= 0)
		index = gfx_preview_->getImage()->getPixelIndex(pos.x, pos.y);
	else
		index = -1;

	// Update preview palette if necessary
	if (index != pal_canvas_preview_->getSelectionStart())
	{
		pal_canvas_preview_->setSelection(index);
		pal_canvas_preview_->Refresh();
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the 'Reverse Target Range' checkbox is (un)checked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onCBTargetReverse(wxCommandEvent& e)
{
	// Get current translation range
	TransRange* tr = translation_.getRange(list_translations_->GetSelection());

	// Update its target range if it's a palette translation
	if (tr && tr->getType() == TRANS_PALETTE)
	{
		TransRangePalette* tpr = (TransRangePalette*)tr;
		if (cb_target_reverse_->GetValue())
		{
			tpr->setDEnd(pal_canvas_target_->getSelectionStart());
			tpr->setDStart(pal_canvas_target_->getSelectionEnd());
		}
		else
		{
			tpr->setDStart(pal_canvas_target_->getSelectionStart());
			tpr->setDEnd(pal_canvas_target_->getSelectionEnd());
		}
	}

	// Update UI
	updateListItem(list_translations_->GetSelection());
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Called when the 'Truecolor' checkbox is (un)checked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onCBTruecolor(wxCommandEvent& e)
{
	updatePreviews();
}

// -----------------------------------------------------------------------------
// Called when the 'Palette translation ony' checkbox is (un)checked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onCBPaletteOnly(wxCommandEvent& e)
{
	updatePreviews();
}


// -----------------------------------------------------------------------------
//
// GfxColouriseDialog Class Functions
//
// -----------------------------------------------------------------------------


GfxColouriseDialog::GfxColouriseDialog(wxWindow* parent, ArchiveEntry* entry, Palette* pal) :
	wxDialog(parent, -1, "Colourise", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// Init variables
	this->entry_   = entry;
	this->palette_ = pal;

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "colourise"));
	SetIcon(icon);

	// Setup main sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, 1, wxEXPAND | wxALL, 6);

	// Add colour chooser
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxALL, 4);

	cb_colour_ = new ColourBox(this, -1, false, true);
	cb_colour_->setColour(COL_RED);
	cb_colour_->setPalette(pal);
	hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(cb_colour_, 0, wxEXPAND);

	// Add preview
	gfx_preview_ = new GfxCanvas(this, -1);
	sizer->Add(gfx_preview_, 1, wxEXPAND | wxALL, 4);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxBOTTOM, 4);

	// Setup preview
	gfx_preview_->setViewType(GFXVIEW_CENTERED);
	gfx_preview_->setPalette(pal);
	gfx_preview_->SetInitialSize(wxSize(192, 192));
	Misc::loadImageFromEntry(gfx_preview_->getImage(), entry);
	rgba_t col = cb_colour_->colour();
	gfx_preview_->getImage()->colourise(col, pal);
	gfx_preview_->updateImageTexture();

	// Init layout
	Layout();

	// Bind events
	cb_colour_->Bind(wxEVT_COLOURBOX_CHANGED, &GfxColouriseDialog::onColourChanged, this);
	Bind(wxEVT_SIZE, &GfxColouriseDialog::onResize, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
	CenterOnParent();
}

rgba_t GfxColouriseDialog::getColour()
{
	return cb_colour_->colour();
}

void GfxColouriseDialog::setColour(string col)
{
	wxColour colour(col);
	rgba_t   rgba = rgba_t(COLWX(colour));
	cb_colour_->setColour(rgba);
	gfx_preview_->getImage()->colourise(rgba, palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
}

// Events
void GfxColouriseDialog::onColourChanged(wxEvent& e)
{
	Misc::loadImageFromEntry(gfx_preview_->getImage(), entry_);
	gfx_preview_->getImage()->colourise(cb_colour_->colour(), palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
}

void GfxColouriseDialog::onResize(wxSizeEvent& e)
{
	wxDialog::OnSize(e);
	gfx_preview_->zoomToFit(true, 0.05f);
	e.Skip();
}


// -----------------------------------------------------------------------------
//
// GfxTintDialog Class Functions
//
// -----------------------------------------------------------------------------


GfxTintDialog::GfxTintDialog(wxWindow* parent, ArchiveEntry* entry, Palette* pal) :
	wxDialog(parent, -1, "Tint", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// Init variables
	this->entry_   = entry;
	this->palette_ = pal;

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "tint"));
	SetIcon(icon);

	// Setup main sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, 1, wxEXPAND | wxALL, 6);

	// Add colour chooser
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxALL, 4);

	cb_colour_ = new ColourBox(this, -1, false, true);
	cb_colour_->setColour(COL_RED);
	cb_colour_->setPalette(pal);
	hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(cb_colour_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	// Add 'amount' slider
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

	slider_amount_ = new wxSlider(this, -1, 50, 0, 100);
	label_amount_  = new wxStaticText(this, -1, "100%");
	hbox->Add(new wxStaticText(this, -1, "Amount:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	hbox->Add(slider_amount_, 1, wxEXPAND | wxRIGHT, 4);
	hbox->Add(label_amount_, 0, wxALIGN_CENTER_VERTICAL);

	// Add preview
	gfx_preview_ = new GfxCanvas(this, -1);
	sizer->Add(gfx_preview_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxBOTTOM, 4);

	// Setup preview
	gfx_preview_->setViewType(GFXVIEW_CENTERED);
	gfx_preview_->setPalette(pal);
	gfx_preview_->SetInitialSize(wxSize(256, 256));
	Misc::loadImageFromEntry(gfx_preview_->getImage(), entry);
	gfx_preview_->getImage()->tint(getColour(), getAmount(), pal);
	gfx_preview_->updateImageTexture();

	// Init layout
	Layout();

	// Bind events
	cb_colour_->Bind(wxEVT_COLOURBOX_CHANGED, &GfxTintDialog::onColourChanged, this);
	slider_amount_->Bind(wxEVT_SLIDER, &GfxTintDialog::onAmountChanged, this);
	Bind(wxEVT_SIZE, &GfxTintDialog::onResize, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
	CenterOnParent();

	// Set values
	label_amount_->SetLabel("50% ");
}

rgba_t GfxTintDialog::getColour()
{
	return rgba_t(cb_colour_->colour());
}

float GfxTintDialog::getAmount()
{
	return (float)slider_amount_->GetValue() * 0.01f;
}

void GfxTintDialog::setValues(string col, int val)
{
	wxColour colour(col);
	cb_colour_->setColour(rgba_t(COLWX(colour)));
	slider_amount_->SetValue(val);
	label_amount_->SetLabel(S_FMT("%d%% ", slider_amount_->GetValue()));
	gfx_preview_->getImage()->tint(getColour(), getAmount(), palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
}

// Events
void GfxTintDialog::onColourChanged(wxEvent& e)
{
	Misc::loadImageFromEntry(gfx_preview_->getImage(), entry_);
	gfx_preview_->getImage()->tint(getColour(), getAmount(), palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
}

void GfxTintDialog::onAmountChanged(wxCommandEvent& e)
{
	Misc::loadImageFromEntry(gfx_preview_->getImage(), entry_);
	gfx_preview_->getImage()->tint(getColour(), getAmount(), palette_);
	gfx_preview_->updateImageTexture();
	gfx_preview_->Refresh();
	label_amount_->SetLabel(S_FMT("%d%% ", slider_amount_->GetValue()));
}

void GfxTintDialog::onResize(wxSizeEvent& e)
{
	wxDialog::OnSize(e);
	gfx_preview_->zoomToFit(true, 0.05f);
	e.Skip();
}
