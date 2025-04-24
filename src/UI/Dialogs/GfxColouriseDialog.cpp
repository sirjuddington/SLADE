
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxColouriseDialog.cpp
// Description: A simple dialog for the 'Colourise' function, allows the user to
//              select a colour and shows a preview of the colourised image
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
#include "GfxColouriseDialog.h"
#include "General/Misc.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
#include "UI/Canvas/Canvas.h"
#include "UI/Canvas/GfxCanvasBase.h"
#include "UI/Controls/ColourBox.h"
#include "UI/Layout.h"
#include "UI/WxUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// GfxColouriseDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxColouriseDialog class constructor
// -----------------------------------------------------------------------------
GfxColouriseDialog::GfxColouriseDialog(wxWindow* parent, ArchiveEntry* entry, const Palette& pal) :
	wxDialog(parent, -1, wxS("Colourise"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	entry_{ entry },
	palette_{ new Palette(pal) }
{
	auto lh = ui::LayoutHelper(this);

	// Set dialog icon
	wxutil::setWindowIcon(this, "colourise");

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
	hbox->Add(cb_colour_, wxSizerFlags().Expand());

	// Add preview
	gfx_preview_ = ui::createGfxCanvas(this);
	sizer->Add(gfx_preview_->window(), lh.sfWithBorder(1, wxBOTTOM).Expand());

	// Add buttons
	sizer->Add(wxutil::createDialogButtonBox(this, "Colourise", "Cancel"), wxSizerFlags().Expand());

	// Setup preview
	gfx_preview_->setPalette(palette_.get());
	gfx_preview_->window()->SetInitialSize(lh.size(192, 192));
	misc::loadImageFromEntry(&gfx_preview_->image(), entry);
	auto col = cb_colour_->colour();
	gfx_preview_->image().colourise(col, palette_.get());
	gfx_preview_->setViewType(GfxView::Centered);

	// Init layout
	wxTopLevelWindowBase::Layout();

	// Bind events
	cb_colour_->Bind(wxEVT_COLOURBOX_CHANGED, &GfxColouriseDialog::onColourChanged, this);
	Bind(wxEVT_SIZE, &GfxColouriseDialog::onResize, this);

	// Setup dialog size
	SetInitialSize(wxSize(-1, -1));
	wxTopLevelWindowBase::SetMinSize(GetSize());
	CenterOnParent();
}

// -----------------------------------------------------------------------------
// Returns the currently selected colour
// -----------------------------------------------------------------------------
ColRGBA GfxColouriseDialog::colour() const
{
	return cb_colour_->colour();
}

// -----------------------------------------------------------------------------
// Sets the colour to use
// -----------------------------------------------------------------------------
void GfxColouriseDialog::setColour(const string& col) const
{
	auto rgba = ColRGBA(wxColour(wxString::FromUTF8(col)));
	cb_colour_->setColour(rgba);
	gfx_preview_->image().colourise(rgba, palette_.get());
	gfx_preview_->window()->Refresh();
}


// -----------------------------------------------------------------------------
//
// GfxColouriseDialog Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the selected colour is changed
// -----------------------------------------------------------------------------
void GfxColouriseDialog::onColourChanged(wxEvent& e)
{
	misc::loadImageFromEntry(&gfx_preview_->image(), entry_);
	gfx_preview_->image().colourise(cb_colour_->colour(), palette_.get());
	gfx_preview_->window()->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the dialog is resized
// -----------------------------------------------------------------------------
void GfxColouriseDialog::onResize(wxSizeEvent& e)
{
	OnSize(e);
	gfx_preview_->zoomToFit(true, 0.05);
	e.Skip();
}
