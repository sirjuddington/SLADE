
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxCanvasBase.cpp
// Description: Base class for Gfx(GL)Canvas containing common functionality
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
#include "GfxCanvasBase.h"
#include "General/UI.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "OpenGL/View.h"
#include "UI/Controls/ZoomControl.h"
#include "UI/SBrush.h"
#include "Utility/MathStuff.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
DEFINE_EVENT_TYPE(wxEVT_GFXCANVAS_OFFSET_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_GFXCANVAS_PIXELS_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_GFXCANVAS_COLOUR_PICKED)


// -----------------------------------------------------------------------------
//
// GfxCanvasBase Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// GfxCanvasBase class constructor
// -----------------------------------------------------------------------------
GfxCanvasBase::GfxCanvasBase() : image_{ new SImage } {}

// -----------------------------------------------------------------------------
// GfxCanvasBase class destructor
// -----------------------------------------------------------------------------
GfxCanvasBase::~GfxCanvasBase() = default;

// -----------------------------------------------------------------------------
// Sets the gfx canvas [scale]
// -----------------------------------------------------------------------------
void GfxCanvasBase::setScale(double scale)
{
	if (zoom_point_.x < 0 && zoom_point_.y < 0)
		view().setScale(scale);
	else
		view().setScale(scale, zoom_point_);
}

// -----------------------------------------------------------------------------
// Sets the gfx canvas view [type]
// -----------------------------------------------------------------------------
void GfxCanvasBase::setViewType(View type)
{
	bool changed = view_type_ != type;
	view_type_   = type;
	if (changed)
	{
		view().setCentered(type != View::Tiled);
		resetViewOffsets();
	}
}

// -----------------------------------------------------------------------------
// Scales the image to fit within the gfx canvas.
// If mag is false, the image will not be stretched to fit the canvas
// (only shrunk if needed).
// Leaves a border around the image if <padding> is specified
// (0.0f = no border, 1.0f = border 100% of canvas size)
// -----------------------------------------------------------------------------
void GfxCanvasBase::zoomToFit(bool mag, double padding)
{
	// Determine padding
	const auto   size = view().size();
	const double pad  = static_cast<double>(std::min<int>(size.x, size.y)) * padding;

	// Get image dimensions
	const double x_dim = image_->width();
	const double y_dim = image_->height();

	// Get max scale for x and y (including padding)
	const double x_scale = (static_cast<double>(size.x) - pad) / x_dim;
	const double y_scale = (static_cast<double>(size.y) - pad) / y_dim;

	// Set scale to smallest of the 2 (so that none of the image will be clipped)
	auto scale = std::min<double>(x_scale, y_scale);

	// If we don't want to magnify the image, clamp scale to a max of 1.0
	if (!mag && scale > 1)
		scale = 1;

	view().setScale(scale);
}

// -----------------------------------------------------------------------------
// Resets the view offsets (depending on view type)
// -----------------------------------------------------------------------------
void GfxCanvasBase::resetViewOffsets()
{
	if (view_type_ == View::HUD)
		view().setOffset(160, 100);
	else if (view_type_ == View::Default || view_type_ == View::Centered)
		view().setOffset(image_->width() / 2., image_->height() / 2.);
	else
		view().setOffset(0, 0);
}

// -----------------------------------------------------------------------------
// Returns true if the given coordinates are 'on' top of the image
// -----------------------------------------------------------------------------
bool GfxCanvasBase::onImage(int x, int y) const
{
	// Don't disable in editing mode; it can be quite useful
	// to have a live preview of how a graphic will tile.
	if (view_type_ == View::Tiled && editing_mode_ == EditMode::None)
		return false;

	// No need to duplicate the imageCoords code.
	return imageCoords(x, y) != Vec2i{ -1, -1 };
}

