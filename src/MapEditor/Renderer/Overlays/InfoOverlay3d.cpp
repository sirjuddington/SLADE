
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
#include "UI/WxStuff.h"
#include "MapEditor/MapEditorWindow.h"
#include "MapEditor/MapEditor.h"
#include "InfoOverlay3d.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "General/ColourConfiguration.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Bool, use_zeth_icons)
EXTERN_CVAR(Int, gl_font_size)


/*******************************************************************
 * INFOOVERLAY3D CLASS FUNCTIONS
 *******************************************************************/

/* InfoOverlay3D::InfoOverlay3D
 * InfoOverlay3D class constructor
 *******************************************************************/
InfoOverlay3D::InfoOverlay3D()
{
}

/* InfoOverlay3D::~InfoOverlay3D
 * InfoOverlay3D class destructor
 *******************************************************************/
InfoOverlay3D::~InfoOverlay3D()
{
}

/* InfoOverlay3D::update
 * Updates the info text for the object of [item_type] at [item_index]
 * in [map]
 *******************************************************************/
void InfoOverlay3D::update(int item_index, int item_type, SLADEMap* map)
{
	// Clear current info
	info.clear();
	info2.clear();

	// Setup variables
	current_type = item_type;
	texname = "";
	texture = NULL;
	thing_icon = false;
	int map_format = theMapEditor->currentMapDesc().format;

	// Wall
	if (item_type == MapEditor::SEL_SIDE_BOTTOM || item_type == MapEditor::SEL_SIDE_MIDDLE || item_type == MapEditor::SEL_SIDE_TOP)
	{
		// Get line and side
		MapSide* side = map->getSide(item_index);
		if (!side) return;
		MapLine* line = side->getParentLine();
		if (!line) return;
		object = side;

		// --- Line/side info ---
		info.push_back(S_FMT("Line #%d", line->getIndex()));
		if (side == line->s1())
			info.push_back(S_FMT("Front Side #%d", side->getIndex()));
		else
			info.push_back(S_FMT("Back Side #%d", side->getIndex()));

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
		info.push_back(flags);

		info.push_back(S_FMT("Length: %d", (int)line->getLength()));

		// Other potential info: special, sector#


		// --- Wall part info ---

		// Part
		if (item_type == MapEditor::SEL_SIDE_BOTTOM)
			info2.push_back("Lower Texture");
		else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			info2.push_back("Middle Texture");
		else
			info2.push_back("Upper Texture");

		// Offsets
		if (map->currentFormat() == MAP_UDMF && theGameConfiguration->udmfTextureOffsets())
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
				xoff_info = S_FMT("%1.2f (%d+%1.2f)", (double)xoff+xoff_part, xoff, xoff_part);
			else
				xoff_info = S_FMT("%1.2f (%d-%1.2f)", (double)xoff+xoff_part, xoff, -xoff_part);

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
				yoff_info = S_FMT("%1.2f (%d+%1.2f)", (double)yoff+yoff_part, yoff, yoff_part);
			else
				yoff_info = S_FMT("%1.2f (%d-%1.2f)", (double)yoff+yoff_part, yoff, -yoff_part);

			info2.push_back(S_FMT("Offsets: %s, %s", xoff_info, yoff_info));
		}
		else
		{
			// Basic offsets
			info2.push_back(S_FMT("Offsets: %d, %d", side->intProperty("offsetx"), side->intProperty("offsety")));
		}

		// UDMF extras
		if (map->currentFormat() == MAP_UDMF && theGameConfiguration->udmfTextureScaling())
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
			info2.push_back(S_FMT("Scale: %1.2fx, %1.2fx", xscale, yscale));
		}
		else
		{
			info2.push_back("");
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
			info2.push_back(S_FMT("Height: %d", (int)left_height));
		else
			info2.push_back(S_FMT("Height: %d ~ %d", (int)left_height, (int)right_height));

		// Texture
		if (item_type == MapEditor::SEL_SIDE_BOTTOM)
			texname = side->getTexLower();
		else if (item_type == MapEditor::SEL_SIDE_MIDDLE)
			texname = side->getTexMiddle();
		else
			texname = side->getTexUpper();
		texture = theMapEditor->textureManager().getTexture(texname, theGameConfiguration->mixTexFlats());
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
		info.push_back(S_FMT("Sector #%d", item_index));

		// Sector height
		info.push_back(S_FMT("Total Height: %d", cheight - fheight));

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
			info2.push_back(S_FMT("Floor Height: %d", fheight));
		else
			info2.push_back(S_FMT("Ceiling Height: %d", cheight));

		// Light
		int light = sector->intProperty("lightlevel");
		if (theGameConfiguration->udmfFlatLighting())
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
				info2.push_back(S_FMT("Light: %d", light));
			else if (fl > 0)
				info2.push_back(S_FMT("Light: %d (%d+%d)", light+fl, light, fl));
			else
				info2.push_back(S_FMT("Light: %d (%d-%d)", light+fl, light, -fl));
		}
		else
			info2.push_back(S_FMT("Light: %d", light));

		// UDMF extras
		if (theMapEditor->currentMapDesc().format == MAP_UDMF)
		{
			// Offsets
			double xoff, yoff;
			xoff = yoff = 0.0;
			if (theGameConfiguration->udmfFlatPanning())
			{
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
			}
			info2.push_back(S_FMT("Offsets: %1.2f, %1.2f", xoff, yoff));

			// Scaling
			double xscale, yscale;
			xscale = yscale = 1.0;
			if (theGameConfiguration->udmfFlatScaling())
			{
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
			}
			info2.push_back(S_FMT("Scale: %1.2fx, %1.2fx", xscale, yscale));
		}

		// Texture
		if (item_type == MapEditor::SEL_FLOOR)
			texname = sector->getFloorTex();
		else
			texname = sector->getCeilingTex();
		texture = theMapEditor->textureManager().getFlat(texname, theGameConfiguration->mixTexFlats());
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
		info.push_back(S_FMT("Thing #%d", item_index));

		// Position
		if (theMapEditor->currentMapDesc().format == MAP_HEXEN || theMapEditor->currentMapDesc().format == MAP_UDMF)
			info.push_back(S_FMT("Position: %d, %d, %d", (int)thing->xPos(), (int)thing->yPos(), (int)thing->floatProperty("height")));
		else
			info.push_back(S_FMT("Position: %d, %d", (int)thing->xPos(), (int)thing->yPos()));


		// Type
		ThingType* tt = theGameConfiguration->thingType(thing->getType());
		if (tt->getName() == "Unknown")
			info2.push_back(S_FMT("Type: %d", thing->getType()));
		else
			info2.push_back(S_FMT("Type: %s", tt->getName()));

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
			string argxstr[2];
			argxstr[0] = thing->stringProperty("arg0str");
			argxstr[1] = thing->stringProperty("arg1str");
			string argstr = tt->getArgsString(args, argxstr);

			if (argstr.IsEmpty())
				info2.push_back("No Args");
			else
				info2.push_back(argstr);
		}

		// Sector
		int sector = map->sectorAt(thing->point());
		if (sector >= 0)
			info2.push_back(S_FMT("In Sector #%d", sector));
		else
			info2.push_back("No Sector");


		// Texture
		texture = theMapEditor->textureManager().getSprite(tt->getSprite(), tt->getTranslation(), tt->getPalette());
		if (!texture)
		{
			if (use_zeth_icons && tt->getZeth() >= 0)
				texture = theMapEditor->textureManager().getEditorImage(S_FMT("zethicons/zeth%02d", tt->getZeth()));
			if (!texture)
				texture = theMapEditor->textureManager().getEditorImage(S_FMT("thing/%s", tt->getIcon()));
			thing_icon = true;
		}
		texname = "";
	}

	last_update = theApp->runTimer();
}

