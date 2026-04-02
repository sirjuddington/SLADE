
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ThingRenderer3D.cpp
// Description: Handles rendering things in 3D mode
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
#include "ThingRenderer3D.h"
#include "App.h"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "Geometry/Geometry.h"
#include "Geometry/Ray.h"
#include "MapEditor/Item.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapRenderer3D.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer3D.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/MapSpecials/SectorLighting.h"
#include "SLADEMap/SLADEMap.h"
#include "SpriteBuffer3D.h"
#include "Utility/MathStuff.h"
#include "Utility/Vector.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
constexpr int SHOWTHINGS_NONE      = 0;
constexpr int SHOWTHINGS_ALL       = 1;
constexpr int SHOWTHINGS_DECORONLY = 2;
} // namespace
CVAR(Int, map3d_thing_icon_size, 16, CVar::Flag::Save)
CVAR(Int, map3d_things, SHOWTHINGS_ALL, CVar::Flag::Save)
CVAR(Bool, map3d_things_boxes, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
void addThingBoxOutline(
	gl::LineBuffer& buffer,
	const MapThing& thing,
	float           z,
	float           radius,
	float           height,
	float           line_width = 1.0f,
	float           pad        = 0.0f,
	glm::vec4       colour     = glm::vec4(1.0f))
{
	glm::vec3 tl{ thing.xPos() - radius - pad, thing.yPos() - radius - pad, z + height + pad };
	glm::vec3 br{ thing.xPos() + radius + pad, thing.yPos() + radius + pad, z - pad };

	// Top
	buffer.add3d({ tl.x, tl.y, tl.z }, { br.x, tl.y, tl.z }, colour, line_width);
	buffer.add3d({ br.x, tl.y, tl.z }, { br.x, br.y, tl.z }, colour, line_width);
	buffer.add3d({ br.x, br.y, tl.z }, { tl.x, br.y, tl.z }, colour, line_width);
	buffer.add3d({ tl.x, br.y, tl.z }, { tl.x, tl.y, tl.z }, colour, line_width);

	// Bottom
	buffer.add3d({ tl.x, tl.y, br.z }, { br.x, tl.y, br.z }, colour, line_width);
	buffer.add3d({ br.x, tl.y, br.z }, { br.x, br.y, br.z }, colour, line_width);
	buffer.add3d({ br.x, br.y, br.z }, { tl.x, br.y, br.z }, colour, line_width);
	buffer.add3d({ tl.x, br.y, br.z }, { tl.x, tl.y, br.z }, colour, line_width);

	// Sides
	buffer.add3d({ tl.x, tl.y, tl.z }, { tl.x, tl.y, br.z }, colour, line_width);
	buffer.add3d({ br.x, tl.y, tl.z }, { br.x, tl.y, br.z }, colour, line_width);
	buffer.add3d({ br.x, br.y, tl.z }, { br.x, br.y, br.z }, colour, line_width);
	buffer.add3d({ tl.x, br.y, tl.z }, { tl.x, br.y, br.z }, colour, line_width);
}

void addSpriteOutline(
	gl::LineBuffer&   buffer,
	const MapThing&   thing,
	float             z,
	const Vec2f&      sprite_size,
	float             line_width,
	const gl::Camera& camera)
{
	float half_width = sprite_size.x * 0.5f;
	auto  cam_right  = static_cast<Vec3f>(camera.strafeVector()) * half_width;
	auto  colour     = glm::vec4(1.0f);
	buffer.add3d(
		glm::vec3{ thing.xPos(), thing.yPos(), z } - cam_right,
		glm::vec3{ thing.xPos(), thing.yPos(), z } + cam_right,
		colour,
		line_width);
	buffer.add3d(
		glm::vec3{ thing.xPos(), thing.yPos(), z } + cam_right,
		glm::vec3{ thing.xPos(), thing.yPos(), z } + cam_right + Vec3f{ 0.0f, 0.0f, sprite_size.y },
		colour,
		line_width);
	buffer.add3d(
		glm::vec3{ thing.xPos(), thing.yPos(), z } - cam_right,
		glm::vec3{ thing.xPos(), thing.yPos(), z } - cam_right + Vec3f{ 0.0f, 0.0f, sprite_size.y },
		colour,
		line_width);
	buffer.add3d(
		glm::vec3{ thing.xPos(), thing.yPos(), z } - cam_right + Vec3f{ 0.0f, 0.0f, sprite_size.y },
		glm::vec3{ thing.xPos(), thing.yPos(), z } + cam_right + Vec3f{ 0.0f, 0.0f, sprite_size.y },
		colour,
		line_width);
}

void addThingBox(gl::VertexBuffer3D& buffer, const MapThing& thing, float z, float radius, float height, float pad)
{
	glm::vec3 tl{ thing.xPos() - radius - pad, thing.yPos() - radius - pad, z + height + pad };
	glm::vec3 br{ thing.xPos() + radius + pad, thing.yPos() + radius + pad, z - pad };

	buffer.addBox(tl, br);
}

void addThingDirectionArrow(
	gl::LineBuffer& line_buffer,
	const MapThing& thing,
	float           z,
	float           radius,
	float           height,
	float           line_width)
{
	// Direction arrow
	Vec2f dir   = geometry::vectorAngle(geometry::degToRad(thing.angle()));
	auto  start = glm::vec3{ thing.xPos(), thing.yPos(), z + height * 0.5f };
	auto  end   = start + glm::vec3(dir * (radius + 8.0f), 0.0f);
	line_buffer.addArrow3d(start, end, glm::vec4{ 1.0f, 1.0f, 1.0f, 0.75f }, line_width, 8.0f, 30.0f);
}

bool thingsNeedUpdate(long last_updated, const SLADEMap* map)
{
	if (last_updated < map->thingsUpdated() || last_updated < map->sectorRenderInfoUpdated())
		return true;

	return false;
}

bool thingNeedsUpdate(long last_updated, const MapThing* thing, const MapSector* sector)
{
	if (last_updated < thing->modifiedTime())
		return true;

	if (sector && (last_updated < sector->modifiedTime() || last_updated < sector->renderInfoLastUpdated()))
		return true;

	return false;
}
} // namespace


