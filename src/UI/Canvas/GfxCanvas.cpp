
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "GLCanvas.h"
#include "General/UI.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include "UI/Controls/ZoomControl.h"
#include "UI/SBrush.h"
#include "Utility/MathStuff.h"
#include <glm/ext/matrix_transform.hpp>

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
GfxCanvas::GfxCanvas(wxWindow* parent) : GLCanvas(parent, BGStyle::Checkered)
{
	view_.setCentered(true);
	view_.setInterpolated(false);
	view_.setScale(ui::scaleFactor());

	// Update texture when the image changes
	sc_image_changed_ = image_.signals().image_changed.connect(&GfxCanvas::updateImageTexture, this);

	// Bind events
	setupMousePanning();
	Bind(wxEVT_LEFT_DOWN, &GfxCanvas::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &GfxCanvas::onMouseRightDown, this);
	Bind(wxEVT_LEFT_UP, &GfxCanvas::onMouseLeftUp, this);
	Bind(wxEVT_MOTION, &GfxCanvas::onMouseMovement, this);
	Bind(wxEVT_LEAVE_WINDOW, &GfxCanvas::onMouseLeaving, this);
	Bind(wxEVT_MOUSEWHEEL, &GfxCanvas::onMouseWheel, this);
	Bind(wxEVT_KEY_DOWN, &GfxCanvas::onKeyDown, this);
}

void GfxCanvas::setPalette(const Palette* pal)
{
	palette_.copyPalette(pal);
	update_texture_ = true;
	Refresh();
}

void GfxCanvas::setViewType(View type)
{
	bool changed = view_type_ != type;
	view_type_   = type;
	if (changed)
	{
		view_.setCentered(type != View::Tiled);
		resetViewOffsets();
	}
}

// -----------------------------------------------------------------------------
// Sets the gfx canvas [scale]
// -----------------------------------------------------------------------------
void GfxCanvas::setScale(double scale)
{
	if (zoom_point_.x < 0 && zoom_point_.y < 0)
		view_.setScale(scale * ui::scaleFactor());
	else
		view_.setScale(scale * ui::scaleFactor(), zoom_point_);
}

// -----------------------------------------------------------------------------
// Draws the image/background/etc
// -----------------------------------------------------------------------------
void GfxCanvas::draw()
{
	// Aspect Ratio Correction
	if (gfx_arc)
		view_.setScale({ view_.scale().x, view_.scale().x * 1.2 });
	else
		view_.setScale(view_.scale().x);

	// Update texture if needed
	if (update_texture_)
	{
		// If the image change isn't caused by drawing, resize drawing mask
		if (!drawing_)
		{
			delete[] drawing_mask_;
			drawing_mask_ = new bool[image_.width() * image_.height()];
			memset(drawing_mask_, false, image_.width() * image_.height());
		}

		gl::Texture::clear(tex_image_);
		tex_image_ = gl::Texture::createFromImage(image_, &palette_);

		update_texture_ = false;
	}

	// Draw offset lines
	if (view_type_ == View::Sprite || view_type_ == View::HUD)
		drawOffsetLines();

	// Draw the image
	if (view_type_ == View::Tiled)
		drawImageTiled();
	else
		drawImage();
}

// -----------------------------------------------------------------------------
// Draws the offset center lines
// -----------------------------------------------------------------------------
void GfxCanvas::drawOffsetLines()
{
	if (view_type_ == View::Sprite)
	{
		if (!lb_sprite_)
		{
			auto colour = ColRGBA::BLACK.asVec4();
			colour.a    = 0.75f;
			lb_sprite_  = std::make_unique<gl::LineBuffer>();

			lb_sprite_->add2d(-99999.0f, 0.0f, 99999.0f, 0.0f, colour, 1.5f);
			lb_sprite_->add2d(0.0f, -99999.0f, 0.0f, 99999.0f, colour, 1.5f);
		}

		view_.setupShader(lb_sprite_->shader());
		lb_sprite_->draw();
	}
	else if (view_type_ == View::HUD)
		gl::draw2d::drawHud(&view_);
}

