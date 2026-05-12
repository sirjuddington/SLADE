
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer2D_Flats.cpp
// Description: MapRenderer2D flat (sector ceiling/floor) rendering functions
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
#include "General/ColourConfiguration.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapRenderer2D.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer2D.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Utility/Polygon.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, map2d_flat_drawtype, 2, CVar::Flag::Save)
CVAR(Float, map2d_flat_brightness, 0.8f, CVar::Flag::Save)
CVAR(Bool, map2d_flat_ignore_light, false, CVar::Flag::Save)
CVAR(Bool, map2d_sector_hilight_fill, true, CVar::Flag::Save)
CVAR(Bool, map2d_sector_selected_fill, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, map_animate_hilight)
EXTERN_CVAR(Bool, map_animate_selection)
EXTERN_CVAR(Bool, map_animate_tagged)
EXTERN_CVAR(Bool, map2d_action_lines)
EXTERN_CVAR(Float, map2d_line_width)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns the [ceiling]/floor texture for [sector]
// -----------------------------------------------------------------------------
const MapTextureManager::Texture& sectorTexture(const MapSector* sector, bool ceiling)
{
	bool mix_tex_flats = game::configuration().featureSupported(game::Feature::MixTexFlats);

	if (ceiling)
		return textureManager().flat(sector->ceiling().texture, mix_tex_flats);
	else
		return textureManager().flat(sector->floor().texture, mix_tex_flats);
}

// -----------------------------------------------------------------------------
// Returns the colour for [sector]
// -----------------------------------------------------------------------------
glm::vec4 sectorColour(const MapSector& sector, bool ceiling)
{
	auto& map_specials = sector.parentMap()->mapSpecials();
	return map_specials.sectorColour(sector, ceiling ? map::SectorPart::Ceiling : map::SectorPart::Floor)
		.ampf(map2d_flat_brightness, map2d_flat_brightness, map2d_flat_brightness, 1.0f);
	// return sector->colourAt(ceiling ? 2 : 1).ampf(flat_brightness, flat_brightness, flat_brightness, 1.0f);
}

// -----------------------------------------------------------------------------
// Generates texture coordinates for the given [vertices]
// -----------------------------------------------------------------------------
void generateTextureCoords(
	vector<gl::Vertex2D>&             vertices,
	MapSector*                        sector,
	bool                              ceiling,
	const MapTextureManager::Texture& texture)
{
	using namespace game;

	auto& game_config = configuration();

	// Get scaling/offset info
	double ox  = 0.;
	double oy  = 0.;
	double sx  = texture.scale.x;
	double sy  = texture.scale.y;
	double rot = 0.;
	// Check for various UDMF extensions
	if (editContext().mapDesc().format == MapFormat::UDMF)
	{
		// Ceiling
		if (ceiling)
		{
			if (game_config.featureSupported(UDMFFeature::FlatPanning))
			{
				ox = sector->floatProperty("xpanningceiling");
				oy = sector->floatProperty("ypanningceiling");
			}
			if (game_config.featureSupported(UDMFFeature::FlatScaling))
			{
				sx *= (1.0 / sector->floatProperty("xscaleceiling"));
				sy *= (1.0 / sector->floatProperty("yscaleceiling"));
			}
			if (game_config.featureSupported(UDMFFeature::FlatRotation))
				rot = sector->floatProperty("rotationceiling");
		}
		// Floor
		else
		{
			if (game_config.featureSupported(UDMFFeature::FlatPanning))
			{
				ox = sector->floatProperty("xpanningfloor");
				oy = sector->floatProperty("ypanningfloor");
			}
			if (game_config.featureSupported(UDMFFeature::FlatScaling))
			{
				sx *= (1.0 / sector->floatProperty("xscalefloor"));
				sy *= (1.0 / sector->floatProperty("yscalefloor"));
			}
			if (game_config.featureSupported(UDMFFeature::FlatRotation))
				rot = sector->floatProperty("rotationfloor");
		}
	}
	// Scaling applies to offsets as well.
	// Note for posterity: worldpanning only applies to textures, not flats
	ox /= sx;
	oy /= sy;

	polygon::generateTextureCoords(vertices, texture.gl_id, sx, sy, ox, oy, rot);
}
} // namespace


// -----------------------------------------------------------------------------
//
// MapRenderer2D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Renders map flats (sectors)
// -----------------------------------------------------------------------------
void MapRenderer2D::renderFlats(bool ceilings, float alpha)
{
	// Don't bother if (practically) invisible
	if (alpha <= 0.01f || map2d_flat_drawtype == 0)
		return;

	// Apply flat alpha from theme
	bool texture = map2d_flat_drawtype > 1;
	if (texture)
		alpha *= colourconfig::flatAlpha();

	// Update flats buffer if needed
	updateFlatsBuffer(ceilings);

	// Setup shader
	const auto& shader = gl::draw2d::defaultShader(texture);
	view_->setupShader(shader);
	shader.setUniform(
		"colour", glm::vec4{ map2d_flat_brightness, map2d_flat_brightness, map2d_flat_brightness, alpha });

	if (texture)
	{
		gl::bindVAO(flats_buffer_->vao());

		for (const auto& fg : flat_groups_)
		{
			gl::Texture::bind(fg.texture);
			fg.index_buffer->bind();
			gl::drawElements(gl::Primitive::Triangles, fg.index_buffer->size(), GL_UNSIGNED_INT);
		}

		gl::bindEBO(0);
		gl::bindVAO(0);
	}
	else
	{
		// Untextured, just draw the entire flats buffer
		flats_buffer_->draw();
	}
}