// -----------------------------------------------------------------------------
// Returns the image coordinates at [x,y] in screen coordinates, or [-1, -1] if
// not on the image
// -----------------------------------------------------------------------------
Vec2i GfxCanvasBase::imageCoords(int x, int y) const
{
	auto canvas_pos = view().canvasPos({ x, y });
	auto image_pos  = canvas_pos;

	if (view_type_ == View::Sprite || view_type_ == View::HUD)
	{
		image_pos.x += image_->offset().x;
		image_pos.y += image_->offset().y;
	}

	if (image_pos.x < 0 || image_pos.y < 0 || image_pos.x >= image_->width() || image_pos.y >= image_->height())
		return { -1, -1 }; // Not on image

	return { static_cast<int>(image_pos.x), static_cast<int>(image_pos.y) };
}

// -----------------------------------------------------------------------------
// Finishes an offset drag
// -----------------------------------------------------------------------------
void GfxCanvasBase::endOffsetDrag()
{
	// Get offset
	const auto x = math::scaleInverse(drag_pos_.x - drag_origin_.x, view().scale().x);
	const auto y = math::scaleInverse(drag_pos_.y - drag_origin_.y, view().scale().y);

	// If there was a change
	if (x != 0 || y != 0)
	{
		// Set image offsets
		image_->setXOffset(image_->offset().x - x);
		image_->setYOffset(image_->offset().y - y);

		// Generate event
		wxNotifyEvent e(wxEVT_GFXCANVAS_OFFSET_CHANGED, window()->GetId());
		e.SetEventObject(window());
		window()->GetEventHandler()->ProcessEvent(e);
	}

	// Stop drag
	drag_origin_ = { -1, -1 };
}

// -----------------------------------------------------------------------------
// Paints a pixel from the image at the given image coordinates.
// -----------------------------------------------------------------------------
void GfxCanvasBase::paintPixel(int x, int y)
{
	// With large brushes, it's very possible that some of the pixels
	// are out of the image area; so don't process them.
	if (x < 0 || y < 0 || x >= image_->width() || y >= image_->height())
		return;

	// Do not process pixels that were already modified in the
	// current drawing operation. This mechanism is needed to
	// allow freehand drawing, because an unpredictable number
	// of mouse events can happen while the mouse moves, leading
	// to the same pixel being processed over and over, and that
	// does not play well when applying translations.
	const size_t pos = x + image_->width() * y;
	if (drawing_mask_[pos])
		return;

	bool painted = false;
	if (editing_mode_ == EditMode::Erase) // eraser
		painted = image_->setPixel(x, y, 255, 0);
	else if (editing_mode_ == EditMode::Translate) // translator
	{
		if (translation_ != nullptr)
		{
			const auto    ocol  = image_->pixelAt(x, y, palette());
			const uint8_t alpha = ocol.a;
			auto          ncol  = (translation_->translate(ocol, palette()));
			ncol.a              = alpha;
			if (!ocol.equals(ncol, false, true))
				painted = image_->setPixel(x, y, ncol);
		}
	}
	else
		painted = image_->setPixel(x, y, paint_colour_);

	// Mark the modification, if any, and announce the modification
	drawing_mask_[pos] = painted;
	if (painted)
	{
		// Generate event
		wxNotifyEvent e(wxEVT_GFXCANVAS_PIXELS_CHANGED, window()->GetId());
		e.SetEventObject(window());
		window()->GetEventHandler()->ProcessEvent(e);
	}
}

// -----------------------------------------------------------------------------
// Finds all the pixels under the brush, and paints them.
// -----------------------------------------------------------------------------
void GfxCanvasBase::brushCanvas(int x, int y)
{
	if (brush_ == nullptr)
		return;
	const auto coord = imageCoords(x, y);
	for (int i = -4; i < 5; ++i)
		for (int j = -4; j < 5; ++j)
			if (brush_->pixel(i, j))
				paintPixel(coord.x + i, coord.y + j);
}