// -----------------------------------------------------------------------------
//
// ThingRenderer3D::ThingGroup Struct Functions
//
// -----------------------------------------------------------------------------


ThingRenderer3D::ThingGroup::ThingGroup(int type_id, const game::ThingType& type_info) : type_info{ &type_info }
{
	// Get thing sprite
	auto tex = textureManager().sprite(type_info.sprite(), type_info.translation(), type_info.palette()).gl_id;

	// If no sprite found, use editor icon
	if (!tex && !type_info.icon().empty())
	{
		tex  = textureManager().editorImage(fmt::format("thing/{}", type_info.icon())).gl_id;
		icon = true;
	}

	if (tex == 0)
		tex = gl::Texture::missingTexture();

	// Create new thing group & buffers
	type          = type_id;
	texture       = tex;
	sprite_buffer = std::make_unique<SpriteBuffer3D>();
	line_buffer   = std::make_unique<gl::LineBuffer>(true);
	if (icon)
	{
		icon        = true;
		sprite_size = { static_cast<float>(map3d_thing_icon_size), static_cast<float>(map3d_thing_icon_size) };
	}
	else
	{
		const auto& tex_info = gl::Texture::info(tex);
		sprite_size          = { static_cast<float>(tex_info.size.x), static_cast<float>(tex_info.size.y) };
	}
}

void ThingRenderer3D::ThingGroup::addThing(const MapThing& thing)
{
	auto map = thing.parentMap();

	float z;
	auto  sector = map->sectors().atPos(thing.position());
	if (sector)
	{
		// Determine z position
		if (type_info->hanging())
			z = sector->ceiling().plane.heightAt(thing.position()) - thing.zPos() - sprite_size.y;
		else
			z = sector->floor().plane.heightAt(thing.position()) + thing.zPos();

		// Center icon if thing has a z height
		if (icon && thing.zPos() != 0)
			z -= sprite_size.y * 0.5f;

		auto light = map->mapSpecials().sectorLightingAt(*sector, map::SectorPart::Interior); // TODO: 3d floor

		if (type_info->fullbright())
			light.brightness = 255;

		sprite_buffer->add(glm::vec3{ thing.xPos(), thing.yPos(), z }, light.brightness / 255.0f, light.colour);
	}
	else
	{
		// Thing is outside the map, use its z position as-is and full brightness
		z = thing.zPos();
		sprite_buffer->add(glm::vec3{ thing.xPos(), thing.yPos(), z }, 1.0f);
	}
	things.push_back({ .index = thing.index(), .z = z, .sector = sector });
}

