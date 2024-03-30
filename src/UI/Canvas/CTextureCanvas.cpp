
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CTextureCanvas.cpp
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
#include "CTextureCanvas.h"
#include "GLCanvas.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include "UI/Controls/ZoomControl.h"
#include <glm/ext/matrix_transform.hpp>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
unique_ptr<gl::Shader> CTextureCanvas::shader_;
wxDEFINE_EVENT(EVT_DRAG_END, wxCommandEvent);
CVAR(Bool, tx_arc, false, CVar::Flag::Save)
EXTERN_CVAR(Bool, gfx_show_border)


// -----------------------------------------------------------------------------
//
// CTextureCanvas Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CTextureCanvas class constructor
// -----------------------------------------------------------------------------
CTextureCanvas::CTextureCanvas(wxWindow* parent) : GLCanvas(parent, BGStyle::Checkered)
{
	palette_ = std::make_unique<Palette>();
	view_.setCentered(true);

	// Bind events
	setupMousePanning();
	Bind(wxEVT_MOTION, &CTextureCanvas::onMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &CTextureCanvas::onMouseEvent, this);
	Bind(wxEVT_LEAVE_WINDOW, &CTextureCanvas::onMouseEvent, this);
	Bind(wxEVT_MOUSEWHEEL, &CTextureCanvas::onMouseEvent, this);
}

CTextureCanvas::~CTextureCanvas() = default;

void CTextureCanvas::setScale(double scale)
{
	if (zoom_point_.x < 0 && zoom_point_.y < 0)
		view_.setScale(scale);
	else
		view_.setScale(scale, zoom_point_);
}

void CTextureCanvas::setViewType(View type)
{
	bool changed = view_type_ != type;
	view_type_   = type;
	if (changed)
		resetViewOffsets();
}

// -----------------------------------------------------------------------------
// Selects the patch at [index]
// -----------------------------------------------------------------------------
void CTextureCanvas::selectPatch(int index)
{
	// Check patch index is ok
	if (index < 0 || static_cast<unsigned>(index) >= texture_->nPatches())
		return;

	// Select the patch
	patches_[index].selected = true;
}

// -----------------------------------------------------------------------------
// De-Selects the patch at [index]
// -----------------------------------------------------------------------------
void CTextureCanvas::deSelectPatch(int index)
{
	// Check patch index is ok
	if (index < 0 || static_cast<unsigned>(index) >= texture_->nPatches())
		return;

	// De-Select the patch
	patches_[index].selected = false;
}

// -----------------------------------------------------------------------------
// Returns true if the patch at [index] is selected, false otherwise
// -----------------------------------------------------------------------------
bool CTextureCanvas::patchSelected(int index) const
{
	// Check index is ok
	if (index < 0 || static_cast<unsigned>(index) >= texture_->nPatches())
		return false;

	// Return if patch index is selected
	return patches_[index].selected;
}

// -----------------------------------------------------------------------------
// Clears the current texture and the patch textures list
// -----------------------------------------------------------------------------
void CTextureCanvas::clearTexture()
{
	// Clear texture
	sc_patches_modified_.disconnect();
	texture_ = nullptr;

	// Clear patch info
	clearPatches();
	hilight_patch_ = -1;

	// Clear buffers
	if (lb_border_)
		lb_border_->buffer().clear();
	if (lb_grid_)
		lb_grid_->buffer().clear();

	// Reset view offset
	resetViewOffsets();

	// Clear full preview
	gl::Texture::clear(tex_preview_);
	tex_preview_ = 0;

	// Refresh canvas
	Refresh();
}

// -----------------------------------------------------------------------------
// Clears the patch textures list
// -----------------------------------------------------------------------------
void CTextureCanvas::clearPatches()
{
	patches_.clear();

	// Refresh canvas
	Refresh();
}

