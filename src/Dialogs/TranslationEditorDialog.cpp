
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TranslationEditorDialog.cpp
 * Description: A dialog with various controls to setup and preview
 *              a palette translation. See Translation.h/cpp
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
#include "TranslationEditorDialog.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "Graphics/Icons.h"
#include "Graphics/SImage/SImage.h"
#include "UI/Canvas/GfxCanvas.h"
#include "UI/Canvas/PaletteCanvas.h"
#include "Utility/MathStuff.h"
#include "Utility/Tokenizer.h"


/*******************************************************************
 * GRADIENTBOX CLASS FUNCTIONS
 *******************************************************************/

/* GradientBox::GradientBox
 * GradientBox class constructor
 *******************************************************************/
GradientBox::GradientBox(wxWindow* parent, int steps) : OGLCanvas(parent, -1)
{
	// Init variables
	col_start = COL_BLACK;
	col_end = COL_WHITE;
	this->steps = steps;

	// Minimum height 16
	SetInitialSize(wxSize(-1, 16));
}

/* GradientBox::~GradientBox
 * GradientBox class destructor
 *******************************************************************/
GradientBox::~GradientBox()
{
}

/* GradientBox::draw
 * Called when the canvas needs to be redrawn
 *******************************************************************/
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
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (OpenGL::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw gradient
	if (steps < 0)
	{
		// No steps defined, draw smooth gradient
		glBegin(GL_QUADS);
		OpenGL::setColour(col_start);
		glVertex2d(0, 0);
		glVertex2d(0, GetSize().y);
		OpenGL::setColour(col_end);
		glVertex2d(GetSize().x, GetSize().y);
		glVertex2d(GetSize().x, 0);
		glEnd();
	}

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}


/*******************************************************************
 * TRANSLATIONEDITORDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* TranslationEditorDialog::TranslationEditorDialog
 * TranslationEditorDialog class constructor
 *******************************************************************/
