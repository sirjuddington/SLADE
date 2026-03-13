
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
#include "Game/Configuration.h"
#include "Game/ThingType.h"
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

using namespace slade;
using namespace mapeditor;



CVAR(Int, map3d_thing_icon_size, 16, CVar::Flag::Save)



void MapRenderer3D::updateThings(bool vis_check)
{
	// TODO: Updates
	if (!thing_groups_.empty())
		return;

	for (const auto thing : map_->things().all())
	{
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
		{
			unsigned tex  = 0;
			bool     icon = false;

			// Get thing sprite
			tex = textureManager().sprite(tt.sprite(), tt.translation(), tt.palette()).gl_id;

			// If no sprite found, use editor icon
			if (!tex && !tt.icon().empty())
			{
				tex  = textureManager().editorImage(fmt::format("thing/{}", tt.icon())).gl_id;
				icon = true;
			}

			if (tex == 0)
				continue; // Shouldn't happen, but just in case

			// Create new thing group & buffer
			group                 = &thing_groups_.emplace_back();
			group->type           = thing->type();
			group->texture        = tex;
			group->decoration     = tt.decoration();
			group->sprite_buffer_ = std::make_unique<SpriteBuffer3D>();
			if (icon)
			{
				group->icon        = true;
				group->sprite_size = { static_cast<float>(map3d_thing_icon_size),
									   static_cast<float>(map3d_thing_icon_size) };
			}
			else
			{
				const auto& tex_info = gl::Texture::info(tex);
				group->sprite_size   = { static_cast<float>(tex_info.size.x), static_cast<float>(tex_info.size.y) };
			}
		}

		// Add thing to group
		float z;
		if (auto sector = map_->sectors().atPos(thing->position()))
		{
			// Determine z position
			if (tt.hanging())
				z = sector->ceiling().plane.heightAt(thing->position()) - thing->zPos() - group->sprite_size.y;
			else
				z = sector->floor().plane.heightAt(thing->position()) + thing->zPos();

			// Center icon if thing has a z height
			if (group->icon && thing->zPos() != 0)
				z -= group->sprite_size.y * 0.5f;

			auto light = map_->mapSpecials().sectorLightingAt(*sector, map::SectorPart::Interior); // TODO: 3d floor
			group->sprite_buffer_->add(
				glm::vec3{ thing->xPos(), thing->yPos(), z }, light.brightness / 255.0f, light.colour);
		}
		else
		{
			// Thing is outside the map, use its z position as-is and full brightness
			z = thing->zPos();
			group->sprite_buffer_->add(glm::vec3{ thing->xPos(), thing->yPos(), z }, 1.0f);
		}
		group->things.push_back({ .index = thing->index(), .z = z });
	}

	// Upload buffers
	for (auto& group : thing_groups_)
		group.sprite_buffer_->push();
}

void MapRenderer3D::renderSprites(const gl::Shader& shader, bool decorations_only) const
{
	for (const auto& group : thing_groups_)
	{
		if (decorations_only && !group.decoration)
			continue;

		gl::Texture::bind(group.texture);
		shader.setUniform("sprite_size", group.sprite_size);
		group.sprite_buffer_->draw();
	}
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

void MapRenderer3D::addThingBoxOutline(const Item& item, gl::LineBuffer& buffer, float line_width) const
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

		float half_width = group.sprite_size.x * 0.5f;
		float height     = group.sprite_size.y;
		auto  colour     = glm::vec4(1.0f);

		glm::vec3 tl{ thing->xPos() - half_width, thing->yPos() - half_width, z + height };
		glm::vec3 br{ thing->xPos() + half_width, thing->yPos() + half_width, z };

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

		return;
	}
}

void MapRenderer3D::addThingBox(const Item& item, gl::VertexBuffer3D& buffer) const
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

		float half_width = group.sprite_size.x * 0.5f;
		float height     = group.sprite_size.y;
		auto  colour     = glm::vec4(1.0f);

		glm::vec3 tl{ thing->xPos() - half_width, thing->yPos() - half_width, z + height };
		glm::vec3 br{ thing->xPos() + half_width, thing->yPos() + half_width, z };

		buffer.addBox(tl, br);

		return;
	}
}