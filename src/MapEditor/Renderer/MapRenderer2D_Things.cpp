
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer2D_Things.cpp
// Description: MapRenderer2D thing rendering (and related) functions
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
#include "App.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "Geometry/Rect.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapRenderer2D.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/PointSpriteBuffer.h"
#include "OpenGL/VertexBuffer2D.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/SLADEMap.h"
#include "ThingBuffer2D.h"
#include "Utility/Vector.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, thing_shape, 0, CVar::Flag::Save)
CVAR(Bool, thing_sprites, true, CVar::Flag::Save)
CVAR(Bool, thing_force_dir, false, CVar::Flag::Save)
CVAR(Bool, thing_overlay_square, false, CVar::Flag::Save)
CVAR(Bool, thing_preview_lights, true, CVar::Flag::Save)
CVAR(Float, thing_light_intensity, 0.5f, CVar::Flag::Save)
CVAR(Float, thing_shadow, 0.7f, CVar::Flag::Save)
CVAR(Int, halo_width, 4, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, map_animate_hilight)
EXTERN_CVAR(Bool, map_animate_selection)
EXTERN_CVAR(Bool, map_animate_tagged)
EXTERN_CVAR(Bool, action_lines)


// -----------------------------------------------------------------------------
//
// MapRenderer2D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Renders overlays for the given [things], adding [radius_extra] to each radius
// -----------------------------------------------------------------------------
void MapRenderer2D::renderThingOverlays(
	gl::draw2d::Context&     dc,
	const vector<MapThing*>& things,
	float                    radius_extra,
	const Vec2d&             offset) const
{
	// Check if we want square overlays
	if (thing_overlay_square)
	{
		dc.texture = 0;
		dc.colour.a /= 2;

		for (const auto thing : things)
		{
			const auto& tt     = game::configuration().thingType(thing->type());
			float       radius = tt.shrinkOnZoom() ? scaledRadius(tt.radius()) : tt.radius();

			// Draw simple rect
			dc.drawRect(
				{ thing->xPos() + offset.x - radius,
				  thing->yPos() + offset.y - radius,
				  thing->xPos() + offset.x + radius,
				  thing->yPos() + offset.y + radius });
		}

		dc.colour.a *= 2;

		return;
	}

	// Otherwise, setup the thing overlay buffer with values from the dc
	gl::setBlend(dc.blend);
	thing_overlay_buffer_->setColour(dc.colour);
	thing_overlay_buffer_->setFillOpacity(0.25f);
	thing_overlay_buffer_->setOutlineWidth(std::min(3.0f / static_cast<float>(view_->scale().x), 4.0f));
	thing_overlay_buffer_->setPointRadius(dc.pointsprite_radius);

	// Populate thing overlay buffer
	auto hwidth = static_cast<float>(halo_width);
	for (const auto thing : things)
	{
		const auto& tt     = game::configuration().thingType(thing->type());
		float       radius = tt.radius() + hwidth;
		if (tt.shrinkOnZoom())
			radius = scaledRadius(radius);

		thing_overlay_buffer_->add(
			glm::vec2{ thing->xPos() + offset.x, thing->yPos() + offset.y }, radius + radius_extra);
	}
	thing_overlay_buffer_->push();

	// Draw the buffer
	thing_overlay_buffer_->draw(
		thing_shape == 1 ? gl::PointSpriteType::RoundedSquareOutline : gl::PointSpriteType::CircleOutline, dc.view);
}

// -----------------------------------------------------------------------------
// Renders all map things
// -----------------------------------------------------------------------------
void MapRenderer2D::renderThings(float alpha, bool force_dir)
{
	// Don't bother if (practically) invisible
	if (alpha <= 0.01f || map_->nThings() == 0)
		return;

	if (thing_buffers_.empty() || map_->thingsUpdated() > things_updated_)
		updateThingBuffers();

	// Draw thing buffers
	gl::setBlend(gl::Blend::Normal);
	for (auto& buffer : thing_buffers_)
		buffer->draw(view_, glm::vec4{ 1.0f, 1.0f, 1.0f, alpha }, thing_shape == 1, thing_force_dir || force_dir);
}