TranslationEditorDialog::TranslationEditorDialog(wxWindow* parent, Palette8bit* pal, string title, SImage* preview_image)
	: wxDialog(parent, -1, title)
{
	// Init variables
	palette = pal;

	// Setup preview image
	if (preview_image)
	{
		image_preview.copyImage(preview_image);
		if (preview_image->hasPalette())
			palette = preview_image->getPalette();
	}

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "remap"));
	SetIcon(icon);

	// Create sizer
	wxBoxSizer* mainsizer = new wxBoxSizer(wxVERTICAL);
	wxGridBagSizer* sizer = new wxGridBagSizer(4, 4);
	mainsizer->Add(sizer, 1, wxEXPAND|wxALL, 10);
	SetSizer(mainsizer);


	// --- Top half (translation origin) ---

	// Translations list
	wxStaticBox* frame = new wxStaticBox(this, -1, "Translation Ranges");
	wxStaticBoxSizer* framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	sizer->Add(framesizer, wxGBPosition(0, 0), wxDefaultSpan, wxEXPAND);

	list_translations = new wxListBox(this, -1);
	framesizer->Add(list_translations, 1, wxEXPAND|wxALL, 4);

	// Add translation button
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	framesizer->Add(vbox, 0, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 4);

	btn_add = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "plus"));
	vbox->Add(btn_add, 0, wxEXPAND|wxBOTTOM, 4);

	// Remove translation button
	btn_remove = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "minus"));
	vbox->Add(btn_remove, 0, wxEXPAND|wxBOTTOM, 4);

	// Move up button
	btn_up = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "up"));
	vbox->Add(btn_up, 0, wxEXPAND|wxBOTTOM, 4);

	// Move down button
	btn_down = new wxBitmapButton(this, -1, Icons::getIcon(Icons::GENERAL, "down"));
	vbox->Add(btn_down, 0, wxEXPAND);


	// Origin range
	frame = new wxStaticBox(this, -1, "Origin Range");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Origin palette
	pal_canvas_original = new PaletteCanvas(this, -1);
	pal_canvas_original->doubleWidth(true);
	pal_canvas_original->setPalette(palette);
	pal_canvas_original->SetInitialSize(wxSize(448, 112));
	pal_canvas_original->allowSelection(2);
	framesizer->Add(pal_canvas_original->toPanel(this), 1, wxALL|wxEXPAND, 4);


	// --- Bottom half (translation target) ---

	// Target type
	frame = new wxStaticBox(this, -1, "Target Range Type");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(1, 0), wxDefaultSpan, wxEXPAND);

	// Palette range
	rb_type_palette = new wxRadioButton(this, -1, "Palette Range", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	framesizer->Add(rb_type_palette, 0, wxEXPAND|wxALL, 4);

	// Colour gradient
	rb_type_colour = new wxRadioButton(this, -1, "Colour Gradient");
	framesizer->Add(rb_type_colour, 0, wxEXPAND|wxLEFT|wxRIGHT, 4);

	// Desaturated colour gradient
	rb_type_desaturate = new wxRadioButton(this, -1, "Desaturated Colour Gradient");
	framesizer->Add(rb_type_desaturate, 0, wxEXPAND|wxALL, 4);

	// Colourise range
	rb_type_colourise = new wxRadioButton(this, -1, "Colourise Range");
	framesizer->Add(rb_type_colourise, 0, wxEXPAND|wxLEFT|wxRIGHT, 4);

	// Tint range
	rb_type_tint = new wxRadioButton(this, -1, "Tint Range");
	framesizer->Add(rb_type_tint, 0, wxEXPAND | wxALL, 4);

	// Target range
	frame = new wxStaticBox(this, -1, "Target Range");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);


	// Target palette range panel
	panel_target_palette = new wxPanel(this, -1);
	vbox = new wxBoxSizer(wxVERTICAL);
	panel_target_palette->SetSizer(vbox);

	// Target palette
	pal_canvas_target = new PaletteCanvas(panel_target_palette, -1);
	pal_canvas_target->doubleWidth(true);
	pal_canvas_target->setPalette(palette);
	pal_canvas_target->SetInitialSize(wxSize(448, 112));
	pal_canvas_target->allowSelection(2);
	vbox->Add(pal_canvas_target->toPanel(panel_target_palette), 1, wxEXPAND);

	// Reverse origin range
	cb_target_reverse = new wxCheckBox(panel_target_palette, -1, "Reverse Selection");
	vbox->Add(cb_target_reverse, 0, wxTOP, 4);


	// Target colour gradient panel
	panel_target_gradient = new wxPanel(this, -1);
	vbox = new wxBoxSizer(wxVERTICAL);
	panel_target_gradient->SetSizer(vbox);

	// Start colour
	vbox->AddStretchSpacer();
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND|wxBOTTOM, 4);

	cb_range_begin = new ColourBox(panel_target_gradient, -1, false, true);
	cb_range_begin->setColour(COL_BLACK);
	cb_range_begin->setPalette(pal);
	hbox->Add(cb_range_begin, 0, wxEXPAND|wxRIGHT, 4);
	hbox->Add(new wxStaticText(panel_target_gradient, -1, "From"), 0, wxALIGN_CENTER_VERTICAL);

	// End colour
	cb_range_end = new ColourBox(panel_target_gradient, -1, false, true);
	cb_range_end->setColour(COL_WHITE);
	cb_range_end->setPalette(pal);
	hbox->AddStretchSpacer();
	hbox->Add(new wxStaticText(panel_target_gradient, -1, "To"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(cb_range_end, 0, wxEXPAND);

	// Gradient preview
	gb_gradient = new GradientBox(panel_target_gradient);
	vbox->Add(gb_gradient->toPanel(panel_target_gradient), 0, wxEXPAND);
	vbox->AddStretchSpacer();

	// Target colourise/tint panel
	panel_target_tint = new wxPanel(this, -1);
	vbox = new wxBoxSizer(wxVERTICAL);
	panel_target_tint->SetSizer(vbox);
	vbox->AddStretchSpacer();

	// Add colour chooser
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND|wxALL, 4);

	cb_target_tint = new ColourBox(panel_target_tint, -1, false, true);
	cb_target_tint->setColour(COL_RED);
	cb_target_tint->setPalette(pal);
	hbox->Add(cb_target_tint, 0, wxEXPAND|wxRIGHT, 4);
	hbox->Add(new wxStaticText(panel_target_tint, -1, "Colour"), 1, wxALIGN_CENTER_VERTICAL);

	// Add 'amount' slider
	slider_tint = new wxSlider(panel_target_tint, -1, 50, 0, 100);
	label_tint = new wxStaticText(panel_target_tint, -1, "50%");
	label_amount = new wxStaticText(panel_target_tint, -1, "Amount");
	hbox->Add(label_amount, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(slider_tint, 3, wxEXPAND|wxRIGHT, 4);
	hbox->Add(label_tint, 0, wxALIGN_CENTER_VERTICAL);

	// Show initial target panel (palette)
	framesizer->Add(panel_target_palette, 1, wxEXPAND|wxALL, 4);
	panel_target_gradient->Show(false);
	panel_target_tint->Show(false);

	// --- Preview section ---
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(2, 0), wxGBSpan(1, 2), wxEXPAND);

	// Palette preview
	frame = new wxStaticBox(this, -1, "Resulting Palette");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 0, wxEXPAND|wxRIGHT, 4);

	pal_canvas_preview = new PaletteCanvas(this, -1);
	pal_canvas_preview->SetInitialSize(wxSize(224, 224));
	pal_canvas_preview->setPalette(palette);
	framesizer->Add(pal_canvas_preview->toPanel(this), 1, wxEXPAND|wxALL, 4);

	// Image preview
	frame = new wxStaticBox(this, -1, "Preview");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, 1, wxEXPAND);

	gfx_preview = new GfxCanvas(this, -1);
	gfx_preview->setPalette(palette);
	gfx_preview->setViewType(GFXVIEW_CENTERED);
	gfx_preview->getImage()->copyImage(&image_preview);
	framesizer->Add(gfx_preview->toPanel(this), 1, wxEXPAND|wxALL, 4);


	// --- Translation string ---
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(3, 0), wxGBSpan(1, 2), wxEXPAND|wxLEFT|wxRIGHT, 4);

	text_string = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	hbox->Add(new wxStaticText(this, -1, "Translation String:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(text_string, 1, wxEXPAND);


	// --- Dialog buttons ---
	wxSizer* buttonsizer = CreateButtonSizer(wxOK|wxCANCEL);
	sizer->Add(buttonsizer, wxGBPosition(4, 0), wxGBSpan(1, 2), wxEXPAND);

	// Load button
	btn_load = new wxButton(this, -1, "Load from File");
	buttonsizer->InsertStretchSpacer(0);
	buttonsizer->Insert(0, btn_load, 0, wxLEFT|wxRIGHT, 4);

	// Save button
	btn_save = new wxButton(this, -1, "Save to File");
	buttonsizer->Insert(1, btn_save, 0, wxLEFT, 4);

	// Truecolor checkbox
	cb_truecolor = new wxCheckBox(this, -1, "Truecolor");
	buttonsizer->Insert(2, cb_truecolor, 0, wxLEFT, 4);

	// Palette translation only
	cb_paletteonly = new wxCheckBox(this, -1, "Palette Translation Only");
	buttonsizer->Insert(3, cb_paletteonly, 0, wxLEFT, 4);

	// Bind events
	Bind(wxEVT_SIZE, &TranslationEditorDialog::onSize, this);
	list_translations->Bind(wxEVT_LISTBOX, &TranslationEditorDialog::onTranslationListItemSelected, this);
	rb_type_palette->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBPaletteSelected, this);
	rb_type_colour->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBColourSelected, this);
	rb_type_desaturate->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBDesaturateSelected, this);
	rb_type_colourise->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBColouriseSelected, this);
	rb_type_tint->Bind(wxEVT_RADIOBUTTON, &TranslationEditorDialog::onRBTintSelected, this);
	cb_range_begin->Bind(wxEVT_COLOURBOX_CHANGED, &TranslationEditorDialog::onBeginColourChanged, this);
	cb_range_end->Bind(wxEVT_COLOURBOX_CHANGED, &TranslationEditorDialog::onEndColourChanged, this);
	cb_target_tint->Bind(wxEVT_COLOURBOX_CHANGED, &TranslationEditorDialog::onTintColourChanged, this);
	slider_tint->Bind(wxEVT_SLIDER, &TranslationEditorDialog::onTintAmountChanged, this);
	pal_canvas_original->Bind(wxEVT_LEFT_UP, &TranslationEditorDialog::onPalOriginLeftUp, this);
	pal_canvas_target->Bind(wxEVT_LEFT_UP, &TranslationEditorDialog::onPalTargetLeftUp, this);
	btn_add->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnAdd, this);
	btn_remove->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnRemove, this);
	btn_up->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnUp, this);
	btn_down->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnDown, this);
	btn_load->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnLoad, this);
	btn_save->Bind(wxEVT_BUTTON, &TranslationEditorDialog::onBtnSave, this);
	gfx_preview->Bind(wxEVT_MOTION, &TranslationEditorDialog::onGfxPreviewMouseMotion, this);
	cb_target_reverse->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBTargetReverse, this);
	cb_truecolor->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBTruecolor, this);
	cb_paletteonly->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBPaletteOnly, this);

	// Setup layout
	Layout();
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
	CenterOnParent();
	list_translations->SetSizeHints(list_translations->GetSize(), list_translations->GetSize());
}

