
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SectorTextureOverlay.cpp
// Description: SectorTextureOverlay class - A full screen map editor overlay
//              that shows (a) sector's textures and allows the user to click a
//              texture to browse for it
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
#include "SectorTextureOverlay.h"
#include "Game/Configuration.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "OpenGL/Drawing.h"
#include "SLADEMap/MapObject/MapSector.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// SectorTextureOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Updates the overlay (animations, etc.)
// -----------------------------------------------------------------------------
void SectorTextureOverlay::update(long frametime)
{
	// Get frame time multiplier
	float mult = (float)frametime / 10.0f;

	// Update animations
	anim_floor_ += 0.1f * mult;
	if (anim_floor_ > tex_floor_.size())
		anim_floor_ = 0.0f;
	anim_ceil_ += 0.1f * mult;
	if (anim_ceil_ > tex_ceil_.size())
		anim_ceil_ = 0.0f;
}

// -----------------------------------------------------------------------------
// Draws the overlay to [width],[height]
// -----------------------------------------------------------------------------
void SectorTextureOverlay::draw(int width, int height, float fade)
{
	// Get colours
	auto& col_fg      = colourconfig::colDef("map_overlay_foreground");
	auto  col_fg_rgba = col_fg.colour;
	col_fg_rgba.a *= fade;

	// Draw background
	glDisable(GL_TEXTURE_2D);
	colourconfig::setGLColour("map_overlay_background", fade);
	drawing::drawFilledRect(0, 0, width, height);

	// Check if any sectors are open
	if (sectors_.empty())
	{
		drawing::drawText(
			"No sectors are open. Just press escape and pretend this never happened.",
			width * 0.5,
			height * 0.5,
			ColRGBA::WHITE,
			drawing::Font::Normal,
			drawing::Align::Center);
		return;
	}

	// Calculate layout related stuff
	middlex_  = width * 0.5;
	middley_  = height * 0.5;
	tex_size_ = middlex_ - 64;
	if (tex_size_ > 256)
		tex_size_ = 256;
	border_ = (middlex_ - tex_size_) * 0.5;
	if (border_ > 48)
		border_ = 48;
	int cur_size = tex_size_;
	if (active_)
		cur_size *= fade;

	// Determine texture name strings
	auto   ftex = tex_floor_[0];
	auto   ctex = tex_ceil_[0];
	string ftex2, ctex2;
	if (tex_floor_.size() > 1)
	{
		ftex  = fmt::format("Multiple ({})", tex_floor_.size());
		ftex2 = tex_floor_[1];
	}
	if (tex_ceil_.size() > 1)
	{
		ctex  = fmt::format("Multiple ({})", tex_ceil_.size());
		ctex2 = tex_ceil_[1];
	}

	// Floor texture
	glEnable(GL_LINE_SMOOTH);
	drawTexture(
		fade,
		middlex_ - border_ - tex_size_ * 0.5 - cur_size * 0.5,
		middley_ - cur_size * 0.5,
		cur_size,
		tex_floor_,
		hover_floor_);
	drawing::drawText(
		"Floor:",
		middlex_ - border_ - tex_size_ * 0.5,
		middley_ - tex_size_ * 0.5 - 18,
		col_fg_rgba,
		drawing::Font::Bold,
		drawing::Align::Center);
	drawing::drawText(
		ftex,
		middlex_ - border_ - tex_size_ * 0.5,
		middley_ + tex_size_ * 0.5 + 2,
		col_fg_rgba,
		drawing::Font::Bold,
		drawing::Align::Center);

	// Ceiling texture
	drawTexture(
		fade,
		middlex_ + border_ + tex_size_ * 0.5 - cur_size * 0.5,
		middley_ - cur_size * 0.5,
		cur_size,
		tex_ceil_,
		hover_ceil_);
	drawing::drawText(
		"Ceiling:",
		middlex_ + border_ + tex_size_ * 0.5,
		middley_ - tex_size_ * 0.5 - 18,
		col_fg_rgba,
		drawing::Font::Bold,
		drawing::Align::Center);
	drawing::drawText(
		ctex,
		middlex_ + border_ + tex_size_ * 0.5,
		middley_ + tex_size_ * 0.5 + 2,
		col_fg_rgba,
		drawing::Font::Bold,
		drawing::Align::Center);
}

// -----------------------------------------------------------------------------
// Draws the texture box for [textures]
// -----------------------------------------------------------------------------
void SectorTextureOverlay::drawTexture(float alpha, int x, int y, int size, vector<string>& textures, bool hover) const
{
	// Get colours
	auto col_bg  = colourconfig::colour("map_overlay_background");
	auto col_fg  = colourconfig::colour("map_overlay_foreground");
	auto col_sel = colourconfig::colour("map_hilight");
	col_fg.a     = col_fg.a * alpha;

	// Draw background
	glEnable(GL_TEXTURE_2D);
	gl::setColour(255, 255, 255, 255 * alpha, gl::Blend::Normal);
	glPushMatrix();
	glTranslated(x, y, 0);
	drawing::drawTextureTiled(gl::Texture::backgroundTexture(), size, size);
	glPopMatrix();

	// Draw first texture
	bool mixed = game::configuration().featureSupported(game::Feature::MixTexFlats);
	gl::setColour(255, 255, 255, 255 * alpha, gl::Blend::Normal);
	drawing::drawTextureWithin(
		mapeditor::textureManager().flat(textures[0], mixed).gl_id, x, y, x + size, y + size, 0, 100);

	// Draw up to 4 subsequent textures (overlaid)
	gl::setColour(255, 255, 255, 127 * alpha, gl::Blend::Normal);
	for (unsigned a = 1; a < textures.size() && a < 5; a++)
		drawing::drawTextureWithin(
			mapeditor::textureManager().flat(textures[a], mixed).gl_id, x, y, x + size, y + size, 0, 100);

	glDisable(GL_TEXTURE_2D);

	// Draw outline
	if (hover)
	{
		gl::setColour(col_sel.r, col_sel.g, col_sel.b, 255 * alpha, gl::Blend::Normal);
		glLineWidth(3.0f);
	}
	else
	{
		gl::setColour(col_fg.r, col_fg.g, col_fg.b, 255 * alpha, gl::Blend::Normal);
		glLineWidth(1.5f);
	}
	drawing::drawRect(x, y, x + size, y + size);
}

