
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "GfxCanvasBase.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/WxGfx.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, tx_arc)


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
CTextureCanvas::CTextureCanvas(wxWindow* parent) : wxPanel(parent), palette_{ new Palette }
{
	SetDoubleBuffered(true);

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
// Clear the patch at [index]'s image data so it is reloaded next draw
// -----------------------------------------------------------------------------
void CTextureCanvas::refreshPatch(unsigned index)
{
	CTextureCanvasBase::refreshPatch(index);

	if (index < patch_bitmaps_.size())
		patch_bitmaps_[index] = wxBitmap();
}

// -----------------------------------------------------------------------------
// Draws the currently opened composite texture
// -----------------------------------------------------------------------------
void CTextureCanvas::drawTexture(wxgfx::Context& ctx, Vec2d scale, Vec2i offset, bool draw_patches)
{
	// Draw all individual patches if needed (eg. while dragging or 'draw outside' is enabled)
	if (draw_patches)
	{
		for (uint32_t a = 0; a < texture_->nPatches(); a++)
			drawPatch(ctx, a);
	}

	// If we aren't currently dragging a patch, draw the fully generated texture
	if (!dragging_)
	{
		// Generate wxBitmap if needed
		if (!tex_preview_ || !tex_bitmap_.IsOk())
		{
			loadTexturePreview();
			sImageToBitmap(*tex_preview_, palette_.get(), tex_bitmap_, view_.scale());
		}

		// Draw the texture
		ctx.drawBitmap(tex_bitmap_, offset.x, offset.y, 1.0, texture_->width() * scale.x, texture_->height() * scale.y);
	}
}

// -----------------------------------------------------------------------------
// Draws a black border around the texture w/ticks, and a grid if dragging
// -----------------------------------------------------------------------------
void CTextureCanvas::drawTextureBorder(wxgfx::Context& ctx, Vec2d scale, Vec2i offset) const
{
	constexpr float ext = 0.0f;
	const auto      x1  = offset.x;
	const auto      x2  = offset.x + texture_->width() * scale.x;
	const auto      y1  = offset.y;
	const auto      y2  = offset.y + texture_->height() * scale.y;

	// Border
	ctx.setPen({ 0, 0, 0 }, 2.0);
	ctx.drawLine(x1 - ext, y1 - ext, x1 - ext, y2 + ext);
	ctx.drawLine(x1 - ext, y2 + ext, x2 + ext, y2 + ext);
	ctx.drawLine(x2 + ext, y2 + ext, x2 + ext, y1 - ext);
	ctx.drawLine(x2 + ext, y1 - ext, x1 - ext, y1 - ext);

	// Vertical ticks
	ctx.setPen({ 0, 0, 0, 150 });
	for (auto y = y1; y <= y2; y += 8.0)
	{
		ctx.drawLine(x1 - 4, y, x1, y);
		ctx.drawLine(x2, y, x2 + 4, y);
	}

	// Horizontal ticks
	for (auto x = x1; x <= x2; x += 8.0)
	{
		ctx.drawLine(x, y1 - 4, x, y1);
		ctx.drawLine(x, y2, x, y2 + 4);
	}

	// Grid
	if (show_grid_)
	{
		auto cm = ctx.gc->GetCompositionMode();
		ctx.gc->SetCompositionMode(wxCOMPOSITION_XOR);
		ctx.setPen({ 255, 255, 255, 128 });
		for (auto y = y1 + 8; y <= y2 - 8; y += 8)
			ctx.drawLine(x1, y, x2, y);
		for (auto x = x1 + 8; x <= x2 - 8; x += 8)
			ctx.drawLine(x, y1, x, y2);
		ctx.gc->SetCompositionMode(cm);
	}
}

// -----------------------------------------------------------------------------
// Draws the patch at index [num] in the composite texture
// -----------------------------------------------------------------------------
void CTextureCanvas::drawPatch(const wxgfx::Context& ctx, int index)
{
	// Get patch to draw
	const auto patch = texture_->patch(index);
	if (!patch)
		return;

	// Load the patch as an opengl texture if it isn't already
	auto patch_image = patches_[index].image.get();
	if (!patch_image || !patch_bitmaps_[index].IsOk())
	{
		loadPatchImage(index);
		patch_image = patches_[index].image.get();
		sImageToBitmap(*patch_image, palette_.get(), patch_bitmaps_[index], view_.scale());
	}

	// Draw patch
	ctx.drawBitmap(
		patch_bitmaps_[index], patch->xOffset(), patch->yOffset(), 1.0, patch_image->width(), patch_image->height());
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

	// Aspect Ratio Correction
	if (tx_arc)
		view_.setScale({ view_.scale().x, view_.scale().x * 1.2 });
	else
		view_.setScale(view_.scale().x);

	// Apply view
	ctx.applyView();

	// Draw offset guides if needed
	if (view_type_ != View::Normal)
		ctx.drawOffsetLines(view_type_ == View::Sprite ? GfxView::Sprite : GfxView::HUD);

	if (!texture_)
		return;

	// Determine offset/scale
	Vec2d offset{ 0.0 }, scale{ 1.0 };
	if (tex_scale_)
	{
		// Apply texture scale
		auto tscalex = texture_->scaleX();
		if (tscalex == 0.0f)
			tscalex = 1.0f;
		auto tscaley = texture_->scaleY();
		if (tscaley == 0.0f)
			tscaley = 1.0f;
		scale = { 1.0f / tscalex, 1.0f / tscaley };
	}
	if (view_type_ != View::Normal)
	{
		// Apply texture offsets
		offset.x = texture_->offsetX();
		offset.y = texture_->offsetY();
	}

	// Init/update patch bitmap list if needed
	if (patch_bitmaps_.size() != texture_->nPatches())
		patch_bitmaps_.resize(texture_->nPatches());

	// Load patch images
	for (unsigned i = 0; i < patches_.size(); ++i)
		if (!patches_[i].image)
		{
			loadPatchImage(i);
			patch_bitmaps_[i] = wxBitmap();
		}

	// Draw the texture
	drawTexture(ctx, scale, offset, draw_outside_ || dragging_);
	drawTextureBorder(ctx, scale, offset);

	// Draw selected patch outlines
	ctx.setPen({ 70, 210, 220 }, 2.0);
	for (unsigned a = 0; a < patches_.size(); a++)
		if (patches_[a].selected)
		{
			auto patch = texture_->patch(a);
			ctx.drawRect(patch->xOffset(), patch->yOffset(), patches_[a].image->width(), patches_[a].image->height());
		}

	// Draw hilighted patch outline
	if (hilight_patch_ >= 0 && hilight_patch_ < static_cast<int>(texture_->nPatches()))
	{
		auto cm = ctx.gc->GetCompositionMode();
		ctx.gc->SetCompositionMode(wxCOMPOSITION_ADD);
		ctx.setPen({ 255, 255, 255, 150 }, 2.0);
		auto patch = texture_->patch(hilight_patch_);
		ctx.drawRect(
			patch->xOffset(),
			patch->yOffset(),
			patches_[hilight_patch_].image->width(),
			patches_[hilight_patch_].image->height());
		ctx.gc->SetCompositionMode(cm);
	}
}