/* TranslationEditorDialog::~TranslationEditorDialog
 * TranslationEditorDialog class destructor
 *******************************************************************/
TranslationEditorDialog::~TranslationEditorDialog()
{
}

/* TranslationEditorDialog::openTranslation
 * Opens the translation [trans] for preview/modification
 *******************************************************************/
void TranslationEditorDialog::openTranslation(Translation& trans)
{
	// Read translation
	translation.copy(trans);

	// Update translation list
	list_translations->Clear();
	for (unsigned a = 0; a < translation.nRanges(); a++)
		list_translations->Append(translation.getRange(a)->asText());

	// Select+open first range if it exists
	if (list_translations->GetCount() > 0)
	{
		list_translations->SetSelection(0);
		openRange(0);
	}

	// Update previews
	updatePreviews();
}

/* TranslationEditorDialog::openRange
 * Opens the translation range [index] from the current translation
 *******************************************************************/
void TranslationEditorDialog::openRange(int index)
{
	// Check index
	if (index < 0 || index >= (int)translation.nRanges())
		return;

	// Get translation range to open
	TransRange* tr = translation.getRange(index);

	// Set origin selection
	pal_canvas_original->setSelection(tr->oStart(), tr->oEnd());
	pal_canvas_original->Refresh();

	// Check translation range type
	if (tr->getType() == TRANS_PALETTE)
	{
		// Palette range
		TransRangePalette* tpr = (TransRangePalette*)tr;

		// Select palette type radiobox
		rb_type_palette->SetValue(true);
		showPaletteTarget();

		// Set target range selection
		if (tpr->dStart() <= tpr->dEnd())
		{
			pal_canvas_target->setSelection(tpr->dStart(), tpr->dEnd());
			cb_target_reverse->SetValue(false);
		}
		else
		{
			pal_canvas_target->setSelection(tpr->dEnd(), tpr->dStart());
			cb_target_reverse->SetValue(true);
		}
		pal_canvas_target->Refresh();
	}
	else if (tr->getType() == TRANS_COLOUR)
	{
		// Colour gradient
		TransRangeColour* tcr = (TransRangeColour*)tr;

		// Select colour type radiobox
		rb_type_colour->SetValue(true);
		showGradientTarget();

		// Set beginning colour
		gb_gradient->setStartCol(tcr->dStart());
		cb_range_begin->setColour(tcr->dStart());

		// Set ending colour
		gb_gradient->setEndCol(tcr->dEnd());
		cb_range_end->setColour(tcr->dEnd());

		// Update UI
		gb_gradient->Refresh();
	}
	else if (tr->getType() == TRANS_DESAT)
	{
		// Desaturated colour gradient
		TransRangeDesat* tdr = (TransRangeDesat*)tr;

		// Select desaturated colour type radiobox
		rb_type_desaturate->SetValue(true);
		showGradientTarget();

		// Set beginning colour
		rgba_t col;
		col.r = MathStuff::clamp(tdr->dSr() * 128, 0, 255);
		col.g = MathStuff::clamp(tdr->dSg() * 128, 0, 255);
		col.b = MathStuff::clamp(tdr->dSb() * 128, 0, 255);
		cb_range_begin->setColour(col);
		gb_gradient->setStartCol(col);

		// Set ending colour
		col.r = MathStuff::clamp(tdr->dEr() * 128, 0, 255);
		col.g = MathStuff::clamp(tdr->dEg() * 128, 0, 255);
		col.b = MathStuff::clamp(tdr->dEb() * 128, 0, 255);
		cb_range_end->setColour(col);
		gb_gradient->setEndCol(col);

		// Update UI
		gb_gradient->Refresh();
	}
	else if (tr->getType() == TRANS_COLOURISE)
	{
		// Colourise range
		TransRangeColourise* tcr = (TransRangeColourise*)tr;

		// Select colourise type radiobox
		rb_type_colourise->SetValue(true);
		showTintTarget(false);

		// Set colour
		cb_target_tint->setColour(tcr->getColour());
	}
	else if (tr->getType() == TRANS_TINT)
	{
		// Tint range
		TransRangeTint* ttr = (TransRangeTint*)tr;

		// Select colourise type radiobox
		rb_type_tint->SetValue(true);
		showTintTarget(true);

		// Set colour
		cb_target_tint->setColour(ttr->getColour());

		// Set amount
		slider_tint->SetValue(ttr->getAmount());
		label_tint->SetLabel(S_FMT("%d%% ", ttr->getAmount()));
	}
}

