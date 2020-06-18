
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SectorInfoOverlay.cpp
// Description: SectorInfoOverlay class - a map editor overlay that displays
//              information about the currently highlighted sector in sectors
//              mode
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
#include "SectorInfoOverlay.h"
#include "Game/Configuration.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/MapObject/MapSector.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// SectorInfoOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SectorInfoOverlay class constructor
// -----------------------------------------------------------------------------
SectorInfoOverlay::SectorInfoOverlay()
{
	text_box_ = std::make_unique<TextBox>("", drawing::Font::Condensed, 100, 16 * (drawing::fontSize() / 12.0));
}

// -----------------------------------------------------------------------------
// Updates the overlay with info from [sector]
// -----------------------------------------------------------------------------
void SectorInfoOverlay::update(MapSector* sector)
{
	if (!sector)
		return;

	string info_text;

	// Info (index + type)
	auto type = fmt::format("{} (Type {})", game::configuration().sectorTypeName(sector->special()), sector->special());
	if (global::debug)
		info_text += fmt::format("Sector #{} ({}): {}\n", sector->index(), sector->objId(), type);
	else
		info_text += fmt::format("Sector #{}: {}\n", sector->index(), type);

	// Height
	int fh = sector->floor().height;
	int ch = sector->ceiling().height;
	info_text += fmt::format("Height: {} to {} ({} total)\n", fh, ch, ch - fh);

	// Brightness
	info_text += fmt::format("Brightness: {}\n", sector->lightLevel());

	// Tag
	info_text += fmt::format("Tag: {}", sector->tag());

	// Textures
	ftex_ = sector->floor().texture;
	ctex_ = sector->ceiling().texture;

	// Setup text box
	text_box_->setText(info_text);
}

// -----------------------------------------------------------------------------
// Draws the overlay at [bottom] from 0 to [right]
// -----------------------------------------------------------------------------
void SectorInfoOverlay::draw(int bottom, int right, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Determine overlay height
	double scale = (drawing::fontSize() / 12.0);
	text_box_->setLineHeight(16 * scale);
	if (last_size_ != right)
	{
		last_size_ = right;
		text_box_->setSize(right - 88 - 92);
	}
	int height = text_box_->height() + 4;

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += height * alpha_inv * alpha_inv;

	// Get colours
	auto col_bg = colourconfig::colour("map_overlay_background");
	auto col_fg = colourconfig::colour("map_overlay_foreground");
	col_fg.a    = col_fg.a * alpha;
	col_bg.a    = col_bg.a * alpha;
	ColRGBA col_border(0, 0, 0, 140);

	// Draw overlay background
	int tex_box_size = 80 * scale;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	drawing::drawBorderedRect(0, bottom - height - 4, right - (tex_box_size * 2) - 30, bottom + 2, col_bg, col_border);
	drawing::drawBorderedRect(
		right - (tex_box_size * 2) - 28, bottom - height - 4, right, bottom + 2, col_bg, col_border);

	// Draw info text lines
	text_box_->draw(2, bottom - height, col_fg);

	// Ceiling texture
	drawTexture(alpha, right - tex_box_size - 8, bottom - 4, ctex_, "C");

	// Floor texture
	drawTexture(alpha, right - (tex_box_size * 2) - 20, bottom - 4, ftex_, "F");

	// Done
	glEnable(GL_LINE_SMOOTH);
}

// -----------------------------------------------------------------------------
// Draws a texture box with name underneath for [texture]
// -----------------------------------------------------------------------------
void SectorInfoOverlay::drawTexture(float alpha, int x, int y, string_view texture, string_view pos) const
{
	double scale        = (drawing::fontSize() / 12.0);
	int    tex_box_size = 80 * scale;
	int    line_height  = 16 * scale;

	// Get colours
	auto col_bg = colourconfig::colour("map_overlay_background");
	auto col_fg = colourconfig::colour("map_overlay_foreground");
	col_fg.a    = col_fg.a * alpha;

	// Get texture
	auto tex = mapeditor::textureManager()
				   .flat(texture, game::configuration().featureSupported(game::Feature::MixTexFlats))
				   .gl_id;

	// Valid texture
	if (texture != "-" && tex != gl::Texture::missingTexture())
	{
		// Draw background
		glEnable(GL_TEXTURE_2D);
		gl::setColour(255, 255, 255, 255 * alpha, gl::Blend::Normal);
		glPushMatrix();
		glTranslated(x, y - tex_box_size - line_height, 0);
		drawing::drawTextureTiled(gl::Texture::backgroundTexture(), tex_box_size, tex_box_size);
		glPopMatrix();

		// Draw texture
		gl::setColour(255, 255, 255, 255 * alpha, gl::Blend::Normal);
		drawing::drawTextureWithin(tex, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0);

		glDisable(GL_TEXTURE_2D);

		// Draw outline
		gl::setColour(col_fg.r, col_fg.g, col_fg.b, 255 * alpha, gl::Blend::Normal);
		glDisable(GL_LINE_SMOOTH);
		drawing::drawRect(x, y - tex_box_size - line_height, x + tex_box_size, y - line_height);
	}

	// Unknown texture
	else if (tex == gl::Texture::missingTexture())
	{
		// Draw unknown icon
		auto icon = mapeditor::textureManager().editorImage("thing/unknown").gl_id;
		glEnable(GL_TEXTURE_2D);
		gl::setColour(180, 0, 0, 255 * alpha, gl::Blend::Normal);
		drawing::drawTextureWithin(icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.15);

		// Set colour to red (for text)
		col_fg = col_fg.ampf(1.0f, 0.0f, 0.0f, 1.0f);
	}

	// Draw texture name
	auto tex_name = fmt::format("{}:{}", pos, texture.size() > 8 ? texture.substr(0, 8) : texture);
	drawing::drawText(
		tex_name, x + (tex_box_size * 0.5), y - line_height, col_fg, drawing::Font::Condensed, drawing::Align::Center);
}
