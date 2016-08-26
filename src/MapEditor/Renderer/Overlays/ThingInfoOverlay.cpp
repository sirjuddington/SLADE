
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ThingInfoOverlay.cpp
 * Description: ThingInfoOverlay class - a map editor overlay that
 *              displays information about the currently highlighted
 *              thing in things mode
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
#include "ThingInfoOverlay.h"
#include "MapEditor/SLADEMap/MapThing.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLUI/ImageBox.h"
#include "OpenGL/GLUI/LayoutHelpers.h"
#include "OpenGL/GLUI/Animator.h"
#include "OpenGL/GLUI/TextBox.h"
#include "OpenGL/OpenGL.h"

 // Really need to move the texture manager out of the map editor window,
 // shouldn't need WxStuff.h just to get the map texture manager
#include "MapEditor/MapEditorWindow.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, use_zeth_icons)
EXTERN_CVAR(Int, gl_font_size)


/*******************************************************************
 * THINGINFOOVERLAY CLASS FUNCTIONS
 *******************************************************************/
using namespace GLUI;

/* ThingInfoOverlay::ThingInfoOverlay
 * ThingInfoOverlay class constructor
 *******************************************************************/
ThingInfoOverlay::ThingInfoOverlay()
{
	text_info = new TextBox(this, "");
	image_sprite = new ImageBox(this);
	image_sprite->setBackgroundStyle(ImageBox::BACKGROUND_NONE);
	image_sprite->setBorder(1.0f, Border::None);

	setBGCol(rgba_t(0, 0, 0, 160));
}

/* ThingInfoOverlay::~ThingInfoOverlay
 * ThingInfoOverlay class destructor
 *******************************************************************/
ThingInfoOverlay::~ThingInfoOverlay()
{
}

/* ThingInfoOverlay::update
 * Update the info overlay with MapThing [thing]
 *******************************************************************/
void ThingInfoOverlay::update(MapThing* thing)
{
	if (!thing)
	{
		// Animate
		activate(false);
		return;
	}

	// Animate
	activate(true);

	string info_text;
	int map_format = theMapEditor->currentMapDesc().format;

	// Index + type
	ThingType* tt = theGameConfiguration->thingType(thing->getType());
	string type = S_FMT("%s (Type %d)", tt->getName(), thing->getType());
	if (Global::debug)
		info_text += S_FMT("Thing #%d (%d): %s\n", thing->getIndex(), thing->getId(), type);
	else
		info_text += S_FMT("Thing #%d: %s\n", thing->getIndex(), type);

	// Position
	if (map_format != MAP_DOOM)
		info_text += S_FMT("Position: %d, %d, %d\n", (int)thing->xPos(), (int)thing->yPos(), (int)(thing->floatProperty("height")));
	else
		info_text += S_FMT("Position: %d, %d\n", (int)thing->xPos(), (int)thing->yPos());

	// Direction
	int angle = thing->intProperty("angle");
	string dir = S_FMT("%d degrees", angle);
	if (angle == 0)
		dir = "East";
	else if (angle == 45)
		dir = "Northeast";
	else if (angle == 90)
		dir = "North";
	else if (angle == 135)
		dir = "Northwest";
	else if (angle == 180)
		dir = "West";
	else if (angle == 225)
		dir = "Southwest";
	else if (angle == 270)
		dir = "South";
	else if (angle == 315)
		dir = "Southeast";
	info_text += S_FMT("Direction: %s\n", dir);

	// Special and Args (if in hexen format or udmf with thing args)
	if (map_format == MAP_HEXEN ||
		(map_format == MAP_UDMF && theGameConfiguration->getUDMFProperty("arg0", MOBJ_THING)))
	{
		int as_id = thing->intProperty("special");
		info_text += S_FMT("Special: %d (%s)\n", as_id, theGameConfiguration->actionSpecialName(as_id));
		int args[5];
		args[0] = thing->intProperty("arg0");
		args[1] = thing->intProperty("arg1");
		args[2] = thing->intProperty("arg2");
		args[3] = thing->intProperty("arg3");
		args[4] = thing->intProperty("arg4");
		string argstr;
		if (tt->getArgspec().count > 0)
			argstr = tt->getArgsString(args);
		else
			argstr = theGameConfiguration->actionSpecial(as_id)->getArgsString(args);

		if (!argstr.IsEmpty())
			info_text += S_FMT("%s\n", argstr);
		else
			info_text += "No Args\n";
	}

	// Flags
	if (map_format != MAP_UDMF)
		info_text += S_FMT("Flags: %s\n", theGameConfiguration->thingFlagsString(thing->intProperty("flags")));

	// TID (if in doom64/hexen/udmf format)
	if (map_format != MAP_DOOM)
		info_text += S_FMT("TID: %i", thing->intProperty("id"));

	if (info_text.EndsWith("\n"))
		info_text.RemoveLast(1);

	text_info->setText(info_text);

	// Load sprite
	bool isicon = false;
	GLTexture* tex = theMapEditor->textureManager().getSprite(tt->getSprite(), tt->getTranslation(), tt->getPalette());
	if (!tex)
	{
		if (use_zeth_icons && tt->getZeth() >= 0)
			tex = theMapEditor->textureManager().getEditorImage(S_FMT("zethicons/zeth%02d", tt->getZeth()));
		if (!tex)
			tex = theMapEditor->textureManager().getEditorImage(S_FMT("thing/%s", tt->getIcon()));
		isicon = true;
	}
	image_sprite->setTexture(tex);
}

/* ThingInfoOverlay::drawWidget
 * Draw the widget at [pos] with [alpha] transparency
 *******************************************************************/
void ThingInfoOverlay::drawWidget(point2_t pos, float alpha)
{
	glDisable(GL_TEXTURE_2D);

	OpenGL::setColour(getBGCol(alpha));
	Drawing::drawFilledRect(
		fpoint2_t(pos.x, pos.y + text_info->top() - 4),
		fpoint2_t(pos.x + size.x, pos.y + getHeight())
		);
}

/* ThingInfoOverlay::updateLayout
 * Update the widget layout to fit within [fit]. A negative [fit]
 * dimension indicates no size limit
 *******************************************************************/
void ThingInfoOverlay::updateLayout(dim2_t fit)
{
	image_sprite->setSizeFromImage();
	LayoutHelpers::placeWidgetWithin(image_sprite, rect_t(point2_t(0, 0), point2_t(fit.x, 0)), ALIGN_RIGHT, ALIGN_TOP, padding_t(8, 0));

	text_info->updateLayout(dim2_t(image_sprite->left() - 8, -1));
	LayoutHelpers::alignBottoms(text_info, image_sprite);

	fitToChildren(padding_t(4, 8, 8, 4));
}
