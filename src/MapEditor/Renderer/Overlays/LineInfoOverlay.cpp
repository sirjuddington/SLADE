
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    LineInfoOverlay.cpp
 * Description: LineInfoOverlay class - a map editor overlay that
 *              displays information about the currently highlighted
 *              line and its sides in lines mode
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "Game/Configuration.h"
#include "General/ColourConfiguration.h"
#include "LineInfoOverlay.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapSide.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/OpenGL.h"
#include "Utility/MathStuff.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, gl_font_size)


/*******************************************************************
 * LINEINFOOVERLAY CLASS FUNCTIONS
 *******************************************************************/

/* LineInfoOverlay::LineInfoOverlay
 * LineInfoOverlay class constructor
 *******************************************************************/
LineInfoOverlay::LineInfoOverlay()
{
	scale = gl_font_size / 12.0;
	text_box = new TextBox("", Drawing::FONT_CONDENSED, 100, 16 * scale);
	last_size = 100;
}

/* LineInfoOverlay::~LineInfoOverlay
 * LineInfoOverlay class destructor
 *******************************************************************/
LineInfoOverlay::~LineInfoOverlay()
{
	delete text_box;
}

/* LineInfoOverlay::update
 * Updates the overlay with info from [line]
 *******************************************************************/
void LineInfoOverlay::update(MapLine* line)
{
	if (!line)
		return;

	//info.clear();
	string info_text;
	int map_format = MapEditor::editContext().mapDesc().format;

	// General line info
	if (Global::debug)
		info_text += (S_FMT("Line #%d (%d)\n", line->getIndex(), line->getId()));
	else
		info_text += (S_FMT("Line #%d\n", line->getIndex()));
	info_text += (S_FMT("Length: %d\n", MathStuff::round(line->getLength())));

	// Line special
	int as_id = line->intProperty("special");
	if (line->props().propertyExists("macro"))
	{
		int macro = line->intProperty("macro");
		info_text += (S_FMT("Macro: #%d\n", macro));
	}
	else
		info_text += (S_FMT("Special: %d (%s)\n", as_id, Game::configuration().actionSpecialName(as_id)));

	// Line trigger
	if (map_format == MAP_HEXEN || map_format == MAP_UDMF)
		info_text += (S_FMT("Trigger: %s\n", Game::configuration().spacTriggerString(line, map_format)));

	// Line args (or sector tag)
	if (map_format == MAP_HEXEN || map_format == MAP_UDMF)
	{
		int args[5];
		args[0] = line->intProperty("arg0");
		args[1] = line->intProperty("arg1");
		args[2] = line->intProperty("arg2");
		args[3] = line->intProperty("arg3");
		args[4] = line->intProperty("arg4");
		string argxstr[2];
		argxstr[0] = line->stringProperty("arg0str");
		argxstr[1] = line->stringProperty("arg1str");
		string argstr = Game::configuration().actionSpecial(as_id).argSpec().stringDesc(args, argxstr);
		if (!argstr.IsEmpty())
			info_text += (S_FMT("%s", argstr));
		else
			info_text += ("No Args");
	}
	else
		info_text += (S_FMT("Sector Tag: %d", line->intProperty("arg0")));

	// Line flags
	if (map_format != MAP_UDMF)
		info_text += (S_FMT("\nFlags: %s", Game::configuration().lineFlagsString(line)));

	// Setup text box
	text_box->setText(info_text);

	// Check needed textures
	int needed_tex = line->needsTexture();

	// Front side
	MapSide* s = line->s1();
	if (s)
	{
		int xoff = s->intProperty("offsetx");
		int yoff = s->intProperty("offsety");
		side_front.exists = true;
		if (Global::debug)
			side_front.info = S_FMT("Front Side #%d (%d) (Sector %d)", s->getIndex(), s->getId(), s->getSector()->getIndex());
		else
			side_front.info = S_FMT("Front Side #%d (Sector %d)", s->getIndex(), s->getSector()->getIndex());
		side_front.offsets = S_FMT("Offsets: (%d, %d)", xoff, yoff);
		side_front.tex_upper = s->getTexUpper();
		side_front.tex_middle = s->getTexMiddle();
		side_front.tex_lower = s->getTexLower();
		side_front.needs_lower = ((needed_tex & TEX_FRONT_LOWER) > 0);
		side_front.needs_middle = ((needed_tex & TEX_FRONT_MIDDLE) > 0);
		side_front.needs_upper = ((needed_tex & TEX_FRONT_UPPER) > 0);
	}
	else side_front.exists = false;

	// Back side
	s = line->s2();
	if (s)
	{
		int xoff = s->intProperty("offsetx");
		int yoff = s->intProperty("offsety");
		side_back.exists = true;
		if (Global::debug)
			side_back.info = S_FMT("Back Side #%d (%d) (Sector %d)", s->getIndex(), s->getId(), s->getSector()->getIndex());
		else
			side_back.info = S_FMT("Back Side #%d (Sector %d)", s->getIndex(), s->getSector()->getIndex());
		side_back.offsets = S_FMT("Offsets: (%d, %d)", xoff, yoff);
		side_back.tex_upper = s->getTexUpper();
		side_back.tex_middle = s->getTexMiddle();
		side_back.tex_lower = s->getTexLower();
		side_back.needs_lower = ((needed_tex & TEX_BACK_LOWER) > 0);
		side_back.needs_middle = ((needed_tex & TEX_BACK_MIDDLE) > 0);
		side_back.needs_upper = ((needed_tex & TEX_BACK_UPPER) > 0);
	}
	else side_back.exists = false;
}