// -----------------------------------------------------------------------------
// Renders given [things] including an optional [offset]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderThings(const vector<MapThing*>& things, float alpha, const Vec2d& offset) const
{
	gl::setBlend(gl::Blend::Normal);

	// Render given things (by type)
	vector<short> types_rendered;
	unsigned      index = 0;
	while (index < things.size())
	{
		// Ignore if things of this type were already rendered
		auto ttype = things[index]->type();
		if (vectorContains(types_rendered, ttype))
		{
			++index;
			continue;
		}

		// Setup buffer with thing type properties from game configuration
		setupThingBuffer(*temp_things_buffer_, game::configuration().thingType(ttype));

		// Add all subsequent things of same type to buffer
		for (unsigned i = index; i < things.size(); ++i)
			if (things[i]->type() == ttype)
				temp_things_buffer_->add(
					things[i]->xPos() + offset.x, things[i]->yPos() + offset.y, things[i]->angle());
		temp_things_buffer_->push();

		// Render
		temp_things_buffer_->draw(view_, glm::vec4{ 1.0f, 1.0f, 1.0f, alpha }, thing_shape == 1, thing_force_dir);

		// Continue
		types_rendered.push_back(ttype);
		++index;
	}
}

// -----------------------------------------------------------------------------
// Renders the thing hilight overlay for thing [index]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderThingHilight(gl::draw2d::Context& dc, int index, float fade, bool redraw_thing) const
{
	// Check hilight
	auto thing = map_->thing(index);
	if (!thing)
		return;

	// Render the thing again (so it's drawn in front)
	if (redraw_thing)
		renderThings(vector{ thing });

	// Reset fade if hilight animation is disabled
	if (!map_animate_hilight)
		fade = 1.0f;

	// Set hilight colour
	dc.setColourFromConfig("map_hilight", fade);

	// Check if we want square overlays
	if (thing_overlay_square)
	{
		// Get thing info
		auto& tt = game::configuration().thingType(thing->type());
		float x  = thing->xPos();
		float y  = thing->yPos();

		// Get thing radius
		float radius = tt.radius();

		// Draw simple rect+outline
		Rectf rect{ x - radius, y - radius, x + radius, y + radius };
		dc.texture        = 0;
		dc.line_thickness = 3.0f;
		dc.drawRectOutline(rect);
		dc.colour.a /= 2;
		dc.drawRect(rect);

		return;
	}

	// Otherwise draw point sprite overlay
	renderThingOverlays(dc, { map_->thing(index) }); //, /*4.0f * */1.0f + fade);
}

// -----------------------------------------------------------------------------
// Renders the thing selection overlay for thing indices in [selection]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderThingSelection(gl::draw2d::Context& dc, const ItemSelection& selection, float fade) const
{
	// Check anything is selected
	if (selection.empty())
		return;

	// Reset fade if selection animation is disabled
	if (!map_animate_selection)
		fade = 1.0f;

	// Build list of things
	vector<MapThing*> things;
	for (const auto& item : selection)
		if (auto thing = item.asThing(*map_))
			things.push_back(thing);

	// Render overlays
	dc.setColourFromConfig("map_selection", fade);
	renderThingOverlays(dc, things);
}

// -----------------------------------------------------------------------------
// Renders the tagged thing overlay for things in [things]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderTaggedThings(gl::draw2d::Context& dc, const vector<MapThing*>& things, float fade) const
{
	// Reset fade if tagged animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Render overlays
	dc.setColourFromConfig("map_tagged", fade);
	renderThingOverlays(dc, things);

	// Action lines
	auto object = editContext().selection().hilightedObject();
	if (object && action_lines)
	{
		// Build list of lines (arrow: hilighted object -> thing)
		vector<Rectf> r_arrows;
		for (auto thing : things)
			r_arrows.emplace_back(
				object->getPoint(MapObject::Point::Within), thing->getPoint(MapObject::Point::Within));

		// Render action lines
		dc.line_thickness    = /*line_width * */ 1.5f;
		dc.line_arrow_length = 24.0f / dc.view->scale(true).x;
		dc.drawLines(r_arrows);
		dc.line_arrow_length = 0.0f;
	}
}