// -----------------------------------------------------------------------------
// Renders the flat hilight overlay for sector [index]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderFlatHilight(gl::draw2d::Context& dc, int index, float fade) const
{
	// Check hilight
	if (!map_->sector(index) || index >= flats_.size())
		return;

	// Reset fade if hilight animation is disabled
	if (!map_animate_hilight)
		fade = 1.0f;

	// Set render options
	dc.setColourFromConfig("map_hilight", fade);
	dc.line_thickness = map2d_line_width * (colourconfig::lineHilightWidth() * fade);

	// Fill if cvar is set
	if (map2d_sector_hilight_fill)
	{
		const auto& shader = gl::draw2d::defaultShader(false);
		shader.setUniform("colour", dc.colour.ampf(1.0f, 1.0f, 1.0f, 0.2f));
		dc.view->setupShader(shader);
		flats_buffer_->draw(
			gl::Primitive::Triangles, nullptr, nullptr, flats_[index].buffer_offset, flats_[index].vertex_count);
		dc.line_thickness *= 0.75f;
	}

	// Get all lines belonging to the hilighted sector
	vector<MapLine*> lines;
	map_->sector(index)->putLines(lines);

	// Build list of lines to render
	vector<Rectf> render_lines;
	for (auto line : lines)
		if (line)
			render_lines.emplace_back(line->x1(), line->y1(), line->x2(), line->y2());

	// Render lines
	dc.drawLines(render_lines);

	//// Draw sector split lines
	// if (test_ssplit)
	//{
	//	glColor4f(col.fr(), col.fg(), col.fb(), col.fa() * 0.5f);
	//	glLineWidth(1.0f);
	//	map_->sector(index)->polygon()->renderWireframe();
	// }

	//// TEST draw text point
	// glPointSize(8.0f);
	// glBegin(GL_POINTS);
	// glVertex2d(map->getSector(index)->getPoint(map::ObjectPoint::Within).x,
	// map->getSector(index)->getPoint(map::ObjectPoint::Within).y); glEnd();
}

// -----------------------------------------------------------------------------
// Renders flat overlays for the given [sectors]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderFlatOverlays(const gl::draw2d::Context& dc, const vector<MapSector*>& sectors) const
{
	// Setup shader (for fill)
	const auto& shader = gl::draw2d::defaultShader(false);
	shader.setUniform("colour", dc.colour.ampf(1.0f, 1.0f, 1.0f, 0.2f));
	dc.view->setupShader(shader);

	// Go through selection, render fill (if needed) and build list of lines to render (for outline)
	vector<uint8_t> lines_added(map_->nLines(), 0);
	vector<Rectf>   render_lines;
	for (const auto sector : sectors)
	{
		// Don't draw if outside screen (but still draw if it's small)
		// if (vis_s_[sector->index()] > 0 && vis_s_[sector->index()] != VIS_SMALL)
		//	continue;

		// Render fill if needed
		if (map2d_sector_selected_fill)
			flats_buffer_->draw(
				gl::Primitive::Triangles,
				nullptr,
				nullptr,
				flats_[sector->index()].buffer_offset,
				flats_[sector->index()].vertex_count);

		// Go through sides
		for (const auto side : sector->connectedSides())
		{
			// Get line
			auto line = side->parentLine();
			if (lines_added[line->index()] > 0)
				continue; // Ignore if already added

			// Add line to render list
			render_lines.emplace_back(line->x1(), line->y1(), line->x2(), line->y2());
			lines_added[line->index()] = 1;
		}
	}

	// Render lines
	dc.drawLines(render_lines);
}

// -----------------------------------------------------------------------------
// Renders the flat selection overlay for sectors in [selection]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderFlatSelection(gl::draw2d::Context& dc, const ItemSelection& selection, float fade) const
{
	// Check anything is selected
	if (selection.empty())
		return;

	// Reset fade if selection animation is disabled
	if (!map_animate_selection)
		fade = 1.0f;

	// Set render options
	dc.setColourFromConfig("map_selection", fade);
	dc.line_thickness = map2d_line_width * 2.0f;

	// Render flat overlays for selection
	vector<MapSector*> sectors;
	for (const auto& item : selection)
		sectors.push_back(item.asSector(*map_));
	renderFlatOverlays(dc, sectors);
}