// -----------------------------------------------------------------------------
// Draws the image
// (reloads the image as a texture each time, will change this later...)
// -----------------------------------------------------------------------------
void GfxCanvas::drawImage() const
{
	// Check image is valid
	if (!image_.isValid())
		return;

	bool  dragging = drag_origin_.x > 0;
	Rectf img_rect{ 0.0f, 0.0f, static_cast<float>(image_.width()), static_cast<float>(image_.height()), false };

	// Apply offsets for sprite/hud view
	if (view_type_ == View::Sprite || view_type_ == View::HUD)
		img_rect.move(-image_.offset().x, -image_.offset().y);

	gl::draw2d::RenderOptions opt{ tex_image_ };
	if (dragging)
	{
		// Draw image in original position (semitransparent)
		opt.colour.a = 128;
		gl::draw2d::drawRect(img_rect, opt, &view_);

		// Draw image in dragged position
		img_rect.move(
			math::scaleInverse(drag_pos_.x - drag_origin_.x, view_.scale().x),
			math::scaleInverse(drag_pos_.y - drag_origin_.y, view_.scale().y));
		opt.colour.a = 255;
		gl::draw2d::drawRect(img_rect, opt, &view_);
	}
	else
	{
		// Not dragging, just draw image
		gl::draw2d::drawRect(img_rect, opt, &view_);

		// Hilight if needed
		if (image_hilight_ && gfx_hilight_mouseover && editing_mode_ == EditMode::None)
		{
			gl::setBlend(gl::Blend::Additive);
			opt.colour.a = 50;
			gl::draw2d::drawRect(img_rect, opt, &view_);
			gl::setBlend(gl::Blend::Normal);
		}
	}

	// Draw brush shadow when in editing mode
	if (editing_mode_ != EditMode::None && gl::Texture::isCreated(tex_brush_) && cursor_pos_ != Vec2i{ -1, -1 })
	{
		opt.colour.a = 160;
		opt.texture = tex_brush_;
		gl::draw2d::drawRect(img_rect, opt, &view_);
	}

	// Draw outline
	if (gfx_show_border)
	{
		opt.colour.set(0, 0, 0, 64);
		opt.texture = 0;
		gl::draw2d::drawRectOutline(img_rect, opt, &view_);
	 }
}

void GfxCanvas::drawImageTiled() const
{
	auto widthf    = static_cast<float>(view_.size().x / view_.scale().x);
	auto heightf   = static_cast<float>(view_.size().y / view_.scale().y);
	auto i_widthf  = static_cast<float>(image_.width());
	auto i_heightf = static_cast<float>(image_.height());

	// Setup vertex buffer
	gl::VertexBuffer2D vb_tiled;
	vb_tiled.add({ { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f } });
	vb_tiled.add({ { 0.f, heightf }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, heightf / i_heightf } });
	vb_tiled.add({ { widthf, heightf }, { 1.f, 1.f, 1.f, 1.f }, { widthf / i_widthf, heightf / i_heightf } });
	vb_tiled.add({ { widthf, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { widthf / i_widthf, 0.f } });

	// Calculate view matrix (no offset/panning)
	auto view_matrix = glm::translate(glm::mat4(1.f), { 0.375f, 0.375f, 0.f });
	view_matrix      = glm::scale(view_matrix, { view_.scale().x, view_.scale().y, 1. });

	// Setup default shader
	auto& shader = gl::draw2d::defaultShader();
	shader.bind();
	shader.setUniform("mvp", view_.projectionMatrix() * view_matrix);
	shader.setUniform("colour", glm::vec4(1.0f));
	shader.setUniform("viewport_size", glm::vec2(view_.size().x, view_.size().y));

	// Draw
	glEnable(GL_TEXTURE_2D);
	gl::Texture::bind(tex_image_);
	vb_tiled.draw(gl::Primitive::Quads);
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
	const double x_dim = image_.width();
	const double y_dim = image_.height();

	// Get max scale for x and y (including padding)
	const double x_scale = (static_cast<double>(size.x) - pad) / x_dim;
	const double y_scale = (static_cast<double>(size.y) - pad) / y_dim;

	// Set scale to smallest of the 2 (so that none of the image will be clipped)
	auto scale = std::min<double>(x_scale, y_scale);

	// If we don't want to magnify the image, clamp scale to a max of 1.0
	if (!mag && scale > 1)
		scale = 1;

	view_.setScale(scale);
}

void GfxCanvas::resetViewOffsets()
{
	if (view_type_ == View::HUD)
		view_.setOffset(160, 100);
	else if (view_type_ == View::Default || view_type_ == View::Centered)
		view_.setOffset(image_.width() / 2., image_.height() / 2.);
	else
		view_.setOffset(0, 0);
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
	auto canvas_pos = view_.canvasPos({ x, y });
	auto image_pos  = canvas_pos;

	if (view_type_ == View::Sprite || view_type_ == View::HUD)
	{
		image_pos.x += image_.offset().x;
		image_pos.y += image_.offset().y;
	}

	if (image_pos.x < 0 || image_pos.y < 0 || image_pos.x >= image_.width() || image_pos.y >= image_.height())
		return { -1, -1 }; // Not on image

	return { static_cast<int>(image_pos.x), static_cast<int>(image_pos.y) };
}