/* TranslationEditorDialog::updateListItem
 * Updates the translation range [index] in the list
 *******************************************************************/
void TranslationEditorDialog::updateListItem(int index)
{
	// Check index
	if (index < 0 || index >= (int)list_translations->GetCount())
		return;

	// Get translation range
	TransRange* tr = translation.getRange(index);

	// Update list item text
	if (tr)
	{
		list_translations->Delete(index);
		list_translations->Insert(tr->asText(), index);
		list_translations->SetSelection(index);
	}
}

/* TranslationEditorDialog::setStartColour
 * Sets the current translation range's destination starting colour
 * to [col]
 *******************************************************************/
void TranslationEditorDialog::setStartColour(rgba_t col)
{
	// Get currently selected translation range
	TransRange* tr = translation.getRange(list_translations->GetSelection());

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
	gb_gradient->setStartCol(col);

	// Update UI
	gb_gradient->Refresh();
	updateListItem(list_translations->GetSelection());
	updatePreviews();
}

/* TranslationEditorDialog::setEndColour
 * Sets the current translation range's destination ending colour
 * to [col]
 *******************************************************************/
void TranslationEditorDialog::setEndColour(rgba_t col)
{
	// Get currently selected translation range
	TransRange* tr = translation.getRange(list_translations->GetSelection());

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
	gb_gradient->setEndCol(col);

	// Update UI
	gb_gradient->Refresh();
	updateListItem(list_translations->GetSelection());
	updatePreviews();
}

/* TranslationEditorDialog::setTintColour
 * Sets the current translation range's tint colour to [col]
 *******************************************************************/
void TranslationEditorDialog::setTintColour(rgba_t col)
{
	// Get currently selected translation range
	TransRange* tr = translation.getRange(list_translations->GetSelection());

	// Check its type
	if (tr->getType() == TRANS_COLOURISE)
	{
		// Colour range
		TransRangeColourise* tcr = (TransRangeColourise*)tr;

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
	updateListItem(list_translations->GetSelection());
	updatePreviews();
}

/* TranslationEditorDialog::setTintAmount
* Sets the current translation range's tint colour to [col]
*******************************************************************/
void TranslationEditorDialog::setTintAmount(int amount)
{
	// Get currently selected translation range
	TransRange* tr = translation.getRange(list_translations->GetSelection());

	// Check its type
	if (tr->getType() == TRANS_TINT)

	{
		// Desaturated colour range
		TransRangeTint* ttr = (TransRangeTint*)tr;

		// Set destination end colour
		ttr->setAmount(amount);
	}

	// Update UI
	updateListItem(list_translations->GetSelection());
	updatePreviews();
}

/* TranslationEditorDialog::showPaletteTarget
 * Shows the palette range translation target controls
 *******************************************************************/
void TranslationEditorDialog::showPaletteTarget()
{
	// Nothing to do
	if (panel_target_palette->IsShown())
		return;

	// Swap gradient panel for palette panel
	if (panel_target_gradient->IsShown())
	{
		panel_target_gradient->Show(false);
		GetSizer()->Replace(panel_target_gradient, panel_target_palette, true);
	}
	// Swap tint panel for palette panel
	if (panel_target_tint->IsShown())
	{
		panel_target_tint->Show(false);
		GetSizer()->Replace(panel_target_tint, panel_target_palette, true);
	}

	// Update UI
	panel_target_palette->Show(true);
	Layout();
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
}

/* TranslationEditorDialog::showGradientTarget
 * Shows the colour gradient translation target controls
 *******************************************************************/
void TranslationEditorDialog::showGradientTarget()
{
	// Nothing to do
	if (panel_target_gradient->IsShown())
		return;

	// Swap palette panel for gradient panel
	if (panel_target_palette->IsShown())
	{
		panel_target_palette->Show(false);
		GetSizer()->Replace(panel_target_palette, panel_target_gradient, true);
	}
	// Swap tint panel for gradient panel
	else if (panel_target_tint->IsShown())
	{
		panel_target_tint->Show(false);
		GetSizer()->Replace(panel_target_tint, panel_target_gradient, true);
	}

	// Update UI
	panel_target_gradient->Show(true);
	Layout();
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());

}

/* TranslationEditorDialog::showTintTarget
 * Shows the colourise/tint translation target controls
 *******************************************************************/
void TranslationEditorDialog::showTintTarget(bool tint)
{
	// Update slider status
	slider_tint->Enable(tint);
	label_tint->Enable(tint);
	label_amount->Enable(tint);

	// Nothing further to do
	if (panel_target_tint->IsShown())
		return;

	// Swap palette panel for tint panel
	if (panel_target_palette->IsShown())
	{
		panel_target_palette->Show(false);
		GetSizer()->Replace(panel_target_palette, panel_target_tint, true);
	}
	// Swap gradient panel for tint panel
	else if (panel_target_gradient->IsShown())
	{
		panel_target_gradient->Show(false);
		GetSizer()->Replace(panel_target_gradient, panel_target_tint, true);
	}

	// Update UI
	panel_target_tint->Show(true);
	Layout();
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
}

/* TranslationEditorDialog::updatePreviews
 * Updates the image and resulting palette previews according to the
 * current translation
 *******************************************************************/
