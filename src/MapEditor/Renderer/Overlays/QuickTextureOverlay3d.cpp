
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    QuickTextureOverlay.cpp
// Description: QuickTextureOverlay class - A full screen map editor overlay
//              that shows a scrollable list of textures in 3d mode.
//              The currently selected texture is applied in real-time to the
//              currently selected walls/flats, giving a quick 'preview' of how
//              it would look
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
#include "QuickTextureOverlay3d.h"
#include "Game/Configuration.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// QuickTextureOverlay3d Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// QuickTextureOverlay3d class constructor
// -----------------------------------------------------------------------------
QuickTextureOverlay3d::QuickTextureOverlay3d(MapEditContext* editor) : MCOverlay(true), editor_{ editor }
{
	if (editor)
	{
		auto& sel = editor->selection();

		if (!ok(sel))
		{
			active_ = false;
			return;
		}

		// Determine texture type
		int initial = 0;
		if (!game::configuration().featureSupported(game::Feature::MixTexFlats))
		{
			sel_walls_ = false;
			for (unsigned a = 0; a < sel.size(); a++)
			{
				if (sel[a].type != mapeditor::ItemType::Thing && sel[a].type != mapeditor::ItemType::Ceiling
					&& sel[a].type != mapeditor::ItemType::Floor)
				{
					sel_walls_ = true;
					initial    = a;
					break;
				}
			}
		}

		// Get initial texture
		string tex_init;
		if (sel[initial].type == mapeditor::ItemType::Ceiling)
			tex_init = editor->map().sector(sel[initial].index)->ceiling().texture;
		else if (sel[initial].type == mapeditor::ItemType::Floor)
			tex_init = editor->map().sector(sel[initial].index)->floor().texture;
		else if (sel[initial].type == mapeditor::ItemType::WallTop)
			tex_init = editor->map().side(sel[initial].index)->texUpper();
		else if (sel[initial].type == mapeditor::ItemType::WallMiddle)
			tex_init = editor->map().side(sel[initial].index)->texMiddle();
		else if (sel[initial].type == mapeditor::ItemType::WallBottom)
			tex_init = editor->map().side(sel[initial].index)->texLower();

		auto map_format = editor->map().currentFormat();

		// Get all available texture names (sorted alphabetically)
		vector<string> tex_names;

		if (sel_walls_)
		{
			for (auto& tex_info : mapeditor::textureManager().allTexturesInfo())
			{
				bool skip = false;
				for (auto& texname : tex_names)
				{
					if (texname == tex_info.short_name)
					{
						skip = true;
						break;
					}
				}

				if (map_format == MapFormat::UDMF && game::configuration().featureSupported(game::Feature::LongNames)
					&& !strutil::equalCI(tex_info.short_name, tex_info.long_name))
					tex_names.push_back(tex_info.long_name);

				if (skip)
					continue;

				if (map_format == MapFormat::UDMF || tex_info.short_name.size() <= 8)
					tex_names.push_back(tex_info.short_name);
			}
		}
		if (sel_flats_)
		{
			for (auto& tex_info : mapeditor::textureManager().allFlatsInfo())
			{
				bool skip = false;
				for (auto& texname : tex_names)
				{
					if (texname == tex_info.short_name)
					{
						skip = true;
						break;
					}
				}

				if (map_format == MapFormat::UDMF && game::configuration().featureSupported(game::Feature::LongNames)
					&& !strutil::equalCI(tex_info.short_name, tex_info.long_name))
					tex_names.push_back(tex_info.long_name);

				if (skip)
					continue;

				if (map_format == MapFormat::UDMF || tex_info.short_name.size() <= 8)
					tex_names.push_back(tex_info.short_name);
			}
		}
		std::sort(tex_names.begin(), tex_names.end());

		// Init textures
		for (const auto& tex_name : tex_names)
			textures_.emplace_back(tex_name);

		// Set initial texture
		setTexture(tex_init);

		// Begin recording undo step
		editor->beginUndoRecord("Quick Texture", true, false, false);
	}
}