// -----------------------------------------------------------------------------
// Renders the tagging thing overlay for things in [things]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderTaggingThings(gl::draw2d::Context& dc, const vector<MapThing*>& things, float fade) const
{
	// Reset fade if tagged animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Render overlays
	dc.setColourFromConfig("map_tagged", fade);
	renderThingOverlays(dc, things);

	// Action lines
	auto object = editContext().selection().hilightedObject();
	if (object && action_lines)
	{
		// Build list of lines (arrow: thing -> hilighted object)
		vector<Rectf> r_arrows;
		for (auto thing : things)
			r_arrows.emplace_back(
				thing->getPoint(MapObject::Point::Within), object->getPoint(MapObject::Point::Within));

		// Render action lines
		dc.line_thickness    = /*line_width * */ 1.5f;
		dc.line_arrow_length = 24.0f / dc.view->scale(true).x;
		dc.drawLines(r_arrows);
		dc.line_arrow_length = 0.0f;
	}
}

// -----------------------------------------------------------------------------
// Renders thing pathing lines/arrows for [things]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderPathedThings(gl::draw2d::Context& dc, const vector<MapThing*>& things)
{
	// Skip if action lines are not desired, or if there's nothing to do
	if (!action_lines || things.empty())
		return;

	// Check if paths need updating
	bool update = false;
	if (thing_paths_.empty())
		update = true;
	else if (map_->thingsUpdated() > thing_paths_updated_)
	{
		for (auto& thing : things)
		{
			if (thing->modifiedTime() > thing_paths_updated_)
			{
				update = true;
				break;
			}
		}
		if (!update)
			thing_paths_updated_ = app::runTimer();
	}
	if (update)
		updateThingPaths(things);

	// Build line lists
	vector<Rectf> lines_path;
	vector<Rectf> lines_dragon;
	for (auto& thing_path : thing_paths_)
	{
		if (thing_path.from_index == thing_path.to_index)
			continue;

		auto from = map_->thing(thing_path.from_index);

		if (from && ((from->arg(3) | (from->arg(4) << 8)) > 0))
		{
			auto to = map_->thing(thing_path.to_index);
			if (!to)
				continue;

			if (thing_path.type == PathType::DragonBoth || thing_path.type == PathType::Dragon)
				lines_dragon.emplace_back(from->getPoint(MapObject::Point::Mid), to->getPoint(MapObject::Point::Mid));
			else
				lines_path.emplace_back(from->getPoint(MapObject::Point::Mid), to->getPoint(MapObject::Point::Mid));
		}
	}

	// Draw path lines
	dc.line_thickness    = /*line_width * */ 1.5f;
	dc.line_arrow_length = 24.0f / dc.view->scale(true).x;
	if (!lines_path.empty())
	{
		dc.setColourFromConfig("map_thing_path");
		dc.drawLines(lines_path);
	}

	// Draw dragon path lines
	if (!lines_dragon.empty())
	{
		dc.setColourFromConfig("map_thing_path_dragon");
		dc.drawLines(lines_dragon);
	}

	dc.line_arrow_length = 0.0f;
}

