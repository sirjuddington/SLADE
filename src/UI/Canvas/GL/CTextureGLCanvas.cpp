
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
#include "GLCanvas.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
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
CVAR(Bool, tx_arc, false, CVar::Flag::Save)
EXTERN_CVAR(Bool, gfx_show_border)


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
	// Aspect Ratio Correction
	if (tx_arc)
		view_.setScale({ view_.scale().x, view_.scale().x * 1.2 });
	else
		view_.setScale(view_.scale().x);

	// Draw offset guides if needed
	gl::draw2d::Context dc(&view_);
	drawOffsetLines(dc);

	if (!texture_)
		return;

	auto tex_rect = textureRect(tex_scale_, view_type_ != View::Normal);

	// Setup shader
	initShader();
	shader_->bind();
	shader_->setUniform("view_tl", glm::vec2(view_.screenX(tex_rect.tl.x), view_.screenY(tex_rect.tl.y)));
	shader_->setUniform("view_br", glm::vec2(view_.screenX(tex_rect.br.x), view_.screenY(tex_rect.br.y)));
	shader_->setUniform("outside_colour", draw_outside_ ? glm::vec4{ 0.8f, 0.2f, 0.2f, 0.3f } : glm::vec4{ 0.0f });
	shader_->setUniform("colour", glm::vec4{ 1.0f });
	view_.setupShader(*shader_);

	// Load patch images
	for (unsigned i = 0; i < patches_.size(); ++i)
		if (!patches_[i].image)
			loadPatchImage(i);

	// Draw the texture
	drawTexture(dc, tex_rect, draw_outside_ || dragging_);
	drawTextureBorder(tex_rect);

	// Draw selected patch outlines
	dc.colour         = { 70, 210, 220, 255 };
	dc.line_thickness = 2.0f;
	dc.line_aa_radius = 0.0f;
	for (unsigned a = 0; a < patches_.size(); a++)
		if (patches_[a].selected)
			drawPatchOutline(dc, a, tex_rect);

	// Draw hilighted patch outline
	if (hilight_patch_ >= 0 && std::cmp_less(hilight_patch_, texture_->nPatches()))
	{
		dc.colour = { 255, 255, 255, 150 };
		dc.blend  = gl::Blend::Additive;
		drawPatchOutline(dc, hilight_patch_, tex_rect);
	}
}

// -----------------------------------------------------------------------------
// Draws the currently opened composite texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawTexture(gl::draw2d::Context& dc, const Rectd& tex_rect, bool draw_patches)
{
	// Draw all individual patches if needed (eg. while dragging or 'draw outside' is enabled)
	if (draw_patches)
	{
		for (uint32_t a = 0; a < texture_->nPatches(); a++)
			drawPatch(a, tex_rect);
	}

	// If we aren't currently dragging a patch, draw the fully generated texture
	if (!dragging_)
	{
		// Generate if needed
		if (!tex_preview_ || gl_tex_preview_ == 0)
		{
			loadTexturePreview();
			gl_tex_preview_ = gl::Texture::createFromImage(*tex_preview_, palette_.get());
		}

		// Draw the texture
		dc.texture = gl_tex_preview_;
		dc.drawRect({ tex_rect.x1(), tex_rect.y1(), tex_rect.x2(), tex_rect.y2() });
	}
}

// -----------------------------------------------------------------------------
// Draws the patch at index [num] in the composite texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawPatch(int num, const Rectd& tex_rect)
{
	// Get patch to draw
	const auto patch = texture_->patch(num);
	if (!patch)
		return;

	// Init Patch GLTexture list if needed
	if (patch_gl_textures_.empty())
		patch_gl_textures_.resize(texture_->nPatches());

	// Load the patch as an opengl texture if it isn't already
	if (!patches_[num].image || !gl::Texture::isLoaded(patch_gl_textures_[num]))
	{
		loadPatchImage(num);
		patch_gl_textures_[num] = gl::Texture::createFromImage(*patches_[num].image, palette_.get());
	}

	auto rect   = patchRect(num, tex_scale_);
	auto colour = glm::vec4{ 1.0f };

	gl::VertexBuffer2D vb_patch;
	vb_patch.add({ tex_rect.x1() + rect.x1(), tex_rect.y1() + rect.y1() }, colour, { 0.0f, 0.0f });
	vb_patch.add({ tex_rect.x1() + rect.x1(), tex_rect.y1() + rect.y2() }, colour, { 0.0f, 1.0f });
	vb_patch.add({ tex_rect.x1() + rect.x2(), tex_rect.y1() + rect.y2() }, colour, { 1.0f, 1.0f });
	vb_patch.add({ tex_rect.x1() + rect.x2(), tex_rect.y1() + rect.y1() }, colour, { 1.0f, 0.0f });

	gl::Texture::bind(patch_gl_textures_[num]);
	vb_patch.push();
	vb_patch.draw(gl::Primitive::TriangleFan);
}

