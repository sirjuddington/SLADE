
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LineInfoOverlay.cpp
// Description: LineInfoOverlay class - a map editor overlay that displays
//              information about the currently highlighted line and its sides
//              in lines mode
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
#include "LineInfoOverlay.h"
#include "Game/Configuration.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "Utility/MathStuff.h"

using namespace slade;
using namespace gl;


constexpr float TEXBOX_SIZE = 96.0f;

// -----------------------------------------------------------------------------
//
// LineInfoOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// LineInfoOverlay class constructor
// -----------------------------------------------------------------------------
LineInfoOverlay::LineInfoOverlay()
{
	text_box_ = std::make_unique<draw2d::TextBox>("", 100.0f, draw2d::Font::Condensed);
}

// -----------------------------------------------------------------------------
// Updates the overlay with info from [line]
// -----------------------------------------------------------------------------
void LineInfoOverlay::update(MapLine* line)
{
	if (!line)
		return;

	// info.clear();
	string info_text;
	auto   map_format = mapeditor::editContext().mapDesc().format;

	// General line info
	if (global::debug)
		info_text += (fmt::format("Line #{} ({})\n", line->index(), line->objId()));
	else
		info_text += (fmt::format("Line #{}\n", line->index()));
	info_text += (fmt::format("Length: {}\n", math::round(line->length())));

	// Line special
	int as_id = line->special();
	if (line->hasProp("macro"))
	{
		int macro = line->intProperty("macro");
		info_text += (fmt::format("Macro: #{}\n", macro));
	}
	else
		info_text += (fmt::format("Special: {} ({})\n", as_id, game::configuration().actionSpecialName(as_id)));

	// Line trigger
	if (map_format == MapFormat::Hexen || map_format == MapFormat::UDMF)
		info_text += (fmt::format("Trigger: {}\n", game::configuration().spacTriggerString(line, map_format)));

	// Line args (or sector tag)
	if (map_format == MapFormat::Hexen || map_format == MapFormat::UDMF)
	{
		string argxstr[2];
		argxstr[0]    = line->stringProperty("arg0str");
		argxstr[1]    = line->stringProperty("arg1str");
		string argstr = game::configuration().actionSpecial(as_id).argSpec().stringDesc(line->args().data(), argxstr);
		if (!argstr.empty())
			info_text += (fmt::format("{}", argstr));
		else
			info_text += ("No Args");
	}
	else
		info_text += (fmt::format("Sector Tag: {}", line->arg(0)));

	// Line flags
	if (map_format != MapFormat::UDMF)
		info_text += (fmt::format("\nFlags: {}", game::configuration().lineFlagsString(line)));

	// Setup text box
	text_box_->setText(info_text);

	// Check needed textures
	int needed_tex = line->needsTexture();

	// Front side
	auto s = line->s1();
	if (s)
	{
		side_front_.exists = true;
		if (global::debug)
			side_front_.info = fmt::format(
				"Front Side #{} ({}) (Sector {})", s->index(), s->objId(), s->sector()->index());
		else
			side_front_.info = fmt::format("Front Side #{} (Sector {})", s->index(), s->sector()->index());
		side_front_.offsets      = fmt::format("Offsets: ({}, {})", s->texOffsetX(), s->texOffsetY());
		side_front_.tex_upper    = s->texUpper();
		side_front_.tex_middle   = s->texMiddle();
		side_front_.tex_lower    = s->texLower();
		side_front_.needs_lower  = ((needed_tex & MapLine::Part::FrontLower) > 0);
		side_front_.needs_middle = ((needed_tex & MapLine::Part::FrontMiddle) > 0);
		side_front_.needs_upper  = ((needed_tex & MapLine::Part::FrontUpper) > 0);
	}
	else
		side_front_.exists = false;

	// Back side
	s = line->s2();
	if (s)
	{
		side_back_.exists = true;
		if (global::debug)
			side_back_.info = fmt::format(
				"Back Side #{} ({}) (Sector {})", s->index(), s->objId(), s->sector()->index());
		else
			side_back_.info = fmt::format("Back Side #{} (Sector {})", s->index(), s->sector()->index());
		side_back_.offsets      = fmt::format("Offsets: ({}, {})", s->texOffsetX(), s->texOffsetY());
		side_back_.tex_upper    = s->texUpper();
		side_back_.tex_middle   = s->texMiddle();
		side_back_.tex_lower    = s->texLower();
		side_back_.needs_lower  = ((needed_tex & MapLine::Part::BackLower) > 0);
		side_back_.needs_middle = ((needed_tex & MapLine::Part::BackMiddle) > 0);
		side_back_.needs_upper  = ((needed_tex & MapLine::Part::BackUpper) > 0);
	}
	else
		side_back_.exists = false;
}

