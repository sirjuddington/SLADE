
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxCanvas.cpp
// Description: GfxCanvas class. An OpenGL canvas that displays an image and can
//              take offsets into account etc
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
#include "General/UI.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/OpenGL.h"
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
CVAR(Bool, gfx_show_border, true, CVar::Flag::Save)
CVAR(Bool, gfx_hilight_mouseover, true, CVar::Flag::Save)
CVAR(Bool, gfx_arc, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// GfxCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxCanvas class constructor
// -----------------------------------------------------------------------------
GfxCanvas::GfxCanvas(wxWindow* parent, int id) :
	OGLCanvas(parent, id),
	image_{ new SImage() },
	scale_{ ui::scaleFactor() }
{
	// Update texture when the image changes
	sc_image_changed_ = image_->signals().image_changed.connect(&GfxCanvas::updateImageTexture, this);

	// Bind events
	Bind(wxEVT_LEFT_DOWN, &GfxCanvas::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &GfxCanvas::onMouseRightDown, this);
	Bind(wxEVT_LEFT_UP, &GfxCanvas::onMouseLeftUp, this);
	Bind(wxEVT_MOTION, &GfxCanvas::onMouseMovement, this);
	Bind(wxEVT_LEAVE_WINDOW, &GfxCanvas::onMouseLeaving, this);
	Bind(wxEVT_MOUSEWHEEL, &GfxCanvas::onMouseWheel, this);
	Bind(wxEVT_KEY_DOWN, &GfxCanvas::onKeyDown, this);
}

// -----------------------------------------------------------------------------
// Sets the gfx canvas [scale]
// -----------------------------------------------------------------------------
void GfxCanvas::setScale(double scale)
{
	scale_ = scale * ui::scaleFactor();
}

// -----------------------------------------------------------------------------
// Draws the image/background/etc
// -----------------------------------------------------------------------------
void GfxCanvas::draw()
{
	// Setup the viewport
	const wxSize size = GetSize() * GetContentScaleFactor();
	glViewport(0, 0, size.x, size.y);

	// Setup the screen projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, size.x, size.y, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Translate to inside of pixel (otherwise inaccuracies can occur on certain gl implementations)
	if (gl::accuracyTweak())
		glTranslatef(0.375f, 0.375f, 0);

	// Draw the background
	drawCheckeredBackground();

	// Pan by view offset
	if (allow_scroll_)
		glTranslated(offset_.x, offset_.y, 0);

	// Pan if offsets
	if (view_type_ == View::Centered || view_type_ == View::Sprite || view_type_ == View::HUD)
	{
		const int mid_x = size.x / 2;
		const int mid_y = size.y / 2;
		glTranslated(mid_x, mid_y, 0);
	}

	// Scale by UI scale
	// glScaled(UI::scaleFactor(), UI::scaleFactor(), 1.);

	// Draw offset lines
	if (view_type_ == View::Sprite || view_type_ == View::HUD)
		drawOffsetLines();

	// Draw the image
	drawImage();

	// Swap buffers (ie show what was drawn)
	SwapBuffers();
}

// -----------------------------------------------------------------------------
// Draws the offset center lines
// -----------------------------------------------------------------------------
void GfxCanvas::drawOffsetLines() const
{
	if (view_type_ == View::Sprite)
	{
		gl::setColour(ColRGBA::BLACK, gl::Blend::Normal);

		glBegin(GL_LINES);
		glVertex2d(-9999, 0);
		glVertex2d(9999, 0);
		glVertex2d(0, -9999);
		glVertex2d(0, 9999);
		glEnd();
	}
	else if (view_type_ == View::HUD)
	{
		const double yscale = (gfx_arc ? scale_ * 1.2 : scale_);
		glPushMatrix();
		glEnable(GL_LINE_SMOOTH);
		glScaled(scale_, yscale, 1);
		drawing::drawHud();
		glDisable(GL_LINE_SMOOTH);
		glPopMatrix();
	}
}

// -----------------------------------------------------------------------------
// Draws the image
// (reloads the image as a texture each time, will change this later...)
// -----------------------------------------------------------------------------
void GfxCanvas::drawImage()
{
	// Check image is valid
	if (!image_->isValid())
		return;

	// Save current matrix
	glPushMatrix();

	// Zoom
	const double yscale = (gfx_arc ? scale_ * 1.2 : scale_);
	glScaled(scale_, yscale, 1.0);

	// Pan
	if (view_type_ == View::Centered)
		glTranslated(-(image_->width() * 0.5), -(image_->height() * 0.5), 0); // Pan to center image
	else if (view_type_ == View::Sprite)
		glTranslated(-image_->offset().x, -image_->offset().y, 0); // Pan by offsets
	else if (view_type_ == View::HUD)
	{
		glTranslated(-160, -100, 0);                               // Pan to hud 'top left'
		glTranslated(-image_->offset().x, -image_->offset().y, 0); // Pan by offsets
	}

	// Enable textures
	glEnable(GL_TEXTURE_2D);

	// Update texture if needed
	if (update_texture_)
	{
		// If the image change isn't caused by drawing, resize drawing mask
		if (!drawing_)
		{
			delete[] drawing_mask_;
			drawing_mask_ = new bool[image_->width() * image_->height()];
			memset(drawing_mask_, false, image_->width() * image_->height());
		}

		gl::Texture::clear(tex_image_);
		tex_image_ = gl::Texture::createFromImage(*image_, palette_.get());

		update_texture_ = false;
	}

	// Determine (texture)coordinates
	const double x = image_->width();
	const double y = image_->height();

	// If tiled view
	if (view_type_ == View::Tiled)
	{
		// Draw tiled image
		gl::setColour(255, 255, 255, 255, gl::Blend::Normal);
		const wxSize size = GetSize() * GetContentScaleFactor();
		drawing::drawTextureTiled(tex_image_, math::scaleInverse(size.x, scale_), math::scaleInverse(size.y, scale_));
	}
	else if (drag_origin_.x < 0) // If not dragging
	{
		// Draw the image
		gl::setColour(255, 255, 255, 255, gl::Blend::Normal);
		drawing::drawTexture(tex_image_);

		// Draw hilight otherwise
		if (image_hilight_ && gfx_hilight_mouseover && editing_mode_ == EditMode::None)
		{
			gl::setColour(255, 255, 255, 80, gl::Blend::Additive);
			drawing::drawTexture(tex_image_);

			// Reset colour
			gl::setColour(255, 255, 255, 255, gl::Blend::Normal);
		}
	}
	else // Dragging
	{
		// Draw the original
		gl::setColour(ColRGBA(0, 0, 0, 180), gl::Blend::Normal);
		drawing::drawTexture(tex_image_);

		// Draw the dragged image
		const auto off_x = static_cast<int>((drag_pos_.x - drag_origin_.x) / scale_);
		const auto off_y = static_cast<int>((drag_pos_.y - drag_origin_.y) / scale_);
		glTranslated(off_x, off_y, 0);
		gl::setColour(255, 255, 255, 255, gl::Blend::Normal);
		drawing::drawTexture(tex_image_);
	}
	// Draw brush shadow when in editing mode
	if (editing_mode_ != EditMode::None && cursor_pos_ != Vec2i{ -1, -1 })
	{
		gl::setColour(255, 255, 255, 160, gl::Blend::Normal);
		drawing::drawTexture(tex_brush_);
		gl::setColour(255, 255, 255, 255, gl::Blend::Normal);
	}

	// Disable textures
	glDisable(GL_TEXTURE_2D);

	// Draw outline
	if (gfx_show_border)
	{
		gl::setColour(0, 0, 0, 64);
		glBegin(GL_LINE_LOOP);
		glVertex2d(0, 0);
		glVertex2d(0, y);
		glVertex2d(x, y);
		glVertex2d(x, 0);
		glEnd();
	}

	// Restore previous matrix
	glPopMatrix();
}

// -----------------------------------------------------------------------------
// Forces (Re)Generation of the image texture
// -----------------------------------------------------------------------------
void GfxCanvas::updateImageTexture()
{
	update_texture_ = true;
	Refresh();
}

// -----------------------------------------------------------------------------
// Scales the image to fit within the gfx canvas.
// If mag is false, the image will not be stretched to fit the canvas
// (only shrunk if needed).
// Leaves a border around the image if <padding> is specified
// (0.0f = no border, 1.0f = border 100% of canvas size)
// -----------------------------------------------------------------------------
void GfxCanvas::zoomToFit(bool mag, double padding)
{
	// Determine padding
	const wxSize size = GetSize() * GetContentScaleFactor();
	const double pad  = static_cast<double>(std::min<int>(size.x, size.y)) * padding;

	// Get image dimensions
	const double x_dim = image_->width();
	const double y_dim = image_->height();

	// Get max scale for x and y (including padding)
	const double x_scale = (static_cast<double>(size.x) - pad) / x_dim;
	const double y_scale = (static_cast<double>(size.y) - pad) / y_dim;

	// Set scale to smallest of the 2 (so that none of the image will be clipped)
	scale_ = std::min<double>(x_scale, y_scale);

	// If we don't want to magnify the image, clamp scale to a max of 1.0
	if (!mag && scale_ > 1)
		scale_ = 1;
}

// -----------------------------------------------------------------------------
// Returns true if the given coordinates are 'on' top of the image
// -----------------------------------------------------------------------------
bool GfxCanvas::onImage(int x, int y) const
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
Vec2i GfxCanvas::imageCoords(int x, int y) const
{
	// Determine top-left coordinates of image in screen coords
	const wxSize size   = GetSize() * GetContentScaleFactor();
	double       left   = size.x * 0.5 + offset_.x;
	double       top    = size.y * 0.5 + offset_.y;
	const double yscale = scale_ * (gfx_arc ? 1.2 : 1);

	if (view_type_ == View::Default || view_type_ == View::Tiled)
	{
		left = offset_.x;
		top  = offset_.y;
	}
	else if (view_type_ == View::Centered)
	{
		left -= static_cast<double>(image_->width()) * 0.5 * scale_;
		top -= static_cast<double>(image_->height()) * 0.5 * yscale;
	}
	else if (view_type_ == View::Sprite)
	{
		left -= image_->offset().x * scale_;
		top -= image_->offset().y * yscale;
	}
	else if (view_type_ == View::HUD)
	{
		left -= 160 * scale_;
		top -= 100 * scale_ * (gfx_arc ? 1.2 : 1);
		left -= image_->offset().x * scale_;
		top -= image_->offset().y * yscale;
	}

	// Determine bottom-right coordinates of image in screen coords
	const double right  = left + image_->width() * scale_;
	const double bottom = top + image_->height() * yscale;

	// Check if the pointer is within the image
	if (x >= left && x <= right && y >= top && y <= bottom)
	{
		// Determine where in the image it is
		const double w    = right - left;
		const double h    = bottom - top;
		const double xpos = (x - left) / w;
		const double ypos = (y - top) / h;

		return { static_cast<int>(xpos * image_->width()), static_cast<int>(ypos * image_->height()) };
	}
	else
		return { -1, -1 };
}

// -----------------------------------------------------------------------------
// Finishes an offset drag
// -----------------------------------------------------------------------------
void GfxCanvas::endOffsetDrag()
{
	// Get offset
	const auto x = math::scaleInverse(drag_pos_.x - drag_origin_.x, scale_);
	const auto y = math::scaleInverse(drag_pos_.y - drag_origin_.y, scale_);

	// If there was a change
	if (x != 0 || y != 0)
	{
		// Set image offsets
		image_->setXOffset(image_->offset().x - x);
		image_->setYOffset(image_->offset().y - y);

		// Generate event
		wxNotifyEvent e(wxEVT_GFXCANVAS_OFFSET_CHANGED, GetId());
		e.SetEventObject(this);
		GetEventHandler()->ProcessEvent(e);
	}

	// Stop drag
	drag_origin_ = { -1, -1 };
}

// -----------------------------------------------------------------------------
// Paints a pixel from the image at the given image coordinates.
// -----------------------------------------------------------------------------
void GfxCanvas::paintPixel(int x, int y)
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
			const auto    ocol  = image_->pixelAt(x, y, palette_.get());
			const uint8_t alpha = ocol.a;
			auto          ncol  = (translation_->translate(ocol, palette_.get()));
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
		wxNotifyEvent e(wxEVT_GFXCANVAS_PIXELS_CHANGED, GetId());
		e.SetEventObject(this);
		GetEventHandler()->ProcessEvent(e);
	}
}

