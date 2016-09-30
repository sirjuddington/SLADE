
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
#include "SectorInfoOverlay.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "MapEditor/SLADEMap/MapSector.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLUI/LayoutHelpers.h"
#include "OpenGL/GLUI/Animator.h"
#include "OpenGL/GLUI/TextBox.h"
#include "OpenGL/GLUI/TextureBox.h"
#include "OpenGL/OpenGL.h"


/*******************************************************************
 * SECTORINFOOVERLAY CLASS FUNCTIONS
 *******************************************************************/
using namespace GLUI;

/* SectorInfoOverlay::SectorInfoOverlay
 * SectorInfoOverlay class constructor
 *******************************************************************/
SectorInfoOverlay::SectorInfoOverlay() :
	text_info(new TextBox(this, "")),
	tex_floor(new TextureBox(this)),
	tex_ceiling(new TextureBox(this))
{
	setBGCol(rgba_t(0, 0, 0, 160));
	tex_ceiling->setMargin(padding_t(4, 8, 8, 2));
	tex_floor->setMargin(padding_t(4, 8, 4, 2));
	text_info->setMargin(padding_t(4, 8, 4, 4));
}

/* SectorInfoOverlay::~SectorInfoOverlay
 * SectorInfoOverlay class destructor
 *******************************************************************/
SectorInfoOverlay::~SectorInfoOverlay()
{
}

/* SectorInfoOverlay::update
 * Update the info overlay with MapSector [sector]
 *******************************************************************/
void SectorInfoOverlay::update(MapSector* sector)
{
	if (!sector)
	{
		// Animate
		activate(false);
		return;
	}

	// Animate
	activate(true);

	string info_text;

	// Info (index + type)
	int t = sector->intProperty("special");
	string type = S_FMT("%s (Type %d)", theGameConfiguration->sectorTypeName(t), t);
	if (Global::debug)
		info_text += S_FMT("Sector #%d (%d): %s\n", sector->getIndex(), sector->getId(), type);
	else
		info_text += S_FMT("Sector #%d: %s\n", sector->getIndex(), type);

	// Height
	int fh = sector->intProperty("heightfloor");
	int ch = sector->intProperty("heightceiling");
	info_text += S_FMT("Height: %d to %d (%d total)\n", fh, ch, ch - fh);

	// Brightness
	info_text += S_FMT("Brightness: %d\n", sector->intProperty("lightlevel"));

	// Tag
	info_text += S_FMT("Tag: %d", sector->intProperty("id"));

	// Info text
	text_info->setText(info_text);

	// Textures
	tex_floor->setTexture(TextureBox::FLAT, sector->getFloorTex(), "F: ");
	tex_ceiling->setTexture(TextureBox::FLAT, sector->getCeilingTex(), "C: ");
}

/* SectorInfoOverlay::drawWidget
 * Draw the widget at [pos] with [alpha] transparency
 *******************************************************************/
void SectorInfoOverlay::drawWidget(fpoint2_t pos, float alpha, fpoint2_t scale)
{
	glDisable(GL_TEXTURE_2D);

	OpenGL::setColour(getBGCol(alpha));
	Drawing::drawFilledRect(
		fpoint2_t(pos.x, pos.y + text_info->top() - 4),
		fpoint2_t(pos.x + size.x, pos.y + getHeight())
		);
}

/* SectorInfoOverlay::updateLayout
 * Update the widget layout to fit within [fit]. A negative [fit]
 * dimension indicates no size limit
 *******************************************************************/
void SectorInfoOverlay::updateLayout(dim2_t fit)
{
	rect_t rect(0, 0, fit.x, 1000);

	// Ceiling texture
	tex_ceiling->updateLayout();
	LayoutHelpers::placeWidgetWithin(tex_ceiling, rect, Align::Right, Align::Bottom);

	// Floor texture
	tex_floor->updateLayout();
	LayoutHelpers::placeWidgetToLeft(tex_floor, tex_ceiling, USE_MARGIN, Align::Bottom);

	// Info text
	text_info->updateLayout(dim2_t(tex_floor->left(true) - text_info->getMargin().horizontal(), -1));
	LayoutHelpers::placeWidgetWithin(text_info, rect, Align::Left, Align::Bottom);

	fitToChildren();
}
