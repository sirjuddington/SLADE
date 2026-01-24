
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
#include "OpenGL/GLTexture.h"
#include "OpenGL/VertexBuffer3D.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapSpecials/ExtraFloor.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Polygon.h"

using namespace slade;
using namespace mapeditor;


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
	if (mapeditor::editContext().mapDesc().format == MapFormat::UDMF)
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

static void generateFlatVertices(
	const Flat3D&                     flat,
	const MapTextureManager::Texture& tex,
	vector<gl::Vertex3D>&             vertices)
{
	auto& sector         = *flat.sector;
	auto& control_sector = *flat.controlSector();
	auto& tex_info       = gl::Texture::info(flat.texture);
	auto  ttf = getSectorTextureTransformInfo(control_sector, flat.source_tex == SectorSurfaceType::Ceiling, tex.scale);
	auto& plane = (flat.source_surface == SectorSurfaceType::Ceiling) ? control_sector.ceiling().plane
																	  : control_sector.floor().plane;

	if (flat.surface_type == SectorSurfaceType::Ceiling)
	{
		for (auto& vertex : sector.polygonVertices())
		{
			vertices.emplace_back(
				glm::vec3(vertex, plane.heightAt(vertex)),
				polygon::calculateTexCoords(
					vertex.x, vertex.y, tex_info.size.x, tex_info.size.y, ttf.sx, ttf.sy, ttf.ox, ttf.oy, ttf.rot),
				flat.colour);
		}
	}
	else
	{
		for (int i = sector.polygonVertices().size(); i > 0; --i) // Floor polygons need to be flipped
		{
			auto& vertex = sector.polygonVertices()[i - 1];
			vertices.emplace_back(
				glm::vec3(vertex, plane.heightAt(vertex)),
				polygon::calculateTexCoords(
					vertex.x, vertex.y, tex_info.size.x, tex_info.size.y, ttf.sx, ttf.sy, ttf.ox, ttf.oy, ttf.rot),
				flat.colour);
		}
	}
}

static void setupFlat3D(
	Flat3D&                  flat,
	unsigned                 vertex_index,
	vector<gl::Vertex3D>&    vertices,
	std::optional<glm::vec4> colour = std::nullopt)
{
	auto& sector = *flat.controlSector();

	// Colour
	bool ceiling = flat.source_tex == SectorSurfaceType::Ceiling;
	if (colour.has_value())
		flat.colour = colour.value();
	else
		flat.colour = sector.colourAt(ceiling ? 2 : 1);
	if (flat.colour.a < 1.0f)
		flat.setFlag(Flat3D::Flags::Transparent);

	// Texture
	bool  mix_tex_flats = game::configuration().featureSupported(game::Feature::MixTexFlats);
	auto  tex_name      = ceiling ? sector.ceiling().texture : sector.floor().texture;
	auto& texture       = textureManager().flat(tex_name, mix_tex_flats);
	flat.texture        = texture.gl_id;

	// Sky
	if (strutil::equalCI(tex_name, game::configuration().skyFlat()))
		flat.setFlag(Flat3D::Flags::Sky);

	// Vertices
	flat.vertex_offset = vertex_index;
	generateFlatVertices(flat, texture, vertices);

	flat.updated_time = app::runTimer();
}

// -----------------------------------------------------------------------------
// Generates 3D flats and vertices for [sector]
// -----------------------------------------------------------------------------
std::tuple<vector<Flat3D>, vector<gl::Vertex3D>> generateSectorFlats(const MapSector& sector, unsigned vertex_index)
{
	vector<Flat3D>       flats;
	vector<gl::Vertex3D> vertices;

	auto sector_vertex_count = static_cast<unsigned>(sector.polygonVertices().size());

	// Ceiling
	auto& flat_ceiling = flats.emplace_back(&sector, SectorSurfaceType::Ceiling);
	glm::vec4 colour       = sector.colourAt(2);
	setupFlat3D(flat_ceiling, vertex_index, vertices, colour);
	vertex_index += sector_vertex_count;

	// 3d floors
	for (auto& extrafloor : sector.parentMap()->mapSpecials().sectorExtraFloors(&sector))
	{
		colour.a = extrafloor.alpha;

		// Top
		auto& flat_top = flats.emplace_back(
			&sector,
			SectorSurfaceType::Floor,
			extrafloor.control_sector,
			SectorSurfaceType::Ceiling,
			SectorSurfaceType::Ceiling);
		flat_top.setFlag(Flat3D::Flags::ExtraFloor);
		setupFlat3D(flat_top, vertex_index, vertices, colour);
		vertex_index += sector_vertex_count;

		// Inner (if needed)
		colour   = extrafloor.control_sector->colourAt(2);
		colour.a = extrafloor.alpha;
		if (extrafloor.hasFlag(ExtraFloor::Flags::DrawInside))
		{
			// Top
			if (extrafloor.height < sector.ceiling().height)
			{
				auto& flat_inner_top = flats.emplace_back(
					&sector,
					SectorSurfaceType::Ceiling,
					extrafloor.control_sector,
					SectorSurfaceType::Ceiling,
					SectorSurfaceType::Ceiling);
				flat_inner_top.setFlag(Flat3D::Flags::ExtraFloor);
				setupFlat3D(flat_inner_top, vertex_index, vertices, colour);
				vertex_index += sector_vertex_count;
			}

			// Bottom
			if (extrafloor.control_sector->floor().height > sector.floor().height)
			{
				auto& flat_inner_bottom = flats.emplace_back(
					&sector,
					SectorSurfaceType::Floor,
					extrafloor.control_sector,
					SectorSurfaceType::Floor,
					SectorSurfaceType::Floor);
				flat_inner_bottom.setFlag(Flat3D::Flags::ExtraFloor);
				colour   = extrafloor.control_sector->colourAt(1);
				colour.a = extrafloor.alpha;
				setupFlat3D(flat_inner_bottom, vertex_index, vertices, colour);
				vertex_index += sector_vertex_count;
			}
		}

		// Bottom
		auto& flat_bottom = flats.emplace_back(
			&sector,
			SectorSurfaceType::Ceiling,
			extrafloor.control_sector,
			extrafloor.hasFlag(ExtraFloor::Flags::FlatAtCeiling) ? SectorSurfaceType::Ceiling
																 : SectorSurfaceType::Floor,
			SectorSurfaceType::Floor);
		flat_bottom.setFlag(Flat3D::Flags::ExtraFloor);
		setupFlat3D(flat_bottom, vertex_index, vertices, colour);
		vertex_index += sector_vertex_count;
	}

	// Floor
	auto& flat_floor = flats.emplace_back(&sector, SectorSurfaceType::Floor);
	colour.a         = 1.0f;
	setupFlat3D(flat_floor, vertex_index, vertices, colour);

	return { flats, vertices };
}

// -----------------------------------------------------------------------------
// Updates [flat] and generates new [vertices] for it
// -----------------------------------------------------------------------------
void updateFlat(Flat3D& flat, vector<gl::Vertex3D>& vertices)
{
	// Update flat
	auto& sector  = *flat.controlSector();
	auto& texture = flat.source_tex == SectorSurfaceType::Ceiling
						? textureManager().flat(sector.ceiling().texture, true)
						: textureManager().flat(sector.floor().texture, true);
	flat.texture  = texture.gl_id;
	flat.colour   = sector.colourAt(flat.source_tex == SectorSurfaceType::Ceiling ? 2 : 1);

	// Generate new vertices
	generateFlatVertices(flat, texture, vertices);

	flat.updated_time = app::runTimer();
}

} // namespace slade::mapeditor