// -----------------------------------------------------------------------------
// Renders point light previews
// -----------------------------------------------------------------------------
void MapRenderer2D::renderPointLightPreviews(gl::draw2d::Context& dc, float alpha, int hilight_index) const
{
	if (!thing_preview_lights)
		return;

	// Build light preview buffer
	glm::vec2 hl_position;
	glm::vec4 hl_colour;
	float     hl_radius = 0.0f;
	for (const auto& thing : map_->things())
	{
		const auto& ttype = game::configuration().thingType(thing->type());

		// Not a point light
		if (ttype.pointLight().empty())
			continue;

		auto light_col    = glm::vec4{ 1.0f };
		auto light_radius = 0.0f;

		// ZDoom point light
		if (ttype.pointLight() == "zdoom")
		{
			light_col.r  = thing->arg(0) / 255.0f;
			light_col.g  = thing->arg(1) / 255.0f;
			light_col.b  = thing->arg(2) / 255.0f;
			light_radius = thing->arg(3);
		}

		// Vavoom point light
		else if (ttype.pointLight() == "vavoom")
		{
			light_col.r  = thing->arg(1) / 255.0f;
			light_col.g  = thing->arg(2) / 255.0f;
			light_col.b  = thing->arg(3) / 255.0f;
			light_radius = thing->arg(0);
		}

		// Vavoom white light
		else if (ttype.pointLight() == "vavoom_white")
		{
			light_radius = thing->arg(0);
		}

		light_radius *= 2; // Doubling the radius value matches better with in-game results

		// Add to buffer
		thing_light_preview_buffer_->addQuadTriangles(
			{ thing->xPos() - light_radius, thing->yPos() - light_radius },
			{ thing->xPos() + light_radius, thing->yPos() + light_radius },
			light_col);

		// Set hilight infor if hilighted
		if (thing->index() == hilight_index)
		{
			hl_colour   = light_col;
			hl_position = glm::vec2{ thing->xPos(), thing->yPos() };
			hl_radius   = light_radius;
		}
	}
	thing_light_preview_buffer_->push();

	// Setup rendering
	const auto& shader = gl::draw2d::defaultShader();
	dc.texture         = textureManager().editorImage("thing/light_preview").gl_id;
	dc.colour.set(255, 255, 255, static_cast<uint8_t>(alpha * (thing_light_intensity * 255.f)));
	dc.blend = gl::Blend::Additive;
	dc.setupToDraw(shader);

	// Draw buffer
	thing_light_preview_buffer_->draw(gl::Primitive::Triangles);

	// Draw hilight ring if needed
	if (hl_radius > 0.0f)
	{
		dc.pointsprite_type          = gl::PointSpriteType::CircleOutline;
		dc.pointsprite_radius        = hl_radius;
		dc.pointsprite_fill_opacity  = 0.0f;
		dc.pointsprite_outline_width = std::min(2.0f / (float)view_->scale().x, 4.0f);
		dc.colour.set(hl_colour.r * 255, hl_colour.g * 255, hl_colour.b * 255, alpha * 255);
		dc.drawPointSprites(vector{ Vec2f{ hl_position.x, hl_position.y } });
		dc.pointsprite_radius = 1.0f;
	}
}

// -----------------------------------------------------------------------------
// (Re)builds the map thing buffers
// -----------------------------------------------------------------------------
void MapRenderer2D::updateThingBuffers()
{
	thing_buffers_.clear();

	std::unordered_map<int, gl::ThingBuffer2D*> buffers;

	for (const auto& thing : map_->things())
	{
		// Create buffer for thing type if needed
		gl::ThingBuffer2D* buffer = buffers[thing->type()];
		if (!buffer)
		{
			thing_buffers_.emplace_back(new gl::ThingBuffer2D());
			buffer                 = thing_buffers_.back().get();
			buffers[thing->type()] = buffer;

			// Setup buffer with thing type properties from game configuration
			setupThingBuffer(*buffer, game::configuration().thingType(thing->type()));
		}

		// Add to buffer
		buffer->add(thing->xPos(), thing->yPos(), thing->angle(), thing->isFiltered() ? 0.25f : 1.0f);
	}

	// Upload buffers
	for (auto& buffer : thing_buffers_)
		buffer->push();

	things_updated_ = app::runTimer();
}

