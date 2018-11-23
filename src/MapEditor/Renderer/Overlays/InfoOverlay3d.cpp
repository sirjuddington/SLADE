
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    InfoOverlay3d.cpp
// Description: InfoOverlay3d class - a map editor overlay that displays
//              information about the currently highlighted wall/floor/thing in
//              3d mode
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
#include "InfoOverlay3d.h"
#include "App.h"
#include "Game/Configuration.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/OpenGL.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, use_zeth_icons)


// -----------------------------------------------------------------------------
//
// InfoOverlay3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// InfoOverlay3D class constructor
// -----------------------------------------------------------------------------
InfoOverlay3D::InfoOverlay3D() :
	current_type_(MapEditor::ItemType::WallMiddle),
	texture_(nullptr),
	thing_icon_(false),
	object_(nullptr),
	last_update_(0)
{
}

// -----------------------------------------------------------------------------
// InfoOverlay3D class destructor
// -----------------------------------------------------------------------------
InfoOverlay3D::~InfoOverlay3D() {}

// -----------------------------------------------------------------------------
// Updates the info text for the object of [item_type] at [item_index] in [map]
// -----------------------------------------------------------------------------
void InfoOverlay3D::update(MapEditor::Item item, SLADEMap* map)
{
	using Game::Feature;
	using Game::UDMFFeature;

	int item_index = item.index;
	MapEditor::ItemType item_type = item.type;

	// Clear current info
	info_.clear();
	info2_.clear();

	// Setup variables
	current_type_  = item_type;
	current_item_  = item;
	texname_       = "";
	texture_       = nullptr;
	thing_icon_    = false;
	int map_format = MapEditor::editContext().mapDesc().format;

	// Wall
	if (item_type == MapEditor::ItemType::WallBottom ||
		item_type == MapEditor::ItemType::WallMiddle ||
		item_type == MapEditor::ItemType::WallTop/* ||
		item_type == MapEditor::ItemType::Wall3DFloor*/)
	{
		// Get line and side
		MapSide* side = map->side(item_index);
		if (!side)
			return;
		MapLine* line = side->parentLine();
		if (!line) return;
		/*
		MapLine* line3d = nullptr;
		if (item_type == MapEditor::ItemType::Wall3DFloor) {
			line3d = line;
			line = map->getLine(item.control_line);
			side = line->s1();
		}
		*/
		object_ = side;

		// TODO 3d floors

		// --- Line/side info ---
		if(item.real_index >= 0) {
			info_.push_back(S_FMT("3D floor line #%d on line #%d", line->index(), map->side(item.real_index)->parentLine()->index()));
		} else {
			info_.push_back(S_FMT("Line #%d", line->index()));
		}
		if (side == line->s1())
			info_.push_back(S_FMT("Front Side #%d", side->index()));
		else
			info_.push_back(S_FMT("Back Side #%d", side->index()));

		// Relevant flags
		string flags = "";
		if (Game::configuration().lineBasicFlagSet("dontpegtop", line, map_format))
			flags += "Upper Unpegged, ";
		if (Game::configuration().lineBasicFlagSet("dontpegbottom", line, map_format))
			flags += "Lower Unpegged, ";
		if (Game::configuration().lineBasicFlagSet("blocking", line, map_format))
			flags += "Blocking, ";
		if (!flags.IsEmpty())
			flags.RemoveLast(2);
		info_.push_back(flags);

		info_.push_back(S_FMT("Length: %d", (int)line->length()));

		// Other potential info: special, sector#


		// --- Wall part info ---

		// Part
		if (item_type == MapEditor::ItemType::WallBottom)
			info2_.push_back("Lower Texture");
		else if (item_type == MapEditor::ItemType::WallMiddle)
			info2_.push_back("Middle Texture");
		else if (item_type == MapEditor::ItemType::WallTop)
			info2_.push_back("Upper Texture");
		/*else if (item_type == MapEditor::ItemType::Wall3DFloor)
			info2.push_back("3D Floor Texture"); // TODO: determine*/

		// Offsets
		if (map->currentFormat() == MAP_UDMF && Game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			// Get x offset info
			int    xoff      = side->intProperty("offsetx");
			double xoff_part = 0;
			if (item_type == MapEditor::ItemType::WallBottom)
				xoff_part = side->floatProperty("offsetx_bottom");
			else if (item_type == MapEditor::ItemType::WallMiddle)
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
			int    yoff      = side->intProperty("offsety");
			double yoff_part = 0;
			if (item_type == MapEditor::ItemType::WallBottom)
				yoff_part = side->floatProperty("offsety_bottom");
			else if (item_type == MapEditor::ItemType::WallMiddle)
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

			info2_.push_back(S_FMT("Offsets: %s, %s", xoff_info, yoff_info));
		}
		else
		{
			// Basic offsets
			info2_.push_back(S_FMT("Offsets: %d, %d", side->intProperty("offsetx"), side->intProperty("offsety")));
		}

		// UDMF extras
		if (map->currentFormat() == MAP_UDMF && Game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			// Scale
			double xscale, yscale;
			if (item_type == MapEditor::ItemType::WallBottom)
			{
				xscale = side->floatProperty("scalex_bottom");
				yscale = side->floatProperty("scaley_bottom");
			}
			else if (item_type == MapEditor::ItemType::WallMiddle)
			{
				xscale = side->floatProperty("scalex_mid");
				yscale = side->floatProperty("scaley_mid");
			}
			else
			{
				xscale = side->floatProperty("scalex_top");
				yscale = side->floatProperty("scaley_top");
			}
			info2_.push_back(S_FMT("Scale: %1.2fx, %1.2fx", xscale, yscale));
		}
		else
		{
			info2_.push_back("");
		}

		// Height of this section of the wall
		Vec2f     left_point, right_point;
		MapSide*  other_side;
		if (side == line->s1())
		{
			left_point  = line->v1()->point();
			right_point = line->v2()->point();
			other_side  = line->s2();
		}
		else
		{
			left_point  = line->v2()->point();
			right_point = line->v1()->point();
			other_side  = line->s1();
		}

		MapSector* this_sector  = side->sector();
		MapSector* other_sector = nullptr;
		if (other_side)
			other_sector = other_side->sector();

		double left_height, right_height;
		if ((item_type == MapEditor::ItemType::WallMiddle/* || item_type == MapEditor::ItemType::Wall3DFloor*/) && other_sector)
		{
			// A two-sided line's middle area is the smallest distance between
			// both sides' floors and ceilings, which is more complicated with
			// slopes.
			Plane floor1   = this_sector->floor().plane;
			Plane floor2   = other_sector->floor().plane;
			Plane ceiling1 = this_sector->ceiling().plane;
			Plane ceiling2 = other_sector->ceiling().plane;
			left_height    = min(ceiling1.height_at(left_point), ceiling2.height_at(left_point))
						  - max(floor1.height_at(left_point), floor2.height_at(left_point));
			right_height = min(ceiling1.height_at(right_point), ceiling2.height_at(right_point))
						   - max(floor1.height_at(right_point), floor2.height_at(right_point));
		}
		else
		{
			Plane top_plane, bottom_plane;
			if (item_type == MapEditor::ItemType::WallMiddle/* || item_type == MapEditor::ItemType::Wall3DFloor*/)
			{
				top_plane    = this_sector->ceiling().plane;
				bottom_plane = this_sector->floor().plane;
			}
			else
			{
				if (!other_sector)
					return;
				if (item_type == MapEditor::ItemType::WallTop)
				{
					top_plane    = this_sector->ceiling().plane;
					bottom_plane = other_sector->ceiling().plane;
				}
				else
				{
					top_plane    = other_sector->floor().plane;
					bottom_plane = this_sector->floor().plane;
				}
			}

			left_height  = top_plane.height_at(left_point) - bottom_plane.height_at(left_point);
			right_height = top_plane.height_at(right_point) - bottom_plane.height_at(right_point);
		}
		if (fabs(left_height - right_height) < 0.001)
			info2_.push_back(S_FMT("Height: %d", (int)left_height));
		else
			info2_.push_back(S_FMT("Height: %d ~ %d", (int)left_height, (int)right_height));

		// Texture
		if (item_type == MapEditor::ItemType::WallBottom)
			texname_ = side->texLower();
		else if (item_type == MapEditor::ItemType::WallMiddle)
			texname_ = side->texMiddle();
		else if (item_type == MapEditor::ItemType::WallTop)
			texname_ = side->texUpper();
		/*else if (item_type == MapEditor::ItemType::Wall3DFloor)
			texname = side->getTexMiddle(); // TODO: Upper/lower flags*/
		texture_ =
			MapEditor::textureManager().texture(texname_, Game::configuration().featureSupported(Feature::MixTexFlats));
	}


	// Floor
	else if (item_type == MapEditor::ItemType::Floor || item_type == MapEditor::ItemType::Ceiling)
	{
		bool floor = (item_type == MapEditor::ItemType::Floor);

		// Get sector
		MapSector* real_sector = map->sector(item_index);
		if (!real_sector)
			return;
		object_ = real_sector;

		// For a 3D floor, use the control sector for most properties
		// TODO this is already duplicated elsewhere that examines a highlight; maybe need a map method?
		MapSector* sector = real_sector;

		// Get basic info
		int fheight = sector->intProperty("heightfloor");
		int cheight = sector->intProperty("heightceiling");

		// --- Sector info ---

		// Sector index
		if (sector == real_sector)
			info_.push_back(S_FMT("Sector #%d", item_index));
		else
			info_.push_back(S_FMT("3D floor in sector #%d", item_index));

		// Sector height
		info_.push_back(S_FMT("Total Height: %d", cheight - fheight));

		// ZDoom UDMF extras
		/*
		if (Game::configuration().udmfNamespace() == "zdoom") {
			// Sector colour
			ColRGBA col = sector->getColour(0, true);
			info.push_back(S_FMT("Colour: R%d, G%d, B%d", col.r, col.g, col.b));
		}
		*/


		// --- Flat info ---

		// Height
		if (floor)
			info2_.push_back(S_FMT("Floor Height: %d", fheight));
		else
			info2_.push_back(S_FMT("Ceiling Height: %d", cheight));

		// Light
		// TODO this is more complex with 3D floors -- also i would like to not dupe code from the renderer, so maybe this should be MapSpecials's problem?
		int light = sector->intProperty("lightlevel");
		if (Game::configuration().featureSupported(UDMFFeature::FlatLighting))
		{
			// Get extra light info
			int  fl  = 0;
			bool abs = false;
			if (floor)
			{
				fl  = sector->intProperty("lightfloor");
				abs = sector->boolProperty("lightfloorabsolute");
			}
			else
			{
				fl  = sector->intProperty("lightceiling");
				abs = sector->boolProperty("lightceilingabsolute");
			}

			// Set if absolute
			if (abs)
			{
				light = fl;
				fl    = 0;
			}

			// Add info string
			if (fl == 0)
				info2_.push_back(S_FMT("Light: %d", light));
			else if (fl > 0)
				info2_.push_back(S_FMT("Light: %d (%d+%d)", light + fl, light, fl));
			else
				info2_.push_back(S_FMT("Light: %d (%d-%d)", light + fl, light, -fl));
		}
		else
			info2_.push_back(S_FMT("Light: %d", light));

		// UDMF extras
		if (MapEditor::editContext().mapDesc().format == MAP_UDMF)
		{
			// Offsets
			double xoff, yoff;
			xoff = yoff = 0.0;
			if (Game::configuration().featureSupported(UDMFFeature::FlatPanning))
			{
				if (floor)
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
			info2_.push_back(S_FMT("Offsets: %1.2f, %1.2f", xoff, yoff));

			// Scaling
			double xscale, yscale;
			xscale = yscale = 1.0;
			if (Game::configuration().featureSupported(UDMFFeature::FlatScaling))
			{
				if (floor)
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
			info2_.push_back(S_FMT("Scale: %1.2fx, %1.2fx", xscale, yscale));
		}

		// Texture
		if (floor)
			texname_ = sector->floor().texture;
		else
			texname_ = sector->ceiling().texture;
		texture_ =
			MapEditor::textureManager().flat(texname_, Game::configuration().featureSupported(Feature::MixTexFlats));
	}

	// Thing
	else if (item_type == MapEditor::ItemType::Thing)
	{
		// index, type, position, sector, zpos, height?, radius?

		// Get thing
		MapThing* thing = map->thing(item_index);
		if (!thing)
			return;
		object_ = thing;

		// Index
		info_.push_back(S_FMT("Thing #%d", item_index));

		// Position
		if (MapEditor::editContext().mapDesc().format == MAP_HEXEN
			|| MapEditor::editContext().mapDesc().format == MAP_UDMF)
			info_.push_back(S_FMT(
				"Position: %d, %d, %d", (int)thing->xPos(), (int)thing->yPos(), (int)thing->floatProperty("height")));
		else
			info_.push_back(S_FMT("Position: %d, %d", (int)thing->xPos(), (int)thing->yPos()));


		// Type
		auto& tt = Game::configuration().thingType(thing->type());
		if (!tt.defined())
			info2_.push_back(S_FMT("Type: %d", thing->type()));
		else
			info2_.push_back(S_FMT("Type: %s", tt.name()));

		// Args
		if (MapEditor::editContext().mapDesc().format == MAP_HEXEN
			|| (MapEditor::editContext().mapDesc().format == MAP_UDMF
				&& Game::configuration().getUDMFProperty("arg0", MapObject::Type::Thing)))
		{
			// Get thing args
			int args[5];
			args[0] = thing->intProperty("arg0");
			args[1] = thing->intProperty("arg1");
			args[2] = thing->intProperty("arg2");
			args[3] = thing->intProperty("arg3");
			args[4] = thing->intProperty("arg4");
			string argxstr[2];
			argxstr[0]    = thing->stringProperty("arg0str");
			argxstr[1]    = thing->stringProperty("arg1str");
			string argstr = tt.argSpec().stringDesc(args, argxstr);

			if (argstr.IsEmpty())
				info2_.push_back("No Args");
			else
				info2_.push_back(argstr);
		}

		// Sector
		int sector = map->sectorAt(thing->point());
		if (sector >= 0)
			info2_.push_back(S_FMT("In Sector #%d", sector));
		else
			info2_.push_back("No Sector");


		// Texture
		texture_ = MapEditor::textureManager().sprite(tt.sprite(), tt.translation(), tt.palette());
		if (!texture_)
		{
			if (use_zeth_icons && tt.zethIcon() >= 0)
				texture_ = MapEditor::textureManager().editorImage(S_FMT("zethicons/zeth%02d", tt.zethIcon()));
			if (!texture_)
				texture_ = MapEditor::textureManager().editorImage(S_FMT("thing/%s", tt.icon()));
			thing_icon_ = true;
		}
		texname_ = "";
	}

	last_update_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Draws the overlay
// -----------------------------------------------------------------------------
void InfoOverlay3D::draw(int bottom, int right, int middle, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Don't bother if no info
	if (info_.size() == 0)
		return;

	// Update if needed
	if (object_
		&& (object_->modifiedTime() > last_update_ || // object_ updated
			(object_->objType() == MapObject::Type::Side
			 && (((MapSide*)object_)->parentLine()->modifiedTime() > last_update_ || // parent line updated
				 ((MapSide*)object_)->sector()->modifiedTime() > last_update_))))    // parent sector updated
	{
		MapEditor::Item newitem(object_->index(), current_type_);
		newitem.real_index = current_item_.real_index;
		update(newitem, object_->parentMap());
	}

	// Init GL stuff
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);

	// Determine overlay height
	int nlines = MAX(info_.size(), info2_.size());
	if (nlines < 4)
		nlines = 4;
	double scale       = (Drawing::fontSize() / 12.0);
	int    line_height = 16 * scale;
	int    height      = nlines * line_height + 4;

	// Get colours
	ColRGBA col_bg = ColourConfiguration::colour("map_3d_overlay_background");
	ColRGBA col_fg = ColourConfiguration::colour("map_3d_overlay_foreground");
	col_fg.a       = col_fg.a * alpha;
	col_bg.a       = col_bg.a * alpha;
	ColRGBA col_border(0, 0, 0, 140);

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	int   bottom2   = bottom;
	bottom += height * alpha_inv * alpha_inv;

	// Draw overlay background
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Drawing::drawBorderedRect(0, bottom - height - 4, right, bottom + 2, col_bg, col_border);

	// Draw info text lines (left)
	int y = height;
	for (unsigned a = 0; a < info_.size(); a++)
	{
		Drawing::drawText(
			info_[a], middle - (40 * scale) - 4, bottom - y, col_fg, Drawing::Font::Condensed, Drawing::Align::Right);
		y -= line_height;
	}

	// Draw info text lines (right)
	y = height;
	for (unsigned a = 0; a < info2_.size(); a++)
	{
		Drawing::drawText(info2_[a], middle + (40 * scale) + 4, bottom - y, col_fg, Drawing::Font::Condensed);
		y -= line_height;
	}

	// Draw texture if any
	drawTexture(alpha, middle - (40 * scale), bottom);

	// Done
	glEnable(GL_LINE_SMOOTH);
}

// -----------------------------------------------------------------------------
// Draws the item texture/graphic box (if any)
// -----------------------------------------------------------------------------
void InfoOverlay3D::drawTexture(float alpha, int x, int y)
{
	double scale        = (Drawing::fontSize() / 12.0);
	int    tex_box_size = 80 * scale;
	int    line_height  = 16 * scale;

	// Get colours
	ColRGBA col_bg = ColourConfiguration::colour("map_3d_overlay_background");
	ColRGBA col_fg = ColourConfiguration::colour("map_3d_overlay_foreground");
	col_fg.a       = col_fg.a * alpha;

	// Check texture exists
	if (texture_)
	{
		// Draw background
		glEnable(GL_TEXTURE_2D);
		OpenGL::setColour(255, 255, 255, 255 * alpha, 0);
		glPushMatrix();
		glTranslated(x, y - tex_box_size - line_height, 0);
		GLTexture::bgTex().draw2dTiled(tex_box_size, tex_box_size);
		glPopMatrix();

		// Draw texture
		if (texture_ && texture_ != &(GLTexture::missingTex()))
		{
			OpenGL::setColour(255, 255, 255, 255 * alpha, 0);
			Drawing::drawTextureWithin(
				texture_, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0);
		}
		else if (texname_ == "-")
		{
			// Draw missing icon
			GLTexture* icon = MapEditor::textureManager().editorImage("thing/minus");
			glEnable(GL_TEXTURE_2D);
			OpenGL::setColour(180, 0, 0, 255 * alpha, 0);
			Drawing::drawTextureWithin(
				icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.2);
		}
		else if (texname_ != "-" && texture_ == &(GLTexture::missingTex()))
		{
			// Draw unknown icon
			GLTexture* icon = MapEditor::textureManager().editorImage("thing/unknown");
			glEnable(GL_TEXTURE_2D);
			OpenGL::setColour(180, 0, 0, 255 * alpha, 0);
			Drawing::drawTextureWithin(
				icon, x, y - tex_box_size - line_height, x + tex_box_size, y - line_height, 0, 0.2);
		}

		glDisable(GL_TEXTURE_2D);

		// Draw outline
		OpenGL::setColour(col_fg.r, col_fg.g, col_fg.b, 255 * alpha, 0);
		glLineWidth(1.0f);
		glDisable(GL_LINE_SMOOTH);
		Drawing::drawRect(x, y - tex_box_size - line_height, x + tex_box_size, y - line_height);
	}

	// Draw texture name (even if texture is blank)
	if (texname_.Length() > 8)
		texname_ = texname_.Truncate(8) + "...";
	Drawing::drawText(
		texname_, x + (tex_box_size * 0.5), y - line_height, col_fg, Drawing::Font::Condensed, Drawing::Align::Center);
}