float ThingRenderer3D::ThingGroup::thingZ(unsigned index) const
{
	for (const auto& ti : things)
		if (ti.index == index)
			return ti.z;

	return 0.0f;
}

const MapSector* ThingRenderer3D::ThingGroup::thingSector(unsigned index) const
{
	for (const auto& ti : things)
		if (ti.index == index)
			return ti.sector;

	return nullptr;
}

float ThingRenderer3D::ThingGroup::height() const
{
	return type_info && type_info->height() >= 0 ? static_cast<float>(type_info->height()) : sprite_size.y;
}


// -----------------------------------------------------------------------------
//
// ThingRenderer3D Class Functions
//
// -----------------------------------------------------------------------------


ThingRenderer3D::ThingRenderer3D(MapRenderer3D* renderer) : renderer_{ renderer } {}

ThingRenderer3D::~ThingRenderer3D() = default;

void ThingRenderer3D::clear()
{
	groups_.clear();
}

void ThingRenderer3D::updateVisibility(const gl::Camera& camera, float max_dist)
{
	if (max_dist == 0.0f)
	{
		thing_visibility_.clear();
		return;
	}

	auto map = renderer_->map();

	if (thing_visibility_.size() != map->things().size())
		thing_visibility_.resize(map->things().size(), 0);

	for (const auto thing : map->things().all())
	{
		if (camera.pointInFrustum2d(thing->position())
			&& glm::distance(camera.position().xy(), thing->position()) <= max_dist)
			thing_visibility_[thing->index()] = 1;
		else
			thing_visibility_[thing->index()] = 0;
	}
}

void ThingRenderer3D::update(bool vis_check)
{
	// Ignore if things are disabled
	if (map3d_things == SHOWTHINGS_NONE)
		return;

	auto map = renderer_->map();

	// Check if we need to rebuild thing groups
	if (vis_check || force_update_groups_ || groups_updated_ < map->geometryUpdated())
	{
		groups_.clear();
		force_update_groups_ = false;
	}

	// (Re)Build all thing groups if none exist
	if (groups_.empty())
	{
		for (const auto thing : map->things().all())
		{
			// Ignore if not visible
			if (vis_check && thing_visibility_[thing->index()] == 0)
				continue;

			const auto& tt = game::configuration().thingType(thing->type());

			// Find thing group for type, create if needed
			auto group = thingGroup(thing->type());
			if (!group)
				group = &groups_.emplace_back(thing->type(), tt);

			// Add thing to group
			group->addThing(*thing);
		}

		// Upload buffers
		for (auto& group : groups_)
			group.sprite_buffer->push();

		groups_updated_ = app::runTimer();
	}

	else if (thingsNeedUpdate(groups_updated_, map))
	{
		vector<ThingGroup*> groups_to_rebuild;

		for (const auto thing : map->things().all())
		{
			// Find the group and sector the thing is in
			const MapSector* sector = nullptr;
			auto             group  = thingGroup(thing->type());
			if (group)
				sector = group->thingSector(thing->index());

			// Check if it needs an update
			if (!thingNeedsUpdate(groups_updated_, thing, sector))
				continue;

			// Create group if needed
			if (!group)
				group = &groups_.emplace_back(thing->type(), game::configuration().thingType(thing->type()));

			// If the thing is in a group with the wrong type, it's type has
			// been changed, so we will need to rebuild both groups
			if (group->type != thing->type())
			{
				auto new_group = thingGroup(thing->type());
				if (!new_group)
					new_group = &groups_.emplace_back(thing->type(), game::configuration().thingType(thing->type()));

				vectorAddUnique(groups_to_rebuild, new_group);
			}

			// Mark group for rebuild
			vectorAddUnique(groups_to_rebuild, group);
		}

		// Rebuild groups that need it
		// TODO: This can probably be optimized by not iterating through *all*
		//       things each time
		for (auto group : groups_to_rebuild)
		{
			group->things.clear();
			group->line_buffer->buffer().clear();

			for (const auto& thing : map->things().all())
				if (thing->type() == group->type)
					group->addThing(*thing);

			group->sprite_buffer->push();
		}

		groups_updated_ = app::runTimer();
	}


	// Rebuild thing groups next frame if vis checking is enabled
	if (vis_check)
		force_update_groups_ = true;
}