// -----------------------------------------------------------------------------
// Finishes an offset drag
// -----------------------------------------------------------------------------
void GfxCanvas::endOffsetDrag()
{
	// Get offset
	const auto x = math::scaleInverse(drag_pos_.x - drag_origin_.x, view_.scale().x);
	const auto y = math::scaleInverse(drag_pos_.y - drag_origin_.y, view_.scale().y);

	// If there was a change
	if (x != 0 || y != 0)
	{
		// Set image offsets
		image_.setXOffset(image_.offset().x - x);
		image_.setYOffset(image_.offset().y - y);

		// Generate event
		wxNotifyEvent e(wxEVT_GFXCANVAS_OFFSET_CHANGED, GetId());
		e.SetEventObject(this);
		GetEventHandler()->ProcessEvent(e);
	}

	// Stop drag
	drag_origin_.set({ -1, -1 });
}

// -----------------------------------------------------------------------------
// Paints a pixel from the image at the given image coordinates.
// -----------------------------------------------------------------------------
void GfxCanvas::paintPixel(int x, int y)
{
	// With large brushes, it's very possible that some of the pixels
	// are out of the image area; so don't process them.
	if (x < 0 || y < 0 || x >= image_.width() || y >= image_.height())
		return;

	// Do not process pixels that were already modified in the
	// current drawing operation. This mechanism is needed to
	// allow freehand drawing, because an unpredictable number
	// of mouse events can happen while the mouse moves, leading
	// to the same pixel being processed over and over, and that
	// does not play well when applying translations.
	const size_t pos = x + image_.width() * y;
	if (drawing_mask_[pos])
		return;

	bool painted = false;
	if (editing_mode_ == EditMode::Erase) // eraser
		painted = image_.setPixel(x, y, 255, 0);
	else if (editing_mode_ == EditMode::Translate) // translator
	{
		if (translation_ != nullptr)
		{
			const auto    ocol  = image_.pixelAt(x, y, &palette_);
			const uint8_t alpha = ocol.a;
			auto          ncol  = (translation_->translate(ocol, &palette_));
			ncol.a              = alpha;
			if (!ocol.equals(ncol, false, true))
				painted = image_.setPixel(x, y, ncol);
		}
	}
	else
		painted = image_.setPixel(x, y, paint_colour_);

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
	paint_colour_ = image_.pixelAt(coord.x, coord.y, &palette_);

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
	img.create(image_.width(), image_.height(), SImage::Type::RGBA);
	for (int i = -4; i < 5; ++i)
		for (int j = -4; j < 5; ++j)
			if (brush_->pixel(i, j))
			{
				auto col = paint_colour_;
				if (editing_mode_ == EditMode::Translate && translation_)
					col = translation_->translate(
						image_.pixelAt(cursor_pos_.x + i, cursor_pos_.y + j, &palette_), &palette_);
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
			drag_origin_.set(x, y);
			drag_pos_.set(x, y);
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
		memset(drawing_mask_, false, image_.width() * image_.height());
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
			drag_pos_.set(e.GetPosition().x * GetContentScaleFactor(), e.GetPosition().y * GetContentScaleFactor());
			refresh = true;
		}
	}

	// Right mouse down
	if (e.RightIsDown() && on_image)
		pickColour(x, y);

	if (refresh)
		Refresh();

	mouse_prev_.set(e.GetPosition().x * GetContentScaleFactor(), e.GetPosition().y * GetContentScaleFactor());

	e.Skip();
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
				view_.pan(8 * view_.scale().x, 0);
			else
				view_.pan(-8 * view_.scale().x, 0);
		}
		else if (e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
		{
			if (e.GetWheelRotation() > 0)
				view_.pan(0, 8 * view_.scale().y);
			else
				view_.pan(0, -8 * view_.scale().y);
		}
	}

	if (!wxGetKeyState(WXK_CONTROL) && linked_zoom_control_ && e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
	{
		// Zoom towards cursor
		zoom_point_.set(e.GetPosition().x, e.GetPosition().y);

		if (e.GetWheelRotation() > 0)
			linked_zoom_control_->zoomIn(true);
		else
			linked_zoom_control_->zoomOut(true);

		zoom_point_.set(-1, -1);
	}
}

// -----------------------------------------------------------------------------
// Called when a key is pressed while the canvas has focus
// -----------------------------------------------------------------------------
void GfxCanvas::onKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_UP)
	{
		view_.pan(0, 8 * view_.scale().y);
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_DOWN)
	{
		view_.pan(0, -8 * view_.scale().y);
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_LEFT)
	{
		view_.pan(8 * view_.scale().x, 0);
		Refresh();
	}

	else if (e.GetKeyCode() == WXK_RIGHT)
	{
		view_.pan(-8 * view_.scale().x, 0);
		Refresh();
	}

	else
		e.Skip();
}
