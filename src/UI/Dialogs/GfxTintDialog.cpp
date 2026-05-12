
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxTintDialog.cpp
// Description: A simple dialog for the 'Tint' function, allows the user to
//              select tint colour + amount and shows a preview of the tinted
//              image
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
#include "GfxTintDialog.h"
#include "General/Misc.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Canvas/GL/GfxGLCanvas.h"
#include "UI/Controls/ColourBox.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// GfxTintDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxTintDialog class constructor
// -----------------------------------------------------------------------------
GfxTintDialog::GfxTintDialog(wxWindow* parent, ArchiveEntry* entry, const Palette& pal) :
	wxDialog(parent, -1, wxS("Tint"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	entry_{ entry },
	palette_{ new Palette(pal) }
{
	auto lh = ui::LayoutHelper(this);

	// Set dialog icon
	wxutil::setWindowIcon(this, "tint");

	// Setup main sizer
	auto msizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(msizer);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	msizer->Add(sizer, lh.sfWithLargeBorder(1).Expand());

	// Add colour chooser
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxBOTTOM).Expand());

	cb_colour_ = new ColourBox(this, -1, false, true);
	cb_colour_->setColour(ColRGBA::RED);
	cb_colour_->setPalette(palette_.get());
	hbox->Add(new wxStaticText(this, -1, wxS("Colour:")), lh.sfWithBorder(1, wxRIGHT).CenterVertical());
	hbox->Add(cb_colour_, wxSizerFlags().CenterVertical());

	// Add 'amount' slider
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, lh.sfWithBorder(0, wxBOTTOM).Expand());

	slider_amount_ = new wxSlider(this, -1, 50, 0, 100);
	label_amount_  = new wxStaticText(this, -1, wxS("100%"));
	label_amount_->SetInitialSize(label_amount_->GetBestSize());
	hbox->Add(new wxStaticText(this, -1, wxS("Amount:")), lh.sfWithBorder(0, wxRIGHT).CenterVertical());
	hbox->Add(slider_amount_, lh.sfWithBorder(1, wxRIGHT).Expand());
	hbox->Add(label_amount_, wxSizerFlags().CenterVertical());

	// Add preview
	gfx_preview_ = ui::createGfxCanvas(this);
	sizer->Add(gfx_preview_->window(), lh.sfWithBorder(1, wxBOTTOM).Expand());

	// Add buttons
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Expand());

	// Setup preview
	gfx_preview_->setViewType(GfxView::Centered);
	gfx_preview_->setPalette(palette_.get());
	gfx_preview_->window()->SetInitialSize(lh.size(256, 256));
	misc::loadImageFromEntry(&gfx_preview_->image(), entry);
	gfx_preview_->image().tint(colour(), amount(), palette_.get());
	gfx_preview_->resetViewOffsets();

	// Init layout
	wxTopLevelWindowBase::Layout();

	// Bind events
	cb_colour_->Bind(wxEVT_COLOURBOX_CHANGED, &GfxTintDialog::onColourChanged, this);
	slider_amount_->Bind(wxEVT_SLIDER, &GfxTintDialog::onAmountChanged, this);
	Bind(wxEVT_SIZE, &GfxTintDialog::onResize, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	const wxSize size = GetSize() * wxWindowBase::GetContentScaleFactor();
	wxTopLevelWindowBase::SetMinSize(size);
	CenterOnParent();

	// Set values
	label_amount_->SetLabel(wxS("50% "));
}

// -----------------------------------------------------------------------------
// Returns the currently selected colour
// -----------------------------------------------------------------------------
ColRGBA GfxTintDialog::colour() const
{
	return cb_colour_->colour();
}

// -----------------------------------------------------------------------------
// Returns the currently selected tint amount
// -----------------------------------------------------------------------------
float GfxTintDialog::amount() const
{
	return static_cast<float>(slider_amount_->GetValue()) * 0.01f;
}

// -----------------------------------------------------------------------------
// Sets the colour and tint amount to use
// -----------------------------------------------------------------------------
void GfxTintDialog::setValues(const string& col, int val) const
{
	cb_colour_->setColour(ColRGBA(wxColour(wxString::FromUTF8(col))));
	slider_amount_->SetValue(val);
	label_amount_->SetLabel(WX_FMT("{}% ", slider_amount_->GetValue()));
	gfx_preview_->image().tint(colour(), amount(), palette_.get());
	gfx_preview_->window()->Refresh();
}


// -----------------------------------------------------------------------------
//
// GfxTintDialog Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the selected colour is changed
// -----------------------------------------------------------------------------
void GfxTintDialog::onColourChanged(wxEvent& e)
{
	misc::loadImageFromEntry(&gfx_preview_->image(), entry_);
	gfx_preview_->image().tint(colour(), amount(), palette_.get());
	gfx_preview_->window()->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the tint amount is changed
// -----------------------------------------------------------------------------
void GfxTintDialog::onAmountChanged(wxCommandEvent& e)
{
	misc::loadImageFromEntry(&gfx_preview_->image(), entry_);
	gfx_preview_->image().tint(colour(), amount(), palette_.get());
	gfx_preview_->window()->Refresh();
	label_amount_->SetLabel(WX_FMT("{}% ", slider_amount_->GetValue()));
}

// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void GfxTintDialog::onResize(wxSizeEvent& e)
{
	OnSize(e);
	gfx_preview_->zoomToFit(true, 0.05f);
	e.Skip();
}