// -----------------------------------------------------------------------------
// 'Opens' all sectors in [list], adds both textures from each
// -----------------------------------------------------------------------------
void SectorTextureOverlay::openSectors(vector<MapSector*>& list)
{
	// Clear current sectors list (if any)
	sectors_.clear();
	tex_ceil_.clear();
	tex_floor_.clear();

	// Add list to sectors
	for (auto& sector : list)
	{
		// Add sector
		sectors_.push_back(sector);

		// Get textures
		auto ftex = sector->floor().texture;
		auto ctex = sector->ceiling().texture;

		// Add floor texture if different
		bool exists = false;
		for (const auto& tex : tex_floor_)
		{
			if (tex == ftex)
			{
				exists = true;
				break;
			}
		}
		if (!exists)
			tex_floor_.push_back(ftex);

		// Add ceiling texture if different
		exists = false;
		for (const auto& tex : tex_ceil_)
		{
			if (tex == ctex)
			{
				exists = true;
				break;
			}
		}
		if (!exists)
			tex_ceil_.push_back(ctex);
	}
}

// -----------------------------------------------------------------------------
// Called when the user closes the overlay. Applies changes if [cancel] is false
// -----------------------------------------------------------------------------
void SectorTextureOverlay::close(bool cancel)
{
	// Deactivate
	active_ = false;

	// Set textures if not cancelled
	if (!cancel)
	{
		mapeditor::editContext().beginUndoRecord("Change Sector Texture", true, false, false);
		for (auto& sector : sectors_)
		{
			if (tex_floor_.size() == 1)
				sector->setFloorTexture(tex_floor_[0]);
			if (tex_ceil_.size() == 1)
				sector->setCeilingTexture(tex_ceil_[0]);
		}
		mapeditor::editContext().endUndoRecord();
	}
}

// -----------------------------------------------------------------------------
// Called when the mouse cursor is moved
// -----------------------------------------------------------------------------
void SectorTextureOverlay::mouseMotion(int x, int y)
{
	// Check if the mouse is over the floor texture
	if (x >= middlex_ - border_ - tex_size_ && x <= middlex_ - border_ && y >= middley_ - tex_size_ * 0.5
		&& y <= middley_ + tex_size_ * 0.5)
		hover_floor_ = true;
	else
		hover_floor_ = false;

	// Check if the mouse is over the ceiling texture
	if (x >= middlex_ + border_ && x <= middlex_ + border_ + tex_size_ && y >= middley_ - tex_size_ * 0.5
		&& y <= middley_ + tex_size_ * 0.5)
		hover_ceil_ = true;
	else
		hover_ceil_ = false;
}

// -----------------------------------------------------------------------------
// Called when the left mouse button is clicked
// -----------------------------------------------------------------------------
void SectorTextureOverlay::mouseLeftClick()
{
	// Do nothing if no sectors open
	if (sectors_.empty())
		return;

	// Left clicked on floor texture
	if (hover_floor_)
		browseFloorTexture();

	// Left clicked on ceiling texture
	else if (hover_ceil_)
		browseCeilingTexture();
}

// -----------------------------------------------------------------------------
// Called when a key is pressed
// -----------------------------------------------------------------------------
void SectorTextureOverlay::keyDown(string_view key)
{
	// Browse floor texture
	if (key == "F" || key == "f")
		browseFloorTexture();

	// Browse ceiling texture
	if (key == "C" || key == "c")
		browseCeilingTexture();
}

// -----------------------------------------------------------------------------
// Opens the texture browser for the floor texture
// -----------------------------------------------------------------------------
void SectorTextureOverlay::browseFloorTexture()
{
	// Get initial texture
	string texture;
	if (tex_floor_.empty())
		texture = sectors_[0]->floor().texture;
	else
		texture = tex_floor_[0];

	// Open texture browser
	MapTextureBrowser browser(
		mapeditor::windowWx(), mapeditor::TextureType::Flat, texture, &(mapeditor::editContext().map()));
	browser.SetTitle(wxS("Browse Floor Texture"));
	if (browser.ShowModal() == wxID_OK)
	{
		// Set texture
		tex_floor_.clear();
		tex_floor_.push_back(browser.selectedItem()->name());
		close(false);
	}
}

// -----------------------------------------------------------------------------
// Opens the texture browser for the ceiling texture
// -----------------------------------------------------------------------------
void SectorTextureOverlay::browseCeilingTexture()
{
	// Get initial texture
	string texture;
	if (tex_ceil_.empty())
		texture = sectors_[0]->ceiling().texture;
	else
		texture = tex_ceil_[0];

	// Open texture browser
	MapTextureBrowser browser(
		mapeditor::windowWx(), mapeditor::TextureType::Flat, texture, &(mapeditor::editContext().map()));
	browser.SetTitle(wxS("Browse Ceiling Texture"));
	if (browser.ShowModal() == wxID_OK)
	{
		// Set texture
		tex_ceil_.clear();
		tex_ceil_.push_back(browser.selectedItem()->name());
		close(false);
	}
}
