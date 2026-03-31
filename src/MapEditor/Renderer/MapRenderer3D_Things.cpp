
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer3D_Things.cpp
// Description:
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
#include "Geometry/Geometry.h"
#include "MapEditor/Item.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapRenderer3D.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer3D.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapObjectList/ThingList.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/MapSpecials/SectorLighting.h"
#include "SLADEMap/SLADEMap.h"
#include "SpriteBuffer3D.h"
#include <memory>

using namespace slade;
using namespace mapeditor;



CVAR(Int, map3d_thing_icon_size, 16, CVar::Flag::Save)



namespace
{
void addThingBoxOutline(
	const MapThing& thing,
	float           z,
	float           radius,
	float           height,
	gl::LineBuffer& buffer,
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
} // namespace



MapRenderer3D::ThingGroup::ThingGroup(int type_id, const game::ThingType& type_info) : type_info{ &type_info }
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

	// Create new thing group & buffer
	type           = type_id;
	texture        = tex;
	sprite_buffer_ = std::make_unique<SpriteBuffer3D>();
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

void MapRenderer3D::ThingGroup::addThing(const MapThing& thing)
{
	auto map = thing.parentMap();

	float z;
	if (auto sector = map->sectors().atPos(thing.position()))
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

		sprite_buffer_->add(glm::vec3{ thing.xPos(), thing.yPos(), z }, light.brightness / 255.0f, light.colour);
	}
	else
	{
		// Thing is outside the map, use its z position as-is and full brightness
		z = thing.zPos();
		sprite_buffer_->add(glm::vec3{ thing.xPos(), thing.yPos(), z }, 1.0f);
	}
	things.push_back({ .index = thing.index(), .z = z });
}


void MapRenderer3D::updateThingVisibility(const gl::Camera& camera, float max_dist)
{
	if (thing_visibility_.size() != map_->things().size())
		thing_visibility_.resize(map_->things().size(), 0);

	for (const auto thing : map_->things().all())
	{
		if (camera.pointInFrustum2d(thing->position())
			&& glm::distance(camera.position().xy(), thing->position()) <= max_dist)
			thing_visibility_[thing->index()] = 1;
		else
			thing_visibility_[thing->index()] = 0;
	}
}

void MapRenderer3D::updateThings(bool vis_check)
{
	// Check if we need to rebuild or update thing groups
	if (vis_check || force_update_thing_groups_)
	{
		thing_groups_.clear();
		thing_box_line_buffer_.reset();
		thing_arrow_line_buffer_.reset();
		force_update_thing_groups_ = false;
	}

	// TODO: Updates
	if (!thing_groups_.empty())
		return;

	for (const auto thing : map_->things().all())
	{
		// Ignore if not visible
		if (vis_check && thing_visibility_[thing->index()] == 0)
			continue;

		const auto& tt = game::configuration().thingType(thing->type());

		// Find thing group for type
		ThingGroup* group = nullptr;
		for (auto& g : thing_groups_)
		{
			if (g.type == thing->type())
			{
				group = &g;
				break;
			}
		}

		// Create thing group if needed
		if (!group)
			group = &thing_groups_.emplace_back(thing->type(), tt);

		// Add thing to group
		group->addThing(*thing);
	}

	// Upload buffers
	for (auto& group : thing_groups_)
		group.sprite_buffer_->push();

	things_updated_ = app::runTimer();

	// Rebuild thing groups next frame if vis checking is enabled
	if (vis_check)
		force_update_thing_groups_ = true;
}

void MapRenderer3D::renderSprites(const gl::Shader& shader, bool decorations_only) const
{
	for (const auto& group : thing_groups_)
	{
		if (decorations_only && !group.type_info->decoration())
			continue;

		gl::Texture::bind(group.texture);
		shader.setUniform("sprite_size", group.sprite_size);
		group.sprite_buffer_->draw();
	}
}