// -----------------------------------------------------------------------------
// Finds all the pixels under the brush, and paints them.
// -----------------------------------------------------------------------------
void GfxCanvas::brushCanvas(int x, int y)
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
void GfxCanvas::pickColour(int x, int y)
{
	// Get the pixel
	const auto coord = imageCoords(x, y);

	// Pick its colour
	paint_colour_ = image_->pixelAt(coord.x, coord.y, palette_.get());

	// Announce it triumphantly to the world
	wxNotifyEvent e(wxEVT_GFXCANVAS_COLOUR_PICKED, GetId());
	e.SetEventObject(this);
	GetEventHandler()->ProcessEvent(e);
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
	img.create(image_->width(), image_->height(), SImage::Type::RGBA);
	for (int i = -4; i < 5; ++i)
		for (int j = -4; j < 5; ++j)
			if (brush_->pixel(i, j))
			{
				auto col = paint_colour_;
				if (editing_mode_ == EditMode::Translate && translation_)
					col = translation_->translate(
						image_->pixelAt(cursor_pos_.x + i, cursor_pos_.y + j, palette_.get()), palette_.get());
				// Not sure what's the best way to preview cutting out
				// Mimicking the checkerboard pattern perhaps?
				// Cyan will do for now
				else if (editing_mode_ == EditMode::Erase)
					col = ColRGBA::CYAN;
				img.setPixel(cursor_pos_.x + i, cursor_pos_.y + j, col);
			}

	// Load it as a GL texture
	gl::Texture::clear(tex_brush_);
	tex_brush_ = gl::Texture::createFromImage(img);
}


