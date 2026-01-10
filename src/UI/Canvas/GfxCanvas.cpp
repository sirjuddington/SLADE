
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxCanvas.cpp
// Description: GfxCanvas class. A canvas that displays an image and can take
//              offsets into account etc
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
#include "GfxCanvas.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/WxGfx.h"
#include "Utility/MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, gfx_arc)
EXTERN_CVAR(Bool, gfx_hilight_mouseover)
EXTERN_CVAR(Bool, gfx_show_border)


// -----------------------------------------------------------------------------
//
// GfxCanvas Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// GfxCanvas class constructor
// -----------------------------------------------------------------------------
GfxCanvas::GfxCanvas(wxWindow* parent) : wxPanel(parent)
{
	view_.setCentered(true);
	resetViewOffsets();

	SetDoubleBuffered(true);

	// Bind Events
	Bind(wxEVT_PAINT, &GfxCanvas::onPaint, this);
	Bind(wxEVT_LEFT_DOWN, &GfxCanvas::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &GfxCanvas::onMouseRightDown, this);
	Bind(wxEVT_LEFT_UP, &GfxCanvas::onMouseLeftUp, this);
	Bind(wxEVT_MOTION, &GfxCanvas::onMouseMovement, this);
	Bind(wxEVT_LEAVE_WINDOW, &GfxCanvas::onMouseLeaving, this);
	Bind(wxEVT_MOUSEWHEEL, &GfxCanvas::onMouseWheel, this);
	Bind(wxEVT_KEY_DOWN, &GfxCanvas::onKeyDown, this);

	// Update on resize
	Bind(
		wxEVT_SIZE,
		[this](wxSizeEvent&)
		{
			view_.setSize(ToPhys(GetSize().x), ToPhys(GetSize().y));
			Refresh();
		});

	// Update buffer when the image changes
	sc_image_changed_ = image_->signals().image_changed.connect([this]() { update_image_ = true; });
}

// -----------------------------------------------------------------------------
// GfxCanvas class destructor
// -----------------------------------------------------------------------------
GfxCanvas::~GfxCanvas() = default;

// -----------------------------------------------------------------------------
// Sets the canvas palette to [pal]
// -----------------------------------------------------------------------------
void GfxCanvas::setPalette(const Palette* pal)
{
	if (!palette_)
		palette_ = std::make_unique<Palette>(*pal);
	else
		palette_->copyPalette(pal);

	update_image_ = true;
}

// -----------------------------------------------------------------------------
// Creates a mask texture of the brush to preview its effect
// -----------------------------------------------------------------------------
void GfxCanvas::generateBrushShadow()
{
	if (brush_ == nullptr)
		return;

	// Generate image
	SImage img;
	generateBrushShadowImage(img);

	// Create wx image
	auto wx_img = wxgfx::createImageFromSImage(img, palette_.get());

	// Resize if nearest interpolation not supported
	if (!wxgfx::nearestInterpolationSupported())
		wx_img = wx_img.Scale(img.width() * view_.scale().x, img.height() * view_.scale().y, wxIMAGE_QUALITY_NEAREST);

	// Load it to the brush bitmap
	brush_bitmap_ = wxBitmap(wx_img);
}

// -----------------------------------------------------------------------------
// Returns true if the image bitmap needs to be updated
// -----------------------------------------------------------------------------
bool GfxCanvas::shouldUpdateImage() const
{
	if (update_image_)
		return true;

	// Check if resize required
	if (!wxgfx::nearestInterpolationSupported() && image_bitmap_.GetWidth() != image_->width() * view_.scale().x
		|| image_bitmap_.GetHeight() != image_->height() * view_.scale().y)
		return true;

	return false;
}

// -----------------------------------------------------------------------------
// Updates the wx bitmap(s) for the image and other related data
// -----------------------------------------------------------------------------
void GfxCanvas::updateImage(bool hilight)
{
	if (!image_->isValid())
		return;

	// If the image change isn't caused by drawing, resize drawing mask
	if (!drawing_)
	{
		delete[] drawing_mask_;
		drawing_mask_ = new bool[image_->width() * image_->height()];
		memset(drawing_mask_, false, image_->width() * image_->height());
	}

	// Create wx image
	auto img = wxgfx::createImageFromSImage(*image_, palette_.get());
	if (hilight)
		img.ChangeBrightness(0.25); // Hilight if needed

	// Resize the image itself if we can't interpolate correctly (eg. wxGTK/Cairo renderer)
	if (!wxgfx::nearestInterpolationSupported())
		img = img.Scale(img.GetWidth() * view_.scale().x, img.GetHeight() * view_.scale().y, wxIMAGE_QUALITY_NEAREST);

	// Create wx bitmap from image
	image_bitmap_ = wxBitmap(img);

	update_image_    = false;
	image_hilighted_ = hilight;
}