// -----------------------------------------------------------------------------
// Unloads all patch textures, so they are reloaded on next draw
// -----------------------------------------------------------------------------
void CTextureCanvas::updatePatchTextures()
{
	// Unload single patch textures
	for (auto& p : patches_)
	{
		gl::Texture::clear(p.texture);
		p.texture = 0;
	}

	// Unload full preview
	gl::Texture::clear(tex_preview_);
	tex_preview_ = 0;
}

// -----------------------------------------------------------------------------
// Unloads the full preview texture, so it is reloaded on next draw
// -----------------------------------------------------------------------------
void CTextureCanvas::updateTexturePreview()
{
	// Unload full preview
	gl::Texture::clear(tex_preview_);
	tex_preview_ = 0;
}

// -----------------------------------------------------------------------------
// Loads a composite texture to be displayed
// -----------------------------------------------------------------------------
bool CTextureCanvas::openTexture(CTexture* tex, Archive* parent)
{
	// Clear the current texture
	clearTexture();

	// Set texture
	texture_ = tex;
	parent_  = parent;

	// Init patches
	clearPatches();
	for (uint32_t a = 0; a < tex->nPatches(); a++)
		patches_.push_back({ gl::Texture::create(), false, {} });

	// Update when texture patches are modified
	sc_patches_modified_ = tex->signals().patches_modified.connect(
		[this](CTexture&)
		{
			// Reload patches
			clearPatches();
			hilight_patch_ = -1;
			for (uint32_t a = 0; a < texture_->nPatches(); a++)
				patches_.push_back({ gl::Texture::create(), false, {} });

			redraw(true);
		});

	// Redraw
	resetViewOffsets();
	Refresh();

	return true;
}

// -----------------------------------------------------------------------------
// Draws the canvas contents
// -----------------------------------------------------------------------------
void CTextureCanvas::draw()
{
	if (!texture_)
		return;

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
void CTextureCanvas::drawTexture(gl::draw2d::Context& dc, glm::vec2 scale, glm::vec2 offset, bool draw_patches)
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
		if (!tex_preview_)
		{
			// Determine image type
			auto type = SImage::Type::PalMask;
			if (blend_rgba_)
				type = SImage::Type::RGBA;

			// CTexture -> temp Image -> GLTexture
			SImage temp(type);
			texture_->toImage(temp, parent_, palette_.get(), blend_rgba_);
			tex_preview_ = gl::Texture::createFromImage(temp, palette_.get());
		}

		// Draw the texture
		dc.texture = tex_preview_;
		dc.drawRect({ offset.x, offset.y, offset.x + width * scale.x, offset.y + height * scale.y, false });
	}
}