// -----------------------------------------------------------------------------
//
// GfxCanvas Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the left button is pressed within the canvas
// -----------------------------------------------------------------------------
void GfxCanvas::onMouseLeftDown(wxMouseEvent& e)
{
	const int  x        = e.GetPosition().x * GetContentScaleFactor();
	const int  y        = e.GetPosition().y * GetContentScaleFactor();
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
			Refresh();
		}
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the left button is pressed within the canvas
// -----------------------------------------------------------------------------
void GfxCanvas::onMouseRightDown(wxMouseEvent& e)
{
	const int x = e.GetPosition().x * GetContentScaleFactor();
	const int y = e.GetPosition().y * GetContentScaleFactor() - 2;

	// Right mouse down
	if (e.RightDown() && onImage(x, y))
		pickColour(x, y);

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the left button is released within the canvas
// -----------------------------------------------------------------------------
void GfxCanvas::onMouseLeftUp(wxMouseEvent& e)
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
		Refresh();
	}
}

// -----------------------------------------------------------------------------
// Called when the mouse pointer is moved within the canvas
// -----------------------------------------------------------------------------
void GfxCanvas::onMouseMovement(wxMouseEvent& e)
{
	bool refresh = false;

	// Check if the mouse is over the image
	const int  x        = e.GetPosition().x * GetContentScaleFactor();
	const int  y        = e.GetPosition().y * GetContentScaleFactor() - 2;
	const bool on_image = onImage(x, y);
	cursor_pos_         = imageCoords(x, y);
	if (on_image && editing_mode_ != EditMode::None)
	{
		if (cursor_pos_ != prev_pos_)
			generateBrushShadow();
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
				SetCursor(wxCursor(wxCURSOR_PENCIL));
			else if (allow_drag_)
				SetCursor(wxCursor(wxCURSOR_SIZING));
		}
		else if (allow_drag_ && !e.LeftIsDown())
			SetCursor(wxNullCursor);
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
			drag_pos_ = { e.GetPosition().x * GetContentScaleFactor(), e.GetPosition().y * GetContentScaleFactor() };
			refresh   = true;
		}
	}
	else if (e.MiddleIsDown())
	{
		offset_ = offset_
				  + Vec2d(
					  e.GetPosition().x * GetContentScaleFactor() - mouse_prev_.x,
					  e.GetPosition().y * GetContentScaleFactor() - mouse_prev_.y);
		refresh = true;
	}
	// Right mouse down
	if (e.RightIsDown() && on_image)
		pickColour(x, y);

	if (refresh)
		Refresh();

	mouse_prev_ = { e.GetPosition().x * GetContentScaleFactor(), e.GetPosition().y * GetContentScaleFactor() };
}

