
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

#include "App.h"
#include "Flat3D.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapGeometry.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/VertexBuffer3D.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "Utility/Polygon.h"

using namespace slade;
using namespace mapeditor;


namespace slade::mapeditor
{
// -----------------------------------------------------------------------------
// Get texture scaling/offset/rotation info for a sector texture
// -----------------------------------------------------------------------------
TexTransformInfo getTextureTransformInfo(const MapSector& sector, bool ceiling, Vec2d tex_scale)
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
	auto& sector   = *flat.sector;
	auto& tex_info = gl::Texture::info(flat.texture);
	auto  ttf      = getTextureTransformInfo(sector, flat.ceiling, tex.scale);

	if (flat.ceiling)
	{
		for (auto& vertex : sector.polygonVertices())
		{
			vertices.emplace_back(
				glm::vec3(vertex, sector.ceiling().plane.heightAt(vertex)),
				polygon::calculateTexCoords(
					vertex.x, vertex.y, tex_info.size.x, tex_info.size.y, ttf.sx, ttf.sy, ttf.ox, ttf.oy, ttf.rot));
		}
	}
	else
	{
		for (auto i = flat.vertex_count; i > 0; --i) // Floor polygons need to be flipped
		{
			auto& vertex = sector.polygonVertices()[i - 1];
			vertices.emplace_back(
				glm::vec3(vertex, sector.floor().plane.heightAt(vertex)),
				polygon::calculateTexCoords(
					vertex.x, vertex.y, tex_info.size.x, tex_info.size.y, ttf.sx, ttf.sy, ttf.ox, ttf.oy, ttf.rot));
		}
	}
}

static Flat3D generateFlat3D(
	const MapSector&      sector,
	bool                  ceiling,
	unsigned              vertex_index,
	vector<gl::Vertex3D>& vertices)
{
	Flat3D flat;
	flat.sector        = &sector;
	flat.ceiling       = ceiling;
	flat.vertex_offset = vertex_index;
	flat.vertex_count  = sector.polygonVertices().size();

	// Colour
	flat.colour = sector.colourAt(ceiling ? 2 : 1);

	// Normal
	flat.normal = ceiling ? sector.ceiling().plane.normal() : sector.floor().plane.normal();

	// Texture
	bool  mix_tex_flats = game::configuration().featureSupported(game::Feature::MixTexFlats);
	auto& texture       = ceiling ? textureManager().flat(sector.ceiling().texture, mix_tex_flats)
								  : textureManager().flat(sector.floor().texture, mix_tex_flats);
	flat.texture        = texture.gl_id;

	// Vertices
	generateFlatVertices(flat, texture, vertices);

	flat.updated_time = app::runTimer();

	return flat;
}

// -----------------------------------------------------------------------------
// Generates 3D flats and vertices for [sector]
// -----------------------------------------------------------------------------
std::tuple<vector<Flat3D>, vector<gl::Vertex3D>> generateSectorFlats(const MapSector& sector, unsigned vertex_index)
{
	vector<Flat3D>       flats;
	vector<gl::Vertex3D> vertices;

	// Floor
	flats.push_back(generateFlat3D(sector, false, vertex_index, vertices));
	vertex_index += sector.polygonVertices().size();

	// Ceiling
	flats.push_back(generateFlat3D(sector, true, vertex_index, vertices));

	// TODO: 3d floors

	return { flats, vertices };
}

// -----------------------------------------------------------------------------
// Updates [flat] and generates new [vertices] for it
// -----------------------------------------------------------------------------
void updateFlat(Flat3D& flat, vector<gl::Vertex3D>& vertices)
{
	// Update flat
	auto& sector  = *flat.sector;
	auto& texture = flat.ceiling ? textureManager().flat(sector.ceiling().texture, true)
								 : textureManager().flat(sector.floor().texture, true);
	flat.texture  = texture.gl_id;
	flat.colour   = sector.colourAt(flat.ceiling ? 2 : 1);

	// Generate new vertices
	generateFlatVertices(flat, texture, vertices);

	flat.updated_time = app::runTimer();
}

} // namespace slade::mapeditor
