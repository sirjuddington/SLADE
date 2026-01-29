
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
#include "App.h"
#include "Flat3D.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapGeometryBuffer3D.h"
#include "OpenGL/GLTexture.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
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
	const MapSector* sector = nullptr;
	vector<Flat3D>   flats;
	vector<MGVertex> vertices;
	unsigned         vertex_offset       = 0;
	unsigned         sector_vertex_count = 0;
	bool             extrafloor_lighting = false;
};

struct FlatInfo
{
	SurfaceType      surface_type;
	const MapSector* control_sector;
	SurfaceType      control_sector_surface;
	Plane            plane;
	u8               brightness  = 255;
	glm::vec4        colour      = glm::vec4(1.0f);
	bool             extra_floor = false;
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
	auto brightness = static_cast<float>(flat.brightness) / 255.0f;

	if (flat.surface_type == SurfaceType::Ceiling)
	{
		for (auto& vertex : context.sector->polygonVertices())
		{
			context.vertices.emplace_back(
				glm::vec3(vertex, flat.plane.heightAt(vertex)),
				polygon::calculateTexCoords(
					vertex.x, vertex.y, tex_info.size.x, tex_info.size.y, tti.sx, tti.sy, tti.ox, tti.oy, tti.rot),
				brightness);
		}
	}
	else
	{
		for (int i = context.sector->polygonVertices().size(); i > 0; --i) // Floor polygons need to be flipped
		{
			auto& vertex = context.sector->polygonVertices()[i - 1];
			context.vertices.emplace_back(
				glm::vec3(vertex, flat.plane.heightAt(vertex)),
				polygon::calculateTexCoords(
					vertex.x, vertex.y, tex_info.size.x, tex_info.size.y, tti.sx, tti.sy, tti.ox, tti.oy, tti.rot),
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

	Flat3D flat_3d{ .sector        = context.sector,
					.vertex_offset = context.vertex_offset,
					.texture       = texture.gl_id,
					.colour        = flat.colour };

	// Check for sky flat
	if (strutil::equalCI(tex_name, game::configuration().skyFlat()))
		flat_3d.setFlag(Flat3D::Flags::Sky);

	// ExtraFloor
	if (flat.extra_floor)
		flat_3d.setFlag(Flat3D::Flags::ExtraFloor);

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
	flat.colour.a               = extrafloor.alpha;
	flat.extra_floor            = true;
	addFlat(context, flat);

	// Save lighting info in case the LightingInsideOnly flag is set
	u8        brightness_above = flat.brightness;
	glm::vec4 colour_above     = flat.colour;

	// Update lighting if not disabled
	if (!extrafloor.hasFlag(EFFlags::DisableLighting))
	{
		auto& map_specials = context.sector->parentMap()->mapSpecials();
		flat.brightness    = extrafloor.control_sector->lightAt(SectorPart::Interior);
		flat.colour        = map_specials.sectorColour(*extrafloor.control_sector, SectorPart::Interior);
		flat.colour.a      = extrafloor.alpha;

		if (!extrafloor.hasFlag(EFFlags::LightingInsideOnly))
			context.extrafloor_lighting = true;
	}

	// Inner flats
	if (extrafloor.hasFlag(EFFlags::DrawInside) && !extrafloor.hasFlag(EFFlags::FlatAtCeiling))
	{
		// Top
		flat.surface_type           = SurfaceType::Ceiling;
		flat.control_sector_surface = SurfaceType::Ceiling;
		flat.plane                  = extrafloor.plane_top;
		addFlat(context, flat);

		// Bottom
		flat.surface_type           = SurfaceType::Floor;
		flat.control_sector_surface = SurfaceType::Floor;
		flat.plane                  = extrafloor.plane_bottom;
		addFlat(context, flat);

		// Restore lighting from above if LightingInsideOnly flag set
		if (extrafloor.hasFlag(EFFlags::LightingInsideOnly))
		{
			flat.brightness = brightness_above;
			flat.colour     = colour_above;
		}
	}

	// Lastly, bottom outer flat
	flat.surface_type           = SurfaceType::Ceiling;
	flat.control_sector_surface = extrafloor.hasFlag(EFFlags::FlatAtCeiling) ? SurfaceType::Ceiling
																			 : SurfaceType::Floor;
	flat.plane                  = extrafloor.plane_bottom;
	addFlat(context, flat);

	// Restore normal flat state
	flat.colour.a    = 1.0f;
	flat.extra_floor = false;
}

// -----------------------------------------------------------------------------
// Generates 3D flats and vertices for [sector]
// -----------------------------------------------------------------------------
std::tuple<vector<Flat3D>, vector<MGVertex>> generateSectorFlats(const MapSector& sector, unsigned vertex_index)
{
	auto& map_specials = sector.parentMap()->mapSpecials();

	SectorFlatsContext context{ .sector              = &sector,
								.vertex_offset       = vertex_index,
								.sector_vertex_count = static_cast<unsigned>(sector.polygonVertices().size()) };

	// Start with ceiling flat
	FlatInfo info{ .surface_type           = SurfaceType::Ceiling,
				   .control_sector         = &sector,
				   .control_sector_surface = SurfaceType::Ceiling,
				   .plane                  = sector.ceiling().plane,
				   .brightness             = sector.lightAt(SectorPart::Ceiling),
				   .colour                 = map_specials.sectorColour(sector, SectorPart::Ceiling) };
	addFlat(context, info);

	// Then ExtraFloors, from top to bottom, if any
	if (map_specials.sectorHasExtraFloors(&sector))
	{
		// Start with sector interior light/colour
		info.brightness = sector.lightAt(SectorPart::Interior);
		info.colour     = map_specials.sectorColour(sector, SectorPart::Interior);

		for (const auto& extra_floor : map_specials.sectorExtraFloors(&sector))
			generateExtraFloorFlats(context, info, extra_floor);
	}

	// Lastly, the floor flat
	info.surface_type           = SurfaceType::Floor;
	info.control_sector         = &sector;
	info.control_sector_surface = SurfaceType::Floor;
	info.plane                  = sector.floor().plane;
	if (!context.extrafloor_lighting)
	{
		// If no ExtraFloors affected lighting, use floor light/colour
		info.brightness = sector.lightAt(SectorPart::Floor);
		info.colour     = map_specials.sectorColour(sector, SectorPart::Floor);
	}
	addFlat(context, info);

	return { context.flats, context.vertices };
}
} // namespace slade::mapeditor