// -----------------------------------------------------------------------------
// Draws the image (and offset drag preview if needed)
// -----------------------------------------------------------------------------
void GfxCanvas::drawImage(const wxgfx::Context& ctx)
{
	auto dragging = drag_origin_.x > 0;
	auto hilight  = show_hilight_ && !dragging && image_hover_ && gfx_hilight_mouseover
				   && editing_mode_ == EditMode::None;

	// Load/update image if needed
	if (shouldUpdateImage() || hilight != image_hilighted_)
		updateImage(hilight);

	// Get top left coord to draw at
	Vec2i tl;
	if (view_type_ == View::Sprite || view_type_ == View::HUD)
	{
		// Apply offsets for sprite/hud view
		tl.x -= image().offset().x;
		tl.y -= image().offset().y;
	}

	// Draw image
	ctx.drawBitmap(image_bitmap_, tl.x, tl.y, dragging ? 0.5 : 1.0, image_->width(), image_->height());

	// Draw brush shadow when in editing mode
	if (editing_mode_ != EditMode::None && brush_bitmap_.IsOk() && cursor_pos_ != Vec2i{ -1, -1 })
		ctx.drawBitmap(brush_bitmap_, tl.x, tl.y, 0.6);

	// Draw dragging image
	if (dragging)
	{
		tl.x += math::scaleInverse(drag_pos_.x - drag_origin_.x, view().scale().x);
		tl.y += math::scaleInverse(drag_pos_.y - drag_origin_.y, view().scale().y);
		ctx.drawBitmap(image_bitmap_, tl.x, tl.y, 1.0, image_->width(), image_->height());
	}

	// Draw outline
	if (gfx_show_border && show_border_)
	{
		ctx.setPen({ 0, 0, 0, 64 });
		ctx.setTransparentBrush();
		ctx.drawRect({ tl.x, tl.y, image_->width(), image_->height(), false });
	}
}

// -----------------------------------------------------------------------------
// Draws the image tiled to fill the canvas
// -----------------------------------------------------------------------------
void GfxCanvas::drawImageTiled(const wxgfx::Context& ctx)
{
	// Load/update image if needed
	if (shouldUpdateImage() || image_hilighted_)
		updateImage(false);

	// Draw image multiple times to fill canvas
	auto left   = view().canvasX(0);
	auto y      = view().canvasY(0);
	auto right  = view().canvasX(ToPhys(GetSize().x));
	auto bottom = view().canvasY(ToPhys(GetSize().y));
	while (y < bottom)
	{
		auto x = left;
		while (x < right)
		{
			ctx.drawBitmap(image_bitmap_, x, y);
			x += image_->width();
		}

		y += image_->height();
	}
}

// -----------------------------------------------------------------------------
// Draws the current cropping rectangle overlay
// -----------------------------------------------------------------------------
void GfxCanvas::drawCropRect(const wxgfx::Context& ctx) const
{
	Recti vr{ view_.visibleRegion().tl, view_.visibleRegion().br };

	// Expand visible region by 1 pixel to ensure everything is drawn right to the edges
	vr.tl.x--;
	vr.tl.y--;
	vr.br.x++;
	vr.br.y++;

	// Draw cropping lines
	ctx.setPen(ColRGBA::BLACK);
	ctx.drawLine(crop_rect_->left(), vr.top(), crop_rect_->left(), vr.bottom());     // Left
	ctx.drawLine(vr.left(), crop_rect_->top(), vr.right(), crop_rect_->top());       // Top
	ctx.drawLine(crop_rect_->right(), vr.top(), crop_rect_->right(), vr.bottom());   // Right
	ctx.drawLine(vr.left(), crop_rect_->bottom(), vr.right(), crop_rect_->bottom()); // Bottom

	// Shade cropped-out area
	ctx.setTransparentPen();
	ctx.setBrush({ 0, 0, 0, 100 });
	ctx.drawRect(vr.left(), vr.top(), crop_rect_->left() - vr.left(), vr.height());                // Left
	ctx.drawRect(crop_rect_->right(), vr.top(), vr.right() - crop_rect_->right(), vr.height());    // Right
	ctx.drawRect(crop_rect_->left(), vr.top(), crop_rect_->width(), crop_rect_->top() - vr.top()); // Top
	ctx.drawRect(
		crop_rect_->left(), crop_rect_->bottom(), crop_rect_->width(), vr.bottom() - crop_rect_->bottom()); // Bottom
}


// -----------------------------------------------------------------------------
//
// GfxCanvas Class Events
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Called when the canvas requires redrawing
// -----------------------------------------------------------------------------
void GfxCanvas::onPaint(wxPaintEvent& e)
{
	auto dc  = wxPaintDC(this);
	auto ctx = wxgfx::Context{ dc, &view_ };

	// Background
	wxgfx::generateCheckeredBackground(background_bitmap_, view_.size().x, view_.size().y);
	ctx.drawBitmap(background_bitmap_, 0, 0);

	// Aspect Ratio Correction
	if (gfx_arc)
		view_.setScale({ view_.scale().x, view_.scale().x * 1.2 });
	else
		view_.setScale(view_.scale().x);

	// Apply view to wxGraphicsContext
	ctx.applyView();

	// Offset/guide lines
	ctx.drawOffsetLines(view_type_);

	// Image
	if (image_->isValid())
	{
		ctx.gc->SetInterpolationQuality(wxINTERPOLATION_NONE);
		if (editing_mode_ == GfxEditMode::None && view_type_ == View::Tiled)
			drawImageTiled(ctx);
		else
			drawImage(ctx);
	}

	// Cropping overlay
	if (crop_rect_)
		drawCropRect(ctx);
}
