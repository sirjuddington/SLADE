
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

	// Clear buffers
	if (lb_border_)
		lb_border_->buffer().clear();
	if (lb_grid_)
		lb_grid_->buffer().clear();

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
	if (!texture_)
		return;

	// Aspect Ratio Correction
	if (tx_arc)
		view_.setScale({ view_.scale().x, view_.scale().x * 1.2 });
	else
		view_.setScale(view_.scale().x);

	// Draw offset guides if needed
	gl::draw2d::Context dc(&view_);
	drawOffsetLines(dc);

	// Determine offset/scale
	glm::vec2 offset{ 0.0f }, scale{ 1.0f };
	if (tex_scale_)
	{
		// Apply texture scale
		auto tscalex = static_cast<float>(texture_->scaleX());
		if (tscalex == 0.0f)
			tscalex = 1.0f;
		auto tscaley = static_cast<float>(texture_->scaleY());
		if (tscaley == 0.0f)
			tscaley = 1.0f;
		scale = { 1.0f / tscalex, 1.0f / tscaley };
	}
	if (view_type_ != View::Normal)
	{
		// Apply texture offsets
		offset.x = static_cast<float>(texture_->offsetX());
		offset.y = static_cast<float>(texture_->offsetY());
	}

	// Setup shader
	initShader();
	shader_->bind();
	shader_->setUniform("view_tl", glm::vec2(view_.screenX(0), view_.screenY(0)));
	shader_->setUniform("view_br", glm::vec2(view_.screenX(texture_->width()), view_.screenY(texture_->height())));
	shader_->setUniform("outside_colour", draw_outside_ ? glm::vec4{ 0.8f, 0.2f, 0.2f, 0.3f } : glm::vec4{ 0.0f });
	shader_->setUniform("colour", glm::vec4{ 1.0f });
	view_.setupShader(*shader_);

	// Load patch images
	for (unsigned i = 0; i < patches_.size(); ++i)
		if (!patches_[i].image)
			loadPatchImage(i);

	// Draw the texture
	drawTexture(dc, scale, offset, draw_outside_ || dragging_);
	drawTextureBorder(scale, offset);

	// Draw selected patch outlines
	dc.colour         = { 70, 210, 220, 255 };
	dc.line_thickness = 2.0f;
	dc.line_aa_radius = 0.0f;
	for (unsigned a = 0; a < patches_.size(); a++)
		if (patches_[a].selected)
			drawPatchOutline(dc, a);

	// Draw hilighted patch outline
	if (hilight_patch_ >= 0 && hilight_patch_ < static_cast<int>(texture_->nPatches()))
	{
		dc.colour = { 255, 255, 255, 150 };
		dc.blend  = gl::Blend::Additive;
		drawPatchOutline(dc, hilight_patch_);
	}
}

// -----------------------------------------------------------------------------
// Draws the currently opened composite texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawTexture(gl::draw2d::Context& dc, glm::vec2 scale, glm::vec2 offset, bool draw_patches)
{
	auto width  = texture_->width();
	auto height = texture_->height();

	// Draw all individual patches if needed (eg. while dragging or 'draw outside' is enabled)
	if (draw_patches)
	{
		for (uint32_t a = 0; a < texture_->nPatches(); a++)
			drawPatch(a);
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
		dc.drawRect({ offset.x, offset.y, offset.x + width * scale.x, offset.y + height * scale.y, false });
	}
}

// -----------------------------------------------------------------------------
// Draws the patch at index [num] in the composite texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawPatch(int num)
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

	auto xoff   = static_cast<float>(patch->xOffset());
	auto yoff   = static_cast<float>(patch->yOffset());
	auto width  = static_cast<float>(patches_[num].image->width());
	auto height = static_cast<float>(patches_[num].image->height());
	auto colour = glm::vec4{ 1.0f };

	gl::VertexBuffer2D vb_patch;
	vb_patch.add({ xoff, yoff }, colour, { 0.0f, 0.0f });
	vb_patch.add({ xoff, yoff + height }, colour, { 0.0f, 1.0f });
	vb_patch.add({ xoff + width, yoff + height }, colour, { 1.0f, 1.0f });
	vb_patch.add({ xoff + width, yoff }, colour, { 1.0f, 0.0f });

	gl::Texture::bind(patch_gl_textures_[num]);
	vb_patch.push();
	vb_patch.draw(gl::Primitive::TriangleFan);
}

