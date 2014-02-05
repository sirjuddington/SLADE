
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SectorInfoOverlay.cpp
 * Description: SectorInfoOverlay class - a map editor overlay that
 *              displays information about the currently highlighted
 *              sector in sectors mode
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
#include "WxStuff.h"
#include "SectorInfoOverlay.h"
#include "MapSector.h"
#include "Drawing.h"
#include "MapEditorWindow.h"
#include "ColourConfiguration.h"
#include "GameConfiguration.h"
#include "OpenGL.h"


/*******************************************************************
 * SECTORINFOOVERLAY CLASS FUNCTIONS
 *******************************************************************/

/* SectorInfoOverlay::SectorInfoOverlay
 * SectorInfoOverlay class constructor
 *******************************************************************/
SectorInfoOverlay::SectorInfoOverlay()
{
}

/* SectorInfoOverlay::~SectorInfoOverlay
 * SectorInfoOverlay class destructor
 *******************************************************************/
SectorInfoOverlay::~SectorInfoOverlay()
{
}

/* SectorInfoOverlay::update
 * Updates the overlay with info from [sector]
 *******************************************************************/
void SectorInfoOverlay::update(MapSector* sector)
{
	if (!sector)
		return;

	info.clear();

	// Info (index + type)
	int t = sector->intProperty("special");
	string type = S_FMT("%s (Type %d)", theGameConfiguration->sectorTypeName(t, theMapEditor->currentMapDesc().format), t);
	if (Global::debug)
		info.push_back(S_FMT("Sector #%d (%d): %s", sector->getIndex(), sector->getId(), type));
	else
		info.push_back(S_FMT("Sector #%d: %s", sector->getIndex(), type));

	// Height
	int fh = sector->intProperty("heightfloor");
	int ch = sector->intProperty("heightceiling");
	info.push_back(S_FMT("Height: %d to %d (%d total)", fh, ch, ch - fh));

	// Brightness
	info.push_back(S_FMT("Brightness: %d", sector->intProperty("lightlevel")));

	// Tag
	info.push_back(S_FMT("Tag: %d", sector->intProperty("id")));

	// Textures
	ftex = sector->getFloorTex();
	ctex = sector->getCeilingTex();
}

/* SectorInfoOverlay::draw
 * Draws the overlay at [bottom] from 0 to [right]
 *******************************************************************/
void SectorInfoOverlay::draw(int bottom, int right, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Determine overlay height
	int height = info.size() * 16 + 4;

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	bottom += height*alpha_inv*alpha_inv;

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;
	col_bg.a = col_bg.a*alpha;
	rgba_t col_border(0, 0, 0, 140);

	// Draw overlay background
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Drawing::drawBorderedRect(0, bottom - height - 4, right - 190, bottom+2, col_bg, col_border);
	Drawing::drawBorderedRect(right - 188, bottom - height - 4, right, bottom+2, col_bg, col_border);

	// Draw info text lines
	int y = height;
	for (unsigned a = 0; a < info.size(); a++)
	{
		Drawing::drawText(info[a], 2, bottom - y, col_fg, Drawing::FONT_CONDENSED);
		y -= 16;
	}

	// Ceiling texture
	drawTexture(alpha, right - 88, bottom - 4, ctex, "C");

	// Floor texture
	drawTexture(alpha, right - 88 - 92, bottom - 4, ftex, "F");

	// Done
	glEnable(GL_LINE_SMOOTH);
}

/* SectorInfoOverlay::drawTexture
 * Draws a texture box with name underneath for [texture]
 *******************************************************************/
void SectorInfoOverlay::drawTexture(float alpha, int x, int y, string texture, string pos)
{
	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_overlay_foreground");
	col_fg.a = col_fg.a*alpha;

	// Check texture isn't blank
	if (!(S_CMPNOCASE(texture, "-")))
	{
		// Draw background
		glEnable(GL_TEXTURE_2D);
		OpenGL::setColour(255, 255, 255, 255*alpha, 0);
		glPushMatrix();
		glTranslated(x, y-96, 0);
		GLTexture::bgTex().draw2dTiled(80, 80);
		glPopMatrix();

		// Get texture
		GLTexture* tex = theMapEditor->textureManager().getFlat(texture, theGameConfiguration->mixTexFlats());

		// Draw texture
		if (tex)
		{
			OpenGL::setColour(255, 255, 255, 255*alpha, 0);
			Drawing::drawTextureWithin(tex, x, y - 96, x + 80, y - 16, 0, 100);
		}

		glDisable(GL_TEXTURE_2D);

		// Draw outline
		OpenGL::setColour(col_fg.r, col_fg.g, col_fg.b, 255*alpha, 0);
		glLineWidth(1.0f);
		glDisable(GL_LINE_SMOOTH);
		Drawing::drawRect(x, y-96, x+80, y-16);
	}

	// Draw texture name (even if texture is blank)
	texture.Prepend(":");
	texture.Prepend(pos);
	Drawing::drawText(texture, x + 40, y - 16, col_fg, Drawing::FONT_CONDENSED, Drawing::ALIGN_CENTER);
}
