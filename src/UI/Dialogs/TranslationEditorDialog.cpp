
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
// #include "UI/UI.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Canvas/GL/GfxGLCanvas.h"
#include "UI/Canvas/PaletteCanvas.h"
#include "UI/Controls/ColourBox.h"
#include "UI/Controls/SIconButton.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
#include "Utility/MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, translation_editor_condensed, false, CVar::Save)


// -----------------------------------------------------------------------------
//
// GradientBox Class
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GradientBox class constructor
// -----------------------------------------------------------------------------
GradientBox::GradientBox(wxWindow* parent, int steps) :
	wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE),
	steps_{ steps }
{
	Bind(
		wxEVT_PAINT,
		[&](wxPaintEvent& e)
		{
			wxPaintDC dc(this);
			dc.GradientFillLinear({ 0, 0, GetSize().x, GetSize().y }, col_start_, col_end_);
		});

	// Minimum height 16
	SetInitialSize(wxSize(-1, FromDIP(16)));
}



// -----------------------------------------------------------------------------
//
// TranslationEditorDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TranslationEditorDialog class constructor
// -----------------------------------------------------------------------------
TranslationEditorDialog::TranslationEditorDialog(
	wxWindow*      parent,
	const Palette& pal,
	const string&  title,
	const SImage*  preview_image) :
	wxDialog(parent, -1, wxString::FromUTF8(title)),
	palette_{ new Palette(pal) },
	translation_{ new Translation },
	image_preview_{ new SImage }
{
	// Setup preview image
	if (preview_image)
	{
		image_preview_->copyImage(preview_image);
		if (preview_image->hasPalette())
			palette_->copyPalette(preview_image->palette());
	}

	// Set dialog icon
	wxutil::setWindowIcon(this, "remap");

	// Create sizer
	auto lh        = ui::LayoutHelper(this);
	auto mainsizer = new wxBoxSizer(wxVERTICAL);
	auto sizer     = new wxGridBagSizer(lh.pad(), lh.pad());
	mainsizer->Add(sizer, 1, wxEXPAND | wxALL, lh.padLarge());
	SetSizer(mainsizer);


	// --- Top half (translation origin) ---

	// Translations list
	auto frame      = new wxStaticBox(this, -1, wxS("Translation Ranges"));
	auto framesizer = new wxStaticBoxSizer(frame, wxHORIZONTAL);
	sizer->Add(framesizer, wxGBPosition(0, 0), wxDefaultSpan, wxEXPAND);

	list_translations_ = new wxListBox(this, -1);
	framesizer->Add(list_translations_, lh.sfWithBorder(1).Expand());

	// Add translation button
	auto vbox = new wxBoxSizer(wxVERTICAL);
	framesizer->Add(vbox, lh.sfWithBorder(0, wxTOP | wxBOTTOM | wxRIGHT).Expand());

	btn_add_ = new SIconButton(this, icons::General, "plus");
	vbox->Add(btn_add_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());

	// Remove translation button
	btn_remove_ = new SIconButton(this, icons::General, "minus");
	vbox->Add(btn_remove_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());

	// Move up button
	btn_up_ = new SIconButton(this, icons::General, "up");
	vbox->Add(btn_up_, lh.sfWithSmallBorder(0, wxBOTTOM).Expand());

	// Move down button
	btn_down_ = new SIconButton(this, icons::General, "down");
	vbox->Add(btn_down_, wxSizerFlags().Expand());


	// Origin range
	frame      = new wxStaticBox(this, -1, wxS("Origin Range"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

	// Origin palette
	pal_canvas_original_ = new PaletteCanvas(this);
	pal_canvas_original_->doubleWidth(true);
	pal_canvas_original_->setPalette(palette_.get());
	pal_canvas_original_->SetInitialSize(lh.size(448, 112));
	pal_canvas_original_->setSelectionType(PaletteCanvas::SelectionType::Range);
	framesizer->Add(pal_canvas_original_, lh.sfWithBorder(1).Expand());


	// --- Bottom half (translation target) ---

	// Target type
	frame      = new wxStaticBox(this, -1, wxS("Target Range Type"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(1, 0), wxDefaultSpan, wxEXPAND);

	// Palette range
	rb_type_palette_ = new wxRadioButton(this, -1, wxS("Palette Range"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	framesizer->Add(rb_type_palette_, lh.sfWithBorder().Expand());

	// Colour gradient
	rb_type_colour_ = new wxRadioButton(this, -1, wxS("Colour Gradient"));
	framesizer->Add(rb_type_colour_, lh.sfWithBorder(0, wxLEFT | wxRIGHT).Expand());

	// Desaturated colour gradient
	rb_type_desaturate_ = new wxRadioButton(this, -1, wxS("Desaturated Colour Gradient"));
	framesizer->Add(rb_type_desaturate_, lh.sfWithBorder().Expand());

	// Colourise range
	rb_type_colourise_ = new wxRadioButton(this, -1, wxS("Colourise Range"));
	framesizer->Add(rb_type_colourise_, lh.sfWithBorder(0, wxLEFT | wxRIGHT).Expand());

	// Tint range
	rb_type_tint_ = new wxRadioButton(this, -1, wxS("Tint Range"));
	framesizer->Add(rb_type_tint_, lh.sfWithBorder().Expand());

	// Target range
	frame      = new wxStaticBox(this, -1, wxS("Target Range"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);


	// Target palette range panel
	panel_target_palette_ = new wxPanel(this, -1);
	vbox                  = new wxBoxSizer(wxVERTICAL);
	panel_target_palette_->SetSizer(vbox);

	// Target palette
	pal_canvas_target_ = new PaletteCanvas(panel_target_palette_);
	pal_canvas_target_->doubleWidth(true);
	pal_canvas_target_->setPalette(palette_.get());
	pal_canvas_target_->SetInitialSize(lh.size(448, 112));
	pal_canvas_target_->setSelectionType(PaletteCanvas::SelectionType::Range);
	vbox->Add(pal_canvas_target_, wxSizerFlags(1).Expand());

	// Reverse origin range
	cb_target_reverse_ = new wxCheckBox(panel_target_palette_, -1, wxS("Reverse Selection"));
	vbox->Add(cb_target_reverse_, lh.sfWithSmallBorder(0, wxTOP));


	// Target colour gradient panel
	panel_target_gradient_ = new wxPanel(this, -1);
	vbox                   = new wxBoxSizer(wxVERTICAL);
	panel_target_gradient_->SetSizer(vbox);

	// Start colour
	vbox->AddStretchSpacer();
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, lh.sfWithBorder(0, wxBOTTOM).Expand());

	cb_range_begin_ = new ColourBox(panel_target_gradient_, -1, false, true);
	cb_range_begin_->setColour(ColRGBA::BLACK);
	cb_range_begin_->setPalette(palette_.get());
	hbox->Add(cb_range_begin_, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->Add(new wxStaticText(panel_target_gradient_, -1, wxS("From")), 0, wxALIGN_CENTER_VERTICAL);

	// End colour
	cb_range_end_ = new ColourBox(panel_target_gradient_, -1, false, true);
	cb_range_end_->setColour(ColRGBA::WHITE);
	cb_range_end_->setPalette(palette_.get());
	hbox->AddStretchSpacer();
	hbox->Add(new wxStaticText(panel_target_gradient_, -1, wxS("To")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(cb_range_end_, wxSizerFlags().Expand());

	// Gradient preview
	gb_gradient_ = new GradientBox(panel_target_gradient_);
	vbox->Add(gb_gradient_, wxSizerFlags().Expand());
	vbox->AddStretchSpacer();

	// Target colourise/tint panel
	panel_target_tint_ = new wxPanel(this, -1);
	vbox               = new wxBoxSizer(wxVERTICAL);
	panel_target_tint_->SetSizer(vbox);
	vbox->AddStretchSpacer();

	// Add colour chooser
	hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, lh.sfWithBorder().Expand());

	cb_target_tint_ = new ColourBox(panel_target_tint_, -1, false, true);
	cb_target_tint_->setColour(ColRGBA::RED);
	cb_target_tint_->setPalette(palette_.get());
	hbox->Add(cb_target_tint_, lh.sfWithBorder(0, wxRIGHT).Expand());
	hbox->Add(new wxStaticText(panel_target_tint_, -1, wxS("Colour")), wxSizerFlags(1).CenterVertical());

	// Add 'amount' slider
	slider_tint_  = new wxSlider(panel_target_tint_, -1, 50, 0, 100);
	label_tint_   = new wxStaticText(panel_target_tint_, -1, wxS("50%"));
	label_amount_ = new wxStaticText(panel_target_tint_, -1, wxS("Amount"));
	hbox->Add(label_amount_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(slider_tint_, lh.sfWithBorder(3, wxRIGHT).Expand());
	hbox->Add(label_tint_, wxSizerFlags().CenterVertical());

	// Show initial target panel (palette)
	framesizer->Add(panel_target_palette_, lh.sfWithBorder(1).Expand());
	panel_target_gradient_->Show(false);
	panel_target_tint_->Show(false);

	// --- Preview section ---
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(2, 0), wxGBSpan(1, 2), wxEXPAND);

	// Palette preview
	frame      = new wxStaticBox(this, -1, wxS("Resulting Palette"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, lh.sfWithBorder(0, wxRIGHT).Expand());

	pal_canvas_preview_ = new PaletteCanvas(this);
	pal_canvas_preview_->doubleWidth(translation_editor_condensed);
	if (translation_editor_condensed)
		pal_canvas_preview_->SetInitialSize(lh.size(320, 80));
	else
		pal_canvas_preview_->SetInitialSize(lh.size(160, 160));
	pal_canvas_preview_->setPalette(palette_.get());
	framesizer->Add(pal_canvas_preview_, lh.sfWithBorder(1).Expand());

	// Image preview
	frame      = new wxStaticBox(this, -1, wxS("Preview"));
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	hbox->Add(framesizer, wxSizerFlags(1).Expand());

	gfx_preview_ = ui::createGfxCanvas(this);
	gfx_preview_->setPalette(palette_.get());
	gfx_preview_->setViewType(GfxView::Centered);
	gfx_preview_->image().copyImage(image_preview_.get());
	framesizer->Add(gfx_preview_->window(), lh.sfWithBorder(1).Expand());


	// --- Translation string ---
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(3, 0), wxGBSpan(1, 2), wxEXPAND);

	text_string_ = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	hbox->Add(new wxStaticText(this, -1, wxS("Translation String:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(text_string_, wxSizerFlags(1).Expand());


	// --- Dialog buttons ---
	auto buttonsizer = wxutil::createDialogButtonBox(this);
	sizer->Add(buttonsizer, wxGBPosition(4, 0), wxGBSpan(1, 2), wxEXPAND);

	// Load button
	btn_load_ = new wxButton(this, -1, wxS("Load from File"));
	buttonsizer->InsertStretchSpacer(0);
	buttonsizer->Insert(0, btn_load_, lh.sfWithBorder(0, wxRIGHT));
	// Save button
	btn_save_ = new wxButton(this, -1, wxS("Save to File"));
	buttonsizer->Insert(1, btn_save_, lh.sfWithBorder(0, wxRIGHT));

	// Truecolor checkbox
	cb_truecolor_ = new wxCheckBox(this, -1, wxS("Truecolor"));
	buttonsizer->Insert(2, cb_truecolor_, lh.sfWithBorder(0, wxRIGHT).CenterVertical());

	// Palette translation only
	cb_paletteonly_ = new wxCheckBox(this, -1, wxS("Palette Translation Only"));
	buttonsizer->Insert(3, cb_paletteonly_, wxSizerFlags().CenterVertical());

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
	gfx_preview_->window()->Bind(wxEVT_MOTION, &TranslationEditorDialog::onGfxPreviewMouseMotion, this);
	cb_target_reverse_->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBTargetReverse, this);
	cb_truecolor_->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBTruecolor, this);
	cb_paletteonly_->Bind(wxEVT_CHECKBOX, &TranslationEditorDialog::onCBPaletteOnly, this);

	// Setup layout
	wxWindowBase::Layout();
	SetInitialSize(wxSize(-1, -1));
	wxTopLevelWindowBase::SetMinSize(GetSize());
	CenterOnParent();
	list_translations_->SetSizeHints(list_translations_->GetSize(), list_translations_->GetSize());
}

// -----------------------------------------------------------------------------
// TranslationEditorDialog class destructor
// -----------------------------------------------------------------------------
TranslationEditorDialog::~TranslationEditorDialog() = default;

// -----------------------------------------------------------------------------
// Opens the translation [trans] for preview/modification
// -----------------------------------------------------------------------------
void TranslationEditorDialog::openTranslation(const Translation& trans)
{
	// Read translation
	translation_->copy(trans);

	// Update translation list
	list_translations_->Clear();
	for (unsigned a = 0; a < translation_->nRanges(); a++)
		list_translations_->Append(wxString::FromUTF8(translation_->range(a)->asText()));

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
	if (index < 0 || index >= static_cast<int>(translation_->nRanges()))
		return;

	// Get translation range to open
	auto tr = translation_->range(index);

	// Set origin selection
	pal_canvas_original_->setSelection(tr->start(), tr->end());
	pal_canvas_original_->Refresh();

	// Check translation range type
	if (tr->type() == TransRange::Type::Palette)
	{
		// Palette range
		auto tpr = dynamic_cast<TransRangePalette*>(tr);

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
	else if (tr->type() == TransRange::Type::Colour)
	{
		// Colour gradient
		auto tcr = dynamic_cast<TransRangeColour*>(tr);

		// Select colour type radiobox
		rb_type_colour_->SetValue(true);
		showGradientTarget();

		// Set beginning colour
		gb_gradient_->setStartCol(tcr->startColour());
		cb_range_begin_->setColour(tcr->startColour());

		// Set ending colour
		gb_gradient_->setEndCol(tcr->endColour());
		cb_range_end_->setColour(tcr->endColour());

		// Update UI
		gb_gradient_->Refresh();
	}
	else if (tr->type() == TransRange::Type::Desat)
	{
		// Desaturated colour gradient
		auto tdr = dynamic_cast<TransRangeDesat*>(tr);

		// Select desaturated colour type radiobox
		rb_type_desaturate_->SetValue(true);
		showGradientTarget();

		// Set beginning colour
		ColRGBA col;
		col.r = math::clamp(tdr->rgbStart().r * 128, 0, 255);
		col.g = math::clamp(tdr->rgbStart().g * 128, 0, 255);
		col.b = math::clamp(tdr->rgbStart().b * 128, 0, 255);
		cb_range_begin_->setColour(col);
		gb_gradient_->setStartCol(col);

		// Set ending colour
		col.r = math::clamp(tdr->rgbEnd().r * 128, 0, 255);
		col.g = math::clamp(tdr->rgbEnd().g * 128, 0, 255);
		col.b = math::clamp(tdr->rgbEnd().b * 128, 0, 255);
		cb_range_end_->setColour(col);
		gb_gradient_->setEndCol(col);

		// Update UI
		gb_gradient_->Refresh();
	}
	else if (tr->type() == TransRange::Type::Blend)
	{
		// Colourise range
		auto tcr = dynamic_cast<TransRangeBlend*>(tr);

		// Select colourise type radiobox
		rb_type_colourise_->SetValue(true);
		showTintTarget(false);

		// Set colour
		cb_target_tint_->setColour(tcr->colour());
	}
	else if (tr->type() == TransRange::Type::Tint)
	{
		// Tint range
		auto ttr = dynamic_cast<TransRangeTint*>(tr);

		// Select colourise type radiobox
		rb_type_tint_->SetValue(true);
		showTintTarget(true);

		// Set colour
		cb_target_tint_->setColour(ttr->colour());

		// Set amount
		slider_tint_->SetValue(ttr->amount());
		label_tint_->SetLabel(WX_FMT("{}% ", ttr->amount()));
	}
}

// -----------------------------------------------------------------------------
// Updates the translation range [index] in the list
// -----------------------------------------------------------------------------
void TranslationEditorDialog::updateListItem(int index) const
{
	// Check index
	if (index < 0 || index >= static_cast<int>(list_translations_->GetCount()))
		return;

	// Get translation range
	auto tr = translation_->range(index);

	// Update list item text
	if (tr)
	{
		list_translations_->Delete(index);
		list_translations_->Insert(wxString::FromUTF8(tr->asText()), index);
		list_translations_->SetSelection(index);
	}
}

// -----------------------------------------------------------------------------
// Sets the current translation range's destination starting colour to [col]
// -----------------------------------------------------------------------------
void TranslationEditorDialog::setStartColour(const ColRGBA& col) const
{
	// Get currently selected translation range
	auto tr = translation_->range(list_translations_->GetSelection());

	// Check its type
	if (tr->type() == TransRange::Type::Colour)
	{
		// Colour range
		auto tcr = dynamic_cast<TransRangeColour*>(tr);

		// Set destination start colour
		tcr->setStartColour(col);
	}
	else if (tr->type() == TransRange::Type::Desat)
	{
		// Desaturated colour range
		auto tdr = dynamic_cast<TransRangeDesat*>(tr);

		// Determine start colour
		float sr = math::clamp(col.r / 127.0f, 0, 2);
		float sg = math::clamp(col.g / 127.0f, 0, 2);
		float sb = math::clamp(col.b / 127.0f, 0, 2);

		// Set destination start colour
		tdr->setRGBStart(sr, sg, sb);
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
void TranslationEditorDialog::setEndColour(const ColRGBA& col) const
{
	// Get currently selected translation range
	auto tr = translation_->range(list_translations_->GetSelection());

	// Check its type
	if (tr->type() == TransRange::Type::Colour)
	{
		// Colour range
		auto tcr = dynamic_cast<TransRangeColour*>(tr);

		// Set destination end colour
		tcr->setEndColour(col);
	}
	else if (tr->type() == TransRange::Type::Desat)
	{
		// Desaturated colour range
		auto tdr = dynamic_cast<TransRangeDesat*>(tr);

		// Determine end colour
		float er = math::clamp(col.r / 127.0f, 0, 2);
		float eg = math::clamp(col.g / 127.0f, 0, 2);
		float eb = math::clamp(col.b / 127.0f, 0, 2);

		// Set destination end colour
		tdr->setRGBEnd(er, eg, eb);
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
void TranslationEditorDialog::setTintColour(const ColRGBA& col) const
{
	// Get currently selected translation range
	auto tr = translation_->range(list_translations_->GetSelection());

	// Check its type
	if (tr->type() == TransRange::Type::Blend)
	{
		// Colour range
		auto tcr = dynamic_cast<TransRangeBlend*>(tr);

		// Set destination end colour
		tcr->setColour(col);
	}
	else if (tr->type() == TransRange::Type::Tint)
	{
		// Desaturated colour range
		auto ttr = dynamic_cast<TransRangeTint*>(tr);

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
void TranslationEditorDialog::setTintAmount(int amount) const
{
	// Get currently selected translation range
	auto tr = translation_->range(list_translations_->GetSelection());

	// Check its type
	if (tr->type() == TransRange::Type::Tint)

	{
		// Desaturated colour range
		auto ttr = dynamic_cast<TransRangeTint*>(tr);

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
void TranslationEditorDialog::updatePreviews() const
{
	// Update palette preview
	Palette pal_translated{ *palette_ };
	pal_translated.applyTranslation(translation_.get());
	pal_canvas_preview_->setPalette(&pal_translated);
	pal_canvas_preview_->Refresh();

	// Update image preview
	gfx_preview_->image().copyImage(image_preview_.get());
	gfx_preview_->image().applyTranslation(translation_.get(), palette_.get(), cb_truecolor_->GetValue());

	// Update UI
	gfx_preview_->resetViewOffsets();
	gfx_preview_->window()->Refresh();

	// Update text string
	if (cb_paletteonly_->GetValue())
	{
		// Create a palette image
		SImage img(SImage::Type::PalMask);
		img.create(256, 1, SImage::Type::PalMask, palette_.get());
		for (int i = 0; i < 256; ++i)
			img.setPixel(i, 0, i);
		// Apply translation to image
		img.applyTranslation(translation_.get(), palette_.get());
		// Create new translation from image data
		Translation newtrans;
		MemChunk    mc;
		if (img.putIndexedData(mc))
		{
			newtrans.read(mc.data());
			text_string_->SetValue(wxString::FromUTF8(newtrans.asText()));
		}
	}
	else
		text_string_->SetValue(wxString::FromUTF8(translation_->asText()));
}

// -----------------------------------------------------------------------------
// Returns whether the truecolor checkbox is checked
// -----------------------------------------------------------------------------
bool TranslationEditorDialog::getTruecolor() const
{
	return cb_truecolor_->GetValue();
}


// -----------------------------------------------------------------------------
//
// TranslationEditorDialog Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onSize(wxSizeEvent& e)
{
	// Update image preview
	gfx_preview_->zoomToFit(true, 0.05);

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
		translation_->removeRange(index);

		// Recreate it
		translation_->addRange(TransRange::Type::Palette, index);
		auto tr = dynamic_cast<TransRangePalette*>(translation_->range(index));
		// Origin range
		tr->setStart(pal_canvas_original_->selectionStart());
		tr->setEnd(pal_canvas_original_->selectionEnd());
		// Target range
		if (cb_target_reverse_->GetValue())
		{
			tr->setDEnd(pal_canvas_target_->selectionStart());
			tr->setDStart(pal_canvas_target_->selectionEnd());
		}
		else
		{
			tr->setDStart(pal_canvas_target_->selectionStart());
			tr->setDEnd(pal_canvas_target_->selectionEnd());
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
		translation_->removeRange(index);

		// Recreate it
		translation_->addRange(TransRange::Type::Colour, index);
		auto tr = dynamic_cast<TransRangeColour*>(translation_->range(index));
		// Origin range
		tr->setStart(pal_canvas_original_->selectionStart());
		tr->setEnd(pal_canvas_original_->selectionEnd());
		// Target colour gradient
		tr->setStartColour(cb_range_begin_->colour());
		tr->setEndColour(cb_range_end_->colour());

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
		translation_->removeRange(index);

		// Recreate it
		translation_->addRange(TransRange::Type::Desat, index);
		auto tr = dynamic_cast<TransRangeDesat*>(translation_->range(index));
		// Origin range
		tr->setStart(pal_canvas_original_->selectionStart());
		tr->setEnd(pal_canvas_original_->selectionEnd());
		// Target colour gradient
		auto sc = cb_range_begin_->colour();
		auto ec = cb_range_end_->colour();
		tr->setRGBStart(
			math::clamp(static_cast<double>(sc.r) / 127.0f, 0, 2),
			math::clamp(static_cast<double>(sc.g) / 127.0f, 0, 2),
			math::clamp(static_cast<double>(sc.b) / 127.0f, 0, 2));
		tr->setRGBEnd(
			math::clamp(static_cast<double>(ec.r) / 127.0f, 0, 2),
			math::clamp(static_cast<double>(ec.g) / 127.0f, 0, 2),
			math::clamp(static_cast<double>(ec.b) / 127.0f, 0, 2));

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
		translation_->removeRange(index);

		// Recreate it
		translation_->addRange(TransRange::Type::Blend, index);
		auto tr = dynamic_cast<TransRangeBlend*>(translation_->range(index));
		// Origin range
		tr->setStart(pal_canvas_original_->selectionStart());
		tr->setEnd(pal_canvas_original_->selectionEnd());
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
		translation_->removeRange(index);

		// Recreate it
		translation_->addRange(TransRange::Type::Tint, index);
		auto tr = dynamic_cast<TransRangeTint*>(translation_->range(index));
		// Origin range
		tr->setStart(pal_canvas_original_->selectionStart());
		tr->setEnd(pal_canvas_original_->selectionEnd());
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
	label_tint_->SetLabel(WX_FMT("{}% ", slider_tint_->GetValue()));
}

// -----------------------------------------------------------------------------
// Called when the left mouse button is released in the origin palette range
// canvas
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onPalOriginLeftUp(wxMouseEvent& e)
{
	// Get current translation range
	auto tr = translation_->range(list_translations_->GetSelection());

	// Update its origin range
	if (tr)
	{
		tr->setStart(pal_canvas_original_->selectionStart());
		tr->setEnd(pal_canvas_original_->selectionEnd());
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
	auto tr = translation_->range(list_translations_->GetSelection());

	// Update its target range if it's a palette translation
	if (tr && tr->type() == TransRange::Type::Palette)
	{
		auto tpr = dynamic_cast<TransRangePalette*>(tr);
		if (cb_target_reverse_->GetValue())
		{
			tpr->setDEnd(pal_canvas_target_->selectionStart());
			tpr->setDStart(pal_canvas_target_->selectionEnd());
		}
		else
		{
			tpr->setDStart(pal_canvas_target_->selectionStart());
			tpr->setDEnd(pal_canvas_target_->selectionEnd());
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
		translation_->addRange(TransRange::Type::Palette, index);
	else if (rb_type_colour_->GetValue())
		translation_->addRange(TransRange::Type::Colour, index);
	else if (rb_type_desaturate_->GetValue())
		translation_->addRange(TransRange::Type::Desat, index);
	else if (rb_type_colourise_->GetValue())
		translation_->addRange(TransRange::Type::Blend, index);
	else if (rb_type_tint_->GetValue())
		translation_->addRange(TransRange::Type::Tint, index);

	// Add it to the list
	list_translations_->Insert(wxString::FromUTF8(translation_->range(index)->asText()), index);

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
	if (translation_->nRanges() == 1)
		return;

	// Get index of range to remove
	int index = list_translations_->GetSelection();

	// Remove it from the translation
	translation_->removeRange(index);

	// Remove it from the list
	list_translations_->Delete(index);

	// Update UI
	if (index >= static_cast<int>(list_translations_->GetCount()))
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
	translation_->swapRanges(index, index - 1);
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
	if (index >= static_cast<int>(translation_->nRanges()) - 1)
		return;

	// Swap item with above
	translation_->swapRanges(index, index + 1);
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
	auto dir = app::path("translations", app::Dir::User);

	// Create open file dialog
	wxFileDialog dialog_open(
		this,
		wxS("Load Translation from File"),
		wxString::FromUTF8(dir),
		wxEmptyString,
		wxS("Text Files (*.txt)|*.txt"),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_open.ShowModal() == wxID_OK)
	{
		// Load selected file to string
		wxFile   file;
		wxString tstring;
		if (file.Open(dialog_open.GetPath()))
			file.ReadAll(&tstring);

		// Parse as a translation
		Translation trans;
		trans.parse(tstring.utf8_string());

		// Open it if parsed ok
		if (trans.nRanges() > 0)
			openTranslation(trans);
		else
			wxMessageBox(wxS("Not a valid translation file"), wxS("Error"), wxICON_ERROR);
	}
}

// -----------------------------------------------------------------------------
// Called when the 'Save Translation' button is clicked
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onBtnSave(wxCommandEvent& e)
{
	// If the directory doesn't exist create it
	auto dir = app::path("translations", app::Dir::User);
	if (!fileutil::dirExists(dir))
		fileutil::createDir(dir);

	// Create save file dialog
	wxFileDialog dialog_save(
		this,
		wxS("Save Translation to File"),
		wxString::FromUTF8(dir),
		wxEmptyString,
		wxS("Text File (*.txt)|*.txt"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
		wxDefaultPosition);

	// Run the dialog & check that the user didn't cancel
	if (dialog_save.ShowModal() == wxID_OK)
	{
		// Get translation as text string
		auto str = translation_->asText();

		// Open file for writing
		SFile file(dialog_save.GetPath().utf8_string(), SFile::Mode::Write);

		// Write string to file
		file.writeStr(str);

		// Close file
		file.close();
	}
}

// -----------------------------------------------------------------------------
// Called when the mouse pointer is moved over the gfx preview canvas
// -----------------------------------------------------------------------------
void TranslationEditorDialog::onGfxPreviewMouseMotion(wxMouseEvent& e)
{
	// Get the image coordinates at the mouse pointer
	auto pos = gfx_preview_->imageCoords(e.GetX() * GetContentScaleFactor(), e.GetY() * GetContentScaleFactor() - 2);

	int index = pal_canvas_preview_->selectionStart();

	// Get palette index at position
	if (pos.x >= 0)
		index = gfx_preview_->image().pixelIndexAt(pos.x, pos.y);
	else
		index = -1;

	// Update preview palette if necessary
	if (index != pal_canvas_preview_->selectionStart())
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
	auto tr = translation_->range(list_translations_->GetSelection());

	// Update its target range if it's a palette translation
	if (tr && tr->type() == TransRange::Type::Palette)
	{
		auto tpr = dynamic_cast<TransRangePalette*>(tr);
		if (cb_target_reverse_->GetValue())
		{
			tpr->setDEnd(pal_canvas_target_->selectionStart());
			tpr->setDStart(pal_canvas_target_->selectionEnd());
		}
		else
		{
			tpr->setDStart(pal_canvas_target_->selectionStart());
			tpr->setDEnd(pal_canvas_target_->selectionEnd());
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
