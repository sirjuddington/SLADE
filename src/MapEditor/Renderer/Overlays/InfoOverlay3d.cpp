
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    InfoOverlay3d.cpp
 * Description: InfoOverlay3d class - a map editor overlay that
 *              displays information about the currently highlighted
 *              wall/floor/thing in 3d mode
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
#include "InfoOverlay3d.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLUI/LayoutHelpers.h"
#include "OpenGL/GLUI/Animator.h"
#include "OpenGL/GLUI/TextBox.h"
#include "OpenGL/GLUI/TextureBox.h"
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
 * INFOOVERLAY3D CLASS FUNCTIONS
 *******************************************************************/
using namespace GLUI;

/* InfoOverlay3D::InfoOverlay3D
 * InfoOverlay3D class constructor
 *******************************************************************/
InfoOverlay3D::InfoOverlay3D()
	: text_info1(new TextBox(this, "", Drawing::FONT_CONDENSED, Drawing::ALIGN_RIGHT)),
	text_info2(new TextBox(this, "")),
	tex_box(new TextureBox(this)),
	object(NULL)
{
	text_info1->setMargin(padding_t(0, 4));
	text_info2->setMargin(padding_t(0, 4));
	tex_box->setMargin(padding_t(8, 4));
	animators.push_back(anim_activate = new GLUI::SlideAnimator(100, 0, GLUI::SlideAnimator::SLIDE_UP));
}

/* InfoOverlay3D::~InfoOverlay3D
 * InfoOverlay3D class destructor
 *******************************************************************/
InfoOverlay3D::~InfoOverlay3D()
{
}

/* InfoOverlay3D::update
 * Updates the info overlay with an item from [map], of type
 * [item_type] at [item_index]
 *******************************************************************/