/* LineInfoOverlay::draw
 * Draws the overlay at [bottom] from 0 to [right]
 *******************************************************************/
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
	if (side_front.exists)
		sides_width += 256;
	if (side_back.exists)
		sides_width += 256;
	if (last_size != right - sides_width)
	{
		last_size = right - sides_width;
		text_box->setSize(right - sides_width);
	}
	int height = text_box->getHeight() + 4;

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;
	col_bg.a = col_bg.a*alpha;
	rgba_t col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += height*alpha_inv*alpha_inv;

	// Determine widths
	int n_side_panels = 0;
	if (side_front.exists)
		n_side_panels++;
	if (side_back.exists)
		n_side_panels++;

	// Draw overlay background
	scale = gl_font_size / 12.0;
	int tex_box_size = 80 * scale;
	int sinf_size = ((tex_box_size * 3) + 16);
	int main_panel_end = right - (n_side_panels * (sinf_size +2));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(1.0f);
	Drawing::drawBorderedRect(0, bottom-height-4, main_panel_end, bottom+2, col_bg, col_border);

	// Draw info text lines
	text_box->setLineHeight(16 * scale);
	text_box->draw(2, bottom - height, col_fg);

	// Side info
	int x = right - sinf_size;
	if (side_front.exists)
	{
		// Background
		glDisable(GL_TEXTURE_2D);
		Drawing::drawBorderedRect(x, bottom-height-4, x + sinf_size, bottom+2, col_bg, col_border);

		drawSide(bottom-4, right, alpha, side_front, x);
		x -= (sinf_size + 2);
	}
	if (side_back.exists)
	{
		// Background
		glDisable(GL_TEXTURE_2D);
		Drawing::drawBorderedRect(x, bottom-height-4, x + sinf_size, bottom+2, col_bg, col_border);

		drawSide(bottom-4, right, alpha, side_back, x);
	}

	// Done
	glEnable(GL_LINE_SMOOTH);
}

/* LineInfoOverlay::drawSide
 * Draws side/texture info for [side]
 *******************************************************************/
void LineInfoOverlay::drawSide(int bottom, int right, float alpha, side_t& side, int xstart)
{
	// Get colours
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;

	// Index and sector index
	Drawing::drawText(side.info, xstart + 4, bottom - (32 * scale), col_fg, Drawing::FONT_CONDENSED);

	// Texture offsets
	Drawing::drawText(side.offsets, xstart + 4, bottom - (16 * scale), col_fg, Drawing::FONT_CONDENSED);

	// Textures
	int tex_box_size = 80 * scale;
	drawTexture(alpha, xstart + 4, bottom - (32 * scale), side.tex_upper, side.needs_upper);
	drawTexture(alpha, xstart + tex_box_size + 8, bottom - (32 * scale), side.tex_middle, side.needs_middle, "M");
	drawTexture(alpha, xstart + tex_box_size + 12 + tex_box_size, bottom - (32 * scale), side.tex_lower, side.needs_lower, "L");
}

/* LineInfoOverlay::drawTexture
 * Draws a texture box with name underneath for [texture]
 *******************************************************************/
void LineInfoOverlay::drawTexture(float alpha, int x, int y, string texture, bool needed, string pos)
{
	bool required = (needed && texture == "-");
	int tex_box_size = 80 * scale;
	int line_height = 16 * scale;

	// Get colours
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;

	// Get texture
	GLTexture* tex = MapEditor::textureManager().getTexture(
		texture,
		Game::configuration().featureSupported(Game::Feature::MixTexFlats)
	);

	// Valid texture
	if (texture != "-" && tex != &(GLTexture::missingTex()))
	{
		// Draw background
		glEnable(GL_TEXTURE_2D);
		OpenGL::setColour(255, 255, 255, 255*alpha, 0);
		glPushMatrix();
		glTranslated(x, y - tex_box_size - line_height, 0);
		GLTexture::bgTex().draw2dTiled(tex_box_size, tex_box_size);
		glPopMatrix();

		// Draw texture
		OpenGL::setColour(255, 255, 255, 255*alpha, 0);
		Drawing::drawTextureWithin(tex, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0);

		glDisable(GL_TEXTURE_2D);

		// Draw outline
		OpenGL::setColour(col_fg.r, col_fg.g, col_fg.b, 255*alpha, 0);
		glDisable(GL_LINE_SMOOTH);
		Drawing::drawRect(x, y - tex_box_size - line_height, x + tex_box_size, y - line_height);
	}

	// Unknown texture
	else if (tex == &(GLTexture::missingTex()) && texture != "-")
	{
		// Draw unknown icon
		GLTexture* icon = MapEditor::textureManager().getEditorImage("thing/unknown");
		glEnable(GL_TEXTURE_2D);
		OpenGL::setColour(180, 0, 0, 255*alpha, 0);
		Drawing::drawTextureWithin(icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.15);

		// Set colour to red (for text)
		col_fg = col_fg.ampf(1.0f, 0.0f, 0.0f, 1.0f);
	}

	// Missing texture
	else if (required)
	{
		// Draw missing icon
		GLTexture* icon = MapEditor::textureManager().getEditorImage("thing/minus");
		glEnable(GL_TEXTURE_2D);
		OpenGL::setColour(180, 0, 0, 255*alpha, 0);
		Drawing::drawTextureWithin(icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.15);

		// Set colour to red (for text)
		col_fg = col_fg.ampf(1.0f, 0.0f, 0.0f, 1.0f);
	}

	// Draw texture name (even if texture is blank)
	if (required)
		texture = "MISSING";
	else if (texture.Length() > 8)
		texture = texture.Truncate(8) + "...";
	texture.Prepend(":");
	texture.Prepend(pos);
	Drawing::drawText(texture, x + (tex_box_size * 0.5), y - line_height, col_fg, Drawing::FONT_CONDENSED, Drawing::ALIGN_CENTER);
}
