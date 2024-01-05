
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "MapEditor/UI/MapEditorWindow.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/StringUtils.h"

using namespace slade;


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
// Updates the info text for the object of [item_type] at [item_index] in [map]
// -----------------------------------------------------------------------------
void InfoOverlay3D::update(mapeditor::Item item, SLADEMap* map)
{
	using game::Feature;
	using game::UDMFFeature;

	int  item_index = item.index;
	auto item_type  = item.type;

	// Clear current info
	info_.clear();
	info2_.clear();

	// Setup variables
	current_type_   = item_type;
	current_item_   = item;
	texname_        = "";
	texture_        = 0;
	thing_icon_     = false;
	auto map_format = mapeditor::editContext().mapDesc().format;

	// Wall
	if (item_type == mapeditor::ItemType::WallBottom || item_type == mapeditor::ItemType::WallMiddle
		|| item_type == mapeditor::ItemType::WallTop /* || item_type == MapEditor::ItemType::Wall3DFloor*/)
	{
		// Get line and side
		auto side = map->side(item_index);
		if (!side)
			return;
		auto line = side->parentLine();
		if (!line)
			return;
		/*
		MapLine* line3d = nullptr;
		if (item_type == MapEditor::ItemType::Wall3DFloor) {
			line3d = line;
			line = map->getLine(item.control_line);
			side = line->s1();
		}
		*/
		object_ = side;

		// TODO: 3d floors

		// --- Line/side info ---
		if (item.real_index >= 0)
			info_.push_back(fmt::format(
				"3D floor line {} on Line #{}", line->index(), map->side(item.real_index)->parentLine()->index()));
		else
			info_.push_back(fmt::format("Line #{}", line->index()));
		if (side == line->s1())
			info_.push_back(fmt::format("Front Side #{}", side->index()));
		else
			info_.push_back(fmt::format("Back Side #{}", side->index()));

		// Relevant flags
		string flags;
		if (game::configuration().lineBasicFlagSet("dontpegtop", line, map_format))
			flags += "Upper Unpegged, ";
		if (game::configuration().lineBasicFlagSet("dontpegbottom", line, map_format))
			flags += "Lower Unpegged, ";
		if (game::configuration().lineBasicFlagSet("blocking", line, map_format))
			flags += "Blocking, ";
		if (!flags.empty())
			strutil::removeLast(flags, 2);
		info_.push_back(flags);

		info_.push_back(fmt::format("Length: {}", (int)line->length()));

		// Other potential info: special, sector#


		// --- Wall part info ---

		// Part
		if (item_type == mapeditor::ItemType::WallBottom)
			info2_.emplace_back("Lower Texture");
		else if (item_type == mapeditor::ItemType::WallMiddle)
			info2_.emplace_back("Middle Texture");
		else if (item_type == mapeditor::ItemType::WallTop)
			info2_.emplace_back("Upper Texture");
		/*else if (item_type == mapeditor::ItemType::Wall3DFloor)
			info2.push_back("3D Floor Texture"); // TODO: determine*/

		// Offsets
		if (map->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureOffsets))
		{
			// Get x offset info
			int    xoff      = side->texOffsetX();
			double xoff_part = 0;
			if (item_type == mapeditor::ItemType::WallBottom)
				xoff_part = side->floatProperty("offsetx_bottom");
			else if (item_type == mapeditor::ItemType::WallMiddle)
				xoff_part = side->floatProperty("offsetx_mid");
			else
				xoff_part = side->floatProperty("offsetx_top");

			// Add x offset string
			string xoff_info;
			if (xoff_part == 0)
				xoff_info = fmt::format("{}", xoff);
			else if (xoff_part > 0)
				xoff_info = fmt::format("{:1.2f} ({}+{:1.2f})", (double)xoff + xoff_part, xoff, xoff_part);
			else
				xoff_info = fmt::format("{:1.2f} ({}-{:1.2f})", (double)xoff + xoff_part, xoff, -xoff_part);

			// Get y offset info
			int    yoff      = side->texOffsetY();
			double yoff_part = 0;
			if (item_type == mapeditor::ItemType::WallBottom)
				yoff_part = side->floatProperty("offsety_bottom");
			else if (item_type == mapeditor::ItemType::WallMiddle)
				yoff_part = side->floatProperty("offsety_mid");
			else
				yoff_part = side->floatProperty("offsety_top");

			// Add y offset string
			string yoff_info;
			if (yoff_part == 0)
				yoff_info = fmt::format("{}", yoff);
			else if (yoff_part > 0)
				yoff_info = fmt::format("{:1.2f} ({}+{:1.2f})", (double)yoff + yoff_part, yoff, yoff_part);
			else
				yoff_info = fmt::format("{:1.2f} ({}-{:1.2f})", (double)yoff + yoff_part, yoff, -yoff_part);

			info2_.push_back(fmt::format("Offsets: {}, {}", xoff_info, yoff_info));
		}
		else
		{
			// Basic offsets
			info2_.push_back(fmt::format("Offsets: {}, {}", side->texOffsetX(), side->texOffsetY()));
		}

		// UDMF extras
		if (map->currentFormat() == MapFormat::UDMF
			&& game::configuration().featureSupported(UDMFFeature::TextureScaling))
		{
			// Scale
			double xscale, yscale;
			if (item_type == mapeditor::ItemType::WallBottom)
			{
				xscale = side->floatProperty("scalex_bottom");
				yscale = side->floatProperty("scaley_bottom");
			}
			else if (item_type == mapeditor::ItemType::WallMiddle)
			{
				xscale = side->floatProperty("scalex_mid");
				yscale = side->floatProperty("scaley_mid");
			}
			else
			{
				xscale = side->floatProperty("scalex_top");
				yscale = side->floatProperty("scaley_top");
			}
			info2_.push_back(fmt::format("Scale: {:1.2f}x, {:1.2f}x", xscale, yscale));
		}
		else
		{
			info2_.emplace_back("");
		}

		// Height of this section of the wall
		// TODO this is wrong in the case of slopes
		Vec2d    left_point, right_point;
		MapSide* other_side;
		if (side == line->s1())
		{
			left_point  = line->v1()->position();
			right_point = line->v2()->position();
			other_side  = line->s2();
		}
		else
		{
			left_point  = line->v2()->position();
			right_point = line->v1()->position();
			other_side  = line->s1();
		}

		auto       this_sector  = side->sector();
		MapSector* other_sector = nullptr;
		if (other_side)
			other_sector = other_side->sector();

		double left_height, right_height;
		if ((item_type == mapeditor::ItemType::WallMiddle /* || item_type == mapeditor::ItemType::Wall3DFloor*/)
			&& other_sector)
		{
			// A two-sided line's middle area is the smallest distance between
			// both sides' floors and ceilings, which is more complicated with
			// slopes.
			auto floor1   = this_sector->floor().plane;
			auto floor2   = other_sector->floor().plane;
			auto ceiling1 = this_sector->ceiling().plane;
			auto ceiling2 = other_sector->ceiling().plane;
			left_height   = min(ceiling1.heightAt(left_point), ceiling2.heightAt(left_point))
						  - max(floor1.heightAt(left_point), floor2.heightAt(left_point));
			right_height = min(ceiling1.heightAt(right_point), ceiling2.heightAt(right_point))
						   - max(floor1.heightAt(right_point), floor2.heightAt(right_point));
		}
		else
		{
			Plane top_plane, bottom_plane;
			if (item_type == mapeditor::ItemType::WallMiddle /* || item_type == MapEditor::ItemType::Wall3DFloor*/)
			{
				top_plane    = this_sector->ceiling().plane;
				bottom_plane = this_sector->floor().plane;
			}
			else
			{
				if (!other_sector)
					return;
				if (item_type == mapeditor::ItemType::WallTop)
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

			left_height  = top_plane.heightAt(left_point) - bottom_plane.heightAt(left_point);
			right_height = top_plane.heightAt(right_point) - bottom_plane.heightAt(right_point);
		}
		if (fabs(left_height - right_height) < 0.001)
			info2_.push_back(fmt::format("Height: {}", (int)left_height));
		else
			info2_.push_back(fmt::format("Height: {} ~ {}", (int)left_height, (int)right_height));

		// Texture
		if (item_type == mapeditor::ItemType::WallBottom)
			texname_ = side->texLower();
		else if (item_type == mapeditor::ItemType::WallMiddle)
			texname_ = side->texMiddle();
		else if (item_type == mapeditor::ItemType::WallTop)
			texname_ = side->texUpper();
		/*else if (item_type == mapeditor::ItemType::Wall3DFloor)
			texname = side->getTexMiddle(); // TODO: Upper/lower flags*/
		texture_ = mapeditor::textureManager()
					   .texture(texname_, game::configuration().featureSupported(Feature::MixTexFlats))
					   .gl_id;
	}


	// Flat
	else if (item_type == mapeditor::ItemType::Floor || item_type == mapeditor::ItemType::Ceiling)
	{
		bool floor = (item_type == mapeditor::ItemType::Floor);

		// Get sector
		auto real_sector = map->sector(item_index);
		if (!real_sector)
			return;
		object_ = real_sector;

		// For a 3D floor, use the control sector for most properties
		// TODO this is already duplicated elsewhere that examines a highlight; maybe need a map method?
		MapSector* sector = real_sector;

		// Get basic info
		int fheight = sector->floor().height;
		int cheight = sector->ceiling().height;

		// --- Sector info ---

		// Sector index
		if (sector == real_sector)
			info_.push_back(fmt::format("Sector #{}", item_index));
		else
			info_.push_back(fmt::format("3D floor in sector #{}", item_index));

		// Sector height
		info_.push_back(fmt::format("Total Height: {}", cheight - fheight));

		// ZDoom UDMF extras
		/*
		if (game::configuration().udmfNamespace() == "zdoom") {
			// Sector colour
			ColRGBA col = sector->getColour(0, true);
			info.push_back(fmt::format("Colour: R{}, G{}, B{}", col.r, col.g, col.b));
		}
		*/


		// --- Flat info ---

		// Height
		if (floor)
			info2_.push_back(fmt::format("Floor Height: {}", fheight));
		else
			info2_.push_back(fmt::format("Ceiling Height: {}", cheight));

		// Light
		// TODO: this is more complex with 3D floors -- also i would like to not dupe code from the renderer, so maybe
		// this should be MapSpecials's problem?
		int light = sector->lightLevel();
		if (game::configuration().featureSupported(UDMFFeature::FlatLighting))
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
				info2_.push_back(fmt::format("Light: {}", light));
			else if (fl > 0)
				info2_.push_back(fmt::format("Light: {} ({}+{})", light + fl, light, fl));
			else
				info2_.push_back(fmt::format("Light: {} ({}-{})", light + fl, light, -fl));
		}
		else
			info2_.push_back(fmt::format("Light: {}", light));

		// UDMF extras
		if (mapeditor::editContext().mapDesc().format == MapFormat::UDMF)
		{
			// Offsets
			double xoff, yoff;
			xoff = yoff = 0.0;
			if (game::configuration().featureSupported(UDMFFeature::FlatPanning))
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
			info2_.push_back(fmt::format("Offsets: {:1.2f}, {:1.2f}", xoff, yoff));

			// Scaling
			double xscale, yscale;
			xscale = yscale = 1.0;
			if (game::configuration().featureSupported(UDMFFeature::FlatScaling))
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
			info2_.push_back(fmt::format("Scale: {:1.2f}x, {:1.2f}x", xscale, yscale));
		}

		// Texture
		if (floor)
			texname_ = sector->floor().texture;
		else
			texname_ = sector->ceiling().texture;
		texture_ = mapeditor::textureManager()
					   .flat(texname_, game::configuration().featureSupported(Feature::MixTexFlats))
					   .gl_id;
	}

	// Thing
	else if (item_type == mapeditor::ItemType::Thing)
	{
		// index, type, position, sector, zpos, height?, radius?

		// Get thing
		auto thing = map->thing(item_index);
		if (!thing)
			return;
		object_ = thing;

		// Index
		info_.push_back(fmt::format("Thing #{}", item_index));

		// Position
		if (mapeditor::editContext().mapDesc().format == MapFormat::Hexen
			|| mapeditor::editContext().mapDesc().format == MapFormat::UDMF)
			info_.push_back(
				fmt::format("Position: {}, {}, {}", (int)thing->xPos(), (int)thing->yPos(), (int)thing->zPos()));
		else
			info_.push_back(fmt::format("Position: {}, {}", (int)thing->xPos(), (int)thing->yPos()));


		// Type
		auto& tt = game::configuration().thingType(thing->type());
		if (!tt.defined())
			info2_.push_back(fmt::format("Type: {}", thing->type()));
		else
			info2_.push_back(fmt::format("Type: {}", tt.name()));

		// Args
		if (mapeditor::editContext().mapDesc().format == MapFormat::Hexen
			|| (mapeditor::editContext().mapDesc().format == MapFormat::UDMF
				&& game::configuration().getUDMFProperty("arg0", MapObject::Type::Thing)))
		{
			// Get thing args
			string argxstr[2];
			argxstr[0]  = thing->stringProperty("arg0str");
			argxstr[1]  = thing->stringProperty("arg1str");
			auto argstr = tt.argSpec().stringDesc(thing->args().data(), argxstr);

			if (argstr.empty())
				info2_.emplace_back("No Args");
			else
				info2_.emplace_back(argstr);
		}

		// Sector
		auto sector = map->sectors().atPos(thing->position());
		if (sector)
			info2_.emplace_back(fmt::format("In Sector #{}", sector->index()));
		else
			info2_.emplace_back("No Sector");


		// Texture
		texture_ = mapeditor::textureManager().sprite(tt.sprite(), tt.translation(), tt.palette()).gl_id;
		if (!texture_)
		{
			if (use_zeth_icons && tt.zethIcon() >= 0)
				texture_ = mapeditor::textureManager()
							   .editorImage(fmt::format("zethicons/zeth{:02d}", tt.zethIcon()))
							   .gl_id;
			if (!texture_)
				texture_ = mapeditor::textureManager().editorImage(fmt::format("thing/{}", tt.icon())).gl_id;
			thing_icon_ = true;
		}
		texname_ = "";
	}

	last_update_ = app::runTimer();
}