void TranslationEditorDialog::updatePreviews()
{
	// Update palette preview
	pal_canvas_preview->setPalette(palette);
	pal_canvas_preview->getPalette().applyTranslation(&translation);
	pal_canvas_preview->Refresh();

	// Update image preview
	gfx_preview->getImage()->copyImage(&image_preview);
	gfx_preview->getImage()->applyTranslation(&translation, palette, cb_truecolor->GetValue());

	// Update UI
	gfx_preview->updateImageTexture();
	gfx_preview->Refresh();

	// Update text string
	if (cb_paletteonly->GetValue())
	{
		// Create a palette image
		SImage img(PALMASK);
		img.create(256, 1, PALMASK, palette);
		for (int i = 0; i < 256; ++i)
			img.setPixel(i, 0, i);
		// Apply translation to image
		img.applyTranslation(&translation, palette);
		// Create new translation from image data
		Translation newtrans;
		MemChunk mc;
		if (img.getIndexedData(mc))
		{
			newtrans.read(mc.getData());
			text_string->SetValue(newtrans.asText());
		}
	}
	else text_string->SetValue(translation.asText());
}

/* TranslationEditorDialog::getTruecolor
 * Returns whether the truecolor checkbox is checked
 *******************************************************************/
bool TranslationEditorDialog::getTruecolor()
{
	return cb_truecolor->GetValue();
}

/*******************************************************************
 * TRANSLATIONEDITORDIALOG CLASS EVENTS
 *******************************************************************/

/* TranslationEditorDialog::onSize
 * Called when the dialog is resized
 *******************************************************************/
void TranslationEditorDialog::onSize(wxSizeEvent& e)
{
	// Update image preview
	gfx_preview->zoomToFit(true, 0.05f);

	e.Skip();
}

/* TranslationEditorDialog::onTranslationListItemSelected
 * Called when a translation range list item is selected
 *******************************************************************/
void TranslationEditorDialog::onTranslationListItemSelected(wxCommandEvent& e)
{
	// Open what was selected
	openRange(e.GetInt());
}

/* TranslationEditorDialog::onRBPaletteSelected
 * Called when the 'palette range' translation type radio button is
 * selected
 *******************************************************************/
void TranslationEditorDialog::onRBPaletteSelected(wxCommandEvent& e)
{
	// Swap to target palette panel
	showPaletteTarget();

	// If a range is selected
	int index = list_translations->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation.removeRange(index);

		// Recreate it
		translation.addRange(TRANS_PALETTE, index);
		TransRangePalette* tr = (TransRangePalette*)translation.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original->getSelectionStart());
		tr->setOEnd(pal_canvas_original->getSelectionEnd());
		// Target range
		if (cb_target_reverse->GetValue())
		{
			tr->setDEnd(pal_canvas_target->getSelectionStart());
			tr->setDStart(pal_canvas_target->getSelectionEnd());
		}
		else
		{
			tr->setDStart(pal_canvas_target->getSelectionStart());
			tr->setDEnd(pal_canvas_target->getSelectionEnd());
		}

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}

/* TranslationEditorDialog::onRBColourSelected
 * Called when the 'colour gradient' translation type radio button is
 * selected
 *******************************************************************/
void TranslationEditorDialog::onRBColourSelected(wxCommandEvent& e)
{
	// Swap to target colour panel
	showGradientTarget();

	// If a range is selected
	int index = list_translations->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation.removeRange(index);

		// Recreate it
		translation.addRange(TRANS_COLOUR, index);
		TransRangeColour* tr = (TransRangeColour*)translation.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original->getSelectionStart());
		tr->setOEnd(pal_canvas_original->getSelectionEnd());
		// Target colour gradient
		tr->setDStart(cb_range_begin->getColour());
		tr->setDEnd(cb_range_end->getColour());

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}

/* TranslationEditorDialog::onRBDesaturateSelected
 * Called when the 'desaturated colour gradient' translation type
 * radio button is selected
 *******************************************************************/
void TranslationEditorDialog::onRBDesaturateSelected(wxCommandEvent& e)
{
	// Swap to target colour panel
	showGradientTarget();

	// If a range is selected
	int index = list_translations->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation.removeRange(index);

		// Recreate it
		translation.addRange(TRANS_DESAT, index);
		TransRangeDesat* tr = (TransRangeDesat*)translation.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original->getSelectionStart());
		tr->setOEnd(pal_canvas_original->getSelectionEnd());
		// Target colour gradient
		rgba_t sc = cb_range_begin->getColour();
		rgba_t ec = cb_range_end->getColour();
		tr->setDStart(MathStuff::clamp((double)sc.r / 127.0f, 0, 2),
		              MathStuff::clamp((double)sc.g / 127.0f, 0, 2),
		              MathStuff::clamp((double)sc.b / 127.0f, 0, 2));
		tr->setDEnd(MathStuff::clamp((double)ec.r / 127.0f, 0, 2),
		            MathStuff::clamp((double)ec.g / 127.0f, 0, 2),
		            MathStuff::clamp((double)ec.b / 127.0f, 0, 2));

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}

/* TranslationEditorDialog::onRBColouriseSelected
 * Called when the 'colourise' translation type radio button is selected
 *******************************************************************/
void TranslationEditorDialog::onRBColouriseSelected(wxCommandEvent& e)
{
	showTintTarget(false);

	// If a range is selected
	int index = list_translations->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation.removeRange(index);

		// Recreate it
		translation.addRange(TRANS_COLOURISE, index);
		TransRangeColourise* tr = (TransRangeColourise*)translation.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original->getSelectionStart());
		tr->setOEnd(pal_canvas_original->getSelectionEnd());
		// Target colour
		tr->setColour(cb_target_tint->getColour());

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}


/* TranslationEditorDialog::onRBTintSelected
 * Called when the 'tint' translation type radio button is selected
 *******************************************************************/
