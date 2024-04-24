
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GfxGLCanvas.cpp
// Description: GfxGLCanvas class. An OpenGL canvas that displays an image and
//              can take offsets into account etc
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
#include "GfxGLCanvas.h"
#include "GLCanvas.h"
#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include "Utility/MathStuff.h"
#include <glm/ext/matrix_transform.hpp>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, gfx_show_border, true, CVar::Flag::Save)
CVAR(Bool, gfx_hilight_mouseover, true, CVar::Flag::Save)
CVAR(Bool, gfx_arc, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// GfxGLCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// GfxGLCanvas class constructor
// -----------------------------------------------------------------------------
GfxGLCanvas::GfxGLCanvas(wxWindow* parent) : GLCanvas(parent, BGStyle::Checkered)
{
	view_.setCentered(true);
	resetViewOffsets();

	// Update texture when the image changes
	sc_image_changed_ = image_->signals().image_changed.connect(&GfxGLCanvas::updateImageTexture, this);

	// Bind events
	// setupMousePanning();
	Bind(wxEVT_LEFT_DOWN, &GfxGLCanvas::onMouseLeftDown, this);
	Bind(wxEVT_RIGHT_DOWN, &GfxGLCanvas::onMouseRightDown, this);
	Bind(wxEVT_LEFT_UP, &GfxGLCanvas::onMouseLeftUp, this);
	Bind(wxEVT_MOTION, &GfxGLCanvas::onMouseMovement, this);
	Bind(wxEVT_LEAVE_WINDOW, &GfxGLCanvas::onMouseLeaving, this);
	Bind(wxEVT_MOUSEWHEEL, &GfxGLCanvas::onMouseWheel, this);
	Bind(wxEVT_KEY_DOWN, &GfxGLCanvas::onKeyDown, this);
}

// -----------------------------------------------------------------------------
// Sets the canvas palette to [pal]
// -----------------------------------------------------------------------------
void GfxGLCanvas::setPalette(const Palette* pal)
{
	GLCanvas::setPalette(pal);
	update_texture_ = true;
	Refresh();
}

// -----------------------------------------------------------------------------
// Draws the image/background/etc
// -----------------------------------------------------------------------------
void GfxGLCanvas::draw()
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
			drawing_mask_ = new bool[image_->width() * image_->height()];
			memset(drawing_mask_, false, image_->width() * image_->height());
		}

		gl::Texture::clear(tex_image_);
		tex_image_ = gl::Texture::createFromImage(*image_, palette_.get());

		update_texture_ = false;
	}

	gl::draw2d::Context dc(&view_);

	// Draw offset lines
	if (view_type_ == View::Sprite || view_type_ == View::HUD)
		drawOffsetLines(dc);

	// Draw the image
	if (editing_mode_ == GfxEditMode::None && view_type_ == View::Tiled)
		drawImageTiled();
	else
		drawImage(dc);
}

// -----------------------------------------------------------------------------
// Draws the offset center/guide lines
// -----------------------------------------------------------------------------
void GfxGLCanvas::drawOffsetLines(const gl::draw2d::Context& dc)
{
	if (view_type_ == View::Sprite)
	{
		if (!lb_sprite_)
		{
			glm::vec4 colour = ColRGBA::BLACK;
			colour.a         = 0.75f;
			lb_sprite_       = std::make_unique<gl::LineBuffer>();

			lb_sprite_->add2d(-99999.0f, 0.0f, 99999.0f, 0.0f, colour, 1.5f);
			lb_sprite_->add2d(0.0f, -99999.0f, 0.0f, 99999.0f, colour, 1.5f);
			lb_sprite_->push();
		}

		view_.setupShader(lb_sprite_->shader());
		lb_sprite_->draw();
	}
	else if (view_type_ == View::HUD)
		dc.drawHud();
}

// -----------------------------------------------------------------------------
// Draws the image (and offset drag preview if needed)
// -----------------------------------------------------------------------------
void GfxGLCanvas::drawImage(gl::draw2d::Context& dc) const
{
	// Check image is valid
	if (!image_->isValid())
		return;

	bool  dragging = drag_origin_.x > 0;
	Rectf img_rect{ 0.0f, 0.0f, static_cast<float>(image_->width()), static_cast<float>(image_->height()), false };

	// Apply offsets for sprite/hud view
	if (view_type_ == View::Sprite || view_type_ == View::HUD)
		img_rect.move(-image_->offset().x, -image_->offset().y);

	dc.texture = tex_image_;
	dc.colour.set(255, 255, 255, 255);
	gl::Texture::setTiling(tex_image_, false);
	if (dragging)
	{
		// Draw image in original position (semitransparent)
		dc.colour.a = 128;
		dc.drawRect(img_rect);

		// Draw image in dragged position
		img_rect.move(
			math::scaleInverse(drag_pos_.x - drag_origin_.x, view_.scale().x),
			math::scaleInverse(drag_pos_.y - drag_origin_.y, view_.scale().y));
		dc.colour.a = 255;
		dc.drawRect(img_rect);
	}
	else
	{
		// Not dragging, just draw image
		dc.drawRect(img_rect);

		// Hilight if needed
		if (show_hilight_ && image_hover_ && gfx_hilight_mouseover && editing_mode_ == EditMode::None)
		{
			gl::setBlend(gl::Blend::Additive);
			dc.colour.a = 50;
			dc.drawRect(img_rect);
			gl::setBlend(gl::Blend::Normal);
		}
	}

	// Draw brush shadow when in editing mode
	if (editing_mode_ != EditMode::None && gl::Texture::isCreated(tex_brush_) && cursor_pos_ != Vec2i{ -1, -1 })
	{
		dc.colour.a = 160;
		dc.texture  = tex_brush_;
		dc.drawRect(img_rect);
	}

	// Draw outline
	if (show_border_ && gfx_show_border)
	{
		dc.colour.set(0, 0, 0, 64);
		dc.drawRectOutline(img_rect);
	}
}

// -----------------------------------------------------------------------------
// Draws the image tiled to fill the canvas
// -----------------------------------------------------------------------------
void GfxGLCanvas::drawImageTiled() const
{
	auto widthf    = static_cast<float>(view_.size().x / view_.scale().x);
	auto heightf   = static_cast<float>(view_.size().y / view_.scale().y);
	auto i_widthf  = static_cast<float>(image_->width());
	auto i_heightf = static_cast<float>(image_->height());

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
	gl::Texture::bind(tex_image_);
	gl::Texture::setTiling(tex_image_, true);
	vb_tiled.draw(gl::Primitive::TriangleFan);
}

// -----------------------------------------------------------------------------
// Forces (Re)Generation of the image texture
// -----------------------------------------------------------------------------
void GfxGLCanvas::updateImageTexture()
{
	update_texture_ = true;
	Refresh();
}

// -----------------------------------------------------------------------------
// Creates a mask texture of the brush to preview its effect
// -----------------------------------------------------------------------------
void GfxGLCanvas::generateBrushShadow()
{
	if (brush_ == nullptr)
		return;

	// Generate image
	SImage img;
	generateBrushShadowImage(img);

	// Load it as a GL texture
	gl::Texture::clear(tex_brush_);
	tex_brush_ = gl::Texture::createFromImage(img, nullptr, gl::TexFilter::Nearest, false);
}