// -----------------------------------------------------------------------------
// Draws the overlay
// -----------------------------------------------------------------------------
void InfoOverlay3D::draw(gl::draw2d::Context& dc, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Don't bother if no info
	if (info_.empty())
		return;

	// Update if needed
	if (object_
		&& (object_->modifiedTime() > last_update_ || // object_ updated
			(object_->objType() == MapObject::Type::Side
			 && (dynamic_cast<MapSide*>(object_)->parentLine()->modifiedTime() > last_update_ || // parent line updated
				 dynamic_cast<MapSide*>(object_)->sector()->modifiedTime() > last_update_)))) // parent sector updated
	{
		mapeditor::Item newitem{ static_cast<int>(object_->index()), current_type_ };
		newitem.real_index = current_item_.real_index;
		update(newitem, object_->parentMap());
	}

	// Determine overlay height
	int nlines = std::max<int>(info_.size(), info2_.size());
	if (nlines < 4)
		nlines = 4;
	auto scale       = dc.textScale();
	auto line_height = dc.textLineHeight();
	auto height      = nlines * line_height + 4.0f;

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	auto  bottom    = dc.viewSize().y + height * alpha_inv * alpha_inv;

	// Draw overlay background
	dc.setColourFromConfig("map_3d_overlay_background", alpha);
	dc.drawRect({ 0.0f, bottom - height, dc.viewSize().x, bottom });

	// Draw info text lines (left)
	int  y            = height;
	auto middle       = dc.viewSize().x / 2;
	dc.font           = gl::draw2d::Font::Condensed;
	dc.text_alignment = gl::draw2d::Align::Right;
	dc.setColourFromConfig("map_3d_overlay_foreground", alpha);
	for (const auto& text : info_)
	{
		dc.drawText(text, { middle - (40 * scale) - 4, bottom - y });
		y -= line_height;
	}

	// Draw info text lines (right)
	y                 = height;
	dc.text_alignment = gl::draw2d::Align::Left;
	for (const auto& text : info2_)
	{
		dc.drawText(text, { middle + (40 * scale) + 4, bottom - y });
		y -= line_height;
	}

	// Draw texture if any
	drawTexture(dc, alpha, middle - (40 * scale), bottom);
}