// -----------------------------------------------------------------------------
// Called when the mouse pointer leaves the gfx canvas
// -----------------------------------------------------------------------------
void GfxCanvas::onMouseLeaving(wxMouseEvent& e)
{
	image_hilight_ = false;
	Refresh();
}

// -----------------------------------------------------------------------------
// Called when the mouse wheel is scrolled
// -----------------------------------------------------------------------------
void GfxCanvas::onMouseWheel(wxMouseEvent& e)
{
	if (wxGetKeyState(WXK_CONTROL) && allow_scroll_)
	{
		if (e.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL || wxGetKeyState(WXK_SHIFT))
		{
			if (e.GetWheelRotation() > 0)
				offset_.x -= 8 * scale_;
			else
				offset_.x += 8 * scale_;
		}
		else if (e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
		{
			if (e.GetWheelRotation() > 0)
				offset_.y += 8 * scale_;
			else
				offset_.y -= 8 * scale_;
		}
	}

	if (!wxGetKeyState(WXK_CONTROL) && linked_zoom_control_ && e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
	{
		if (e.GetWheelRotation() > 0)
			linked_zoom_control_->zoomIn(true);
		else
			linked_zoom_control_->zoomOut(true);
	}
}

// -----------------------------------------------------------------------------
// Called when a key is pressed while the canvas has focus
// -----------------------------------------------------------------------------
void GfxCanvas::onKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_UP)
	{
		offset_.y += 8;
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_DOWN)
	{
		offset_.y -= 8;
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_LEFT)
	{
		offset_.x += 8;
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_RIGHT)
	{
		offset_.x -= 8;
		Refresh();
	}

	else
		e.Skip();
}