// -----------------------------------------------------------------------------
// Finds the pixel under the cursor, and picks its colour.
// -----------------------------------------------------------------------------
void GfxCanvasBase::pickColour(int x, int y)
{
	// Get the pixel
	const auto coord = imageCoords(x, y);

	// Pick its colour
	paint_colour_ = image_->pixelAt(coord.x, coord.y, palette());

	// Announce it triumphantly to the world
	wxNotifyEvent e(wxEVT_GFXCANVAS_COLOUR_PICKED, window()->GetId());
	e.SetEventObject(window());
	window()->GetEventHandler()->ProcessEvent(e);
}

// -----------------------------------------------------------------------------
// Generates the mask image of the current brush to preview its effect
// -----------------------------------------------------------------------------
void GfxCanvasBase::generateBrushShadowImage(SImage& img)
{
	// Generate image
	img.create(image_->width(), image_->height(), SImage::Type::RGBA);
	for (int i = -4; i < 5; ++i)
		for (int j = -4; j < 5; ++j)
			if (brush_->pixel(i, j))
			{
				auto col = paint_colour_;
				if (editing_mode_ == EditMode::Translate && translation_)
					col = translation_->translate(
						image_->pixelAt(cursor_pos_.x + i, cursor_pos_.y + j, palette()), palette());
				// Not sure what's the best way to preview cutting out
				// Mimicking the checkerboard pattern perhaps?
				// Cyan will do for now
				else if (editing_mode_ == EditMode::Erase)
					col = ColRGBA::CYAN;
				img.setPixel(cursor_pos_.x + i, cursor_pos_.y + j, col);
			}
}