// -----------------------------------------------------------------------------
// Draws the item texture/graphic box (if any)
// -----------------------------------------------------------------------------
void InfoOverlay3D::drawTexture(gl::draw2d::Context& dc, float alpha, float x, float y) const
{
	auto scale        = dc.textScale();
	auto tex_box_size = 80 * scale;
	auto line_height  = dc.textLineHeight();

	// Check texture exists
	if (texture_)
	{
		// Valid texture
		if (texture_ && texture_ != gl::Texture::missingTexture())
		{
			// Draw background
			dc.texture = gl::Texture::backgroundTexture();
			dc.colour.set(255, 255, 255, 255 * alpha);
			dc.drawTextureTiled({ x, y - tex_box_size - line_height, tex_box_size, tex_box_size, false });

			// Draw texture
			dc.texture = texture_;
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

		// Missing texture
		else if (texname_ == "-")
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

		// Unknown texture
		else if (texname_ != "-" && texture_ == gl::Texture::missingTexture())
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
	}

	// Draw texture name (even if texture is blank)
	auto tn_truncated = texname_;
	if (tn_truncated.size() > 8)
	{
		strutil::truncateIP(tn_truncated, 8);
		tn_truncated.append("...");
	}
	dc.drawText(tn_truncated, { x + (tex_box_size * 0.5f), y - line_height });
}