// -----------------------------------------------------------------------------
// Renders the tagged flat overlay for sectors in [sectors]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderTaggedFlats(gl::draw2d::Context& dc, const vector<MapSector*>& sectors, float fade) const
{
	// Reset fade if tagged animation is disabled
	if (!map_animate_tagged)
		fade = 1.0f;

	// Setup render options
	dc.setColourFromConfig("map_tagged", fade);
	dc.line_thickness = map2d_line_width * 2.0f;

	// Render overlays for tagged sectors
	renderFlatOverlays(dc, sectors);

	// Action Lines
	auto object = editContext().selection().hilightedObject();
	if (map2d_action_lines && object)
	{
		vector<Rectf> lines;
		for (const auto sector : sectors)
		{
			// Skip if the tagged sector is adjacent
			if (object->objType() == map::ObjectType::Line)
			{
				auto line = dynamic_cast<MapLine*>(object);
				if (line->frontSector() == sector || line->backSector() == sector)
					continue;
			}

			lines.emplace_back(object->getPoint(MapObject::Point::Within), sector->getPoint(MapObject::Point::Within));
		}

		// Render action lines
		dc.line_thickness    = /*line_width * */ 1.5f;
		dc.line_arrow_length = 24.0f / dc.view->scale(true).x;
		dc.drawLines(lines);
		dc.line_arrow_length = 0.0f;
	}
}

// -----------------------------------------------------------------------------
// Updates the map flats buffer & info
// -----------------------------------------------------------------------------
void MapRenderer2D::updateFlatsBuffer(bool ceilings)
{
	bool rebuild = false;

	// Init buffer if needed
	if (!flats_buffer_)
	{
		flats_buffer_ = std::make_unique<gl::VertexBuffer2D>();
		rebuild       = true;
	}

	// Check if we need to rebuild the buffer
	if (map_->geometryUpdated() > flats_updated_ || map_->nSectors() != flats_.size())
		rebuild = true;

	if (rebuild)
	{
		// Init flats info cache
		flats_.clear();
		flats_.resize(map_->nSectors());

		// Write sector triangle vertices to buffer
		for (unsigned i = 0; i < map_->nSectors(); ++i)
		{
			auto  sector  = map_->sector(i);
			auto& texture = sectorTexture(sector, ceilings);
			auto  colour  = sectorColour(*sector, ceilings);

			flats_[i].texture       = texture.gl_id;
			flats_[i].buffer_offset = flats_buffer_->queueSize();
			flats_[i].vertex_count  = sector->polygonVertices().size();
			flats_[i].updated_time  = app::runTimer();

			vector<gl::Vertex2D> vertices;
			for (const auto& vertex : sector->polygonVertices())
				vertices.emplace_back(glm::vec2{ vertex.x, vertex.y }, colour, glm::vec2{ 0.0f });
			generateTextureCoords(vertices, sector, ceilings, texture);
			flats_buffer_->add(vertices);
		}
		flats_buffer_->push();
		flats_updated_ = app::runTimer();

		flat_groups_.clear();
	}
	else
	{
		bool clear_flat_groups = false;

		for (unsigned i = 0; i < map_->nSectors(); ++i)
		{
			auto sector = map_->sector(i);

			if (sector->modifiedTime() > flats_[i].updated_time)
			{
				// Check if texture has changed since last update
				auto& tex = sectorTexture(sector, ceilings);
				if (tex.gl_id != flats_[i].texture)
				{
					flats_[i].texture = tex.gl_id;
					clear_flat_groups = true;
				}

				// Update sector polygon triangle vertices in buffer
				auto                 colour = sectorColour(*sector, ceilings);
				vector<gl::Vertex2D> vertices;
				for (const auto& pv : sector->polygonVertices())
					vertices.emplace_back(pv, colour, glm::vec2{ 0.0f });
				generateTextureCoords(vertices, sector, ceilings, tex);
				flats_buffer_->buffer().update(flats_[i].buffer_offset, vertices);
				flats_[i].updated_time = app::runTimer();
			}
		}

		if (clear_flat_groups)
			flat_groups_.clear();
	}

	gl::bindVAO(0);

	if (flat_groups_.empty())
	{
		vector<uint8_t> flats_processed(flats_.size());

		for (unsigned i = 0; i < flats_.size(); ++i)
		{
			if (flats_processed[i])
				continue;

			// Build list of vertex indices for sectors using this texture
			vector<GLuint> indices;
			for (unsigned f = i; f < flats_.size(); ++f)
			{
				// Check texture match
				auto& flat = flats_[f];
				if (flats_processed[f] || flat.texture != flats_[i].texture)
					continue;

				// Add indices
				auto vi = flat.buffer_offset;
				while (vi < flat.buffer_offset + flat.vertex_count)
					indices.push_back(vi++);

				flats_processed[f] = 1;
			}

			// Add flat group for texture
			flat_groups_.emplace_back();
			flat_groups_.back().texture      = flats_[i].texture;
			flat_groups_.back().index_buffer = std::make_unique<gl::IndexBuffer>();
			flat_groups_.back().index_buffer->upload(indices);

			flats_processed[i] = 1;
		}
	}
}