/* InfoOverlay3D::draw
 * Draws the overlay
 *******************************************************************/
void InfoOverlay3D::draw(int bottom, int right, int middle, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Don't bother if no info
	if (info.size() == 0)
		return;

	// Update if needed
	if ((object->modifiedTime() > last_update) ||									// object updated
		(object->getObjType() == MOBJ_SIDE && (
			((MapSide*)object)->getParentLine()->modifiedTime() > last_update ||	// parent line updated
			((MapSide*)object)->getSector()->modifiedTime() > last_update)))		// parent sector updated
		update(object->getIndex(), current_type, object->getParentMap());

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Determine overlay height
	int nlines = MAX(info.size(), info2.size());
	if (nlines < 4) nlines = 4;
	double scale = (gl_font_size / 12.0);
	int line_height = 16 * scale;
	int height = nlines * line_height + 4;

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_3d_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_3d_overlay_foreground");
	col_fg.a = col_fg.a*alpha;
	col_bg.a = col_bg.a*alpha;
	rgba_t col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	int bottom2 = bottom;
	bottom += height*alpha_inv*alpha_inv;

	// Draw overlay background
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Drawing::drawBorderedRect(0, bottom - height - 4, right, bottom + 2, col_bg, col_border);

	// Draw info text lines (left)
	int y = height;
	for (unsigned a = 0; a < info.size(); a++)
	{
		Drawing::drawText(info[a], middle - (40 * scale) - 4, bottom - y, col_fg, Drawing::FONT_CONDENSED, Drawing::ALIGN_RIGHT);
		y -= line_height;
	}

	// Draw info text lines (right)
	y = height;
	for (unsigned a = 0; a < info2.size(); a++)
	{
		Drawing::drawText(info2[a], middle + (40 * scale) + 4, bottom - y, col_fg, Drawing::FONT_CONDENSED);
		y -= line_height;
	}

	// Draw texture if any
	drawTexture(alpha, middle - (40 * scale), bottom);

	// Done
	glEnable(GL_LINE_SMOOTH);
}

