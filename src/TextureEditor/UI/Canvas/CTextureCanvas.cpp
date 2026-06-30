
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CTextureCanvas.cpp
// Description: A canvas that displays a composite texture
//              (ie from doom's TEXTUREx)
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
#include "CTextureCanvas.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/WxGfx.h"
#include "OpenGL/Draw2D.h"
#include "UI/Canvas/GfxCanvasBase.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
void sImageToBitmap(const SImage& image, const Palette* palette, wxBitmap& bitmap, const Vec2d& scale)
{
	auto img = wxgfx::createImageFromSImage(image, palette);

	// Resize the image itself if we can't interpolate correctly (eg. wxGTK/Cairo renderer)
	if (!wxgfx::nearestInterpolationSupported())
		img = img.Scale(img.GetWidth() * scale.x, img.GetHeight() * scale.y, wxIMAGE_QUALITY_NEAREST);

	bitmap = img;
}
} // namespace


// -----------------------------------------------------------------------------
//
// CTextureCanvas Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// CTextureCanvas class constructor
// -----------------------------------------------------------------------------
CTextureCanvas::CTextureCanvas(wxWindow* parent) : Canvas(parent), palette_{ new Palette }
{
	view_.setCentered(true);

	// Bind Events
	Bind(wxEVT_PAINT, &CTextureCanvas::onPaint, this);
	Bind(wxEVT_MOTION, &CTextureCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &CTextureCanvas::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &CTextureCanvas::onMouseEvent, this);
	Bind(wxEVT_MOUSEWHEEL, &CTextureCanvas::onMouseEvent, this);

	// Update on resize
	Bind(
		wxEVT_SIZE,
		[this](wxSizeEvent&)
		{
			view_.setSize(ToPhys(GetSize().x), ToPhys(GetSize().y));
			Refresh();
		});
}

// -----------------------------------------------------------------------------
// CTextureCanvas class destructor
// -----------------------------------------------------------------------------
CTextureCanvas::~CTextureCanvas() = default;

// -----------------------------------------------------------------------------
// Sets the canvas palette
// -----------------------------------------------------------------------------
void CTextureCanvas::setPalette(const Palette* pal)
{
	palette_->copyPalette(pal);
}

// -----------------------------------------------------------------------------
// Override of CTextureCanvasBase::clearTexture to also clear the cached texture
// bitmap
// -----------------------------------------------------------------------------
void CTextureCanvas::clearTexture()
{
	CTextureCanvasBase::clearTexture();
	tex_bitmap_ = wxBitmap();
}

// -----------------------------------------------------------------------------
// Override of CTextureCanvasBase::clearPatches to also clear the cached patch
// bitmaps
// -----------------------------------------------------------------------------
void CTextureCanvas::clearPatches()
{
	CTextureCanvasBase::clearPatches();
	patch_bitmaps_.clear();
}

// -----------------------------------------------------------------------------
// Clear the patch at [index]'s image data so it is reloaded next draw
// -----------------------------------------------------------------------------
void CTextureCanvas::refreshPatch(unsigned index)
{
	CTextureCanvasBase::refreshPatch(index);

	if (index < patch_bitmaps_.size())
		patch_bitmaps_[index] = wxBitmap();
}

// -----------------------------------------------------------------------------
// Initializes the context and patch bitmaps used for drawing
// -----------------------------------------------------------------------------
void CTextureCanvas::initDrawing(const Rectd& tex_rect)
{
	// Apply view
	gc_->applyView();

	// Init/update patch bitmap list if needed
	if (patch_bitmaps_.size() != texture_->nPatches())
		patch_bitmaps_.resize(texture_->nPatches());
}

// -----------------------------------------------------------------------------
// Draws the offset center lines for the current view type
// -----------------------------------------------------------------------------
void CTextureCanvas::drawOffsetLines()
{
	if (view_type_ != View::Normal)
		gc_->drawOffsetLines(view_type_ == View::Sprite ? GfxView::Sprite : GfxView::HUD);
}

// -----------------------------------------------------------------------------
// Draws the full generated texture
// -----------------------------------------------------------------------------
void CTextureCanvas::drawTexture(const Rectd& tex_rect)
{
	// Generate if needed
	if (!tex_bitmap_.IsOk())
	{
		loadTexturePreview();
		sImageToBitmap(*tex_preview_, palette_.get(), tex_bitmap_, view_.scale());
	}

	gc_->drawBitmap(tex_bitmap_, tex_rect.x1(), tex_rect.y1(), 1.0, tex_rect.width(), tex_rect.height());
}

// -----------------------------------------------------------------------------
// Draws a border around the texture and grid ticks at the grid size intervals
// -----------------------------------------------------------------------------
void CTextureCanvas::drawTextureBorder(const Rectd& tex_rect)
{
	const auto x1 = tex_rect.x1();
	const auto x2 = tex_rect.x2();
	const auto y1 = tex_rect.y1();
	const auto y2 = tex_rect.y2();

	// Border
	gc_->setPen({ 0, 0, 0 }, 2.0);
	gc_->drawLine(x1, y1, x1, y2);
	gc_->drawLine(x1, y2, x2, y2);
	gc_->drawLine(x2, y2, x2, y1);
	gc_->drawLine(x2, y1, x1, y1);

	// Vertical ticks
	gc_->setPen({ 0, 0, 0, 150 });
	for (auto y = y1; y <= y2; y += grid_size_.y)
	{
		gc_->drawLine(x1 - 4, y, x1, y);
		gc_->drawLine(x2, y, x2 + 4, y);
	}

	// Horizontal ticks
	for (auto x = x1; x <= x2; x += grid_size_.x)
	{
		gc_->drawLine(x, y1 - 4, x, y1);
		gc_->drawLine(x, y2, x, y2 + 4);
	}
}