// -----------------------------------------------------------------------------
// Draws the outline of the patch at index [num] in the composite texture
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawPatchOutline(const gl::draw2d::Context& dc, int num) const
{
	// Get patch
	const auto patch = texture_->patch(num);
	if (!patch)
		return;

	auto x1 = static_cast<float>(patch->xOffset());
	auto y1 = static_cast<float>(patch->yOffset());
	auto x2 = x1 + static_cast<float>(patches_[num].image->width());
	auto y2 = y1 + static_cast<float>(patches_[num].image->height());

	vector<Rectf> lines;
	lines.emplace_back(x1, y1, x1, y2);
	lines.emplace_back(x1, y2, x2, y2);
	lines.emplace_back(x2, y2, x2, y1);
	lines.emplace_back(x2, y1, x1, y1);

	dc.drawLines(lines);
}

// -----------------------------------------------------------------------------
// Draws a black border around the texture w/ticks, and a grid if dragging
// -----------------------------------------------------------------------------
void CTextureGLCanvas::drawTextureBorder(glm::vec2 scale, glm::vec2 offset)
{
	constexpr float ext = 0.0f;
	const auto      x1  = offset.x;
	const auto      x2  = offset.x + texture_->width() * scale.x;
	const auto      y1  = offset.y;
	const auto      y2  = offset.y + texture_->height() * scale.y;

	// Setup border buffer if needed
	if (!lb_border_)
	{
		lb_border_ = std::make_unique<gl::LineBuffer>();
		lb_border_->setAaRadius(0.0f, 0.0f);
	}
	if (lb_border_->buffer().empty())
	{
		glm::vec4 colour = ColRGBA::BLACK;

		// Border
		lb_border_->add2d(x1 - ext, y1 - ext, x1 - ext, y2 + ext, colour, 2.0f);
		lb_border_->add2d(x1 - ext, y2 + ext, x2 + ext, y2 + ext, colour, 2.0f);
		lb_border_->add2d(x2 + ext, y2 + ext, x2 + ext, y1 - ext, colour, 2.0f);
		lb_border_->add2d(x2 + ext, y1 - ext, x1 - ext, y1 - ext, colour, 2.0f);

		// Vertical ticks
		colour.a = 0.6f;
		for (float y = y1; y <= y2; y += 8.0f)
		{
			lb_border_->add2d(x1 - 4, y, x1, y, colour);
			lb_border_->add2d(x2, y, x2 + 4, y, colour);
		}

		// Horizontal ticks
		for (float x = x1; x <= x2; x += 8.0f)
		{
			lb_border_->add2d(x, y1 - 4, x, y1, colour);
			lb_border_->add2d(x, y2, x, y2 + 4, colour);
		}

		lb_border_->push();
	}

	// Draw border lines
	lb_border_->draw(&view_);

	// Draw grid if shown
	if (show_grid_)
	{
		// Setup grid buffer if needed
		if (!lb_grid_)
		{
			lb_grid_ = std::make_unique<gl::LineBuffer>();
			lb_grid_->setAaRadius(0.0f, 0.0f);
		}
		if (lb_grid_->buffer().empty())
		{
			auto colour = glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f };

			// Vertical
			for (float y = y1 + 8.0f; y <= y2 - 8.0f; y += 8.0f)
				lb_grid_->add2d(x1, y, x2, y, colour);

			// Horizontal
			for (float x = x1 + 8.0f; x <= x2 - 8.0f; x += 8.0f)
				lb_grid_->add2d(x, y1, x, y2, colour);

			lb_grid_->push();
		}

		// Draw with inverted blending
		gl::setBlend(gl::Blend::Invert);
		lb_grid_->draw(&view_);

		// Draw again with regular blending to darken
		gl::setBlend(gl::Blend::Normal);
		lb_grid_->draw(&view_, { 0.0f, 0.0f, 0.0f, 0.5f });
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

		view_.setupShader(lb_sprite_->shader());
		lb_sprite_->draw();
	}
	else if (view_type_ == View::HUD)
		dc.drawHud();
}
