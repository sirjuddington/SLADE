
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/SImage/SImage.h"
#include "OpenGL/View.h"
#include "UI/Controls/ZoomControl.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
wxDEFINE_EVENT(EVT_DRAG_END, wxCommandEvent);


// -----------------------------------------------------------------------------
//
// CTextureCanvasBase Class Functions
//
// -----------------------------------------------------------------------------

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
// Selects the patch at [index]
// -----------------------------------------------------------------------------
void CTextureCanvasBase::selectPatch(int index)
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
void CTextureCanvasBase::deSelectPatch(int index)
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
bool CTextureCanvasBase::patchSelected(int index) const
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
void CTextureCanvasBase::clearTexture()
{
	// Clear texture
	sc_patches_modified_.disconnect();
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
	patches_.clear();
}

// -----------------------------------------------------------------------------
// Clear the patch at [index]'s image data so it is reloaded next draw
// -----------------------------------------------------------------------------
void CTextureCanvasBase::refreshPatch(unsigned index)
{
	if (index >= patches_.size())
		return;

	patches_[index].image.reset();
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
bool CTextureCanvasBase::openTexture(CTexture* tex, Archive* parent)
{
	// Clear the current texture
	clearTexture();

	// Set texture
	texture_ = tex;
	parent_  = parent;

	// Init patches
	clearPatches();
	for (uint32_t a = 0; a < tex->nPatches(); a++)
		patches_.emplace_back();

	// Update when texture patches are modified
	sc_patches_modified_ = tex->signals().patches_modified.connect(
		[this](CTexture&)
		{
			// Reload patches
			clearPatches();
			hilight_patch_ = -1;
			for (uint32_t a = 0; a < texture_->nPatches(); a++)
				patches_.emplace_back();

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
	for (int a = static_cast<int>(texture_->nPatches()) - 1; a >= 0; a--)
	{
		auto img = patches_[a].image.get();
		if (!img)
			continue;

		// Check if x,y is within patch bounds
		const auto patch = texture_->patch(a);
		if (x >= patch->xOffset() && x < patch->xOffset() + img->width() && y >= patch->yOffset()
			&& y < patch->yOffset() + img->height())
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
bool CTextureCanvasBase::swapPatches(size_t p1, size_t p2)
{
	// Check a texture is open
	if (!texture_)
		return false;

	// Check indices
	if (p1 >= texture_->nPatches() || p2 >= texture_->nPatches())
		return false;

	// Swap patch images
	patches_[p1].image.swap(patches_[p2].image);

	// Swap patches in the texture itself
	return texture_->swapPatches(p1, p2);
}

// -----------------------------------------------------------------------------
// Loads the image for patch [index]
// -----------------------------------------------------------------------------
void CTextureCanvasBase::loadPatchImage(unsigned index)
{
	if (!texture_ || index >= texture_->nPatches())
		return;

	if (!patches_[index].image)
		patches_[index].image = std::make_unique<SImage>(SImage::Type::PalMask);

	if (!texture_->loadPatchImage(index, *patches_[index].image, parent_, palette(), blend_rgba_))
	{
		// Failed to load patch image, set to 32x32 red+black checker pattern
		patches_[index].image->create(32, 32, SImage::Type::RGBA);
		patches_[index].image->generateCheckeredPattern(8, ColRGBA::RED, ColRGBA::BLACK);
	}

	// Flip/rotate if extended
	if (texture_->isExtended())
	{
		auto patch = dynamic_cast<CTPatchEx*>(texture_->patch(index));
		if (!patch)
			return;

		// Flip
		if (patch->flipX())
			patches_[index].image->mirror(false);
		if (patch->flipY())
			patches_[index].image->mirror(true);

		// Rotate
		if (patch->rotation() != 0)
			patches_[index].image->rotate(patch->rotation());
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

	texture_->toImage(*tex_preview_, parent_, palette(), blend_rgba_);
}


// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when and mouse event is generated (movement/clicking/etc)
// -----------------------------------------------------------------------------
void CTextureCanvasBase::onMouseEvent(wxMouseEvent& e)
{
	bool refresh = false;
	auto p_x     = e.GetX() * window()->GetContentScaleFactor();
	auto p_y     = e.GetY() * window()->GetContentScaleFactor();

	// MOUSE MOVEMENT
	if (e.Moving() || e.Dragging())
	{
		dragging_ = e.LeftIsDown();

		// Check if patch hilight changes
		const auto pos   = view().canvasPos({ p_x, p_y });
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

	// Update 'previous' mouse coordinates
	mouse_prev_ = { p_x, p_y };
}