void InfoOverlay3D::update(int item_index, int item_type, SLADEMap* map)
{
	// Clear current info
	string info;
	string info2;

	// Setup variables
	int map_format = theMapEditor->currentMapDesc().format;

	// Wall
	if (item_type == MapEditor::SEL_SIDE_BOTTOM ||
		item_type == MapEditor::SEL_SIDE_MIDDLE ||
		item_type == MapEditor::SEL_SIDE_TOP)
	{
		// Get line and side
		MapSide* side = map->getSide(item_index);
		if (!side) return;
		MapLine* line = side->getParentLine();
		if (!line) return;
		object = side;

		// --- Line/side info ---
		info += S_FMT("Line #%d", line->getIndex());
		if (side == line->s1())
			info += S_FMT("\nFront Side #%d", side->getIndex());
		else
			info += S_FMT("\nBack Side #%d", side->getIndex());

		// Relevant flags
		string flags = "";
		if (theGameConfiguration->lineBasicFlagSet("dontpegtop", line, map_format))
			flags += "Upper Unpegged, ";
		if (theGameConfiguration->lineBasicFlagSet("dontpegbottom", line, map_format))
			flags += "Lower Unpegged, ";
		if (theGameConfiguration->lineBasicFlagSet("blocking", line, map_format))
			flags += "Blocking, ";
		if (!flags.IsEmpty())
			flags.RemoveLast(2);
		info += "\n" + flags;

		info += S_FMT("\nLength: %d", (int)line->getLength());

		// Other potential info: special, sector#


		// --- Wall part info ---

		// Part
		if (item_type == MapEditor::SEL_SIDE_BOTTOM)
			info2 = "Lower Texture";
		else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			info2 = "Middle Texture";
		else
			info2 = "Upper Texture";

		// Offsets
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			// Get x offset info
			int xoff = side->intProperty("offsetx");
			double xoff_part = 0;
			if (item_type == MapEditor::SEL_SIDE_BOTTOM)
				xoff_part = side->floatProperty("offsetx_bottom");
			else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
				xoff_part = side->floatProperty("offsetx_mid");
			else
				xoff_part = side->floatProperty("offsetx_top");

			// Add x offset string
			string xoff_info;
			if (xoff_part == 0)
				xoff_info = S_FMT("%d", xoff);
			else if (xoff_part > 0)
				xoff_info = S_FMT("%1.2f (%d+%1.2f)", (double)xoff + xoff_part, xoff, xoff_part);
			else
				xoff_info = S_FMT("%1.2f (%d-%1.2f)", (double)xoff + xoff_part, xoff, -xoff_part);

			// Get y offset info
			int yoff = side->intProperty("offsety");
			double yoff_part = 0;
			if (item_type == MapEditor::SEL_SIDE_BOTTOM)
				yoff_part = side->floatProperty("offsety_bottom");
			else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
				yoff_part = side->floatProperty("offsety_mid");
			else
				yoff_part = side->floatProperty("offsety_top");

			// Add y offset string
			string yoff_info;
			if (yoff_part == 0)
				yoff_info = S_FMT("%d", yoff);
			else if (yoff_part > 0)
				yoff_info = S_FMT("%1.2f (%d+%1.2f)", (double)yoff + yoff_part, yoff, yoff_part);
			else
				yoff_info = S_FMT("%1.2f (%d-%1.2f)", (double)yoff + yoff_part, yoff, -yoff_part);

			info2 += S_FMT("\nOffsets: %s, %s", xoff_info, yoff_info);
		}
		else
		{
			// Basic offsets
			info2 += S_FMT("\nOffsets: %d, %d", side->intProperty("offsetx"), side->intProperty("offsety"));
		}

		// ZDoom UDMF extras
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			// Scale
			double xscale, yscale;
			if (item_type == MapEditor::SEL_SIDE_BOTTOM)
			{
				xscale = side->floatProperty("scalex_bottom");
				yscale = side->floatProperty("scaley_bottom");
			}
			else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			{
				xscale = side->floatProperty("scalex_mid");
				yscale = side->floatProperty("scaley_mid");
			}
			else
			{
				xscale = side->floatProperty("scalex_top");
				yscale = side->floatProperty("scaley_top");
			}
			info2 += S_FMT("\nScale: %1.2fx, %1.2fx", xscale, yscale);
		}
		else
		{
			info2 += "\n";
		}

		// Height of this section of the wall
		// TODO this is wrong in the case of slopes, but slope support only
		// exists in the 3.1.1 branch
		fpoint2_t left_point, right_point;
		MapSide* other_side;
		if (side == line->s1())
		{
			left_point = line->v1()->getPoint(0);
			right_point = line->v2()->getPoint(0);
			other_side = line->s2();
		}
		else
		{
			left_point = line->v2()->getPoint(0);
			right_point = line->v1()->getPoint(0);
			other_side = line->s1();
		}

		MapSector* this_sector = side->getSector();
		MapSector* other_sector = NULL;
		if (other_side)
			other_sector = other_side->getSector();

		double left_height, right_height;
		if (item_type == MapEditor::SEL_SIDE_MIDDLE && other_sector)
		{
			// A two-sided line's middle area is the smallest distance between
			// both sides' floors and ceilings, which is more complicated with
			// slopes.
			plane_t floor1 = this_sector->getFloorPlane();
			plane_t floor2 = other_sector->getFloorPlane();
			plane_t ceiling1 = this_sector->getCeilingPlane();
			plane_t ceiling2 = other_sector->getCeilingPlane();
			left_height = min(ceiling1.height_at(left_point), ceiling2.height_at(left_point))
				- max(floor1.height_at(left_point), floor2.height_at(left_point));
			right_height = min(ceiling1.height_at(right_point), ceiling2.height_at(right_point))
				- max(floor1.height_at(right_point), floor2.height_at(right_point));
		}
		else
		{
			plane_t top_plane, bottom_plane;
			if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			{
				top_plane = this_sector->getCeilingPlane();
				bottom_plane = this_sector->getFloorPlane();
			}
			else
			{
				if (!other_sector) return;
				if (item_type == MapEditor::SEL_SIDE_TOP)
				{
					top_plane = this_sector->getCeilingPlane();
					bottom_plane = other_sector->getCeilingPlane();
				}
				else
				{
					top_plane = other_sector->getFloorPlane();
					bottom_plane = this_sector->getFloorPlane();
				}
			}

			left_height = top_plane.height_at(left_point) - bottom_plane.height_at(left_point);
			right_height = top_plane.height_at(right_point) - bottom_plane.height_at(right_point);
		}
		if (fabs(left_height - right_height) < 0.001)
			info2 += S_FMT("\nHeight: %d", (int)left_height);
		else
			info2 += S_FMT("\nHeight: %d ~ %d", (int)left_height, (int)right_height);

		// Texture
		string texname;
		if (item_type == MapEditor::SEL_SIDE_BOTTOM)
			texname = side->getTexLower();
		else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			texname = side->getTexMiddle();
		else
			texname = side->getTexUpper();

		tex_box->setTexture(TextureBox::TEXTURE, texname);
	}


	// Floor
	else if (item_type == MapEditor::SEL_FLOOR || item_type == MapEditor::SEL_CEILING)
	{
		// Get sector
		MapSector* sector = map->getSector(item_index);
		if (!sector) return;
		object = sector;

		// Get basic info
		int fheight = sector->intProperty("heightfloor");
		int cheight = sector->intProperty("heightceiling");

		// --- Sector info ---

		// Sector index
		info = S_FMT("Sector #%d", item_index);

		// Sector height
		info += S_FMT("\nTotal Height: %d", cheight - fheight);

		// Make info1 up to 4 lines
		info += "\n\n";

		// ZDoom UDMF extras
		/*
		if (theGameConfiguration->udmfNamespace() == "zdoom") {
		// Sector colour
		rgba_t col = sector->getColour(0, true);
		info.push_back(S_FMT("Colour: R%d, G%d, B%d", col.r, col.g, col.b));
		}
		*/


		// --- Flat info ---

		// Height
		if (item_type == MapEditor::SEL_FLOOR)
			info2 = S_FMT("Floor Height: %d", fheight);
		else
			info2 = S_FMT("Ceiling Height: %d", cheight);

		// Light
		int light = sector->intProperty("lightlevel");
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			// Get extra light info
			int fl = 0;
			bool abs = false;
			if (item_type == MapEditor::SEL_FLOOR)
			{
				fl = sector->intProperty("lightfloor");
				abs = sector->boolProperty("lightfloorabsolute");
			}
			else
			{
				fl = sector->intProperty("lightceiling");
				abs = sector->boolProperty("lightceilingabsolute");
			}

			// Set if absolute
			if (abs)
			{
				light = fl;
				fl = 0;
			}

			// Add info string
			if (fl == 0)
				info2 += S_FMT("\nLight: %d", light);
			else if (fl > 0)
				info2 += S_FMT("\nLight: %d (%d+%d)", light + fl, light, fl);
			else
				info2 += S_FMT("\nLight: %d (%d-%d)", light + fl, light, -fl);
		}
		else
			info2 += S_FMT("\nLight: %d", light);

		// ZDoom UDMF extras
		if (theGameConfiguration->udmfNamespace() == "zdoom")
		{
			// Offsets
			double xoff, yoff;
			if (item_type == MapEditor::SEL_FLOOR)
			{
				xoff = sector->floatProperty("xpanningfloor");
				yoff = sector->floatProperty("ypanningfloor");
			}
			else
			{
				xoff = sector->floatProperty("xpanningceiling");
				yoff = sector->floatProperty("ypanningceiling");
			}
			info2 += S_FMT("\nOffsets: %1.2f, %1.2f", xoff, yoff);

			// Scaling
			double xscale, yscale;
			if (item_type == MapEditor::SEL_FLOOR)
			{
				xscale = sector->floatProperty("xscalefloor");
				yscale = sector->floatProperty("yscalefloor");
			}
			else
			{
				xscale = sector->floatProperty("xscaleceiling");
				yscale = sector->floatProperty("yscaleceiling");
			}
			info2 += S_FMT("\nScale: %1.2fx, %1.2fx", xscale, yscale);
		}
		else
			info2 += "\n";

		// Make it up to 4 lines
		info2 += "\n";

		// Texture
		string texname;
		if (item_type == MapEditor::SEL_FLOOR)
			texname = sector->getFloorTex();
		else
			texname = sector->getCeilingTex();

		tex_box->setTexture(TextureBox::FLAT, texname);
	}

	// Thing
	else if (item_type == MapEditor::SEL_THING)
	{
		// index, type, position, sector, zpos, height?, radius?

		// Get thing
		MapThing* thing = map->getThing(item_index);
		if (!thing) return;
		object = thing;

		// Index
		info = S_FMT("Thing #%d", item_index);

		// Position
		if (theMapEditor->currentMapDesc().format == MAP_HEXEN || theMapEditor->currentMapDesc().format == MAP_UDMF)
			info += S_FMT("\nPosition: %d, %d, %d", (int)thing->xPos(), (int)thing->yPos(), (int)thing->floatProperty("height"));
		else
			info += S_FMT("\nPosition: %d, %d", (int)thing->xPos(), (int)thing->yPos());

		// Make info1 4 lines
		info += "\n\n";

		// Type
		ThingType* tt = theGameConfiguration->thingType(thing->getType());
		if (tt->getName() == "Unknown")
			info2 = S_FMT("Type: %d", thing->getType());
		else
			info2 = S_FMT("Type: %s", tt->getName());

		// Args
		if (theMapEditor->currentMapDesc().format == MAP_HEXEN ||
			(theMapEditor->currentMapDesc().format == MAP_UDMF && theGameConfiguration->getUDMFProperty("arg0", MOBJ_THING)))
		{
			// Get thing args
			int args[5];
			args[0] = thing->intProperty("arg0");
			args[1] = thing->intProperty("arg1");
			args[2] = thing->intProperty("arg2");
			args[3] = thing->intProperty("arg3");
			args[4] = thing->intProperty("arg4");
			string argstr = tt->getArgsString(args);

			if (argstr.IsEmpty())
				info2 += "\nNo Args";
			else
				info2 += "\n" + argstr;
		}
		else
			info2 += "\n";

		// Sector
		int sector = map->sectorAt(thing->point());
		if (sector >= 0)
			info2 += S_FMT("\nIn Sector #%d", sector);
		else
			info2 += "\nNo Sector";

		// Make info2 4 lines
		info2 += "\n";

		// Texture
		GLTexture* texture = theMapEditor->textureManager().getSprite(tt->getSprite(), tt->getTranslation(), tt->getPalette());
		if (!texture)
		{
			if (use_zeth_icons && tt->getZeth() >= 0)
				texture = theMapEditor->textureManager().getEditorImage(S_FMT("zethicons/zeth%02d", tt->getZeth()));
			if (!texture)
				texture = theMapEditor->textureManager().getEditorImage(S_FMT("thing/%s", tt->getIcon()));
			//thing_icon = true;
		}
		
		tex_box->setTexture(texture, "");
	}

	text_info1->setText(info);
	text_info2->setText(info2);

	last_update = theApp->runTimer();
}

