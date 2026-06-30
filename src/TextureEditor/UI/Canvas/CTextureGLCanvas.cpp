
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CTextureGLCanvas.cpp
// Description: An OpenGL canvas that displays a composite texture
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
#include "CTextureGLCanvas.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Palette/Palette.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include <glm/ext/matrix_transform.hpp>
#include <wx/unichar.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
unique_ptr<gl::Shader> CTextureGLCanvas::shader_;


// -----------------------------------------------------------------------------
//
// CTextureGLCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CTextureGLCanvas class constructor
// -----------------------------------------------------------------------------
CTextureGLCanvas::CTextureGLCanvas(wxWindow* parent) : GLCanvas(parent, BGStyle::Checkered)
{
	palette_ = std::make_unique<Palette>();
	view_.setCentered(true);

	// Bind events
	setupMousePanning();
	Bind(wxEVT_MOTION, &CTextureGLCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &CTextureGLCanvas::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &CTextureGLCanvas::onMouseEvent, this);
	Bind(wxEVT_MOUSEWHEEL, &CTextureGLCanvas::onMouseEvent, this);
}

// -----------------------------------------------------------------------------
// CTextureGLCanvas class destructor
// -----------------------------------------------------------------------------
CTextureGLCanvas::~CTextureGLCanvas()
{
	// Cleanup patch GL textures
	for (auto& id : patch_gl_textures_)
		gl::Texture::clear(id);

	// Cleanup preview GL texture
	if (gl_tex_preview_ > 0)
		gl::Texture::clear(gl_tex_preview_);
}

// -----------------------------------------------------------------------------
// Clears the current texture and the patch textures list
// -----------------------------------------------------------------------------
void CTextureGLCanvas::clearTexture()
{
	CTextureCanvasBase::clearTexture();

	// Clear full preview
	gl::Texture::clear(gl_tex_preview_);
	gl_tex_preview_ = 0;

	// Refresh canvas
	Refresh();
}

// -----------------------------------------------------------------------------
// Clears the patch textures list
// -----------------------------------------------------------------------------
void CTextureGLCanvas::clearPatches()
{
	CTextureCanvasBase::clearPatches();

	for (auto& id : patch_gl_textures_)
		gl::Texture::clear(id);

	patch_gl_textures_.clear();

	// Refresh canvas
	Refresh();
}

// -----------------------------------------------------------------------------
// Clear the patch at [index]'s image data so it is reloaded next draw
// -----------------------------------------------------------------------------
void CTextureGLCanvas::refreshPatch(unsigned index)
{
	CTextureCanvasBase::refreshPatch(index);

	if (index < patch_gl_textures_.size())
	{
		gl::Texture::clear(patch_gl_textures_[index]);
		patch_gl_textures_[index] = 0;
	}
}

// -----------------------------------------------------------------------------
// Draws the canvas contents
// -----------------------------------------------------------------------------
void CTextureGLCanvas::draw()
{
	dc_.view = &view_;
	drawContent();
}

// -----------------------------------------------------------------------------
// Initializes the shader and context used for drawing
// -----------------------------------------------------------------------------
void CTextureGLCanvas::initDrawing(const Rectd& tex_rect)
{
	// Setup shader
	initShader();
	shader_->setUniform("view_tl", glm::vec2(view_.screenX(tex_rect.tl.x), view_.screenY(tex_rect.tl.y)));
	shader_->setUniform("view_br", glm::vec2(view_.screenX(tex_rect.br.x), view_.screenY(tex_rect.br.y)));
	shader_->setUniform("outside_colour", draw_outside_ ? glm::vec4{ 0.8f, 0.2f, 0.2f, 0.3f } : glm::vec4{ 0.0f });
	shader_->setUniform("colour", glm::vec4{ 1.0f });
	view_.setupShader(*shader_);

	// Setup draw context
	dc_.colour         = ColRGBA::WHITE;
	dc_.outline_colour = ColRGBA::BLACK;
}

// -----------------------------------------------------------------------------
// Draws the offset center lines for the current view type
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawOffsetLines()
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

		view_.setupShader(*lb_sprite_->shader());
		lb_sprite_->draw();
	}
	else if (view_type_ == View::HUD)
		dc_.drawHud();
}

// -----------------------------------------------------------------------------
// Draws the full generated texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawTexture(const Rectd& tex_rect)
{
	// Generate if needed
	if (gl_tex_preview_ == 0)
		gl_tex_preview_ = gl::Texture::createFromImage(*tex_preview_, palette_.get());

	// Draw the texture
	dc_.texture = gl_tex_preview_;
	dc_.colour  = ColRGBA::WHITE;
	dc_.drawRect({ tex_rect.x1(), tex_rect.y1(), tex_rect.x2(), tex_rect.y2() });
}

// -----------------------------------------------------------------------------
// Draws a border around the texture and grid ticks at the grid size intervals
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawTextureBorder(const Rectd& tex_rect)
{
	const auto x1 = tex_rect.x1();
	const auto x2 = tex_rect.x2();
	const auto y1 = tex_rect.y1();
	const auto y2 = tex_rect.y2();

	// Border
	dc_.texture = 0;
	dc_.colour  = ColRGBA::BLACK;
	float ext   = 2 / view_.scale().x;
	dc_.drawRect({ x1 - ext, y1 - ext, x2 + ext, y1 }); // Top
	dc_.drawRect({ x1, y2, x2 + ext, y2 + ext });       // Bottom
	dc_.drawRect({ x1 - ext, y1 - ext, x1, y2 + ext }); // Left
	dc_.drawRect({ x2, y1, x2 + ext, y2 + ext });       // Right

	// Grid ticks
	vector<Rectf> ticks;
	for (float y = y1; y <= y2; y += grid_size_.y) // Vertical
	{
		ticks.emplace_back(x1 - 4, y, x1, y);
		ticks.emplace_back(x2, y, x2 + 4, y);
	}
	for (float x = x1; x <= x2; x += grid_size_.x) // Horizontal
	{
		ticks.emplace_back(x, y1 - 4, x, y1);
		ticks.emplace_back(x, y2, x, y2 + 4);
	}

	dc_.line_thickness = 1.0f;
	dc_.line_aa_radius = 0.0f;
	dc_.drawLines(ticks);
}

