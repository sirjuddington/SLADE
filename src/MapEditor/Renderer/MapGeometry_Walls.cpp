
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapGeometry_Walls.cpp
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
#include "Geometry/Plane.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapGeometry.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/VertexBuffer3D.h"
#include "Quad3D.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Structs
//
// -----------------------------------------------------------------------------
namespace
{
using MapTexture = MapTextureManager::Texture;

struct LineQuadsContext
{
	const MapLine*       line;
	vector<Quad3D>       quads;
	vector<gl::Vertex3D> vertices;
	unsigned             vertex_index   = 0;
	bool                 mix_tex_flats  = false;
	bool                 upper_unpegged = false;
	bool                 lower_unpegged = false;
};

// Helper struct for building wall quads
struct QuadInfo
{
	const MapLine*    line       = nullptr;
	int               height_top = 0;
	Plane             plane_top;
	int               height_bottom = 0;
	Plane             plane_bottom;
	const MapTexture* texture = nullptr;
	gl::Vertex3D      tl, tr, bl, br;
	Vec2i             offsets;
	int               line_length  = 0;
	int               tex_y_origin = 0; // Absolute height to align texture y origin to
	bool              back_side    = false;
};
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace slade::mapeditor
{
static void setupQuadTexCoords(QuadInfo& info)
{
	// Get texture info
	auto& tex_info = gl::Texture::info(info.texture->gl_id);

	// Calculate texture coordinates
	int x1 = info.offsets.x;
	int x2 = info.line_length + info.offsets.x;
	int y1 = info.tex_y_origin - info.height_top + info.offsets.y;
	int y2 = y1 + (info.height_top - info.height_bottom);

	float tl_diff = info.height_top - info.tl.position.z;
	float tr_diff = info.height_top - info.tr.position.z;
	float bl_diff = info.height_bottom - info.bl.position.z;
	float br_diff = info.height_bottom - info.br.position.z;

	// Set uv tex coords
	info.tl.uv.x = static_cast<float>(x1) / static_cast<float>(tex_info.size.x);
	info.tl.uv.y = (static_cast<float>(y1) + tl_diff) / static_cast<float>(tex_info.size.y);
	info.tr.uv.x = static_cast<float>(x2) / static_cast<float>(tex_info.size.x);
	info.tr.uv.y = (static_cast<float>(y1) + tr_diff) / static_cast<float>(tex_info.size.y);
	info.bl.uv.x = static_cast<float>(x1) / static_cast<float>(tex_info.size.x);
	info.bl.uv.y = (static_cast<float>(y2) + bl_diff) / static_cast<float>(tex_info.size.y);
	info.br.uv.x = static_cast<float>(x2) / static_cast<float>(tex_info.size.x);
	info.br.uv.y = (static_cast<float>(y2) + br_diff) / static_cast<float>(tex_info.size.y);
}

static void addQuad(vector<gl::Vertex3D>& vertices, QuadInfo& info, const glm::vec4& colour)
{
	float x1, y1, x2, y2;
	if (info.back_side)
	{
		x1 = info.line->x2();
		y1 = info.line->y2();
		x2 = info.line->x1();
		y2 = info.line->y1();
	}
	else
	{
		x1 = info.line->x1();
		y1 = info.line->y1();
		x2 = info.line->x2();
		y2 = info.line->y2();
	}

	info.tl = gl::Vertex3D{ glm::vec3{ x1, y1, info.plane_top.heightAt(x1, y1) }, {}, colour };
	info.tr = gl::Vertex3D{ glm::vec3{ x2, y2, info.plane_top.heightAt(x2, y2) }, {}, colour };
	info.bl = gl::Vertex3D{ glm::vec3{ x1, y1, info.plane_bottom.heightAt(x1, y1) }, {}, colour };
	info.br = gl::Vertex3D{ glm::vec3{ x2, y2, info.plane_bottom.heightAt(x2, y2) }, {}, colour };

	setupQuadTexCoords(info);

	// Add vertices (two triangles)
	vertices.emplace_back(info.tl);
	vertices.emplace_back(info.bl);
	vertices.emplace_back(info.br);

	vertices.emplace_back(info.tl);
	vertices.emplace_back(info.br);
	vertices.emplace_back(info.tr);
}

void buildWallPartQuads(LineQuadsContext& context, MapLine::Part part)
{
	// TODO:
	//  - Handle 3D floors splitting wall quads
	//  - Handle quads that are split (or not full-line length) due to slopes

	// Get relative sides depending on wall part
	auto     line      = context.line;
	MapSide* side      = nullptr;
	MapSide* side_back = nullptr;
	switch (part)
	{
	case MapLine::FrontMiddle:
		side      = line->s1();
		side_back = line->s2() ? line->s2() : line->s1(); // For one-sided line, only use first side
		break;
	case MapLine::FrontUpper:
	case MapLine::FrontLower:
		side      = line->s1();
		side_back = line->s2();
		break;
	case MapLine::BackMiddle:
	case MapLine::BackUpper:
	case MapLine::BackLower:
		side      = line->s2();
		side_back = line->s1();
		break;
	}

	// Get heights, texture and alignment info depending on wall part
	int    height_top, height_bottom, tex_y_origin;
	Plane  plane_top, plane_bottom;
	string tex_name;
	switch (part)
	{
	case MapLine::FrontMiddle:
	case MapLine::BackMiddle:
		tex_name      = side->texMiddle();
		height_top    = side->sector()->ceiling().height;
		height_bottom = side->sector()->floor().height;
		plane_top     = side->sector()->ceiling().plane;
		plane_bottom  = side->sector()->floor().plane;
		tex_y_origin  = context.lower_unpegged ? height_bottom : height_top;
		break;

	case MapLine::FrontUpper:
	case MapLine::BackUpper:
		tex_name      = side->texUpper();
		height_top    = side->sector()->ceiling().height;
		height_bottom = side_back->sector()->ceiling().height;
		plane_top     = side->sector()->ceiling().plane;
		plane_bottom  = side_back->sector()->ceiling().plane;
		tex_y_origin  = context.upper_unpegged ? height_top : height_bottom;
		break;

	case MapLine::FrontLower:
	case MapLine::BackLower:
		tex_name      = side->texLower();
		height_top    = side_back->sector()->floor().height;
		height_bottom = side->sector()->floor().height;
		plane_top     = side_back->sector()->floor().plane;
		plane_bottom  = side->sector()->floor().plane;
		tex_y_origin  = context.lower_unpegged ? side->sector()->ceiling().height : height_top;
		break;
	}

	// Setup base quad info
	QuadInfo quad_info{
		.line          = line,
		.height_top    = height_top,
		.plane_top     = plane_top,
		.height_bottom = height_bottom,
		.plane_bottom  = plane_bottom,
		.texture       = &textureManager().texture(tex_name, context.mix_tex_flats),
		.offsets       = side->texOffset(),
		.line_length   = static_cast<int>(line->length()),
		.tex_y_origin  = tex_y_origin,
		.back_side     = (side == line->s2()),
	};

	// Apply per-part texture offsets if supported
	if (line->parentMap()->currentFormat() == MapFormat::UDMF)
	{
		switch (part)
		{
		case MapLine::FrontUpper:
		case MapLine::BackUpper:
			if (side->hasProp("offsetx_top"))
				quad_info.offsets.x += side->intProperty("offsetx_top");
			if (side->hasProp("offsety_top"))
				quad_info.offsets.y += side->intProperty("offsety_top");
			break;
		case MapLine::FrontLower:
		case MapLine::BackLower:
			if (side->hasProp("offsetx_bottom"))
				quad_info.offsets.x += side->intProperty("offsetx_bottom");
			if (side->hasProp("offsety_bottom"))
				quad_info.offsets.y += side->intProperty("offsety_bottom");
			break;
		default: break;
		}
	}

	if (line->parentMap()->mapSpecials().sectorHasExtraFloors(side->sector()))
	{
		// TODO: Split quad by extrafloors
	}
	else
	{
		auto light = side->light() / 255.0f;

		context.quads.push_back(
			{ .side          = side,
			  .vertex_offset = context.vertex_index,
			  .colour        = { light, light, light, 1.0f },
			  .texture       = quad_info.texture->gl_id });
		addQuad(context.vertices, quad_info, context.quads.back().colour);
		context.vertex_index += 6;
	}
}

std::tuple<vector<Quad3D>, vector<gl::Vertex3D>> generateLineQuads(const MapLine& line, unsigned vertex_index)
{
	// Check line is valid
	if (!line.s1())
		return { {}, {} };

	// Setup context for generating line quads
	auto             map_format = line.parentMap()->currentFormat();
	auto&            game_cfg   = game::configuration();
	LineQuadsContext context{ .line           = &line,
							  .vertex_index   = vertex_index,
							  .mix_tex_flats  = game_cfg.featureSupported(game::Feature::MixTexFlats),
							  .upper_unpegged = game_cfg.lineBasicFlagSet("dontpegtop", &line, map_format),
							  .lower_unpegged = game_cfg.lineBasicFlagSet("dontpegbottom", &line, map_format) };

	// One-sided line
	if (!line.s2())
		buildWallPartQuads(context, MapLine::FrontMiddle);

	// Two-sided line
	else
	{
		auto s1      = line.s1();
		auto s2      = line.s2();
		auto sector1 = s1->sector();
		auto sector2 = s2->sector();

		// TODO:
		//  - Middle textures require special handling and need a flag to be drawn last

		// Front middle
		// if (s1->texMiddle() != MapSide::TEX_NONE && sector1->floor().height < sector1->ceiling().height)
		//	buildWallPartQuads(context, MapLine::FrontMiddle);

		// Front upper
		if (sector1->ceiling().height > sector2->ceiling().height)
			buildWallPartQuads(context, MapLine::FrontUpper);
		else if (sector1->ceilingHasSlope() || sector2->ceilingHasSlope())
		{
			auto height_s1_start = sector1->ceiling().plane.heightAt(line.x1(), line.y1());
			auto height_s1_end   = sector1->ceiling().plane.heightAt(line.x2(), line.y2());
			auto height_s2_start = sector2->ceiling().plane.heightAt(line.x1(), line.y1());
			auto height_s2_end   = sector2->ceiling().plane.heightAt(line.x2(), line.y2());
			if (height_s1_start > height_s2_start || height_s1_end > height_s2_end)
				buildWallPartQuads(context, MapLine::FrontUpper);
		}

		// Front lower
		if (sector1->floor().height < sector2->floor().height)
			buildWallPartQuads(context, MapLine::FrontLower);
		else if (sector1->floorHasSlope() || sector2->floorHasSlope())
		{
			auto height_s1_start = sector1->floor().plane.heightAt(line.x1(), line.y1());
			auto height_s1_end   = sector1->floor().plane.heightAt(line.x2(), line.y2());
			auto height_s2_start = sector2->floor().plane.heightAt(line.x1(), line.y1());
			auto height_s2_end   = sector2->floor().plane.heightAt(line.x2(), line.y2());
			if (height_s1_start < height_s2_start || height_s1_end < height_s2_end)
				buildWallPartQuads(context, MapLine::FrontLower);
		}

		// Back middle
		// if (s2->texMiddle() != MapSide::TEX_NONE && sector2->floor().height < sector2->ceiling().height)
		//	buildWallPartQuads(context, MapLine::BackMiddle);

		// Back upper
		if (sector2->ceiling().height > sector1->ceiling().height)
			buildWallPartQuads(context, MapLine::BackUpper);
		else if (sector1->ceilingHasSlope() || sector2->ceilingHasSlope())
		{
			auto height_s1_start = sector1->ceiling().plane.heightAt(line.x1(), line.y1());
			auto height_s1_end   = sector1->ceiling().plane.heightAt(line.x2(), line.y2());
			auto height_s2_start = sector2->ceiling().plane.heightAt(line.x1(), line.y1());
			auto height_s2_end   = sector2->ceiling().plane.heightAt(line.x2(), line.y2());
			if (height_s2_start > height_s1_start || height_s2_end > height_s1_end)
				buildWallPartQuads(context, MapLine::BackUpper);
		}

		// Back lower
		if (sector2->floor().height < sector1->floor().height)
			buildWallPartQuads(context, MapLine::BackLower);
		else if (sector1->floorHasSlope() || sector2->floorHasSlope())
		{
			auto height_s1_start = sector1->floor().plane.heightAt(line.x1(), line.y1());
			auto height_s1_end   = sector1->floor().plane.heightAt(line.x2(), line.y2());
			auto height_s2_start = sector2->floor().plane.heightAt(line.x1(), line.y1());
			auto height_s2_end   = sector2->floor().plane.heightAt(line.x2(), line.y2());
			if (height_s2_start < height_s1_start || height_s2_end < height_s1_end)
				buildWallPartQuads(context, MapLine::BackLower);
		}
	}

	return { context.quads, context.vertices };
}
} // namespace slade::mapeditor