void TranslationEditorDialog::onRBTintSelected(wxCommandEvent& e)
{
	showTintTarget(true);

	// If a range is selected
	int index = list_translations->GetSelection();
	if (index >= 0)
	{
		// Remove it
		translation.removeRange(index);

		// Recreate it
		translation.addRange(TRANS_TINT, index);
		TransRangeTint* tr = (TransRangeTint*)translation.getRange(index);
		// Origin range
		tr->setOStart(pal_canvas_original->getSelectionStart());
		tr->setOEnd(pal_canvas_original->getSelectionEnd());
		// Target colour
		tr->setColour(cb_target_tint->getColour());
		tr->setAmount(slider_tint->GetValue());

		// Update UI
		updateListItem(index);
		openRange(index);
		updatePreviews();
	}
}

/* TranslationEditorDialog::onBeginColourChanged
 * Called when the target gradient start colour is changed
 *******************************************************************/
void TranslationEditorDialog::onBeginColourChanged(wxEvent& e)
{
	// Set start colour to selected colour
	setStartColour(cb_range_begin->getColour());
}

/* TranslationEditorDialog::onEndColourChanged
 * Called when the target gradient end colour is changed
 *******************************************************************/
void TranslationEditorDialog::onEndColourChanged(wxEvent& e)
{
	// Set end colour to selected colour
	setEndColour(cb_range_end->getColour());
}

/* TranslationEditorDialog::onTintColourChanged
 * Called when the target colourise/tint colour is changed
 *******************************************************************/
void TranslationEditorDialog::onTintColourChanged(wxEvent& e)
{
	setTintColour(cb_target_tint->getColour());
}

/* TranslationEditorDialog::onTintAmountChanged
 * Called when the tint amount slider is changed
 *******************************************************************/
void TranslationEditorDialog::onTintAmountChanged(wxCommandEvent& e)
{
	setTintAmount(slider_tint->GetValue());
	label_tint->SetLabel(S_FMT("%d%% ", slider_tint->GetValue()));
}

/* TranslationEditorDialog::onPalOriginLeftUp
 * Called when the left mouse button is released in the origin
 * palette range canvas
 *******************************************************************/
void TranslationEditorDialog::onPalOriginLeftUp(wxMouseEvent& e)
{
	// Get current translation range
	TransRange* tr = translation.getRange(list_translations->GetSelection());

	// Update its origin range
	if (tr)
	{
		tr->setOStart(pal_canvas_original->getSelectionStart());
		tr->setOEnd(pal_canvas_original->getSelectionEnd());
	}

	// Update UI
	updateListItem(list_translations->GetSelection());
	updatePreviews();
}

/* TranslationEditorDialog::onPalTargetLeftUp
 * Called when the left mouse button is released in the target
 * palette range canvas
 *******************************************************************/
void TranslationEditorDialog::onPalTargetLeftUp(wxMouseEvent& e)
{
	// Get current translation range
	TransRange* tr = translation.getRange(list_translations->GetSelection());

	// Update its target range if it's a palette translation
	if (tr && tr->getType() == TRANS_PALETTE)
	{
		TransRangePalette* tpr = (TransRangePalette*)tr;
		if (cb_target_reverse->GetValue())
		{
			tpr->setDEnd(pal_canvas_target->getSelectionStart());
			tpr->setDStart(pal_canvas_target->getSelectionEnd());
		}
		else
		{
			tpr->setDStart(pal_canvas_target->getSelectionStart());
			tpr->setDEnd(pal_canvas_target->getSelectionEnd());
		}
	}

	// Update UI
	updateListItem(list_translations->GetSelection());
	updatePreviews();
}

/* TranslationEditorDialog::onBtnAdd
 * Called when the 'Add Translation' button is clicked
 *******************************************************************/
void TranslationEditorDialog::onBtnAdd(wxCommandEvent& e)
{
	// Get index to add at
	int index = list_translations->GetSelection()+1;

	// Add new range to translation depending on current type selection
	if (rb_type_palette->GetValue())
		translation.addRange(TRANS_PALETTE, index);
	else if (rb_type_colour->GetValue())
		translation.addRange(TRANS_COLOUR, index);
	else if (rb_type_desaturate->GetValue())
		translation.addRange(TRANS_DESAT, index);
	else if (rb_type_colourise->GetValue())
		translation.addRange(TRANS_COLOURISE, index);
	else if (rb_type_tint->GetValue())
		translation.addRange(TRANS_TINT, index);

	// Add it to the list
	list_translations->Insert(translation.getRange(index)->asText(), index);

	// Update UI
	list_translations->SetSelection(index);
	openRange(index);
	updatePreviews();
}

/* TranslationEditorDialog::onBtnRemove
 * Called when the 'Remove Translation' button is clicked
 *******************************************************************/
void TranslationEditorDialog::onBtnRemove(wxCommandEvent& e)
{
	// Do nothing if there is only one translation
	if (translation.nRanges() == 1)
		return;

	// Get index of range to remove
	int index = list_translations->GetSelection();

	// Remove it from the translation
	translation.removeRange(index);

	// Remove it from the list
	list_translations->Delete(index);

	// Update UI
	if (index >= (int)list_translations->GetCount())
		index--;
	if (index >= 0)
	{
		list_translations->SetSelection(index);
		openRange(index);
	}
	updatePreviews();
}

/* TranslationEditorDialog::onBtnUp
 * Called when the 'Move Up' button is clicked
 *******************************************************************/
void TranslationEditorDialog::onBtnUp(wxCommandEvent& e)
{
	// Get selection
	int index = list_translations->GetSelection();

	// Check selection is valid
	if (index <= 0)
		return;

	// Swap item with above
	translation.swapRanges(index, index-1);
	updateListItem(index);
	updateListItem(index-1);

	// Update selection
	list_translations->SetSelection(index-1);
	openRange(index-1);

	// Update previews
	updatePreviews();
}