// -----------------------------------------------------------------------------
// Draws grid lines across the texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawTextureGrid(const Rectd& tex_rect)
{
	const auto x1 = tex_rect.x1();
	const auto x2 = tex_rect.x2();
	const auto y1 = tex_rect.y1();
	const auto y2 = tex_rect.y2();

	// Grid lines
	vector<Rectf> lines;
	for (float y = y1 + grid_size_.y; y <= y2 - grid_size_.y; y += grid_size_.y) // Vertical
		lines.emplace_back(x1, y, x2, y);
	for (float x = x1 + grid_size_.x; x <= x2 - grid_size_.x; x += grid_size_.x) // Horizontal
		lines.emplace_back(x, y1, x, y2);

	// Draw with inverted blending
	dc_.line_thickness = 1.0f;
	dc_.blend          = gl::Blend::Invert;
	dc_.colour         = ColRGBA::WHITE;
	dc_.drawLines(lines);

	// Draw again with regular blending to darken
	dc_.blend  = gl::Blend::Normal;
	dc_.colour = ColRGBA(0, 0, 0, 64);
	dc_.drawLines(lines);
}

// -----------------------------------------------------------------------------
// Draws patch [index] within [patch_rect] with the given [alpha].
// If [highlight] is true, we are drawing the patch highlight overlay
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawPatch(const Rectd& patch_rect, int index, float alpha, bool highlight)
{
	// Get patch to draw
	const auto patch = texture_->patch(index);
	if (!patch)
		return;

	// Init Patch GLTexture list if needed
	if (patch_gl_textures_.empty())
		patch_gl_textures_.resize(texture_->nPatches());

	// Load the patch as an opengl texture if it isn't already
	if (!patches_[index].image || !gl::Texture::isLoaded(patch_gl_textures_[index]))
	{
		loadPatchImage(index);
		patch_gl_textures_[index] = gl::Texture::createFromImage(*patches_[index].image, palette_.get());
	}

	auto colour = glm::vec4{ 1.0f, 1.0f, 1.0f, alpha };

	// If we're drawing the highlight overlay, use additive blending
	if (highlight)
		gl::setBlend(gl::Blend::Additive);

	gl::VertexBuffer2D vb_patch;
	vb_patch.add({ patch_rect.x1(), patch_rect.y1() }, colour, { 0.0f, 0.0f });
	vb_patch.add({ patch_rect.x1(), patch_rect.y2() }, colour, { 0.0f, 1.0f });
	vb_patch.add({ patch_rect.x2(), patch_rect.y2() }, colour, { 1.0f, 1.0f });
	vb_patch.add({ patch_rect.x2(), patch_rect.y1() }, colour, { 1.0f, 0.0f });

	shader_->bind();
	gl::Texture::bind(patch_gl_textures_[index]);
	vb_patch.push();
	vb_patch.draw(gl::Primitive::TriangleFan);

	gl::setBlend(gl::Blend::Normal);
}

// -----------------------------------------------------------------------------
// Draws an outline around [patch_rect] with the given [colour] and [line_width]
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawPatchOutline(const Rectd& patch_rect, const ColRGBA& colour, double line_width)
{
	vector<Rectf> lines;
	lines.emplace_back(patch_rect.x1(), patch_rect.y1(), patch_rect.x1(), patch_rect.y2());
	lines.emplace_back(patch_rect.x1(), patch_rect.y2(), patch_rect.x2(), patch_rect.y2());
	lines.emplace_back(patch_rect.x2(), patch_rect.y2(), patch_rect.x2(), patch_rect.y1());
	lines.emplace_back(patch_rect.x2(), patch_rect.y1(), patch_rect.x1(), patch_rect.y1());

	dc_.colour         = colour;
	dc_.line_thickness = line_width;
	dc_.drawLines(lines);
}

// -----------------------------------------------------------------------------
// Draws all current text overlays
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawTextOverlays()
{
	gl::View screen_view(false, false);
	screen_view.setSize(view_.size().x, view_.size().y);
	dc_.view = &screen_view;

	dc_.colour         = ColRGBA::WHITE;
	dc_.outline_colour = ColRGBA::BLACK;
	dc_.text_size      = 16;
	dc_.text_style     = gl::draw2d::TextStyle::Outline;
	for (const auto& text : texts_)
	{
		dc_.text_alignment = text.alignment;
		auto pos           = view_.screenPos(text.position.x, text.position.y);
		if (text.above)
			pos.y -= dc_.text_size;

		dc_.drawText(text.text, pos);
	}
}

// -----------------------------------------------------------------------------
// Override of CTextureCanvasBase::loadTexturePreview that also resets the GL
// texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::loadTexturePreview()
{
	CTextureCanvasBase::loadTexturePreview();

	// Reset GL texture
	gl::Texture::clear(gl_tex_preview_);
	gl_tex_preview_ = 0;
}

// -----------------------------------------------------------------------------
// Initialises the composite texture shader
// -----------------------------------------------------------------------------
void CTextureGLCanvas::initShader() const
{
	if (!shader_)
	{
		shader_ = std::make_unique<gl::Shader>("composite_texture");
		shader_->loadResourceEntries("default2d.vert", "ctex.frag");
	}
}
