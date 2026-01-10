
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Geometry/Rect.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
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
	text_box_ = std::make_unique<gl::draw2d::TextBox>("", 100.0f, gl::draw2d::Font::Condensed);
}

// -----------------------------------------------------------------------------
// SectorInfoOverlay class destructor
// -----------------------------------------------------------------------------
SectorInfoOverlay::~SectorInfoOverlay() = default;

// -----------------------------------------------------------------------------
// Updates the overlay with info from [sector]
// -----------------------------------------------------------------------------
void SectorInfoOverlay::update(const MapSector* sector)
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
void SectorInfoOverlay::draw(gl::draw2d::Context& dc, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Determine overlay height
	auto right = dc.viewSize().x;
	if (last_size_ != right)
	{
		last_size_ = right;
		text_box_->setWidth(right - 68.0f);
	}
	text_box_->setFont(dc.font, dc.text_size);
	auto height = text_box_->height() + 8.0f;

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	auto  bottom    = dc.viewSize().y + height * alpha_inv * alpha_inv;

	// Draw overlay background
	dc.setColourFromConfig("map_overlay_background");
	dc.colour.a *= alpha;
	dc.drawRect({ 0.0f, bottom - height, right, bottom });

	// Draw info text lines
	dc.setColourFromConfig("map_overlay_foreground");
	dc.colour.a *= alpha;
	text_box_->draw({ 4.0f, bottom - height + 4.0f }, dc);

	// Ceiling texture
	float tex_box_size = 96.0f * dc.textScale();
	drawTexture(dc, alpha, right - tex_box_size - 8, bottom - 4, ctex_, "C");

	// Floor texture
	drawTexture(dc, alpha, right - (tex_box_size * 2) - 20, bottom - 4, ftex_, "F");
}

// -----------------------------------------------------------------------------
// Draws a texture box with name underneath for [texture]
// -----------------------------------------------------------------------------
void SectorInfoOverlay::drawTexture(
	gl::draw2d::Context& dc,
	float                alpha,
	float                x,
	float                y,
	string_view          texture,
	string_view          pos) const
{
	auto tex_box_size = 96.0f * dc.textScale();
	auto line_height  = dc.textLineHeight();

	// Get texture
	auto tex = mapeditor::textureManager()
				   .flat(texture, game::configuration().featureSupported(game::Feature::MixTexFlats))
				   .gl_id;

	// Valid texture
	if (texture != "-" && tex != gl::Texture::missingTexture())
	{
		// Draw background
		dc.texture = gl::Texture::backgroundTexture();
		dc.colour.set(255, 255, 255, 255 * alpha);
		dc.drawTextureTiled({ x, y - tex_box_size - line_height, tex_box_size, tex_box_size, false });

		// Draw texture
		dc.texture = tex;
		dc.drawTextureWithin({ x, y - tex_box_size - line_height, x + tex_box_size, y - line_height }, 0.0f);

		// Draw outline
		dc.setColourFromConfig("map_overlay_foreground");
		dc.colour.a *= alpha;
		dc.texture        = 0;
		dc.line_thickness = 1.0f;
		dc.drawRectOutline({ x, y - tex_box_size - line_height, x + tex_box_size, y - line_height });

		// Set text colour
		dc.setColourFromConfig("map_overlay_foreground");
		dc.colour.a *= alpha;
	}

	// Unknown texture
	else if (tex == gl::Texture::missingTexture())
	{
		// Draw unknown icon
		dc.texture = mapeditor::textureManager().editorImage("thing/unknown").gl_id;
		dc.colour.set(180, 0, 0, 255 * alpha);
		dc.drawTextureWithin({ x, y - tex_box_size - line_height, x + tex_box_size, y - line_height }, 0.0f, 0.2f);

		// Set colour to red (for text)
		dc.setColourFromConfig("map_overlay_foreground");
		dc.colour.a *= alpha;
		dc.colour = dc.colour.ampf(1.0f, 0.0f, 0.0f, 1.0f);
	}

	// Draw texture name
	auto tex_name     = fmt::format("{}: {}", pos, texture.size() > 8 ? texture.substr(0, 8) : texture);
	dc.text_alignment = gl::draw2d::Align::Center;
	dc.drawText(tex_name, { x + tex_box_size * 0.5f, y - line_height });
}
