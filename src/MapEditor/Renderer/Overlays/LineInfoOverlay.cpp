
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
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "Utility/MathStuff.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// LineInfoOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// LineInfoOverlay class constructor
// -----------------------------------------------------------------------------
LineInfoOverlay::LineInfoOverlay() :
	scale_{ drawing::fontSize() / 12.0 },
	text_box_{ "", drawing::Font::Condensed, 100, int(16 * scale_) }
{
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
	text_box_.setText(info_text);

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
void LineInfoOverlay::draw(int bottom, int right, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Determine overlay height
	int sides_width = 2;
	if (side_front_.exists)
		sides_width += 256;
	if (side_back_.exists)
		sides_width += 256;
	if (last_size_ != right - sides_width)
	{
		last_size_ = right - sides_width;
		text_box_.setSize(right - sides_width);
	}
	int height = text_box_.height() + 4;

	// Get colours
	ColRGBA col_bg = colourconfig::colour("map_overlay_background");
	ColRGBA col_fg = colourconfig::colour("map_overlay_foreground");
	col_fg.a       = col_fg.a * alpha;
	col_bg.a       = col_bg.a * alpha;
	ColRGBA col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += height * alpha_inv * alpha_inv;

	// Determine widths
	int n_side_panels = 0;
	if (side_front_.exists)
		n_side_panels++;
	if (side_back_.exists)
		n_side_panels++;

	// Draw overlay background
	scale_             = drawing::fontSize() / 12.0;
	int tex_box_size   = 80 * scale_;
	int sinf_size      = ((tex_box_size * 3) + 16);
	int main_panel_end = right - (n_side_panels * (sinf_size + 2));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(1.0f);
	drawing::drawBorderedRect(0, bottom - height - 4, main_panel_end, bottom + 2, col_bg, col_border);

	// Draw info text lines
	text_box_.setLineHeight(16 * scale_);
	text_box_.draw(2, bottom - height, col_fg);

	// Side info
	int x = right - sinf_size;
	if (side_front_.exists)
	{
		// Background
		glDisable(GL_TEXTURE_2D);
		drawing::drawBorderedRect(x, bottom - height - 4, x + sinf_size, bottom + 2, col_bg, col_border);

		drawSide(bottom - 4, right, alpha, side_front_, x);
		x -= (sinf_size + 2);
	}
	if (side_back_.exists)
	{
		// Background
		glDisable(GL_TEXTURE_2D);
		drawing::drawBorderedRect(x, bottom - height - 4, x + sinf_size, bottom + 2, col_bg, col_border);

		drawSide(bottom - 4, right, alpha, side_back_, x);
	}

	// Done
	glEnable(GL_LINE_SMOOTH);
}

// -----------------------------------------------------------------------------
// Draws side/texture info for [side]
// -----------------------------------------------------------------------------
void LineInfoOverlay::drawSide(int bottom, int right, float alpha, Side& side, int xstart)
{
	// Get colours
	ColRGBA col_fg = colourconfig::colour("map_overlay_foreground");
	col_fg.a       = col_fg.a * alpha;

	// Index and sector index
	drawing::drawText(side.info, xstart + 4, bottom - (32 * scale_), col_fg, drawing::Font::Condensed);

	// Texture offsets
	drawing::drawText(side.offsets, xstart + 4, bottom - (16 * scale_), col_fg, drawing::Font::Condensed);

	// Textures
	int tex_box_size = 80 * scale_;
	drawTexture(alpha, xstart + 4, bottom - (32 * scale_), side.tex_upper, side.needs_upper);
	drawTexture(alpha, xstart + tex_box_size + 8, bottom - (32 * scale_), side.tex_middle, side.needs_middle, "M");
	drawTexture(
		alpha,
		xstart + tex_box_size + 12 + tex_box_size,
		bottom - (32 * scale_),
		side.tex_lower,
		side.needs_lower,
		"L");
}

// -----------------------------------------------------------------------------
// Draws a texture box with name underneath for [texture]
// -----------------------------------------------------------------------------
void LineInfoOverlay::drawTexture(float alpha, int x, int y, string_view texture, bool needed, string_view pos) const
{
	bool required     = (needed && texture == MapSide::TEX_NONE);
	int  tex_box_size = 80 * scale_;
	int  line_height  = 16 * scale_;

	// Get colours
	ColRGBA col_fg = colourconfig::colour("map_overlay_foreground");
	col_fg.a       = col_fg.a * alpha;

	// Get texture
	auto tex = mapeditor::textureManager()
				   .texture(texture, game::configuration().featureSupported(game::Feature::MixTexFlats))
				   .gl_id;

	// Valid texture
	if (texture != MapSide::TEX_NONE && tex != gl::Texture::missingTexture())
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
	else if (tex == gl::Texture::missingTexture() && texture != MapSide::TEX_NONE)
	{
		// Draw unknown icon
		auto icon = mapeditor::textureManager().editorImage("thing/unknown").gl_id;
		glEnable(GL_TEXTURE_2D);
		gl::setColour(180, 0, 0, 255 * alpha, gl::Blend::Normal);
		drawing::drawTextureWithin(icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.15);

		// Set colour to red (for text)
		col_fg = col_fg.ampf(1.0f, 0.0f, 0.0f, 1.0f);
	}

	// Missing texture
	else if (required)
	{
		// Draw missing icon
		auto icon = mapeditor::textureManager().editorImage("thing/minus").gl_id;
		glEnable(GL_TEXTURE_2D);
		gl::setColour(180, 0, 0, 255 * alpha, gl::Blend::Normal);
		drawing::drawTextureWithin(icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.15);

		// Set colour to red (for text)
		col_fg = col_fg.ampf(1.0f, 0.0f, 0.0f, 1.0f);
	}

	// Draw texture name (even if texture is blank)
	string tex_str;
	if (required)
		tex_str = fmt::format("{}:MISSING", pos);
	else if (texture.size() > 8)
		tex_str = fmt::format("{}:{}...", pos, texture.substr(0, 8));
	else
		tex_str = fmt::format("{}:{}", pos, texture);

	drawing::drawText(
		tex_str, x + (tex_box_size * 0.5), y - line_height, col_fg, drawing::Font::Condensed, drawing::Align::Center);
}