// -----------------------------------------------------------------------------
// Draws grid lines across the texture
// -----------------------------------------------------------------------------
void CTextureCanvas::drawTextureGrid(const Rectd& tex_rect)
{
	const auto x1 = tex_rect.x1();
	const auto x2 = tex_rect.x2();
	const auto y1 = tex_rect.y1();
	const auto y2 = tex_rect.y2();

	auto cm = gc_->gc->GetCompositionMode();
	gc_->gc->SetCompositionMode(wxCOMPOSITION_XOR);
	gc_->setPen({ 255, 255, 255, 128 });
	for (auto y = y1 + grid_size_.y; y <= y2 - grid_size_.y; y += grid_size_.y)
		gc_->drawLine(x1, y, x2, y);
	for (auto x = x1 + grid_size_.x; x <= x2 - grid_size_.x; x += grid_size_.x)
		gc_->drawLine(x, y1, x, y2);
	gc_->gc->SetCompositionMode(cm);
}

// -----------------------------------------------------------------------------
// Draws patch [index] within [patch_rect] with the given [alpha].
// If [highlight] is true, we are drawing the patch highlight overlay
// -----------------------------------------------------------------------------
void CTextureCanvas::drawPatch(const Rectd& patch_rect, int index, float alpha, bool highlight)
{
	if (highlight)
	{
		// Additive blending isn't supported by wxGraphicsContext so just draw a
		// plain white overlay
		gc_->setBrush({ 255, 255, 255, 8 });
		gc_->drawRect(patch_rect.x1(), patch_rect.y1(), patch_rect.width(), patch_rect.height());
		return;
	}

	// Load the patch as a bitmap if it isn't already
	auto patch_image = patches_[index].image.get();
	if (!patch_image || !patch_bitmaps_[index].IsOk())
	{
		loadPatchImage(index);
		patch_image = patches_[index].image.get();
		sImageToBitmap(*patch_image, palette_.get(), patch_bitmaps_[index], view_.scale());
	}

	// Draw patch
	gc_->drawBitmap(
		patch_bitmaps_[index], patch_rect.x1(), patch_rect.y1(), alpha, patch_rect.width(), patch_rect.height());
}

// -----------------------------------------------------------------------------
// Draws an outline around [patch_rect] with the given [colour] and [line_width]
// -----------------------------------------------------------------------------
void CTextureCanvas::drawPatchOutline(const Rectd& patch_rect, const ColRGBA& colour, double line_width)
{
	gc_->setPen(colour, line_width);
	gc_->drawRect(patch_rect.x1(), patch_rect.y1(), patch_rect.width(), patch_rect.height());
}

// -----------------------------------------------------------------------------
// Draws all current text overlays
// -----------------------------------------------------------------------------
void CTextureCanvas::drawTextOverlays()
{
	// TODO: Add drawText (and text options) to wxgfx::Context

	// Set up basic screen view for text drawing
	gl::View screen_view(false, false);
	screen_view.setSize(view_.size().x, view_.size().y);
	gc_->view = &screen_view;
	gc_->applyView();

	// Init font
	auto font = wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	gc_->gc->SetFont(font, *wxWHITE);

	for (const auto& text : texts_)
	{
		auto str = wxString::FromUTF8(text.text);

		// Determine text position
		double w, h;
		gc_->gc->GetTextExtent(str, &w, &h);
		double x = view_.screenX(text.position.x);
		double y = view_.screenY(text.position.y);
		if (text.alignment == gl::draw2d::Align::Center)
			x -= w / 2;
		else if (text.alignment == gl::draw2d::Align::Right)
			x -= w;
		if (text.above)
			y -= h;

		// Draw white text with black drop shadow
		gc_->gc->SetFont(font, *wxBLACK);
		gc_->gc->DrawText(wxString::FromUTF8(text.text.c_str()), x + 1, y + 1);
		gc_->gc->SetFont(font, *wxWHITE);
		gc_->gc->DrawText(wxString::FromUTF8(text.text.c_str()), x, y);
	}

	// Reset view
	gc_->view = &view_;
	gc_->applyView();
}

// -----------------------------------------------------------------------------
// Override of CTextureCanvasBase::loadTexturePreview that also resets the
// cached texture bitmap
// -----------------------------------------------------------------------------
void CTextureCanvas::loadTexturePreview()
{
	CTextureCanvasBase::loadTexturePreview();
	tex_bitmap_ = wxBitmap();
}


// -----------------------------------------------------------------------------
//
// CTextureCanvas Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the canvas requires redrawing
// -----------------------------------------------------------------------------
void CTextureCanvas::onPaint(wxPaintEvent& e)
{
	if (GetSize().x <= 0 || GetSize().y <= 0)
		return;

	auto dc  = wxPaintDC(this);
	auto ctx = wxgfx::Context(dc, &view_);

	// Background
	wxgfx::generateCheckeredBackground(background_bitmap_, view_.size().x, view_.size().y);
	ctx.drawBitmap(background_bitmap_, 0, 0);

	gc_ = &ctx;
	drawContent();
	gc_ = nullptr;
}