void ThingRenderer3D::renderSprites(const gl::Shader& shader) const
{
	if (map3d_things == SHOWTHINGS_NONE)
		return;

	for (const auto& group : groups_)
	{
		if (map3d_things == SHOWTHINGS_DECORONLY && !group.type_info->decoration())
			continue;

		gl::Texture::bind(group.texture);
		shader.setUniform("sprite_size", group.sprite_size);
		group.sprite_buffer->draw();
	}
}

void ThingRenderer3D::renderThingBoxes(const gl::Camera& camera, const gl::View& view, float max_dist) const
{
	if (map3d_things == SHOWTHINGS_NONE)
		return;

	glDisable(GL_CULL_FACE);

	gl::Shader* shader = nullptr;
	for (auto& group : groups_)
	{
		if (map3d_things == SHOWTHINGS_DECORONLY && !group.type_info->decoration())
			continue;

		// Populate line buffer if needed
		auto& line_buffer = *group.line_buffer;
		if (line_buffer.buffer().empty())
		{
			// Get thing size info
			auto radius = group.type_info->radius();
			auto height = group.type_info->height();
			if (height < 0)
				height = group.sprite_size.y;

			// Add group thing outlines
			for (const auto& ti : group.things)
			{
				auto thing = renderer_->map()->thing(ti.index);
				if (!thing)
					continue;

				addThingBoxOutline(
					*group.line_buffer, *thing, ti.z, radius, height, 1.5f, -0.2f, group.type_info->colour());

				addThingDirectionArrow(*group.line_buffer, *thing, ti.z, radius, height, 2.5f);
			}

			line_buffer.push();
		}

		if (!shader)
		{
			shader = line_buffer.shader(true);
			line_buffer.setMaxDistance(max_dist);
			line_buffer.setShaderUniforms(*shader);
			shader->setUniform("mvp", camera.projectionMatrix() * camera.viewMatrix());
			shader->setUniform("viewport_size", view.size());
			shader->setUniform("colour", glm::vec4{ 1.0f });
		}

		gl::bindVAO(line_buffer.vao());
		gl::drawElementsInstanced(gl::Primitive::Triangles, 6, GL_UNSIGNED_SHORT, line_buffer.buffer().size());
	}

	gl::bindVAO(0);
}

void ThingRenderer3D::renderHighlight(
	const Item&       item,
	const gl::Camera& camera,
	const gl::View&   view,
	const glm::vec4&  colour,
	bool              outline,
	bool              fill)
{
	// Get highlight thing
	auto thing = item.asThing(*renderer_->map());
	if (!thing)
		return;

	// Get thing group
	auto group = thingGroup(thing->type());
	if (!group)
		return;

	// Determine if we need to update the highlight buffers
	bool update = prev_highlight_ != item;
	if (!highlight_lines_)
	{
		highlight_lines_ = std::make_unique<gl::LineBuffer>();
		highlight_lines_->setWidthMult(2.0f);
		update = true;
	}

	// Update highlight buffers if needed
	if (update)
	{
		highlight_lines_->buffer().clear();
		auto z      = group->thingZ(thing->index());
		auto radius = group->type_info->radius();
		auto height = group->height();

		// Arrow
		addThingDirectionArrow(*highlight_lines_, *thing, z, radius, height, 1.5f);

		// Box or outline
		if (map3d_things_boxes)
			addThingBoxOutline(*highlight_lines_, *thing, z, radius, height, 1.5f, -0.2f);
		else if (outline)
			addSpriteOutline(*highlight_lines_, *thing, z, group->sprite_size, 1.0f, camera);

		highlight_lines_->push();

		// Fill
		if (fill)
		{
			if (!highlight_fill_)
				highlight_fill_ = std::make_unique<SpriteBuffer3D>();

			highlight_fill_->add({ thing->position().x, thing->position().y, z }, 1.0f);
			highlight_fill_->push();
		}
	}

	// Draw lines
	highlight_lines_->draw(camera, view.size(), colour);

	// Draw fill if needed
	if (fill && highlight_fill_)
	{
		// Setup shader
		auto shader = renderer_->spriteShader();
		shader->bind();
		shader->setUniform("modelview", camera.viewMatrix());
		shader->setUniform("projection", camera.projectionMatrix());
		shader->setUniform("fullbright", true);
		shader->setUniform("fog_density", 0.0f);
		shader->setUniform("colour", glm::vec4{ colour.r, colour.g, colour.b, colour.a });
		shader->setUniform("cam_right", static_cast<Vec3f>(camera.strafeVector()));
		shader->setUniform("sprite_size", group->sprite_size);

		// Setup GL state
		glEnable(GL_DEPTH_TEST);

		// Draw buffer
		gl::setBlend(gl::Blend::Additive);
		gl::Texture::bind(group->texture);
		highlight_fill_->draw();
	}

	// Reset GL state
	gl::setBlend(gl::Blend::Normal);
	glDisable(GL_DEPTH_TEST);

	prev_highlight_ = item;
}