/* TranslationEditorDialog::onBtnDown
 * Called when the 'Move Down' button is clicked
 *******************************************************************/
void TranslationEditorDialog::onBtnDown(wxCommandEvent& e)
{
	// Get selection
	int index = list_translations->GetSelection();

	// Check selection is valid
	if (index >= (int)translation.nRanges()-1)
		return;

	// Swap item with above
	translation.swapRanges(index, index+1);
	updateListItem(index);
	updateListItem(index+1);

	// Update selection
	list_translations->SetSelection(index+1);
	openRange(index+1);

	// Update previews
	updatePreviews();
}

/* TranslationEditorDialog::onBtnLoad
 * Called when the 'Load Translation' button is clicked
 *******************************************************************/
void TranslationEditorDialog::onBtnLoad(wxCommandEvent& e)
{
	// Get user directory
	string dir = appPath("translations", DIR_USER);

	// Create open file dialog
	wxFileDialog dialog_open(this, "Load Translation from File", dir, wxEmptyString,
	                         "Text Files (*.txt)|*.txt", wxFD_OPEN|wxFD_FILE_MUST_EXIST, wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Get the selected filename
		string filename = dialog_open.GetPath();

		// Load file in a tokenizer
		Tokenizer tz;
		tz.openFile(dialog_open.GetPath());

		// Parse translation
		Translation trans;
		string token = tz.getToken();
		while (!token.IsEmpty())
		{
			// Parse translation range
			trans.parse(token);

			tz.getToken();			// Skip ,
			token = tz.getToken();
		}

		// Open it if parsed ok
		if (trans.nRanges() > 0)
			openTranslation(trans);
		else
			wxMessageBox("Not a valid translation file", "Error", wxICON_ERROR);
	}
}

/* TranslationEditorDialog::onBtnSave
 * Called when the 'Save Translation' button is clicked
 *******************************************************************/
void TranslationEditorDialog::onBtnSave(wxCommandEvent& e)
{
	// If the directory doesn't exist create it
	string dir = appPath("translations", DIR_USER);
	if (!wxDirExists(dir))
		wxMkdir(dir);

	// Create save file dialog
	wxFileDialog dialog_save(this, "Save Translation to File", dir, wxEmptyString,
	                         "Text File (*.txt)|*.txt", wxFD_SAVE|wxFD_OVERWRITE_PROMPT, wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_save.ShowModal() == wxID_OK)
	{
		// Get translation as text string
		string str = translation.asText();

		// Open file for writing
		wxFile file(dialog_save.GetPath(), wxFile::write);

		// Write string to file
		file.Write(str);

		// Close file
		file.Close();
	}
}

/* TranslationEditorDialog::onGfxPreviewMouseMotion
 * Called when the mouse pointer is moved over the gfx preview canvas
 *******************************************************************/
void TranslationEditorDialog::onGfxPreviewMouseMotion(wxMouseEvent& e)
{
	// Get the image coordinates at the mouse pointer
	point2_t pos = gfx_preview->imageCoords(e.GetX(), e.GetY()-2);

	int index = pal_canvas_preview->getSelectionStart();

	// Get palette index at position
	if (pos.x >= 0)
		index = gfx_preview->getImage()->getPixelIndex(pos.x, pos.y);
	else
		index = -1;

	// Update preview palette if necessary
	if (index != pal_canvas_preview->getSelectionStart())
	{
		pal_canvas_preview->setSelection(index);
		pal_canvas_preview->Refresh();
	}

	e.Skip();
}

/* TranslationEditorDialog::onCBTargetReverse
 * Called when the 'Reverse Target Range' checkbox is (un)checked
 *******************************************************************/
void TranslationEditorDialog::onCBTargetReverse(wxCommandEvent& e)
{
	// Get current translation range
	TransRange* tr = translation.getRange(list_translations->GetSelection());

	// Update its target range if it's a palette translation
	if (tr && tr->getType() == TRANS_PALETTE)
	{
		TransRangePalette* tpr = (TransRangePalette*)tr;
		if (cb_target_reverse->GetValue())
		{
			tpr->setDEnd(pal_canvas_target->getSelectionStart());
			tpr->setDStart(pal_canvas_target->getSelectionEnd());
		}
		else
		{
			tpr->setDStart(pal_canvas_target->getSelectionStart());
			tpr->setDEnd(pal_canvas_target->getSelectionEnd());
		}
	}

	// Update UI
	updateListItem(list_translations->GetSelection());
	updatePreviews();
}

/* TranslationEditorDialog::onCBTruecolor
 * Called when the 'Truecolor' checkbox is (un)checked
 *******************************************************************/
void TranslationEditorDialog::onCBTruecolor(wxCommandEvent& e)
{
	updatePreviews();
}

/* TranslationEditorDialog::onCBPaletteOnly
* Called when the 'Palette translation ony' checkbox is (un)checked
*******************************************************************/
void TranslationEditorDialog::onCBPaletteOnly(wxCommandEvent& e)
{
	updatePreviews();
}