// -----------------------------------------------------------------------------
// Sets the currentl texture to [name], if it exists
// -----------------------------------------------------------------------------
void QuickTextureOverlay3d::setTexture(string_view name)
{
	for (unsigned a = 0; a < textures_.size(); a++)
	{
		if (strutil::equalCI(textures_[a].name, name))
		{
			current_index_ = a;
			anim_offset_   = a;
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Applies the current texture to all selected walls/flats
// -----------------------------------------------------------------------------
void QuickTextureOverlay3d::applyTexture() const
{
	// Check editor is associated
	if (!editor_)
		return;

	// Get selection/hilight
	auto& selection = editor_->selection();

	// Go through items
	if (!selection.empty())
	{
		for (auto& item : selection)
		{
			// Thing (skip)
			if (item.type == mapeditor::ItemType::Thing)
				continue;

			// Floor
			if (item.type == mapeditor::ItemType::Floor && sel_flats_)
			{
				if (auto sector = item.asSector(editor_->map()))
					sector->setFloorTexture(textures_[current_index_].name);
			}

			// Ceiling
			else if (item.type == mapeditor::ItemType::Ceiling && sel_flats_)
			{
				if (auto sector = item.asSector(editor_->map()))
					sector->setCeilingTexture(textures_[current_index_].name);
			}

			// Wall
			else if (sel_walls_)
			{
				if (auto side = item.asSide(editor_->map()))
				{
					// Upper
					if (item.type == mapeditor::ItemType::WallTop)
						side->setTexUpper(textures_[current_index_].name);
					// Middle
					else if (item.type == mapeditor::ItemType::WallMiddle)
						side->setTexMiddle(textures_[current_index_].name);
					// Lower
					else if (item.type == mapeditor::ItemType::WallBottom)
						side->setTexLower(textures_[current_index_].name);
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Updates the overlay (animations, etc.)
// -----------------------------------------------------------------------------
void QuickTextureOverlay3d::update(long frametime)
{
	double target = current_index_;
	float  mult   = (float)frametime / 10.0f;
	if (anim_offset_ < target - 0.01)
		anim_offset_ += (target - anim_offset_) * (0.2 * mult);
	else if (anim_offset_ > target + 0.01)
		anim_offset_ -= (anim_offset_ - target) * (0.2 * mult);
	else
		anim_offset_ = current_index_;
}

// -----------------------------------------------------------------------------
// Draws the overlay
// -----------------------------------------------------------------------------
void QuickTextureOverlay3d::draw(gl::draw2d::Context& dc, float fade)
{
	// Don't draw if invisible
	if (fade < 0.001f)
		return;

	// Draw background
	dc.colour.set(0, 0, 0, 140 * fade);
	dc.drawRect({ 0, dc.viewSize().y - 120, dc.viewSize().x, dc.viewSize().y });

	// Draw textures
	dc.colour.set(255, 255, 255, 255 * fade);
	auto x = (dc.viewSize().x * 0.5f) - (anim_offset_ * 136.0f);
	for (unsigned i = 0; i < textures_.size(); i++)
	{
		// Skip until first texture to show on left
		if (x < -96)
		{
			x += 136;
			continue;
		}

		// Draw texture
		drawTexture(dc, i, x, dc.viewSize().y - dc.textLineHeight() - 2, determineSize(x, dc.viewSize().x), fade);
		x += 136;

		// Stop when we get to the right edge of the screen
		if (x > dc.viewSize().x + 96)
			break;
	}
}

// -----------------------------------------------------------------------------
// Draws the texture in the list at [index]
// -----------------------------------------------------------------------------
void QuickTextureOverlay3d::drawTexture(
	gl::draw2d::Context& dc,
	unsigned             index,
	double               x,
	double               bottom,
	double               size,
	float                fade)
{
	// Get texture if needed
	if (!textures_[index].texture)
	{
		if (sel_walls_ && !sel_flats_)
			textures_[index].texture = mapeditor::textureManager().texture(textures_[index].name, false).gl_id;
		else if (!sel_walls_ && sel_flats_)
			textures_[index].texture = mapeditor::textureManager().flat(textures_[index].name, false).gl_id;
		else
			textures_[index].texture = mapeditor::textureManager().texture(textures_[index].name, true).gl_id;
	}

	// Draw name
	double brightness = 0.5 + (size - 1.0);
	dc.colour.set(brightness * 255, brightness * 255, brightness * 255, brightness * 255 * fade);
	dc.font           = index == current_index_ ? gl::draw2d::Font::Bold : gl::draw2d::Font::Normal;
	dc.text_alignment = gl::draw2d::Align::Center;
	dc.drawText(textures_[index].name, { x, bottom + 2 });

	// Draw texture
	dc.texture = textures_[index].texture;
	dc.drawTextureWithin({ x - 48 * size, bottom - (96 * size), x + 48 * size, bottom }, 0, 2);
}

// -----------------------------------------------------------------------------
// Calculates the size to draw a texture of [width] at position [x]
// (towards the middle of the screen is larger)
// -----------------------------------------------------------------------------
double QuickTextureOverlay3d::determineSize(double x, int width) const
{
	double mid = (double)width * 0.5;
	if (x < mid - 384 || x > mid + 384)
		return 1.0;

	double diff = (x < mid) ? mid - x : x - mid;
	double mult = ((384 - diff) / 384);
	return 1 + (0.5 * mult * mult);
}

// -----------------------------------------------------------------------------
// Called when the user closes the overlay. Applies changes if [cancel] is false
// -----------------------------------------------------------------------------
void QuickTextureOverlay3d::close(bool cancel)
{
	if (editor_)
	{
		editor_->endUndoRecord(true);
		editor_->selection().lockHilight(false);
		if (cancel)
			editor_->doUndo();
	}

	active_ = false;
}

// -----------------------------------------------------------------------------
// Finds and selects the first texture matching the current search
// -----------------------------------------------------------------------------
void QuickTextureOverlay3d::doSearch()
{
	if (!search_.empty())
	{
		for (unsigned a = 0; a < textures_.size(); a++)
		{
			if (strutil::startsWithCI(textures_[a].name, search_))
			{
				current_index_ = a;
				applyTexture();
				return;
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Called when a key is pressed
// -----------------------------------------------------------------------------
void QuickTextureOverlay3d::keyDown(string_view key)
{
	// Up texture
	if ((key == "right" || key == "mwheeldown") && current_index_ < textures_.size() - 1)
	{
		current_index_++;
		search_ = "";
		applyTexture();
	}

	// Down texture
	else if ((key == "left" || key == "mwheelup") && current_index_ > 0)
	{
		current_index_--;
		search_ = "";
		applyTexture();
	}

	// Character (search)
	else if (key.length() == 1)
	{
		search_ += key;
		doSearch();
	}
}

// -----------------------------------------------------------------------------
// Returns true if [sel] is valid for quick texture selection
// -----------------------------------------------------------------------------
bool QuickTextureOverlay3d::ok(const ItemSelection& sel)
{
	// Cancel if no selection
	if (sel.empty())
		return false;

	// Cancel if only things selected
	bool ok = false;
	for (auto item : sel)
	{
		if (item.type != mapeditor::ItemType::Thing)
		{
			ok = true;
			break;
		}
	}
	return ok;
}