/* InfoOverlay3D::drawWidget
 * Draw the widget at [pos] with [alpha] transparency
 *******************************************************************/
void InfoOverlay3D::drawWidget(point2_t pos, float alpha)
{
	glDisable(GL_TEXTURE_2D);

	OpenGL::setColour(getBGCol());
	int top = MIN(text_info1->top(true), text_info2->top(true));
	Drawing::drawFilledRect(pos.x, pos.y + top, pos.x + size.x, pos.y + size.y);
}

/* InfoOverlay3D::updateLayout
 * Update the widget layout to fit within [fit]. A negative [fit]
 * dimension indicates no size limit
 *******************************************************************/
void InfoOverlay3D::updateLayout(dim2_t fit)
{
	rect_t rect(0, 0, fit.x, 0);

	// Image box
	tex_box->updateLayout();
	LayoutHelpers::placeWidgetWithin(tex_box, rect, ALIGN_MIDDLE, ALIGN_TOP);

	// Left text
	text_info1->updateLayout();
	LayoutHelpers::placeWidgetToLeft(text_info1, tex_box, USE_MARGIN, ALIGN_BOTTOM);

	// Right text
	text_info2->updateLayout();
	LayoutHelpers::placeWidgetToRight(text_info2, tex_box, USE_MARGIN, ALIGN_BOTTOM);

	setSize(dim2_t(fit.x, tex_box->bottom(true)));
}