void ThingRenderer3D::addToSelectionOverlay(SelectionOverlay3D& overlay, const Item& item) const
{
	auto thing = item.asThing(*renderer_->map());
	if (!thing)
		return;
	auto group = thingGroup(thing->type());
	if (!group)
		return;

	auto z      = group->thingZ(thing->index());
	auto radius = group->type_info->radius();
	auto height = group->height();

	addThingBoxOutline(*overlay.outline, *thing, z, radius, height, 2.0f, 0.5f);
	addThingBox(*overlay.fill_things, *thing, z, radius, height, 0.5f);
}

optional<Item> ThingRenderer3D::nearestIntersectingThing(const gl::Camera& camera, const Ray& ray, double max_dist)
	const
{
	auto           min_dist = max_dist;
	optional<Item> closest;

	for (const auto& group : groups_)
	{
		// Ignore if not shown
		if (map3d_things == SHOWTHINGS_DECORONLY && !group.type_info->decoration())
			continue;

		auto radius = group.type_info->radius();
		auto height = group.type_info->height();
		if (height < 0)
			height = group.sprite_size.y;

		for (const auto& ti : group.things)
		{
			// Ignore if not visible
			auto thing = renderer_->map()->thing(ti.index);
			if ((!thing_visibility_.empty() && thing_visibility_[thing->index()] == 0)
				|| geometry::lineSide(thing->position(), camera.strafeLine()) > 0)
				continue;

			// If thing boxes are enabled, check for intersection with the box,
			// otherwise we check the billboarded sprite
			if (map3d_things_boxes)
			{
				// Find distance to thing box
				auto tl   = Vec3d(thing->xPos() - radius, thing->yPos() - radius, ti.z + height);
				auto br   = Vec3d(thing->xPos() + radius, thing->yPos() + radius, ti.z);
				auto dist = geometry::distanceRayBBox(ray.origin_3d, ray.dir_3d, tl, br);

				// Ignore if no intersection or something was closer
				if (dist < math::EPSILON || dist >= min_dist)
					continue;

				closest  = Item(ti.index, ItemType::Thing, ti.index);
				min_dist = dist;
			}
			else
			{
				// Find distance to thing sprite
				const auto halfwidth = static_cast<double>(group.sprite_size.x) * 0.5;
				auto       dist      = geometry::distanceRayLine(
                    ray.origin_2d,
                    ray.origin_2d + ray.dir_2d,
                    thing->position() + camera.strafeVector().xy() * halfwidth,
                    thing->position() - camera.strafeVector().xy() * halfwidth);

				// Ignore if no intersection or something was closer
				if (dist < math::EPSILON || dist >= min_dist)
					continue;

				// Check intersection height
				auto height = ray.origin_3d.z + ray.dir_3d.z * dist;
				if (height >= ti.z && height <= ti.z + group.sprite_size.y)
				{
					closest  = Item(ti.index, ItemType::Thing, ti.index);
					min_dist = dist;
				}
			}
		}
	}

	return closest;
}

const ThingRenderer3D::ThingGroup* ThingRenderer3D::thingGroup(unsigned type) const
{
	for (auto& group : groups_)
		if (group.type == type)
			return &group;

	return nullptr;
}

ThingRenderer3D::ThingGroup* ThingRenderer3D::thingGroup(unsigned type)
{
	for (auto& group : groups_)
		if (group.type == type)
			return &group;

	return nullptr;
}
