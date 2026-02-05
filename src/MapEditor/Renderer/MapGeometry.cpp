
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapGeometry.cpp
// Description: Map geometry calculation and helper functions for the 2D and 3D
//              renderers
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
#include "MapGeometry.h"
#include "Flat3D.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapGeometryBuffer3D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/LineBuffer.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapSpecials/ExtraFloor.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Polygon.h"

using namespace slade;
using namespace mapeditor;

using SurfaceType = map::SectorSurfaceType;
using SectorPart  = map::SectorPart;


namespace
{
struct SectorFlatsContext
{
	const MapSector*  sector       = nullptr;
	map::MapSpecials* map_specials = nullptr;
	vector<Flat3D>    flats;
	vector<MGVertex>  vertices;
	unsigned          vertex_offset       = 0;
	unsigned          sector_vertex_count = 0;
};

struct FlatInfo
{
	SurfaceType         surface_type;
	const MapSector*    control_sector;
	SurfaceType         control_sector_surface;
	Plane               plane;
	map::SectorLighting lighting;
	bool                extra_floor = false;
	float               alpha       = 1.0f;
	bool                additive    = false;
};
} // namespace


namespace slade::mapeditor
{
// -----------------------------------------------------------------------------
// Get texture scaling/offset/rotation info for a sector texture
// -----------------------------------------------------------------------------
TexTransformInfo getSectorTextureTransformInfo(const MapSector& sector, bool ceiling, Vec2d tex_scale)
{
	using namespace game;

	TexTransformInfo info;
	info.sx = tex_scale.x;
	info.sy = tex_scale.y;

	auto& game_config = game::configuration();

	// Get scaling/offset info
	// Check for various UDMF extensions
	if (editContext().mapDesc().format == MapFormat::UDMF)
	{
		// Ceiling
		if (ceiling)
		{
			if (game_config.featureSupported(UDMFFeature::FlatPanning))
			{
				info.ox = sector.floatProperty("xpanningceiling");
				info.oy = sector.floatProperty("ypanningceiling");
			}
			if (game_config.featureSupported(UDMFFeature::FlatScaling))
			{
				info.sx *= (1.0 / sector.floatProperty("xscaleceiling"));
				info.sy *= (1.0 / sector.floatProperty("yscaleceiling"));
			}
			if (game_config.featureSupported(UDMFFeature::FlatRotation))
				info.rot = sector.floatProperty("rotationceiling");
		}
		// Floor
		else
		{
			if (game_config.featureSupported(UDMFFeature::FlatPanning))
			{
				info.ox = sector.floatProperty("xpanningfloor");
				info.oy = sector.floatProperty("ypanningfloor");
			}
			if (game_config.featureSupported(UDMFFeature::FlatScaling))
			{
				info.sx *= (1.0 / sector.floatProperty("xscalefloor"));
				info.sy *= (1.0 / sector.floatProperty("yscalefloor"));
			}
			if (game_config.featureSupported(UDMFFeature::FlatRotation))
				info.rot = sector.floatProperty("rotationfloor");
		}
	}
	// Scaling applies to offsets as well.
	// Note for posterity: worldpanning only applies to textures, not flats
	info.ox /= info.sx;
	info.oy /= info.sy;

	return info;
}

static void addFlatVertices(
	SectorFlatsContext&               context,
	const FlatInfo&                   flat,
	const MapTextureManager::Texture& texture)
{
	auto& tex_info = gl::Texture::info(texture.gl_id);
	auto  tti      = getSectorTextureTransformInfo(
        *flat.control_sector, flat.control_sector_surface == SurfaceType::Ceiling, texture.scale);
	auto brightness = static_cast<float>(flat.lighting.brightness) / 255.0f;

	if (flat.surface_type == SurfaceType::Ceiling)
	{
		for (auto& vertex : context.sector->polygonVertices())
		{
			context.vertices.emplace_back(
				glm::vec3(vertex, flat.plane.heightAt(vertex)),
				polygon::calculateTexCoords(vertex, tex_info.size, tti),
				brightness);
		}
	}
	else
	{
		// Floor polygons need to be flipped
		for (int i = context.sector->polygonVertices().size(); i > 0; --i)
		{
			auto& vertex = context.sector->polygonVertices()[i - 1];
			context.vertices.emplace_back(
				glm::vec3(vertex, flat.plane.heightAt(vertex)),
				polygon::calculateTexCoords(vertex, tex_info.size, tti),
				brightness);
		}
	}

	context.vertex_offset += context.sector_vertex_count;
}

static void addFlat(SectorFlatsContext& context, FlatInfo& flat)
{
	// Get texture & info
	bool  mix_tex_flats = game::configuration().featureSupported(game::Feature::MixTexFlats);
	auto  tex_name      = flat.control_sector_surface == SurfaceType::Ceiling ? flat.control_sector->ceiling().texture
																			  : flat.control_sector->floor().texture;
	auto& texture       = textureManager().flat(tex_name, mix_tex_flats);

	Flat3D flat_3d{ .sector               = context.sector,
					.surface_type         = flat.surface_type,
					.control_sector       = flat.control_sector,
					.control_surface_type = flat.control_sector_surface,
					.vertex_offset        = context.vertex_offset,
					.texture              = texture.gl_id,
					.colour               = flat.lighting.colour };

	// Determine render pass/flags
	if (strutil::equalCI(tex_name, game::configuration().skyFlat())) // Check for sky flat
		flat_3d.render_pass = RenderPass::Sky;
	else if (flat.extra_floor)
	{
		flat_3d.setFlag(Flat3D::Flags::ExtraFloor);
		flat_3d.colour.a *= flat.alpha;

		if (flat.additive)
			flat_3d.setFlag(Flat3D::Flags::Additive);

		// ExtraFloor flats are always masked or transparent
		if (flat.alpha < 1.0f || flat.additive)
			flat_3d.render_pass = RenderPass::Transparent;
		else
			flat_3d.render_pass = RenderPass::Masked;
	}

	// Add flat vertices
	addFlatVertices(context, flat, texture);

	// Add flat
	context.flats.push_back(flat_3d);
}

static void generateExtraFloorFlats(SectorFlatsContext& context, FlatInfo& flat, const map::ExtraFloor& extrafloor)
{
	using EFFlags = map::ExtraFloor::Flags;

	// Top outer flat
	flat.surface_type           = SurfaceType::Floor;
	flat.control_sector         = extrafloor.control_sector;
	flat.control_sector_surface = SurfaceType::Ceiling;
	flat.plane                  = extrafloor.plane_top;
	flat.alpha                  = extrafloor.alpha;
	flat.additive               = extrafloor.hasFlag(map::ExtraFloor::Flags::AdditiveTransparency);
	flat.extra_floor            = true;
	flat.lighting               = context.map_specials->sectorLightingAt(
        *context.sector, SectorPart::Interior, extrafloor.plane_top, false);
	addFlat(context, flat);

	// Inner flats
	if (extrafloor.hasFlag(EFFlags::DrawInside) && !extrafloor.hasFlag(EFFlags::FlatAtCeiling))
	{
		// Top
		flat.surface_type           = SurfaceType::Ceiling;
		flat.control_sector_surface = SurfaceType::Ceiling;
		flat.plane                  = extrafloor.plane_top;
		flat.lighting               = context.map_specials->sectorLightingAt(
            *context.sector, SectorPart::Interior, extrafloor.plane_top, true);
		addFlat(context, flat);

		// Bottom
		flat.surface_type           = SurfaceType::Floor;
		flat.control_sector_surface = SurfaceType::Floor;
		flat.plane                  = extrafloor.plane_bottom;
		flat.lighting               = context.map_specials->sectorLightingAt(
            *context.sector, SectorPart::Interior, extrafloor.plane_bottom, false);
		addFlat(context, flat);
	}

	// Bottom outer flat
	flat.surface_type = SurfaceType::Ceiling;
	if (extrafloor.hasFlag(EFFlags::FlatAtCeiling))
	{
		flat.control_sector_surface = SurfaceType::Ceiling;
		flat.plane                  = extrafloor.plane_top;
	}
	else
	{
		flat.control_sector_surface = SurfaceType::Floor;
		flat.plane                  = extrafloor.plane_bottom;
	}
	flat.lighting = context.map_specials->sectorLightingAt(*context.sector, SectorPart::Interior, flat.plane, true);
	addFlat(context, flat);

	// Restore normal flat state
	flat.alpha       = 1.0f;
	flat.extra_floor = false;
}

// -----------------------------------------------------------------------------
// Generates 3D flats and vertices for [sector]
// -----------------------------------------------------------------------------
std::tuple<vector<Flat3D>, vector<MGVertex>> generateSectorFlats(const MapSector& sector, unsigned vertex_index)
{
	auto& map_specials = sector.parentMap()->mapSpecials();

	SectorFlatsContext context{ .sector              = &sector,
								.map_specials        = &map_specials,
								.vertex_offset       = vertex_index,
								.sector_vertex_count = static_cast<unsigned>(sector.polygonVertices().size()) };

	// Start with ceiling flat
	FlatInfo info{ .surface_type           = SurfaceType::Ceiling,
				   .control_sector         = &sector,
				   .control_sector_surface = SurfaceType::Ceiling,
				   .plane                  = sector.ceiling().plane,
				   .lighting               = map_specials.sectorLightingAt(sector, SectorPart::Ceiling) };
	addFlat(context, info);

	// Then ExtraFloors, from top to bottom, if any
	for (const auto& extra_floor : map_specials.sectorExtraFloors(&sector))
		generateExtraFloorFlats(context, info, extra_floor);

	// Lastly, the floor flat
	info.surface_type           = SurfaceType::Floor;
	info.control_sector         = &sector;
	info.control_sector_surface = SurfaceType::Floor;
	info.plane                  = sector.floor().plane;
	info.lighting               = map_specials.sectorLightingAt(sector, SectorPart::Floor);
	addFlat(context, info);

	return { context.flats, context.vertices };
}
} // namespace slade::mapeditor