// -----------------------------------------------------------------------------
// Draws the outline of the patch at index [num] in the composite texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawPatchOutline(const gl::draw2d::Context& dc, int num, const Rectd& tex_rect) const
{
	// Get patch
	const auto patch = texture_->patch(num);
	if (!patch)
		return;

	auto rect = patchRect(num, tex_scale_);

	vector<Rectf> lines;
	lines.emplace_back(
		tex_rect.x1() + rect.x1(), tex_rect.y1() + rect.y1(), tex_rect.x1() + rect.x1(), tex_rect.y1() + rect.y2());
	lines.emplace_back(
		tex_rect.x1() + rect.x1(), tex_rect.y1() + rect.y2(), tex_rect.x1() + rect.x2(), tex_rect.y1() + rect.y2());
	lines.emplace_back(
		tex_rect.x1() + rect.x2(), tex_rect.y1() + rect.y2(), tex_rect.x1() + rect.x2(), tex_rect.y1() + rect.y1());
	lines.emplace_back(
		tex_rect.x1() + rect.x2(), tex_rect.y1() + rect.y1(), tex_rect.x1() + rect.x1(), tex_rect.y1() + rect.y1());

	dc.drawLines(lines);
}

// -----------------------------------------------------------------------------
// Draws a black border around the texture w/ticks, and a grid if dragging
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawTextureBorder(const Rectd& tex_rect) const
{
	constexpr float ext = 0.0f;
	const auto      x1  = tex_rect.x1();
	const auto      x2  = tex_rect.x2();
	const auto      y1  = tex_rect.y1();
	const auto      y2  = tex_rect.y2();

	auto line_buffer = std::make_unique<gl::LineBuffer>();
	line_buffer->setAaRadius(0.0f, 0.0f);

	// Populate line buffer for border
	glm::vec4 colour = ColRGBA::BLACK;

	// Border
	line_buffer->add2d(x1 - ext, y1 - ext, x1 - ext, y2 + ext, colour, 2.0f);
	line_buffer->add2d(x1 - ext, y2 + ext, x2 + ext, y2 + ext, colour, 2.0f);
	line_buffer->add2d(x2 + ext, y2 + ext, x2 + ext, y1 - ext, colour, 2.0f);
	line_buffer->add2d(x2 + ext, y1 - ext, x1 - ext, y1 - ext, colour, 2.0f);

	// Vertical ticks
	colour.a = 0.6f;
	for (float y = y1; y <= y2; y += 8.0f)
	{
		line_buffer->add2d(x1 - 4, y, x1, y, colour);
		line_buffer->add2d(x2, y, x2 + 4, y, colour);
	}

	// Horizontal ticks
	for (float x = x1; x <= x2; x += 8.0f)
	{
		line_buffer->add2d(x, y1 - 4, x, y1, colour);
		line_buffer->add2d(x, y2, x, y2 + 4, colour);
	}

	// Draw border lines
	line_buffer->push();
	line_buffer->draw(&view_);

	// Draw grid if shown
	if (show_grid_)
	{
		// Populate line buffer for grid
		auto col_grid = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };

		// Vertical
		for (float y = y1 + 8.0f; y <= y2 - 8.0f; y += 8.0f)
			line_buffer->add2d(x1, y, x2, y, col_grid);

		// Horizontal
		for (float x = x1 + 8.0f; x <= x2 - 8.0f; x += 8.0f)
			line_buffer->add2d(x, y1, x, y2, col_grid);

		line_buffer->push();

		// Draw with inverted blending
		gl::setBlend(gl::Blend::Invert);
		line_buffer->draw(&view_);

		// Draw again with regular blending to darken
		gl::setBlend(gl::Blend::Normal);
		line_buffer->draw(&view_, { 0.0f, 0.0f, 0.0f, 0.5f });
	}
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

// -----------------------------------------------------------------------------
// Draws the offset center lines
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawOffsetLines(const gl::draw2d::Context& dc)
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
		dc.drawHud();
}