// -----------------------------------------------------------------------------
//
// GfxCanvasBase Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the left button is pressed within the canvas
// -----------------------------------------------------------------------------
void GfxCanvasBase::onMouseLeftDown(wxMouseEvent& e)
{
	const int  x        = e.GetPosition().x * window()->GetContentScaleFactor();
	const int  y        = e.GetPosition().y * window()->GetContentScaleFactor();
	const bool on_image = onImage(x, y - 2);

	// Left mouse down
	if (e.LeftDown() && on_image)
	{
		// Paint in paint mode
		if (editing_mode_ != EditMode::None)
		{
			drawing_ = true;
			brushCanvas(x, y);
		}

		// Begin drag if mouse is over image and dragging allowed
		else if (allow_drag_)
		{
			drag_origin_ = { x, y };
			drag_pos_    = { x, y };
			window()->Refresh();
		}
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the left button is pressed within the canvas
// -----------------------------------------------------------------------------
void GfxCanvasBase::onMouseRightDown(wxMouseEvent& e)
{
	const int x = e.GetPosition().x * window()->GetContentScaleFactor();
	const int y = e.GetPosition().y * window()->GetContentScaleFactor() - 2;

	// Right mouse down
	if (e.RightDown() && onImage(x, y))
		pickColour(x, y);

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the left button is released within the canvas
// -----------------------------------------------------------------------------
void GfxCanvasBase::onMouseLeftUp(wxMouseEvent& e)
{
	// Stop drawing
	if (drawing_)
	{
		drawing_ = false;
		memset(drawing_mask_, false, image_->width() * image_->height());
	}
	// Stop dragging
	if (drag_origin_.x >= 0)
	{
		endOffsetDrag();
		image_hilight_ = true;
		window()->Refresh();
	}
}

// -----------------------------------------------------------------------------
// Called when the mouse pointer is moved within the canvas
// -----------------------------------------------------------------------------
void GfxCanvasBase::onMouseMovement(wxMouseEvent& e)
{
	bool refresh = false;

	// Check if the mouse is over the image
	auto       scalefactor = window()->GetContentScaleFactor();
	const int  x           = e.GetPosition().x * scalefactor;
	const int  y           = e.GetPosition().y * scalefactor - 2;
	const bool on_image    = onImage(x, y);
	cursor_pos_            = imageCoords(x, y);
	if (on_image && editing_mode_ != EditMode::None)
	{
		if (cursor_pos_ != prev_pos_)
		{
			generateBrushShadow();
			refresh = true;
		}

		prev_pos_ = cursor_pos_;
	}
	if (on_image != image_hilight_)
	{
		image_hilight_ = on_image;
		refresh        = true;

		// Update cursor if drag allowed
		if (on_image)
		{
			if (editing_mode_ != EditMode::None)
				window()->SetCursor(wxCursor(wxCURSOR_PENCIL));
			else if (allow_drag_)
				window()->SetCursor(wxCursor(wxCURSOR_SIZING));
		}
		else if (allow_drag_ && !e.LeftIsDown())
			window()->SetCursor(wxNullCursor);
	}

	// Drag
	if (e.LeftIsDown())
	{
		if (editing_mode_ != EditMode::None)
		{
			brushCanvas(x, y);
		}
		else
		{
			drag_pos_ = { e.GetPosition().x * scalefactor, e.GetPosition().y * scalefactor };
			refresh   = true;
		}
	}

	// Right mouse down
	if (e.RightIsDown() && on_image)
		pickColour(x, y);

	// Middle mouse down (pan view)
	if (e.MiddleIsDown())
	{
		auto cpos_current = view().canvasPos({ e.GetPosition().x, e.GetPosition().y });
		auto cpos_prev    = view().canvasPos(mouse_prev_);

		view().pan(cpos_prev.x - cpos_current.x, cpos_prev.y - cpos_current.y);

		refresh = true;
	}

	if (refresh)
		window()->Refresh();

	mouse_prev_ = { e.GetPosition().x * scalefactor, e.GetPosition().y * scalefactor };

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the mouse pointer leaves the gfx canvas
// -----------------------------------------------------------------------------
void GfxCanvasBase::onMouseLeaving(wxMouseEvent& e)
{
	image_hilight_ = false;
	window()->Refresh();
}

// -----------------------------------------------------------------------------
// Called when the mouse wheel is scrolled
// -----------------------------------------------------------------------------
void GfxCanvasBase::onMouseWheel(wxMouseEvent& e)
{
	if (wxGetKeyState(WXK_CONTROL) && allow_scroll_)
	{
		auto scroll_amount = ui::scalePx(24);

		if (e.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL || wxGetKeyState(WXK_SHIFT))
		{
			if (e.GetWheelRotation() > 0)
				view().pan(scroll_amount / view().scale().x, 0);
			else
				view().pan(-scroll_amount / view().scale().x, 0);

			window()->Refresh();
		}
		else if (e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
		{
			if (e.GetWheelRotation() > 0)
				view().pan(0, scroll_amount / view().scale().y);
			else
				view().pan(0, -scroll_amount / view().scale().y);

			window()->Refresh();
		}
	}

	if (!wxGetKeyState(WXK_CONTROL) && linked_zoom_control_ && e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
	{
		// Zoom towards cursor
		zoom_point_ = { e.GetPosition().x, e.GetPosition().y };

		if (e.GetWheelRotation() > 0)
			linked_zoom_control_->zoomIn(true);
		else
			linked_zoom_control_->zoomOut(true);

		zoom_point_ = { -1, -1 };
	}
}

// -----------------------------------------------------------------------------
// Called when a key is pressed while the canvas has focus
// -----------------------------------------------------------------------------
void GfxCanvasBase::onKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_UP)
	{
		view().pan(0, 8 * view().scale().y);
		window()->Refresh();
	}

	else if (e.GetKeyCode() == WXK_DOWN)
	{
		view().pan(0, -8 * view().scale().y);
		window()->Refresh();
	}

	else if (e.GetKeyCode() == WXK_LEFT)
	{
		view().pan(8 * view().scale().x, 0);
		window()->Refresh();
	}

	else if (e.GetKeyCode() == WXK_RIGHT)
	{
		view().pan(-8 * view().scale().x, 0);
		window()->Refresh();
	}

	else
		e.Skip();
}