// -----------------------------------------------------------------------------
// Draws the patch at index [num] in the composite texture
// -----------------------------------------------------------------------------
void CTextureCanvas::drawPatch(int num, bool outside)
{
	// Get patch to draw
	const auto patch = texture_->patch(num);

	// Check it exists
	if (!patch)
		return;

	// Load the patch as an opengl texture if it isn't already
	if (!gl::Texture::isLoaded(patches_[num].texture))
	{
		SImage temp(SImage::Type::PalMask);
		if (texture_->loadPatchImage(num, temp, parent_, palette_.get(), blend_rgba_))
		{
			// Load the image as a texture
			patches_[num].texture = gl::Texture::createFromImage(temp, palette_.get());
		}
		else
			patches_[num].texture = gl::Texture::missingTexture();
	}

	// Determine rotation/flip (if extended)
	auto rotation = 0;
	bool flipx    = false;
	bool flipy    = false;
	if (texture_->isExtended())
	{
		const auto epatch = dynamic_cast<CTPatchEx*>(patch);
		flipx             = epatch->flipX();
		flipy             = epatch->flipY();
		rotation          = epatch->rotation();
		if (rotation < 0)
			rotation = 360 - rotation;
	}

	gl::VertexBuffer2D vb_patch;

	auto& tex_info = gl::Texture::info(patches_[num].texture);
	auto  xoff     = static_cast<float>(patch->xOffset());
	auto  yoff     = static_cast<float>(patch->yOffset());
	auto  width    = static_cast<float>(tex_info.size.x);
	auto  height   = static_cast<float>(tex_info.size.y);
	auto  tx1      = flipx ? 1.0f : 0.0f;
	auto  ty1      = flipy ? 1.0f : 0.0f;
	auto  tx2      = 1.0f - tx1;
	auto  ty2      = 1.0f - ty1;
	auto  colour   = glm::vec4{ 1.0f };
	if (rotation == 90)
	{
		patches_[num].rect.set(xoff, yoff, xoff + height, yoff + width);
		vb_patch.add({ xoff, yoff }, colour, { tx1, ty2 });
		vb_patch.add({ xoff, yoff + width }, colour, { tx2, ty2 });
		vb_patch.add({ xoff + height, yoff + width }, colour, { tx2, ty1 });
		vb_patch.add({ xoff + height, yoff }, colour, { tx1, ty1 });
	}
	else if (rotation == 180)
	{
		patches_[num].rect.set(xoff, yoff, xoff + width, yoff + height);
		vb_patch.add({ xoff, yoff }, colour, { tx2, ty2 });
		vb_patch.add({ xoff, yoff + height }, colour, { tx2, ty1 });
		vb_patch.add({ xoff + width, yoff + height }, colour, { tx1, ty1 });
		vb_patch.add({ xoff + width, yoff }, colour, { tx1, ty2 });
	}
	else if (rotation == 270)
	{
		patches_[num].rect.set(xoff, yoff, xoff + height, yoff + width);
		vb_patch.add({ xoff, yoff }, colour, { tx2, ty1 });
		vb_patch.add({ xoff, yoff + width }, colour, { tx1, ty1 });
		vb_patch.add({ xoff + height, yoff + width }, colour, { tx1, ty2 });
		vb_patch.add({ xoff + height, yoff }, colour, { tx2, ty2 });
	}
	else
	{
		// No rotation
		patches_[num].rect.set(xoff, yoff, xoff + width, yoff + height);
		vb_patch.add({ xoff, yoff }, colour, { tx1, ty1 });
		vb_patch.add({ xoff, yoff + height }, colour, { tx1, ty2 });
		vb_patch.add({ xoff + width, yoff + height }, colour, { tx2, ty2 });
		vb_patch.add({ xoff + width, yoff }, colour, { tx2, ty1 });
	}

	gl::Texture::bind(patches_[num].texture);
	vb_patch.draw(gl::Primitive::TriangleFan);
}

void CTextureCanvas::drawPatchOutline(const gl::draw2d::Context& dc, int num) const
{
	const auto&   rect = patches_[num].rect;
	vector<Rectf> lines;
	lines.emplace_back(rect.tl.x, rect.tl.y, rect.tl.x, rect.br.y);
	lines.emplace_back(rect.tl.x, rect.br.y, rect.br.x, rect.br.y);
	lines.emplace_back(rect.br.x, rect.br.y, rect.br.x, rect.tl.y);
	lines.emplace_back(rect.br.x, rect.tl.y, rect.tl.x, rect.tl.y);

	dc.drawLines(lines);
}

// -----------------------------------------------------------------------------
// Draws a black border around the texture
// -----------------------------------------------------------------------------
void CTextureCanvas::drawTextureBorder(glm::vec2 scale, glm::vec2 offset)
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

void CTextureCanvas::initShader() const
{
	if (!shader_)
	{
		shader_ = std::make_unique<gl::Shader>("composite_texture");
		shader_->loadResourceEntries("default2d.vert", "ctex.frag");
	}
}

void CTextureCanvas::resetViewOffsets()
{
	if (view_type_ == View::HUD)
		view_.setOffset(160, 100);
	else if (view_type_ == View::Normal && texture_)
		view_.setOffset(texture_->width() / 2., texture_->height() / 2.);
	else
		view_.setOffset(0, 0);
}

