
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ThingInfoOverlay.cpp
// Description: ThingInfoOverlay class - a map editor overlay that displays
//              information about the currently highlighted thing in things mode
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
#include "ThingInfoOverlay.h"
#include "Game/ActionSpecial.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "General/ColourConfiguration.h"
#include "Geometry/Rect.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/OpenGL.h"
#include "SLADEMap/MapObject/MapThing.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, use_zeth_icons)


// -----------------------------------------------------------------------------
//
// ThingInfoOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ThingInfoOverlay class constructor
// -----------------------------------------------------------------------------
ThingInfoOverlay::ThingInfoOverlay()
{
	text_box_ = std::make_unique<gl::draw2d::TextBox>("", 100, gl::draw2d::Font::Condensed);
}

// -----------------------------------------------------------------------------
// ThingInfoOverlay class destructor
// -----------------------------------------------------------------------------
ThingInfoOverlay::~ThingInfoOverlay() = default;

// -----------------------------------------------------------------------------
// Updates the overlay with info from [thing]
// -----------------------------------------------------------------------------
void ThingInfoOverlay::update(MapThing* thing)
{
	if (!thing)
		return;

	string info_text;
	sprite_         = "";
	translation_    = "";
	palette_        = "";
	icon_           = "";
	auto map_format = mapeditor::editContext().mapDesc().format;

	// Index + type
	auto& tt   = game::configuration().thingType(thing->type());
	auto  type = fmt::format("{} (Type {})", tt.name(), thing->type());
	if (global::debug)
		info_text += fmt::format("Thing #{} ({}): {}\n", thing->index(), thing->objId(), type);
	else
		info_text += fmt::format("Thing #{}: {}\n", thing->index(), type);

	// Position
	if (map_format != MapFormat::Doom)
		info_text += fmt::format(
			"Position: {}, {}, {}\n",
			static_cast<int>(thing->xPos()),
			static_cast<int>(thing->yPos()),
			static_cast<int>(thing->zPos()));
	else
		info_text += fmt::format(
			"Position: {}, {}\n", static_cast<int>(thing->xPos()), static_cast<int>(thing->yPos()));

	// Direction
	int  angle = thing->angle();
	auto dir   = fmt::format("{} degrees", angle);
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
	info_text += fmt::format("Direction: {}\n", dir);

	// Special and Args (if in hexen format or udmf with thing args)
	if (map_format == MapFormat::Hexen
		|| (map_format == MapFormat::UDMF && game::configuration().getUDMFProperty("arg0", MapObject::Type::Thing)))
	{
		int as_id = thing->special();
		info_text += fmt::format("Special: {} ({})\n", as_id, game::configuration().actionSpecialName(as_id));
		string argxstr[2];
		argxstr[0] = thing->stringProperty("arg0str");
		argxstr[1] = thing->stringProperty("arg1str");
		string argstr;
		if (tt.argSpec().count > 0)
			argstr = tt.argSpec().stringDesc(thing->args().data(), argxstr);
		else
			argstr = game::configuration().actionSpecial(as_id).argSpec().stringDesc(thing->args().data(), argxstr);

		if (!argstr.empty())
			info_text += fmt::format("{}\n", argstr);
		else
			info_text += "No Args\n";
	}

	// Flags
	if (map_format != MapFormat::UDMF)
		info_text += fmt::format("Flags: {}\n", game::configuration().thingFlagsString(thing->flags()));

	// TID (if in doom64/hexen/udmf format)
	if (map_format != MapFormat::Doom)
		info_text += fmt::format("TID: {}", thing->id());

	if (info_text.back() == '\n')
		info_text.pop_back();

	// Set sprite and translation
	sprite_      = tt.sprite();
	translation_ = tt.translation();
	palette_     = tt.palette();
	icon_        = tt.icon();
	zeth_icon_   = tt.zethIcon();

	// Setup text box
	text_box_->setText(info_text);
}

// -----------------------------------------------------------------------------
// Draws the overlay at [bottom] from 0 to [right]
// -----------------------------------------------------------------------------
void ThingInfoOverlay::draw(gl::draw2d::Context& dc, float alpha)
{
	// Don't bother if invisible
	if (alpha <= 0.0f)
		return;

	// Determine overlay height
	auto right = dc.viewSize().x;
	if (last_size_ != right)
	{
		last_size_ = right;
		text_box_->setWidth(right - 68.0f);
	}
	text_box_->setFont(dc.font, dc.text_size);
	auto height = text_box_->height() + 8.0f;

	// Slide in/out animation
	float alpha_inv = 1.0f - alpha;
	auto  bottom    = dc.viewSize().y + height * alpha_inv * alpha_inv;

	// Draw overlay background
	dc.setColourFromConfig("map_overlay_background");
	dc.colour.a *= alpha;
	dc.drawRect({ 0.0f, bottom - height, right, bottom });

	// Draw info text lines
	dc.setColourFromConfig("map_overlay_foreground");
	dc.colour.a *= alpha;
	text_box_->draw({ 4.0f, bottom - height + 4.0f }, dc);

	// Draw sprite
	bool isicon = false;
	auto tex    = mapeditor::textureManager().sprite(sprite_, translation_, palette_).gl_id;
	if (!tex)
	{
		if (use_zeth_icons && zeth_icon_ >= 0)
			tex = mapeditor::textureManager().editorImage(fmt::format("zethicons/zeth{:02d}", zeth_icon_)).gl_id;
		if (!tex)
			tex = mapeditor::textureManager().editorImage(fmt::format("thing/{}", icon_)).gl_id;
		isicon = true;
	}
	dc.colour.set(255, 255, 255, 255 * alpha);
	dc.blend = gl::Blend::Normal;
	if (tex)
	{
		auto& tex_info = gl::Texture::info(tex);
		float twidth   = tex_info.size.x;
		float theight  = tex_info.size.y;
		if (twidth > 128.0 || theight > 128.0)
		{
			float factor = glm::max(twidth, theight) / 128.0;
			twidth /= factor;
			theight /= factor;
		}
		if (isicon)
		{
			twidth  = 64;
			theight = 64;
		}
		dc.texture = tex;
		dc.drawRect({ right - 8.0f - twidth, bottom - 8.0f - theight, right - 8.0f, bottom - 8.0f });
	}

	// Done
	dc.texture = 0;
}
