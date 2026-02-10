
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
#include "MapGeometryBuffer3D.h"
#include "OpenGL/GLTexture.h"
#include "Quad3D.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapSpecials/ExtraFloor.h"
#include "SLADEMap/MapSpecials/LineTranslucency.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/MathStuff.h"

using namespace slade;
using namespace mapeditor;

using ExtraFloor = map::ExtraFloor;
using SidePart   = map::SidePart;


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
	const MapLine*                  line;
	vector<Quad3D>                  quads;
	vector<MGVertex>                vertices;
	unsigned                        vertex_index   = 0;
	bool                            mix_tex_flats  = false;
	bool                            upper_unpegged = false;
	bool                            lower_unpegged = false;
	optional<map::LineTranslucency> translucency;
};

// Helper struct for building wall quads
struct QuadInfo
{
	const MapLine*     line       = nullptr;
	int                height_top = 0;
	Plane              plane_top;
	int                height_bottom = 0;
	Plane              plane_bottom;
	const MapTexture*  texture    = nullptr;
	const gl::Texture* gl_texture = nullptr;
	MGVertex           tl, tr, bl, br;
	Vec2d              offsets;
	int                line_length  = 0;
	int                tex_y_origin = 0; // Absolute height to align texture y origin to
	bool               back_side    = false;
	bool               midtex       = false; // If true, the quad is clipped to the height of the texture
	bool               sky          = false;
	bool               extrafloor   = false; // If true, the quad is part of an ExtraFloor
};
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace slade::mapeditor
{
static bool planeIntersectsQuad(
	const Plane& plane,
	const Plane& quad_top,
	const Plane& quad_bottom,
	Vec2d        line_start,
	Vec2d        line_end)
{
	// Check heights at line start
	float plane_height_start       = plane.heightAt(line_start.x, line_start.y);
	float quad_top_height_start    = quad_top.heightAt(line_start.x, line_start.y);
	float quad_bottom_height_start = quad_bottom.heightAt(line_start.x, line_start.y);
	bool  above_start              = math::fGreaterOrEqual(plane_height_start, quad_top_height_start);
	bool  below_start              = math::fLessOrEqual(plane_height_start, quad_bottom_height_start);

	// Check heights at line end
	float plane_height_end       = plane.heightAt(line_end.x, line_end.y);
	float quad_top_height_end    = quad_top.heightAt(line_end.x, line_end.y);
	float quad_bottom_height_end = quad_bottom.heightAt(line_end.x, line_end.y);
	bool  above_end              = math::fGreaterOrEqual(plane_height_end, quad_top_height_end);
	bool  below_end              = math::fLessOrEqual(plane_height_end, quad_bottom_height_end);

	// If both ends are above or below the quad, no intersection
	if ((above_start && above_end) || (below_start && below_end))
		return false;

	// Otherwise, we have an intersection
	return true;
}

static void setupQuadTexCoords(QuadInfo& info)
{
	// Calculate texture coordinates
	float x1 = info.offsets.x;
	float x2 = info.line_length + info.offsets.x;
	float y1 = info.tex_y_origin - info.height_top;
	if (!info.midtex)
		y1 += info.offsets.y;
	float y2 = y1 + (info.height_top - info.height_bottom);

	float tl_diff = info.height_top - info.tl.position.z;
	float tr_diff = info.height_top - info.tr.position.z;
	float bl_diff = info.height_bottom - info.bl.position.z;
	float br_diff = info.height_bottom - info.br.position.z;

	// Set uv tex coords
	info.tl.uv.x = x1 / static_cast<float>(info.gl_texture->size.x);
	info.tl.uv.y = (y1 + tl_diff) / static_cast<float>(info.gl_texture->size.y);
	info.tr.uv.x = x2 / static_cast<float>(info.gl_texture->size.x);
	info.tr.uv.y = (y1 + tr_diff) / static_cast<float>(info.gl_texture->size.y);
	info.bl.uv.x = x1 / static_cast<float>(info.gl_texture->size.x);
	info.bl.uv.y = (y2 + bl_diff) / static_cast<float>(info.gl_texture->size.y);
	info.br.uv.x = x2 / static_cast<float>(info.gl_texture->size.x);
	info.br.uv.y = (y2 + br_diff) / static_cast<float>(info.gl_texture->size.y);
}

static void addQuad(
	LineQuadsContext& context,
	QuadInfo&         info,
	const MapSide*    side,
	SidePart          part,
	const MapSector*  sector = nullptr)
{
	// Determine lighting
	auto& map_specials = side->parentMap()->mapSpecials();
	auto  lighting     = map_specials.sectorLightingAt(
        sector ? *sector : *side->sector(), map::SectorPart::Interior, info.plane_top, true);

	// Setup & add quad
	Quad3D quad{ .side          = side,
				 .part          = part,
				 .vertex_offset = context.vertex_index,
				 .brightness    = static_cast<float>(lighting.brightness) / 255.0f,
				 .colour        = lighting.colour,
				 .texture       = info.texture->gl_id };

	// Determine render pass
	bool masked = info.midtex || info.extrafloor; // Midtextures and extrafloor sides are always masked
	if (info.sky)
		quad.render_pass = RenderPass::Sky;
	else if (masked && context.translucency.has_value())
	{
		// Transparency only applies to mid textures and extrafloor sides
		if (context.translucency->additive)
			quad.setFlag(Quad3D::Flags::Additive);
		quad.colour.a *= context.translucency->alpha;
		quad.render_pass = RenderPass::Transparent;
	}
	else if (masked)
		quad.render_pass = RenderPass::Masked;
	else
		quad.render_pass = RenderPass::Normal;

	// Setup flags
	if (info.extrafloor)
		quad.setFlag(Quad3D::Flags::ExtraFloor);
	if (info.back_side)
		quad.setFlag(Quad3D::Flags::BackSide);

	// Determine vertex positions
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
	if (info.midtex)
	{
		// Midtextures ignore slopes
		info.tl = { glm::vec3{ x1, y1, info.height_top }, {}, quad.brightness };
		info.tr = { glm::vec3{ x2, y2, info.height_top }, {}, quad.brightness };
		info.bl = { glm::vec3{ x1, y1, info.height_bottom }, {}, quad.brightness };
		info.br = { glm::vec3{ x2, y2, info.height_bottom }, {}, quad.brightness };
	}
	else
	{
		info.tl = { glm::vec3{ x1, y1, info.plane_top.heightAt(x1, y1) }, {}, quad.brightness };
		info.tr = { glm::vec3{ x2, y2, info.plane_top.heightAt(x2, y2) }, {}, quad.brightness };
		info.bl = { glm::vec3{ x1, y1, info.plane_bottom.heightAt(x1, y1) }, {}, quad.brightness };
		info.br = { glm::vec3{ x2, y2, info.plane_bottom.heightAt(x2, y2) }, {}, quad.brightness };
	}

	// Heights
	quad.height[0] = info.tl.position.z;
	quad.height[1] = info.bl.position.z;
	quad.height[2] = info.br.position.z;
	quad.height[3] = info.tr.position.z;

	context.quads.push_back(quad);

	// Add vertices (two triangles)
	setupQuadTexCoords(info);
	context.vertices.emplace_back(info.tl);
	context.vertices.emplace_back(info.bl);
	context.vertices.emplace_back(info.br);
	context.vertices.emplace_back(info.tl);
	context.vertices.emplace_back(info.br);
	context.vertices.emplace_back(info.tr);
	context.vertex_index += 6;
}

void buildWallExtraFloorQuads(LineQuadsContext& context, const ExtraFloor& ef, bool front)
{
	// TODO: 'Use upper/lower texture' flags

	// Setup base quad info
	auto&    texture = textureManager().texture(ef.control_line->s1()->texMiddle(), context.mix_tex_flats);
	auto&    gl_tex  = gl::Texture::info(texture.gl_id);
	QuadInfo quad_info{ .line          = context.line,
						.height_top    = ef.height,
						.plane_top     = ef.plane_top,
						.height_bottom = ef.control_sector->floor().height, // TODO: Vavoom
						.plane_bottom  = ef.plane_bottom,
						.texture       = &texture,
						.gl_texture    = &gl_tex,
						.offsets       = { 0, 0 },
						.line_length   = static_cast<int>(context.line->length()),
						.tex_y_origin  = ef.height,
						.back_side     = front, // Show on back side of line if extrafloor is on front
						.extrafloor    = true };

	auto side = front ? context.line->s2() : context.line->s1();

	// Skip if the quad is completely below the floor
	if (quad_info.plane_top.heightAt(context.line->start())
			<= side->sector()->floor().plane.heightAt(context.line->start())
		&& quad_info.plane_top.heightAt(context.line->end())
			   <= side->sector()->floor().plane.heightAt(context.line->end()))
		return;

	// Skip if the quad is completely above the ceiling
	if (quad_info.plane_bottom.heightAt(context.line->start())
			>= side->sector()->ceiling().plane.heightAt(context.line->start())
		&& quad_info.plane_bottom.heightAt(context.line->end())
			   >= side->sector()->ceiling().plane.heightAt(context.line->end()))
		return;

	// Add quad
	auto control_side = ef.control_line->s1();
	addQuad(context, quad_info, control_side, SidePart::Middle, side->sector());
}

void buildWallPartQuads(LineQuadsContext& context, MapLine::Part part)
{
	// TODO:
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
	int      height_top, height_bottom, tex_y_origin;
	Plane    plane_top, plane_bottom;
	string   tex_name;
	SidePart side_part;
	switch (part)
	{
	case MapLine::FrontMiddle:
	case MapLine::BackMiddle:
		tex_name      = side->texMiddle();
		height_top    = std::min(side->sector()->ceiling().height, side_back->sector()->ceiling().height);
		height_bottom = std::max(side->sector()->floor().height, side_back->sector()->floor().height);
		plane_top     = side_back->sector()->ceiling().plane;
		plane_bottom  = side_back->sector()->floor().plane;
		tex_y_origin  = context.lower_unpegged ? height_bottom : height_top;
		side_part     = SidePart::Middle;
		break;

	case MapLine::FrontUpper:
	case MapLine::BackUpper:
		tex_name      = side->texUpper();
		height_top    = side->sector()->ceiling().height;
		height_bottom = side_back->sector()->ceiling().height;
		plane_top     = side->sector()->ceiling().plane;
		plane_bottom  = side_back->sector()->ceiling().plane;
		tex_y_origin  = context.upper_unpegged ? height_top : height_bottom;
		side_part     = SidePart::Upper;
		break;

	case MapLine::FrontLower:
	case MapLine::BackLower:
		tex_name      = side->texLower();
		height_top    = side_back->sector()->floor().height;
		height_bottom = side->sector()->floor().height;
		plane_top     = side_back->sector()->floor().plane;
		plane_bottom  = side->sector()->floor().plane;
		tex_y_origin  = context.lower_unpegged ? side->sector()->ceiling().height : height_top;
		side_part     = SidePart::Lower;
		break;
	}

	// Setup base quad info
	auto&    texture = textureManager().texture(tex_name, context.mix_tex_flats);
	auto&    gl_tex  = gl::Texture::info(texture.gl_id);
	QuadInfo quad_info{
		.line          = line,
		.height_top    = height_top,
		.plane_top     = plane_top,
		.height_bottom = height_bottom,
		.plane_bottom  = plane_bottom,
		.texture       = &texture,
		.gl_texture    = &gl_tex,
		.offsets       = side->texOffset(side_part),
		.line_length   = static_cast<int>(line->length()),
		.tex_y_origin  = tex_y_origin,
		.back_side     = (side == line->s2()),
	};

	// Handle 2-sided midtextures
	if ((part == MapLine::FrontMiddle || part == MapLine::BackMiddle) && line->s2())
	{
		// Set tex_y_origin to where the top of the mid texture would be
		if (context.lower_unpegged)
			quad_info.tex_y_origin = quad_info.height_bottom + gl_tex.size.y;
		quad_info.tex_y_origin += quad_info.offsets.y;

		// Clip to sector heights
		quad_info.height_top    = std::min(quad_info.tex_y_origin, quad_info.height_top);
		quad_info.height_bottom = std::max(quad_info.tex_y_origin - gl_tex.size.y, quad_info.height_bottom);

		quad_info.midtex = true;
	}

	// Check for sky quad
	if (part == MapLine::FrontUpper || part == MapLine::BackUpper)
	{
		if (strutil::equalCI(side_back->sector()->ceiling().texture, game::configuration().skyFlat()))
			quad_info.sky = true;
	}
	else if (part == MapLine::FrontLower || part == MapLine::BackLower)
	{
		if (strutil::equalCI(side_back->sector()->floor().texture, game::configuration().skyFlat()))
			quad_info.sky = true;
	}

	if (!quad_info.sky && !quad_info.midtex && line->parentMap()->mapSpecials().sectorHasExtraFloors(side->sector()))
	{
		// Split quad by extrafloors
		for (auto& ef : line->parentMap()->mapSpecials().sectorExtraFloors(side->sector()))
		{
			// Check if top or bottom of extrafloor will split the quad
			bool top_intersects = planeIntersectsQuad(
				ef.plane_top, quad_info.plane_top, quad_info.plane_bottom, line->start(), line->end());
			bool bottom_intersects = planeIntersectsQuad(
				ef.plane_bottom, quad_info.plane_top, quad_info.plane_bottom, line->start(), line->end());

			// Split the quad if we have an intersection
			if (top_intersects || bottom_intersects)
			{
				// Add quad down to extrafloor (only if the top of the extrafloor intersects)
				if (top_intersects)
				{
					quad_info.height_bottom = ef.height;
					quad_info.plane_bottom  = ef.plane_top;
					addQuad(context, quad_info, side, side_part);
				}

				// Add inner quad if needed
				if (ef.alpha < 1.0f)
				{
					quad_info.height_top    = ef.height;
					quad_info.plane_top     = ef.plane_top;
					quad_info.height_bottom = ef.control_sector->floor().height; // TODO: Vavoom
					quad_info.plane_bottom  = ef.plane_bottom;
					addQuad(context, quad_info, side, side_part);
				}

				// Setup for next quad below
				quad_info.height_top    = ef.height;
				quad_info.plane_top     = ef.hasFlag(ExtraFloor::Flags::FlatAtCeiling) ? ef.plane_top : ef.plane_bottom;
				quad_info.height_bottom = height_bottom;
				quad_info.plane_bottom  = plane_bottom;
			}
		}

		addQuad(context, quad_info, side, side_part);
	}
	else
	{
		addQuad(context, quad_info, side, side_part);
	}
}

std::tuple<vector<Quad3D>, vector<MGVertex>> generateLineQuads(const MapLine& line, unsigned vertex_index)
{
	// Check line is valid
	if (!line.s1())
		return { {}, {} };

	// Setup context for generating line quads
	auto             map        = line.parentMap();
	auto             map_format = map->currentFormat();
	auto&            game_cfg   = game::configuration();
	LineQuadsContext context{ .line           = &line,
							  .vertex_index   = vertex_index,
							  .mix_tex_flats  = game_cfg.featureSupported(game::Feature::MixTexFlats),
							  .upper_unpegged = game_cfg.lineBasicFlagSet("dontpegtop", &line, map_format),
							  .lower_unpegged = game_cfg.lineBasicFlagSet("dontpegbottom", &line, map_format),
							  .translucency   = map->mapSpecials().lineTranslucency(line) };

	// One-sided line
	if (!line.s2())
		buildWallPartQuads(context, MapLine::FrontMiddle);

	// Two-sided line
	else
	{
		auto s1           = line.s1();
		auto s2           = line.s2();
		auto sector1      = s1->sector();
		auto sector2      = s2->sector();
		bool floor_sloped = sector1->floorHasSlope() || sector2->floorHasSlope();
		bool ceil_sloped  = sector1->ceilingHasSlope() || sector2->ceilingHasSlope();

		// Front middle
		if (s1->texMiddle() != MapSide::TEX_NONE && sector1->floor().height < sector1->ceiling().height)
			buildWallPartQuads(context, MapLine::FrontMiddle);

		// Front upper
		if (!ceil_sloped && sector1->ceiling().height > sector2->ceiling().height)
			buildWallPartQuads(context, MapLine::FrontUpper);
		else if (ceil_sloped)
		{
			auto height_s1_start = sector1->ceiling().plane.heightAt(line.start());
			auto height_s1_end   = sector1->ceiling().plane.heightAt(line.end());
			auto height_s2_start = sector2->ceiling().plane.heightAt(line.start());
			auto height_s2_end   = sector2->ceiling().plane.heightAt(line.end());
			if (math::fGreater(height_s1_start, height_s2_start) || math::fGreater(height_s1_end, height_s2_end))
				buildWallPartQuads(context, MapLine::FrontUpper);
		}

		// Front lower
		if (!floor_sloped && sector1->floor().height < sector2->floor().height)
			buildWallPartQuads(context, MapLine::FrontLower);
		else if (floor_sloped)
		{
			auto height_s1_start = sector1->floor().plane.heightAt(line.start());
			auto height_s1_end   = sector1->floor().plane.heightAt(line.end());
			auto height_s2_start = sector2->floor().plane.heightAt(line.start());
			auto height_s2_end   = sector2->floor().plane.heightAt(line.end());
			if (math::fLess(height_s1_start, height_s2_start) || math::fLess(height_s1_end, height_s2_end))
				buildWallPartQuads(context, MapLine::FrontLower);
		}

		// Back middle
		if (s2->texMiddle() != MapSide::TEX_NONE && sector2->floor().height < sector2->ceiling().height)
			buildWallPartQuads(context, MapLine::BackMiddle);

		// Back upper
		if (!ceil_sloped && sector2->ceiling().height > sector1->ceiling().height)
			buildWallPartQuads(context, MapLine::BackUpper);
		else if (ceil_sloped)
		{
			auto height_s1_start = sector1->ceiling().plane.heightAt(line.start());
			auto height_s1_end   = sector1->ceiling().plane.heightAt(line.end());
			auto height_s2_start = sector2->ceiling().plane.heightAt(line.start());
			auto height_s2_end   = sector2->ceiling().plane.heightAt(line.end());
			if (math::fGreater(height_s2_start, height_s1_start) || math::fGreater(height_s2_end, height_s1_end))
				buildWallPartQuads(context, MapLine::BackUpper);
		}

		// Back lower
		if (!floor_sloped && sector2->floor().height < sector1->floor().height)
			buildWallPartQuads(context, MapLine::BackLower);
		else if (floor_sloped)
		{
			auto height_s1_start = sector1->floor().plane.heightAt(line.start());
			auto height_s1_end   = sector1->floor().plane.heightAt(line.end());
			auto height_s2_start = sector2->floor().plane.heightAt(line.start());
			auto height_s2_end   = sector2->floor().plane.heightAt(line.end());
			if (math::fLess(height_s2_start, height_s1_start) || math::fLess(height_s2_end, height_s1_end))
				buildWallPartQuads(context, MapLine::BackLower);
		}

		// ExtraFloor sides
		auto& ef_front = map->mapSpecials().sectorExtraFloors(sector1);
		auto& ef_back  = map->mapSpecials().sectorExtraFloors(sector2);
		if (!ef_front.empty() || !ef_back.empty())
		{
			// Front side ExtraFloors
			for (auto& ef : ef_front)
			{
				// Ignore flat ExtraFloors
				if (ef.hasFlag(ExtraFloor::Flags::FlatAtCeiling))
					continue;

				// Ignore if the same ExtraFloor exists on back side
				bool found = false;
				for (auto& efb : ef_back)
					if (efb.control_sector == ef.control_sector)
					{
						found = true;
						break;
					}
				if (found)
					continue;

				buildWallExtraFloorQuads(context, ef, true);
			}

			// Back side ExtraFloors
			for (auto& ef : ef_back)
			{
				// Ignore flat ExtraFloors
				if (ef.hasFlag(ExtraFloor::Flags::FlatAtCeiling))
					continue;

				// Ignore if the same ExtraFloor exists on front side
				bool found = false;
				for (auto& eff : ef_front)
					if (eff.control_sector == ef.control_sector)
					{
						found = true;
						break;
					}
				if (found)
					continue;

				buildWallExtraFloorQuads(context, ef, false);
			}
		}
	}

	return { context.quads, context.vertices };
}
} // namespace slade::mapeditor