// -----------------------------------------------------------------------------
// Draws the offset center lines
// -----------------------------------------------------------------------------
void CTextureCanvas::drawOffsetLines(const gl::draw2d::Context& dc)
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
// Redraws the texture, updating it if [update_texture] is true
// -----------------------------------------------------------------------------
void CTextureCanvas::redraw(bool update_texture)
{
	if (update_texture)
		updateTexturePreview();

	Refresh();
}

Vec2i CTextureCanvas::screenToTexPosition(int x, int y) const
{
	return { static_cast<int>(view_.canvasX(x)), static_cast<int>(view_.canvasY(y)) };
}

Vec2i CTextureCanvas::texToScreenPosition(int x, int y) const
{
	return { view_.screenX(x), view_.screenY(y) };
}

// -----------------------------------------------------------------------------
// Returns the index of the patch at [x,y] on the texture, or -1 if no patch is
// at that position
// -----------------------------------------------------------------------------
int CTextureCanvas::patchAt(int x, int y) const
{
	// Check a texture is open
	if (!texture_)
		return -1;

	// Go through texture patches backwards (ie from frontmost to back)
	for (int a = static_cast<int>(texture_->nPatches()) - 1; a >= 0; a--)
	{
		// Check if x,y is within patch bounds
		const auto patch    = texture_->patch(a);
		auto&      tex_info = gl::Texture::info(patches_[a].texture);
		if (x >= patch->xOffset() && x < patch->xOffset() + tex_info.size.x && y >= patch->yOffset()
			&& y < patch->yOffset() + tex_info.size.y)
		{
			return a;
		}
	}

	// No patch at x,y
	return -1;
}

// -----------------------------------------------------------------------------
// Swaps patches at [p1] and [p2] in the texture.
// Returns false if either index is invalid, true otherwise
// -----------------------------------------------------------------------------
bool CTextureCanvas::swapPatches(size_t p1, size_t p2)
{
	// Check a texture is open
	if (!texture_)
		return false;

	// Check indices
	if (p1 >= texture_->nPatches() || p2 >= texture_->nPatches())
		return false;

	// Swap patch gl textures
	std::swap(patches_[p1].texture, patches_[p2].texture);

	// Swap patches in the texture itself
	return texture_->swapPatches(p1, p2);
}

// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when and mouse event is generated (movement/clicking/etc)
// -----------------------------------------------------------------------------
void CTextureCanvas::onMouseEvent(wxMouseEvent& e)
{
	bool refresh = false;

	// MOUSE MOVEMENT
	if (e.Moving() || e.Dragging())
	{
		dragging_ = e.LeftIsDown();

		// Check if patch hilight changes
		const auto pos   = view_.canvasPos({ e.GetX(), e.GetY() });
		const int  patch = patchAt(pos.x, pos.y);
		if (hilight_patch_ != patch)
		{
			hilight_patch_ = patch;
			refresh        = true;
		}

		e.Skip();
	}

	// LEFT BUTTON UP
	else if (e.LeftUp())
	{
		// If we were dragging, generate end drag event
		if (dragging_)
		{
			dragging_ = false;
			updateTexturePreview();
			refresh = true;
			wxCommandEvent evt(EVT_DRAG_END, GetId());
			evt.SetInt(wxMOUSE_BTN_LEFT);
			ProcessWindowEvent(evt);
		}
	}

	// LEAVING
	if (e.Leaving())
	{
		// Set no hilighted patch
		hilight_patch_ = -1;
		refresh        = true;
	}

	// MOUSEWHEEL
	if (e.GetWheelRotation() != 0)
	{
		if (wxGetKeyState(WXK_CONTROL))
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
			zoom_point_ = { e.GetPosition().x, e.GetPosition().y };

			if (e.GetWheelRotation() > 0)
				linked_zoom_control_->zoomIn(true);
			else
				linked_zoom_control_->zoomOut(true);

			zoom_point_ = { -1, -1 };
		}
	}

	// Refresh is needed
	if (refresh)
		Refresh();

	// Update 'previous' mouse coordinates
	mouse_prev_ = { e.GetPosition().x * GetContentScaleFactor(), e.GetPosition().y * GetContentScaleFactor() };
}