// -----------------------------------------------------------------------------
// Draws the overlay at [bottom] from 0 to [right]
// -----------------------------------------------------------------------------
void LineInfoOverlay::draw(draw2d::Context& dc, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Calculate sizes
	scale_            = dc.textScale();
	auto tex_box_size = TEXBOX_SIZE * scale_;
	auto sinf_size    = tex_box_size * 3.0f + 16.0f;

	// Determine overlay height
	auto sides_width = 2.0f;
	auto right       = dc.viewSize().x;
	if (side_front_.exists)
		sides_width += sinf_size;
	if (side_back_.exists)
		sides_width += sinf_size;
	if (last_size_ != right - sides_width)
	{
		last_size_ = right - sides_width;
		text_box_->setWidth(last_size_);
	}
	text_box_->setFont(dc.font, dc.text_size);
	auto height = text_box_->height() + 4.0f;

	// Get colours
	ColRGBA col_bg = colourconfig::colour("map_overlay_background");
	ColRGBA col_fg = colourconfig::colour("map_overlay_foreground");
	col_fg.a       = col_fg.a * alpha;
	col_bg.a       = col_bg.a * alpha;

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	auto  bottom    = dc.viewSize().y + height * alpha_inv * alpha_inv;

	// Determine widths
	int n_side_panels = 0;
	if (side_front_.exists)
		n_side_panels++;
	if (side_back_.exists)
		n_side_panels++;

	// Draw overlay background
	auto main_panel_end = right - (n_side_panels * (sinf_size + 2.0f));
	dc.colour           = col_bg;
	dc.drawRect({ 0.0f, bottom - height - 4.0f, main_panel_end, bottom });

	// Draw info text lines
	dc.colour = col_fg;
	text_box_->draw({ 4.0f, bottom - height }, dc);

	// Side info
	auto x = right - sinf_size;
	if (side_front_.exists)
	{
		// Background
		dc.colour = col_bg;
		dc.drawRect({ x, bottom - height - 4.0f, x + sinf_size, bottom });

		drawSide(dc, bottom - 4, alpha, side_front_, x);
		x -= (sinf_size + 2.0f);
	}
	if (side_back_.exists)
	{
		// Background
		dc.colour = col_bg;
		dc.drawRect({ x, bottom - height - 4.0f, x + sinf_size, bottom });

		drawSide(dc, bottom - 4, alpha, side_back_, x);
	}
}

// -----------------------------------------------------------------------------
// Draws side/texture info for [side]
// -----------------------------------------------------------------------------
void LineInfoOverlay::drawSide(draw2d::Context& dc, float bottom, float alpha, const Side& side, float xstart) const
{
	// Get colours
	dc.colour = colourconfig::colour("map_overlay_foreground");
	dc.colour.a *= alpha;

	// Texture offsets
	auto y            = bottom - dc.textLineHeight() - 2.0f;
	dc.text_alignment = draw2d::Align::Left;
	dc.drawText(side.offsets, { xstart + 4, y });

	// Index and sector index
	y -= dc.textLineHeight();
	dc.drawText(side.info, { xstart + 4.0f, y });

	// Textures
	y -= 2.0f;
	auto tex_box_size = TEXBOX_SIZE * scale_;
	drawTexture(dc, alpha, xstart + 4, y, side.tex_upper, side.needs_upper);
	drawTexture(dc, alpha, xstart + tex_box_size + 8, y, side.tex_middle, side.needs_middle, "M");
	drawTexture(dc, alpha, xstart + tex_box_size + 12 + tex_box_size, y, side.tex_lower, side.needs_lower, "L");
}

// -----------------------------------------------------------------------------
// Draws a texture box with name underneath for [texture]
// -----------------------------------------------------------------------------
void LineInfoOverlay::drawTexture(
	draw2d::Context& dc,
	float            alpha,
	float            x,
	float            y,
	string_view      texture,
	bool             needed,
	string_view      pos) const
{
	bool required     = (needed && texture == MapSide::TEX_NONE);
	auto tex_box_size = TEXBOX_SIZE * scale_;
	auto line_height  = dc.textLineHeight();

	// Get texture
	auto tex = mapeditor::textureManager()
				   .texture(texture, game::configuration().featureSupported(game::Feature::MixTexFlats))
				   .gl_id;

	dc.setColourFromConfig("map_overlay_foreground");
	dc.colour.a *= alpha;

	// Valid texture
	if (texture != MapSide::TEX_NONE && tex != gl::Texture::missingTexture())
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
	}

	// Unknown texture
	else if (tex == gl::Texture::missingTexture() && texture != MapSide::TEX_NONE)
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

	// Missing texture
	else if (required)
	{
		// Draw unknown icon
		dc.texture = mapeditor::textureManager().editorImage("thing/minus").gl_id;
		dc.colour.set(180, 0, 0, 255 * alpha);
		dc.drawTextureWithin({ x, y - tex_box_size - line_height, x + tex_box_size, y - line_height }, 0.0f, 0.2f);

		// Set colour to red (for text)
		dc.setColourFromConfig("map_overlay_foreground");
		dc.colour.a *= alpha;
		dc.colour = dc.colour.ampf(1.0f, 0.0f, 0.0f, 1.0f);
	}

	// Draw texture name (even if texture is blank)
	string tex_str;
	if (required)
		tex_str = fmt::format("{}:MISSING", pos);
	else if (texture.size() > 8)
		tex_str = fmt::format("{}:{}...", pos, texture.substr(0, 8));
	else
		tex_str = fmt::format("{}:{}", pos, texture);

	dc.text_alignment = gl::draw2d::Align::Center;
	dc.drawText(tex_str, { x + tex_box_size * 0.5f, y - line_height });
}