// -----------------------------------------------------------------------------
// Updates the thing paths cache
// -----------------------------------------------------------------------------
void MapRenderer2D::updateThingPaths(const vector<MapThing*>& things)
{
	thing_paths_.clear();

	// Find things that need to be pathed
	for (unsigned a = 0; a < things.size(); ++a)
	{
		auto      thing = things[a];
		ThingPath path;
		path.from_index = 0;
		path.to_index   = 0;

		auto& tt = game::configuration().thingType(thing->type());

		// Dragon Path
		if (tt.flags() & game::ThingType::Flags::Dragon)
		{
			auto first = map_->things().firstWithId(thing->id());
			if (first)
			{
				path.from_index = thing->index();
				path.to_index   = first->index();
				path.type       = PathType::Dragon;
				thing_paths_.push_back(path);

				vector<MapThing*> dragon_things;
				dragon_things.clear();
				map_->putDragonTargets(first, dragon_things);
				for (unsigned d = 0; d < dragon_things.size(); ++d)
				{
					int   id1 = dragon_things[d]->id();
					int   a11 = dragon_things[d]->arg(0);
					int   a12 = dragon_things[d]->arg(1);
					int   a13 = dragon_things[d]->arg(2);
					int   a14 = dragon_things[d]->arg(3);
					int   a15 = dragon_things[d]->arg(4);
					auto& tt1 = game::configuration().thingType(dragon_things[d]->type());
					for (unsigned e = d + 1; e < dragon_things.size(); ++e)
					{
						int   id2   = dragon_things[e]->id();
						int   a21   = dragon_things[e]->arg(0);
						int   a22   = dragon_things[e]->arg(1);
						int   a23   = dragon_things[e]->arg(2);
						int   a24   = dragon_things[e]->arg(3);
						int   a25   = dragon_things[e]->arg(4);
						auto& tt2   = game::configuration().thingType(dragon_things[e]->type());
						bool  l1to2 = ((a11 == id2) || (a12 == id2) || (a13 == id2) || (a14 == id2) || (a15 == id2));
						bool  l2to1 = ((a21 == id1) || (a22 == id1) || (a23 == id1) || (a24 == id1) || (a25 == id1));
						if (!((tt1.flags() | tt2.flags()) & game::ThingType::Flags::Dragon))
						{
							ThingPath dpath;
							if (l1to2)
							{
								dpath.from_index = dragon_things[e]->index();
								dpath.to_index   = dragon_things[d]->index();
								dpath.type       = l2to1 ? PathType::DragonBoth : PathType::Dragon;
							}
							else if (l2to1)
							{
								dpath.from_index = dragon_things[d]->index();
								dpath.to_index   = dragon_things[e]->index();
								dpath.type       = PathType::Dragon;
							}
							thing_paths_.push_back(dpath);
						}
					}
				}
			}
			continue;
		}

		// Normal Path
		int tid = -1, tid2 = -1;
		int nexttype = tt.nextType();
		int nextargs = tt.nextArgs();
		if (nextargs)
		{
			int pos = nextargs % 10;
			tid     = thing->arg(pos - 1);
		}
		if (nextargs >= 10)
		{
			int pos = nextargs / 10;
			tid += (256 * thing->arg(pos - 1));
		}
		for (unsigned b = a + 1; b < things.size(); ++b)
		{
			auto thing2 = things[b];
			if (thing2->type() == nexttype)
			{
				auto& tt2 = game::configuration().thingType(thing2->type());
				nextargs  = tt2.nextArgs();
				if (nextargs)
				{
					int pos = nextargs % 10;
					tid2    = thing2->arg(pos - 1);
				}
				if (nextargs >= 10)
				{
					int pos = nextargs / 10;
					tid2 += (256 * thing2->arg(pos - 1));
				}
				if (thing2->id() == tid)
				{
					path.from_index = thing->index();
					path.to_index   = thing2->index();
					path.type       = (tid2 == thing->id()) ? PathType::NormalBoth : PathType::Normal;
				}
				else if (thing->id() == tid2)
				{
					path.from_index = thing2->index();
					path.to_index   = thing->index();
					path.type       = PathType::Normal;
				}
				thing_paths_.push_back(path);
			}
		}
	}

	thing_paths_updated_ = app::runTimer();
}

// -----------------------------------------------------------------------------
// Sets up the given things [buffer] for drawing things of type [tt]
// -----------------------------------------------------------------------------
void MapRenderer2D::setupThingBuffer(gl::ThingBuffer2D& buffer, const game::ThingType& tt) const
{
	// Determine texture
	auto tex    = 0;
	auto sprite = false;

	// Sprite if we are drawing them
	if (thing_sprites)
	{
		tex    = textureManager().sprite(tt.sprite(), tt.translation(), tt.palette()).gl_id;
		sprite = true;
	}

	// If no sprite found, use editor icon
	if (!tex && !tt.icon().empty())
	{
		tex    = textureManager().editorImage(fmt::format("thing/{}", tt.icon())).gl_id;
		sprite = false;
	}

	// Setup buffer
	buffer.setup(tt);
	buffer.setShadowOpacity(thing_shadow);
	buffer.setTexture(tex, sprite);
}