void MapRenderer3D::renderThingBoxes(
	const gl::Camera& camera,
	const gl::View&   view,
	bool              decorations_only,
	float             max_dist)
{
	if (!thing_box_line_buffer_)
		thing_box_line_buffer_ = std::make_unique<gl::LineBuffer>(true);
	if (!thing_arrow_line_buffer_)
		thing_arrow_line_buffer_ = std::make_unique<gl::LineBuffer>(true);

	for (auto& group : thing_groups_)
	{
		if (decorations_only && !group.type_info->decoration())
			continue;

		// Populate line buffer if needed
		if (group.box_buffer_offset == 1)
		{
			// Get thing size info
			auto radius = group.type_info->radius();
			auto height = group.type_info->height();
			if (height < 0)
				height = group.sprite_size.y;

			// Add group thing outlines
			group.box_buffer_offset   = thing_box_line_buffer_->queueSize();
			group.arrow_buffer_offset = thing_arrow_line_buffer_->queueSize();
			for (const auto& ti : group.things)
			{
				auto thing = map_->things().at(ti.index);
				if (!thing)
					continue;

				::addThingBoxOutline(
					*thing, ti.z, radius, height, *thing_box_line_buffer_, 1.5f, -0.2f, group.type_info->colour());

				// Direction arrow
				Vec2f dir   = geometry::vectorAngle(geometry::degToRad(thing->angle()));
				auto  start = glm::vec3{ thing->xPos(), thing->yPos(), ti.z + height * 0.5f };
				auto  end   = start + glm::vec3(dir * (radius + 8.0f), 0.0f);
				thing_arrow_line_buffer_->addArrow3d(
					start, end, glm::vec4{ 1.0f, 1.0f, 1.0f, 0.75f }, 2.5f, 8.0f, 30.0f);
			}
		}
	}

	// Push buffers if needed
	if (thing_box_line_buffer_->buffer().empty())
		thing_box_line_buffer_->push();
	if (thing_arrow_line_buffer_->buffer().empty())
		thing_arrow_line_buffer_->push();

	// Draw boxes
	thing_box_line_buffer_->setMaxDistance(max_dist);
	thing_box_line_buffer_->draw(camera, view.size());

	// Draw arrows
	thing_arrow_line_buffer_->setMaxDistance(max_dist);
	thing_arrow_line_buffer_->draw(camera, view.size());
}

void MapRenderer3D::addSpriteOutline(
	const Item&       item,
	gl::LineBuffer&   buffer,
	float             line_width,
	const gl::Camera& camera) const
{
	auto thing = item.asThing(*map_);

	for (const auto& group : thing_groups_)
	{
		if (thing->type() != group.type)
			continue;

		// Get z height
		float z = thing->zPos();
		for (const auto& ti : group.things)
		{
			if (ti.index == item.index)
			{
				z = ti.z;
				break;
			}
		}

		// Add outline
		float half_width = group.sprite_size.x * 0.5f;
		auto  cam_right  = static_cast<Vec3f>(camera.strafeVector()) * half_width;
		auto  colour     = glm::vec4(1.0f);
		buffer.add3d(
			glm::vec3{ thing->xPos(), thing->yPos(), z } - cam_right,
			glm::vec3{ thing->xPos(), thing->yPos(), z } + cam_right,
			colour,
			line_width);
		buffer.add3d(
			glm::vec3{ thing->xPos(), thing->yPos(), z } + cam_right,
			glm::vec3{ thing->xPos(), thing->yPos(), z } + cam_right + Vec3f{ 0.0f, 0.0f, group.sprite_size.y },
			colour,
			line_width);
		buffer.add3d(
			glm::vec3{ thing->xPos(), thing->yPos(), z } - cam_right,
			glm::vec3{ thing->xPos(), thing->yPos(), z } - cam_right + Vec3f{ 0.0f, 0.0f, group.sprite_size.y },
			colour,
			line_width);
		buffer.add3d(
			glm::vec3{ thing->xPos(), thing->yPos(), z } - cam_right + Vec3f{ 0.0f, 0.0f, group.sprite_size.y },
			glm::vec3{ thing->xPos(), thing->yPos(), z } + cam_right + Vec3f{ 0.0f, 0.0f, group.sprite_size.y },
			colour,
			line_width);

		return;
	}
}

void MapRenderer3D::addThingBoxOutline(const Item& item, gl::LineBuffer& buffer, float line_width, float pad) const
{
	auto thing = item.asThing(*map_);

	for (const auto& group : thing_groups_)
	{
		if (thing->type() != group.type)
			continue;

		// Get thing size info
		auto radius = group.type_info->radius();
		auto height = group.type_info->height();
		if (height < 0)
			height = group.sprite_size.y;

		// Get z height
		float z = thing->zPos();
		for (const auto& ti : group.things)
		{
			if (ti.index == item.index)
			{
				z = ti.z;
				break;
			}
		}

		::addThingBoxOutline(*thing, z, radius, height, buffer, line_width, pad);

		return;
	}
}

void MapRenderer3D::addThingBox(const Item& item, gl::VertexBuffer3D& buffer, float pad) const
{
	auto thing = item.asThing(*map_);

	for (const auto& group : thing_groups_)
	{
		if (thing->type() != group.type)
			continue;

		// Get z height
		float z = thing->zPos();
		for (const auto& ti : group.things)
		{
			if (ti.index == item.index)
			{
				z = ti.z;
				break;
			}
		}

		// Get thing size info
		auto radius = group.type_info->radius();
		auto height = group.type_info->height();
		if (height < 0)
			height = group.sprite_size.y;

		glm::vec3 tl{ thing->xPos() - radius - pad, thing->yPos() - radius - pad, z + height + pad };
		glm::vec3 br{ thing->xPos() + radius + pad, thing->yPos() + radius + pad, z - pad };

		buffer.addBox(tl, br);

		return;
	}
}