/* InfoOverlay3D::drawTexture
 * Draws the item texture/graphic box (if any)
 *******************************************************************/
void InfoOverlay3D::drawTexture(float alpha, int x, int y)
{
	double scale = (gl_font_size / 12.0);
	int tex_box_size = 80 * scale;
	int line_height = 16 * scale;

	// Get colours
	rgba_t col_bg = ColourConfiguration::getColour("map_3d_overlay_background");
	rgba_t col_fg = ColourConfiguration::getColour("map_3d_overlay_foreground");
	col_fg.a = col_fg.a*alpha;

	// Check texture exists
	if (texture)
	{
		// Draw background
		glEnable(GL_TEXTURE_2D);
		OpenGL::setColour(255, 255, 255, 255*alpha, 0);
		glPushMatrix();
		glTranslated(x, y - tex_box_size - line_height, 0);
		GLTexture::bgTex().draw2dTiled(tex_box_size, tex_box_size);
		glPopMatrix();

		// Draw texture
		if (texture && texture != &(GLTexture::missingTex()))
		{
			OpenGL::setColour(255, 255, 255, 255*alpha, 0);
			Drawing::drawTextureWithin(texture, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0);
		}
		else if (texname == "-")
		{
			// Draw missing icon
			GLTexture* icon = theMapEditor->textureManager().getEditorImage("thing/minus");
			glEnable(GL_TEXTURE_2D);
			OpenGL::setColour(180, 0, 0, 255*alpha, 0);
			Drawing::drawTextureWithin(icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.2);
		}
		else if (texname != "-" && texture == &(GLTexture::missingTex()))
		{
			// Draw unknown icon
			GLTexture* icon = theMapEditor->textureManager().getEditorImage("thing/unknown");
			glEnable(GL_TEXTURE_2D);
			OpenGL::setColour(180, 0, 0, 255*alpha, 0);
			Drawing::drawTextureWithin(icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.2);
		}

		glDisable(GL_TEXTURE_2D);

		// Draw outline
		OpenGL::setColour(col_fg.r, col_fg.g, col_fg.b, 255*alpha, 0);
		glLineWidth(1.0f);
		glDisable(GL_LINE_SMOOTH);
		Drawing::drawRect(x, y - tex_box_size - line_height, x + tex_box_size, y - line_height);
	}

	// Draw texture name (even if texture is blank)
	if (texname.Length() > 8)
		texname = texname.Truncate(8) + "...";
	Drawing::drawText(texname, x + (tex_box_size * 0.5), y - line_height, col_fg, Drawing::FONT_CONDENSED, Drawing::ALIGN_CENTER);
}
