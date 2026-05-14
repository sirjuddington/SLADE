
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer2D_Vertices.cpp
// Description: MapRenderer2D vertex rendering functions
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
#include "General/ColourConfiguration.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapRenderer2D.h"
#include "OpenGL/Draw2D.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/PointSpriteBuffer.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapVertex.h"
#include "SLADEMap/MapObjectList/VertexList.h"
#include "SLADEMap/SLADEMap.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, map2d_vertex_round, true, CVar::Flag::Save)
CVAR(Int, map2d_vertex_size, 10, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, map_animate_hilight)
EXTERN_CVAR(Bool, map_animate_selection)


// -----------------------------------------------------------------------------
//
// MapRenderer2D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the texture to use for vertices (or vertex [overlay]s)
// -----------------------------------------------------------------------------
unsigned MapRenderer2D::vertexTexture(bool overlay)
{
	if (overlay)
		return textureManager().editorImage(map2d_vertex_round ? "vertex/hilight_r" : "vertex/hilight_s").gl_id;

	return textureManager().editorImage(map2d_vertex_round ? "vertex/round" : "vertex/square").gl_id;
}

// -----------------------------------------------------------------------------
// Returns the radius (in map scale) of vertices based on the current view
// -----------------------------------------------------------------------------
float MapRenderer2D::vertexRadius(float scale) const
{
	auto vscale = view_->scale(true).x;
	auto size   = map2d_vertex_size / vscale;

	if (vscale < 1.0f)
		size *= vscale;

	auto min = 4.0f / vscale;
	if (size < min)
		size = min;

	return size * 0.5f * scale;
}

// -----------------------------------------------------------------------------
// Renders map vertices
// -----------------------------------------------------------------------------
void MapRenderer2D::renderVertices(float alpha)
{
	// Check there are any vertices to render
	if (map_->nVertices() == 0)
		return;

	// Don't bother if (practically) invisible
	if (alpha <= 0.01f)
		return;

	// Update vertices buffer if required
	if (!vertices_buffer_ || map_->nVertices() != n_vertices_ || map_->geometryUpdated() > vertices_updated_)
		updateVerticesBuffer();

	// Setup rendering options
	vertices_buffer_->setColour(colourconfig::colour("map_vertex").ampf(1.0f, 1.0f, 1.0f, alpha));
	vertices_buffer_->setPointRadius(vertexRadius());
	gl::setBlend(gl::Blend::Normal);

	// Set texture
	gl::Texture::bind(vertexTexture(false));

	// Render vertices
	vertices_buffer_->draw(gl::PointSpriteType::Textured, view_);
}

// -----------------------------------------------------------------------------
// Renders the vertex hilight overlay for vertex [index]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderVertexHilight(gl::draw2d::Context& dc, int index, float fade) const
{
	// Check hilight
	auto vertex = map_->vertex(index);
	if (!vertex)
		return;

	// Reset fade if hilight animation is disabled
	if (!map_animate_hilight)
		fade = 1.0f;

	// Draw as point sprite
	dc.setColourFromConfig("map_hilight", fade);
	dc.texture            = vertexTexture(true);
	dc.pointsprite_type   = gl::PointSpriteType::Textured;
	dc.pointsprite_radius = 1.8f * vertexRadius();
	dc.drawPointSprites(vector{ vertex->position() });
}

// -----------------------------------------------------------------------------
// Renders the vertex selection overlay for vertex indices in [selection]
// -----------------------------------------------------------------------------
void MapRenderer2D::renderVertexSelection(gl::draw2d::Context& dc, const ItemSelection& selection, float fade) const
{
	// Check anything is selected
	if (selection.empty())
		return;

	// Reset fade if selection animation is disabled
	if (!map_animate_selection)
		fade = 1.0f;

	// Build vertex position list
	vector<Vec2f> vertices;
	for (const auto& item : selection)
		if (auto v = item.asVertex(*map_); v)
			vertices.emplace_back(v->position());

	// Draw as point sprites
	dc.setColourFromConfig("map_selection", fade);
	dc.texture            = vertexTexture(true);
	dc.pointsprite_type   = gl::PointSpriteType::Textured;
	dc.pointsprite_radius = 1.8f * vertexRadius();
	dc.drawPointSprites(vertices);
}

// -----------------------------------------------------------------------------
// (Re)builds the map vertices buffer
// -----------------------------------------------------------------------------
void MapRenderer2D::updateVerticesBuffer()
{
	if (!vertices_buffer_)
		vertices_buffer_ = std::make_unique<gl::PointSpriteBuffer>();

	// Fill vertices buffer
	for (const auto vertex : map_->vertices())
		vertices_buffer_->add({ vertex->xPos(), vertex->yPos() });
	vertices_buffer_->push();

	n_vertices_       = map_->nVertices();
	vertices_updated_ = app::runTimer();
}
