
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CTextureCanvasBase.cpp
// Description: Base class for CTexture(GL)Canvas containing common
//              functionality
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
#include "CTextureCanvasBase.h"
#include "CTextureCanvas.h"
#include "CTextureGLCanvas.h"
#include "Geometry/Geometry.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/Graphics.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/View.h"
#include "TextureEditor/TextureEditor.h"
#include "UI/Controls/ZoomControl.h"
#include "Utility/Vector.h"

using namespace slade;
using namespace texeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
wxDEFINE_EVENT(EVT_DRAG_END, wxCommandEvent);
CVAR(Bool, tx_arc, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// CTextureCanvasBase Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CTextureCanvasBase class constructor
// -----------------------------------------------------------------------------
CTextureCanvasBase::CTextureCanvasBase(TextureEditor& editor) : editor_{ &editor }
{
	// Init settings from cvars
	tex_scale_    = CVar::getBool("tx_apply_scale");
	draw_outside_ = CVar::getBool("tx_show_outside");
	blend_rgba_   = CVar::getBool("tx_truecolour");
}

// -----------------------------------------------------------------------------
// CTextureCanvasBase class destructor
// -----------------------------------------------------------------------------
CTextureCanvasBase::~CTextureCanvasBase() = default;

// -----------------------------------------------------------------------------
// Sets the view scale (zoom)
// -----------------------------------------------------------------------------
void CTextureCanvasBase::setScale(double scale)
{
	if (zoom_point_.x < 0 && zoom_point_.y < 0)
		view().setScale(scale);
	else
		view().setScale(scale, zoom_point_);
}

// -----------------------------------------------------------------------------
// Sets the offset view type
// -----------------------------------------------------------------------------
void CTextureCanvasBase::setViewType(View type)
{
	bool changed = view_type_ != type;
	view_type_   = type;
	if (changed)
		resetViewOffsets();
}

// -----------------------------------------------------------------------------
// Auto-detects the view type based on the texture's properties
// -----------------------------------------------------------------------------
CTextureView CTextureCanvasBase::autoDetectViewType() const
{
	if (!texture_)
		return view_type_;

	// TEXTUREx textures don't have offsets
	if (!texture_->isExtended())
		return View::Normal;

	// If it's a Sprite or Graphic type, determine if it's a HUD or Sprite view based on its offsets
	if (texture_->typeEnum() == CTexture::Type::Sprite || texture_->typeEnum() == CTexture::Type::Graphic)
	{
		if (gfx::isHudOffsets(texture_->width(), texture_->height(), texture_->offsetX(), texture_->offsetY()))
			return View::HUD;
		else
			return View::Sprite;
	}

	return View::Normal;
}

Vec2i CTextureCanvasBase::dragOffset(bool grid_snap) const
{
	auto origin = view().canvasPos(drag_origin_);
	auto offset = view().canvasPos(mouse_pos_) - origin;
	return geometry::snapToGrid(offset, grid_snap ? grid_size_ : Vec2i{ 1, 1 });
}

// -----------------------------------------------------------------------------
// Clears the current texture and the patch textures list
// -----------------------------------------------------------------------------
void CTextureCanvasBase::clearTexture()
{
	// Clear texture
	connections_.disconnect();
	texture_ = nullptr;

	// Clear patch info
	clearPatches();
	hilight_patch_ = -1;

	// Reset view offset
	resetViewOffsets();

	// Unload full preview
	refreshTexturePreview();
}

// -----------------------------------------------------------------------------
// Clears the patch textures list
// -----------------------------------------------------------------------------
void CTextureCanvasBase::clearPatches()
{
	patch_images_.clear();
}

// -----------------------------------------------------------------------------
// Clear the patch at [index]'s image data so it is reloaded next draw
// -----------------------------------------------------------------------------
void CTextureCanvasBase::refreshPatch(unsigned index)
{
	if (index >= patch_images_.size())
		return;

	patch_images_[index].reset();
}

// -----------------------------------------------------------------------------
// Unloads the full preview image, so it is recreated on next draw
// -----------------------------------------------------------------------------
void CTextureCanvasBase::refreshTexturePreview()
{
	// Unload full preview
	tex_preview_.reset();
}

// -----------------------------------------------------------------------------
// Loads a composite texture to be displayed
// -----------------------------------------------------------------------------
bool CTextureCanvasBase::openTexture(CTexture* tex)
{
	// Clear the current texture
	clearTexture();

	// Set texture
	texture_ = tex;

	// Init patches
	clearPatches();
	for (uint32_t a = 0; a < tex->nPatches(); a++)
		patch_images_.emplace_back();

	// Update when texture is modified
	connections_ += tex->signals().texture_modified.connect([this] { redraw(true); });

	// Update when texture patches are modified
	connections_ += tex->signals().patch_list_changed.connect(
		[this]
		{
			// Reload patches
			clearPatches();
			hilight_patch_ = -1;
			for (uint32_t a = 0; a < texture_->nPatches(); a++)
				patch_images_.emplace_back();

			redraw(true);
		});
	connections_ += tex->signals().patch_modified.connect(
		[this](unsigned index)
		{
			refreshPatch(index);
			redraw(true);
		});
	connections_ += tex->signals().patches_modified.connect(
		[this](const vector<unsigned>& indices)
		{
			for (unsigned index : indices)
				refreshPatch(index);
			redraw(true);
		});

	// Redraw
	resetViewOffsets();
	window()->Refresh();

	return true;
}

// -----------------------------------------------------------------------------
// Resets the view offsets to default based on view type
// -----------------------------------------------------------------------------
void CTextureCanvasBase::resetViewOffsets()
{
	if (view_type_ == View::HUD)
		view().setOffset(160, 100);
	else if (view_type_ == View::Normal && texture_)
		view().setOffset(texture_->width() / 2., texture_->height() / 2.);
	else
		view().setOffset(0, 0);
}

// -----------------------------------------------------------------------------
// Redraws the texture, updating it if [update_texture] is true
// -----------------------------------------------------------------------------
void CTextureCanvasBase::redraw(bool update_texture)
{
	if (update_texture)
		refreshTexturePreview();

	window()->Refresh();
}

// -----------------------------------------------------------------------------
// Returns the index of the patch at [x,y] on the texture, or -1 if no patch is
// at that position
// -----------------------------------------------------------------------------
int CTextureCanvasBase::patchAt(int x, int y) const
{
	// Check a texture is open
	if (!texture_)
		return -1;

	// Go through texture patches backwards (ie from frontmost to back)
	auto tex_rect = textureRect(tex_scale_, view_type_ != View::Normal);
	for (int a = static_cast<int>(texture_->nPatches()) - 1; a >= 0; a--)
	{
		if (!patch_images_[a])
			continue;

		// Check if x,y is within patch bounds
		auto rect = patchRect(a, tex_scale_);
		if (x >= rect.x1() + tex_rect.x1()
			&& x < rect.x2() + tex_rect.x1()
			&& y >= rect.y1() + tex_rect.y1()
			&& y < rect.y2() + tex_rect.y1())
			return a;
	}

	// No patch at x,y
	return -1;
}

// -----------------------------------------------------------------------------
// Swaps patches at [p1] and [p2] in the texture.
// Returns false if either index is invalid, true otherwise
// -----------------------------------------------------------------------------
bool CTextureCanvasBase::swapPatches(size_t p1, size_t p2)
{
	// Check a texture is open
	if (!texture_)
		return false;

	// Check indices
	if (p1 >= texture_->nPatches() || p2 >= texture_->nPatches())
		return false;

	// Swap patch images
	patch_images_[p1].swap(patch_images_[p2]);

	// Swap patches in the texture itself
	return texture_->swapPatches(p1, p2);
}

// -----------------------------------------------------------------------------
// Returns the rectangle of the texture relative to the origin of the canvas,
// taking scaling and offset into account
// -----------------------------------------------------------------------------
Rectd CTextureCanvasBase::textureRect(bool scale, bool offset) const
{
	if (!texture_)
		return {};

	auto sf   = scale ? texture_->scaleFactor() : Vec2d{ 1.0, 1.0 };
	auto offs = offset ? texture_->offset() : Vec2<i16>{ 0, 0 };
	return {
		-offs.x * sf.x, -offs.y * sf.y, (texture_->width() - offs.x) * sf.x, (texture_->height() - offs.y) * sf.y
	};
}

// -----------------------------------------------------------------------------
// Returns the rectangle of the patch at [index] relative to the top-left of the
// texture, taking scaling into account
// -----------------------------------------------------------------------------
Rectd CTextureCanvasBase::patchRect(int index, bool scale) const
{
	if (!texture_)
		return {};

	const auto patch = texture_->patch(index);
	if (!patch)
		return {};

	if (!patch_images_[index])
		return {};

	auto sf = scale ? texture_->scaleFactor() : Vec2d{ 1.0, 1.0 };
	return { (patch->xOffset() * sf.x),
			 (patch->yOffset() * sf.y),
			 ((patch->xOffset() + patch_images_[index]->width()) * sf.x),
			 ((patch->yOffset() + patch_images_[index]->height()) * sf.y) };
}

// -----------------------------------------------------------------------------
// Loads the image for patch [index]
// -----------------------------------------------------------------------------
void CTextureCanvasBase::loadPatchImage(unsigned index)
{
	if (!texture_ || index >= texture_->nPatches())
		return;

	if (!patch_images_[index])
		patch_images_[index] = std::make_unique<SImage>(SImage::Type::PalMask);

	if (!texture_->loadPatchImage(index, *patch_images_[index], editor_->archive(), palette(), blend_rgba_))
	{
		// Failed to load patch image, set to 32x32 red+black checker pattern
		patch_images_[index]->create(32, 32, SImage::Type::RGBA);
		patch_images_[index]->generateCheckeredPattern(8, ColRGBA::RED, ColRGBA::BLACK);
	}

	// Flip/rotate if extended
	if (texture_->isExtended())
	{
		auto patch = dynamic_cast<CTPatchEx*>(texture_->patch(index));
		if (!patch)
			return;

		// Flip
		if (patch->flipX())
			patch_images_[index]->mirror(false);
		if (patch->flipY())
			patch_images_[index]->mirror(true);

		// Rotate
		if (patch->rotation() != 0)
			patch_images_[index]->rotate(patch->rotation());
	}
}

// -----------------------------------------------------------------------------
// Loads the full texture preview image
// -----------------------------------------------------------------------------
void CTextureCanvasBase::loadTexturePreview()
{
	if (!texture_)
		return;

	if (!tex_preview_)
	{
		// Determine image type
		auto type = SImage::Type::PalMask;
		if (blend_rgba_)
			type = SImage::Type::RGBA;

		tex_preview_ = std::make_unique<SImage>(type);
	}

	texture_->toImage(*tex_preview_, editor_->archive(), palette(), blend_rgba_);
}

// -----------------------------------------------------------------------------
// Draws the texture canvas content
// -----------------------------------------------------------------------------
void CTextureCanvasBase::drawContent()
{
	// Aspect Ratio Correction
	auto& view = this->view();
	if (tx_arc)
		view.setScale({ view.scale().x, view.scale().x * 1.2 });
	else
		view.setScale(view.scale().x);

	// Draw offset guides if needed
	drawOffsetLines();

	if (!texture_)
		return;

	// Load patch images
	for (unsigned i = 0; i < patch_images_.size(); ++i)
		if (!patch_images_[i])
			loadPatchImage(i);

	// Calcluate texture and patch rectangles
	auto          tex_rect = textureRect(tex_scale_, view_type_ != View::Normal);
	vector<Rectd> patch_rects;
	patch_rects.reserve(patch_images_.size());
	for (unsigned i = 0; i < patch_images_.size(); ++i)
	{
		patch_rects.push_back(patchRect(i, tex_scale_));
		patch_rects.back().move(tex_rect.x1(), tex_rect.y1());
	}

	// Do any required initialization for drawing
	initDrawing(tex_rect);

	// Draw the texture border
	drawTextureBorder(tex_rect);

	// Draw individual patches if we are dragging or 'show outside' is enabled
	const auto& selection = editor_->selectedPatches();
	if (draw_outside_ || dragging_)
	{
		for (unsigned i = 0; i < patch_images_.size(); ++i)
		{
			// If we're dragging, draw selected patches with 50% opacity
			if (dragging_ && vectorContains(selection, i))
				drawPatch(patch_rects[i], i, 0.5f, false);
			else
				drawPatch(patch_rects[i], i, 1.0f, false);
		}
	}

	// Draw full texture preview if we aren't dragging
	if (!dragging_)
	{
		// Generate full texture preview if needed
		if (!tex_preview_)
			loadTexturePreview();

		drawTexture(tex_rect);
	}

	// Draw dragged patches if currently dragging
	if (dragging_ && (drag_origin_.x >= 0 || drag_origin_.y >= 0))
	{
		auto offset = dragOffset(false);
		for (auto i : selection)
		{
			// Draw patch offset by drag amount
			auto rect = patch_rects[i];
			rect.move(offset.x, offset.y);
			drawPatch(rect, i, 1.0f, false);
			drawPatchOutline(rect, { 70, 210, 220, 255 }, 1.0);

			// Add offset info text
			auto patch = texture_->patch(i);
			texts_.push_back(
				{ .text      = fmt::format("{},{}", patch->xOffset() + offset.x, patch->yOffset() + offset.y),
				  .position  = { rect.tl.x + 1, rect.tl.y + 1 },
				  .alignment = gl::draw2d::Align::Left });
		}
	}

	// Draw grid if needed
	if (show_grid_ || dragging_)
		drawTextureGrid(tex_rect);

	// Draw selected patch outlines (if not dragging)
	if (!dragging_)
		for (auto i : selection)
			drawPatchOutline(patch_rects[i], { 70, 210, 220, 255 }, 2.0);

	// Draw hilighted patch (if not dragging)
	if (hilight_patch_ >= 0 && std::cmp_less(hilight_patch_, texture_->nPatches()) && !dragging_)
	{
		// Highlight
		drawPatch(patch_rects[hilight_patch_], hilight_patch_, 0.15f, true);

		// Outline
		drawPatchOutline(patch_rects[hilight_patch_], { 255, 255, 255, 150 }, 1.5);

		// Add info text
		auto patch = texture_->patch(hilight_patch_);
		auto image = patch_images_[hilight_patch_].get();
		auto mid_x = tex_rect.x1() + patch_rects[hilight_patch_].tl.x + (patch_rects[hilight_patch_].width() * 0.5);
		auto mid_y = tex_rect.y1() + patch_rects[hilight_patch_].tl.y + (patch_rects[hilight_patch_].height() * 0.5);
		texts_.push_back(
			{ .text      = patch->name(),
			  .position  = { mid_x, mid_y },
			  .alignment = gl::draw2d::Align::Center,
			  .above     = true });
		texts_.push_back(
			{ .text      = fmt::format("{} x {}", image->width(), image->height()),
			  .position  = { mid_x, mid_y },
			  .alignment = gl::draw2d::Align::Center });
	}

	// Draw any info texts
	if (!texts_.empty())
	{
		drawTextOverlays();
		texts_.clear();
	}
}


// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when and mouse event is generated (movement/clicking/etc)
// -----------------------------------------------------------------------------
void CTextureCanvasBase::onMouseEvent(wxMouseEvent& e)
{
	bool refresh = false;
	auto p_x     = window()->ToPhys(e.GetX());
	auto p_y     = window()->ToPhys(e.GetY());

	// Update mouse position
	mouse_pos_ = { p_x, p_y };

	// MOUSE MOVEMENT
	if (e.Moving())
	{
		// Check if patch hilight changes
		if (!e.LeftIsDown())
		{
			const auto pos   = view().canvasPos({ p_x, p_y });
			const int  patch = patchAt(pos.x, pos.y);
			if (hilight_patch_ != patch)
			{
				hilight_patch_ = patch;
				refresh        = true;
			}
		}

		e.Skip();
	}

	// MOUSE DRAGGING
	else if (e.Dragging())
	{
		// Check if we are starting a drag
		if (e.LeftIsDown()
			&& !dragging_
			&& (std::abs(p_x - drag_origin_.x) >= 4 || std::abs(p_y - drag_origin_.y) >= 4))
			dragging_ = true;

		if (dragging_)
			refresh = true;

		e.Skip();
	}

	// LEFT BUTTON DOWN
	else if (e.LeftDown())
	{
		drag_origin_ = { p_x, p_y };
		e.Skip();
	}

	// LEFT BUTTON UP
	else if (e.LeftUp())
	{
		// If we were dragging, generate end drag event
		if (dragging_)
		{
			dragging_ = false;
			refreshTexturePreview();
			refresh = true;
			wxCommandEvent evt(EVT_DRAG_END, window()->GetId());
			evt.SetInt(wxMOUSE_BTN_LEFT);
			window()->ProcessWindowEvent(evt);
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
					view().pan(8 * view().scale().x, 0);
				else
					view().pan(-8 * view().scale().x, 0);
			}
			else if (e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
			{
				if (e.GetWheelRotation() > 0)
					view().pan(0, 8 * view().scale().y);
				else
					view().pan(0, -8 * view().scale().y);
			}
		}
		if (!wxGetKeyState(WXK_CONTROL) && linked_zoom_control_ && e.GetWheelAxis() == wxMOUSE_WHEEL_VERTICAL)
		{
			zoom_point_ = { p_x, p_y };

			if (e.GetWheelRotation() > 0)
				linked_zoom_control_->zoomIn(true);
			else
				linked_zoom_control_->zoomOut(true);

			zoom_point_ = { -1, -1 };
		}
	}

	// Refresh is needed
	if (refresh)
		window()->Refresh();
}

// -----------------------------------------------------------------------------
// Creates a new CTextureGLCanvas if OpenGL is available, otherwise will fall
// back to a software-rendered CTextureCanvas
// -----------------------------------------------------------------------------
CTextureCanvasBase* CTextureCanvasBase::createCanvas(wxWindow* parent, TextureEditor& editor)
{
	if (gl::contextCreationFailed() || !CVar::getBool("canvas_use_opengl"))
		return new CTextureCanvas(parent, editor);
	else
		return new CTextureGLCanvas(parent, editor);
}