/*******************************************************************
* GFXCOLOURISEDIALOG FUNCTIONS
*******************************************************************/
GfxColouriseDialog::GfxColouriseDialog(wxWindow* parent, ArchiveEntry* entry, Palette8bit* pal)
: wxDialog(parent, -1, "Colourise", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	// Init variables
	this->entry = entry;
	this->palette = pal;

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "colourise"));
	SetIcon(icon);

	// Setup main sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, 1, wxEXPAND|wxALL, 6);

	// Add colour chooser
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);

	cb_colour = new ColourBox(this, -1, false, true);
	cb_colour->setColour(COL_RED);
	cb_colour->setPalette(pal);
	hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(cb_colour, 0, wxEXPAND);

	// Add preview
	gfx_preview = new GfxCanvas(this, -1);
	sizer->Add(gfx_preview, 1, wxEXPAND|wxALL, 4);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxBOTTOM, 4);

	// Setup preview
	gfx_preview->setViewType(GFXVIEW_CENTERED);
	gfx_preview->setPalette(pal);
	gfx_preview->SetInitialSize(wxSize(192, 192));
	Misc::loadImageFromEntry(gfx_preview->getImage(), entry);
	rgba_t col = cb_colour->getColour();
	gfx_preview->getImage()->colourise(col, pal);
	gfx_preview->updateImageTexture();

	// Init layout
	Layout();

	// Bind events
	cb_colour->Bind(wxEVT_COLOURBOX_CHANGED, &GfxColouriseDialog::onColourChanged, this);
	Bind(wxEVT_SIZE, &GfxColouriseDialog::onResize, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
	CenterOnParent();
}

rgba_t GfxColouriseDialog::getColour()
{
	return cb_colour->getColour();
}

void GfxColouriseDialog::setColour(string col)
{
	wxColour colour(col);
	rgba_t rgba = rgba_t(colour.Red(), colour.Green(), colour.Blue());
	cb_colour->setColour(rgba);
	gfx_preview->getImage()->colourise(rgba, palette);
	gfx_preview->updateImageTexture();
	gfx_preview->Refresh();
}

// Events
void GfxColouriseDialog::onColourChanged(wxEvent& e)
{
	Misc::loadImageFromEntry(gfx_preview->getImage(), entry);
	gfx_preview->getImage()->colourise(cb_colour->getColour(), palette);
	gfx_preview->updateImageTexture();
	gfx_preview->Refresh();
}

void GfxColouriseDialog::onResize(wxSizeEvent& e)
{
	wxDialog::OnSize(e);
	gfx_preview->zoomToFit(true, 0.05f);
	e.Skip();
}


/*******************************************************************
* GFXTINTDIALOG FUNCTIONS
*******************************************************************/

GfxTintDialog::GfxTintDialog(wxWindow* parent, ArchiveEntry* entry, Palette8bit* pal)
: wxDialog(parent, -1, "Tint", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	// Init variables
	this->entry = entry;
	this->palette = pal;

	// Set dialog icon
	wxIcon icon;
	icon.CopyFromBitmap(Icons::getIcon(Icons::GENERAL, "tint"));
	SetIcon(icon);

	// Setup main sizer
	wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, 1, wxEXPAND|wxALL, 6);

	// Add colour chooser
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);

	cb_colour = new ColourBox(this, -1, false, true);
	cb_colour->setColour(COL_RED);
	cb_colour->setPalette(pal);
	hbox->Add(new wxStaticText(this, -1, "Colour:"), 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(cb_colour, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);

	// Add 'amount' slider
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	slider_amount = new wxSlider(this, -1, 50, 0, 100);
	label_amount = new wxStaticText(this, -1, "100%");
	hbox->Add(new wxStaticText(this, -1, "Amount:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	hbox->Add(slider_amount, 1, wxEXPAND|wxRIGHT, 4);
	hbox->Add(label_amount, 0, wxALIGN_CENTER_VERTICAL);

	// Add preview
	gfx_preview = new GfxCanvas(this, -1);
	sizer->Add(gfx_preview, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxBOTTOM, 4);

	// Setup preview
	gfx_preview->setViewType(GFXVIEW_CENTERED);
	gfx_preview->setPalette(pal);
	gfx_preview->SetInitialSize(wxSize(256, 256));
	Misc::loadImageFromEntry(gfx_preview->getImage(), entry);
	gfx_preview->getImage()->tint(getColour(), getAmount(), pal);
	gfx_preview->updateImageTexture();

	// Init layout
	Layout();

	// Bind events
	cb_colour->Bind(wxEVT_COLOURBOX_CHANGED, &GfxTintDialog::onColourChanged, this);
	slider_amount->Bind(wxEVT_SLIDER, &GfxTintDialog::onAmountChanged, this);
	Bind(wxEVT_SIZE, &GfxTintDialog::onResize, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	SetMinSize(GetSize());
	CenterOnParent();

	// Set values
	label_amount->SetLabel("50% ");
}

rgba_t GfxTintDialog::getColour()
{
	return rgba_t(cb_colour->getColour());
}

float GfxTintDialog::getAmount()
{
	return (float)slider_amount->GetValue()*0.01f;
}

void GfxTintDialog::setValues(string col, int val)
{
	wxColour colour(col);
	cb_colour->setColour(rgba_t(colour.Red(), colour.Green(), colour.Blue()));
	slider_amount->SetValue(val);
	label_amount->SetLabel(S_FMT("%d%% ", slider_amount->GetValue()));
	gfx_preview->getImage()->tint(getColour(), getAmount(), palette);
	gfx_preview->updateImageTexture();
	gfx_preview->Refresh();
}

// Events
void GfxTintDialog::onColourChanged(wxEvent& e)
{
	Misc::loadImageFromEntry(gfx_preview->getImage(), entry);
	gfx_preview->getImage()->tint(getColour(), getAmount(), palette);
	gfx_preview->updateImageTexture();
	gfx_preview->Refresh();
}

void GfxTintDialog::onAmountChanged(wxCommandEvent& e)
{
	Misc::loadImageFromEntry(gfx_preview->getImage(), entry);
	gfx_preview->getImage()->tint(getColour(), getAmount(), palette);
	gfx_preview->updateImageTexture();
	gfx_preview->Refresh();
	label_amount->SetLabel(S_FMT("%d%% ", slider_amount->GetValue()));
}

void GfxTintDialog::onResize(wxSizeEvent& e)
{
	wxDialog::OnSize(e);
	gfx_preview->zoomToFit(true, 0.05f);
	e.Skip();
}

