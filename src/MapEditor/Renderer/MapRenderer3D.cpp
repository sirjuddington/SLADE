
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapRenderer3D.cpp
// Description: MapRenderer3D class - handles all rendering related stuff for
//              3d mode.
//
//              Vertex data for all flats and walls are stored in vertex buffers
//              (one for all flats, one for all quads), which are updated when
//              the map geometry is changed as needed.
//              This is then split into 'render groups' which group together
//              flats/quads with the same textures and properties to reduce the
//              number of texture binds and draw calls required.
//              Each render group contains an index buffer into the relevant
//              vertex buffer, used to render everything in the group with a
//              single draw call.
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
#include "MapRenderer3D.h"
#include "Flat3D.h"
#include "General/ColourConfiguration.h"
#include "MCAnimations.h"
#include "MapEditor/Item.h"
#include "MapEditor/ItemSelection.h"
#include "MapGeometryBuffer3D.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "Quad3D.h"
#include "Renderer.h"
#include "Skybox.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Float, render_max_dist, 2000, CVar::Flag::Save)
CVAR(Float, render_max_thing_dist, 2000, CVar::Flag::Save)
CVAR(Int, render_thing_icon_size, 16, CVar::Flag::Save)
CVAR(Bool, render_max_dist_adaptive, false, CVar::Flag::Save)
CVAR(Int, render_adaptive_ms, 15, CVar::Flag::Save)
CVAR(Bool, render_3d_sky, true, CVar::Flag::Save)
CVAR(Int, render_3d_things, 1, CVar::Flag::Save)
CVAR(Int, render_3d_things_style, 1, CVar::Flag::Save)
CVAR(Int, render_3d_hilight, 1, CVar::Flag::Save)
CVAR(Float, render_3d_brightness, 1, CVar::Flag::Save)
CVAR(Float, render_fog_density, 1, CVar::Flag::Save)
CVAR(Bool, render_shade_orthogonal_lines, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Sets up common shader uniforms for the given 3d geometry [shader]
// -----------------------------------------------------------------------------
void setupShaderUniforms(const gl::Shader& shader, const gl::Camera& camera, bool fullbright, bool fog)
{
	// Set ModelView/Projection matrix uniforms from camera
	shader.setUniform("modelview", camera.viewMatrix());
	shader.setUniform("projection", camera.projectionMatrix());

	// Set option uniforms
	shader.setUniform("fullbright", fullbright);
	shader.setUniform("fog_density", fog ? render_fog_density : 0.0f);
	shader.setUniform("brightness_mult", render_3d_brightness.value);
	shader.setUniform("fake_contrast", render_shade_orthogonal_lines);
}
} // namespace


// -----------------------------------------------------------------------------
//
// MapRenderer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapRenderer3D class constructor
// -----------------------------------------------------------------------------
MapRenderer3D::MapRenderer3D(SLADEMap* map, Renderer* renderer) : map_{ map }, renderer_{ renderer }
{
	vb_flats_ = std::make_unique<MapGeometryBuffer3D>();
	vb_quads_ = std::make_unique<MapGeometryBuffer3D>();
	skybox_   = std::make_unique<Skybox>();
}

// -----------------------------------------------------------------------------
// MapRenderer3D class destructor
// -----------------------------------------------------------------------------
MapRenderer3D::~MapRenderer3D() = default;

// -----------------------------------------------------------------------------
// Sets the skybox textures to [tex1] and [tex2]
// -----------------------------------------------------------------------------
void MapRenderer3D::setSkyTexture(string_view tex1, string_view tex2) const
{
	skybox_->setSkyTextures(tex1, tex2);
}

// -----------------------------------------------------------------------------
// Renders the 3d view from the given [camera]'s perspective
// -----------------------------------------------------------------------------
void MapRenderer3D::render(const gl::Camera& camera)
{
	// Create shaders if needed
	if (!shader_3d_)
	{
		shader_3d_ = std::make_unique<gl::Shader>("map_3d");
		shader_3d_->loadResourceEntries("map_geometry3d.vert", "map_geometry3d.frag");
	}
	if (!shader_3d_alphatest_)
	{
		shader_3d_alphatest_ = std::make_unique<gl::Shader>("map_3d_alphatest");
		shader_3d_alphatest_->define("ALPHA_TEST");
		shader_3d_alphatest_->loadResourceEntries("map_geometry3d.vert", "map_geometry3d.frag");
	}

	// Setup GL stuff
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GEQUAL);
	glClearDepth(0.0f);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glAlphaFunc(GL_GREATER, 0.0f);
	gl::setBlend(gl::Blend::Normal);

	// Render skybox first (before depth buffer is populated)
	if (render_3d_sky)
		skybox_->render(camera);

	// Setup shaders
	setupShaderUniforms(*shader_3d_, camera, fullbright_, fog_);
	setupShaderUniforms(*shader_3d_alphatest_, camera, fullbright_, fog_);

	// Update flats and walls
	updateFlats();
	updateWalls();

	// Render sky flats/quads first if needed
	shader_3d_->bind();
	if (render_3d_sky)
		renderSkyFlatsQuads();

	// First pass, render solid flats/walls
	renderGroups(*vb_flats_, flat_groups_, *shader_3d_, RenderPass::Normal); // Flats
	renderGroups(*vb_quads_, quad_groups_, *shader_3d_, RenderPass::Normal); // Walls
	if (!render_3d_sky)
	{
		// Not rendering the skybox, render sky walls/flats as normal
		renderGroups(*vb_flats_, flat_groups_, *shader_3d_, RenderPass::Sky); // Flats
		renderGroups(*vb_quads_, quad_groups_, *shader_3d_, RenderPass::Sky); // Walls
	}

	// Second pass, render masked flats/walls
	shader_3d_alphatest_->bind();
	renderGroups(*vb_flats_, flat_groups_, *shader_3d_alphatest_, RenderPass::Masked); // Flats
	renderGroups(*vb_quads_, quad_groups_, *shader_3d_alphatest_, RenderPass::Masked); // Walls

	// Third pass, render transparent flats/walls
	shader_3d_->bind();
	glDepthMask(GL_FALSE);
	renderGroups(*vb_flats_, flat_groups_, *shader_3d_, RenderPass::Transparent); // Flats
	renderGroups(*vb_quads_, quad_groups_, *shader_3d_, RenderPass::Transparent); // Walls

	// Cleanup gl state
	glDepthMask(GL_TRUE);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

// -----------------------------------------------------------------------------
// Renders a highlight for the given [item] using the specified [camera] and
// [view]
// ----------------------------------------------------------------------------
void MapRenderer3D::renderHighlight(const Item& item, const gl::Camera& camera, const gl::View& view, float alpha)
{
	if (!highlight_enabled_ || render_3d_hilight == 0)
		return;

	// Create buffers if needed
	if (!highlight_lines_)
	{
		highlight_lines_ = std::make_unique<gl::LineBuffer>();
		highlight_lines_->setWidthMult(2.0f);

		highlight_fill_ = std::make_unique<gl::IndexBuffer>();
	}

	// Determine item type and highlight colour
	auto  base_type = baseItemType(item.type);
	auto& def       = colourconfig::colDef("map_3d_hilight");
	auto  hl_colour = def.colour.ampf(1.0f, 1.0f, 1.0f, alpha);

	// Outline
	if (render_3d_hilight == 1 || render_3d_hilight == 2)
	{
		// Update line buffer
		highlight_lines_->buffer().clear();
		if (base_type == ItemType::Sector)
			addFlatOutline(item, *highlight_lines_, 1.0f);
		else if (base_type == ItemType::Side)
			addQuadOutline(item, *highlight_lines_, 1.0f);
		highlight_lines_->push();

		// Draw outline
		gl::setBlend(def.blendMode());
		highlight_lines_->draw(camera, view.size(), hl_colour);
	}

	// Fill
	if (render_3d_hilight == 2 || render_3d_hilight == 3)
	{
		// Setup fill buffer
		MapGeometryBuffer3D* vertex_buffer = nullptr;
		vector<GLuint>       indices;
		if (base_type == ItemType::Sector)
		{
			addItemFlatIndices(item, indices);
			vertex_buffer = vb_flats_.get();
		}
		else if (base_type == ItemType::Side)
		{
			addItemQuadIndices(item, indices);
			vertex_buffer = vb_quads_.get();
		}

		// Draw fill (if any)
		if (vertex_buffer && !indices.empty())
		{
			highlight_fill_->upload(indices);

			// Setup shader
			shader_3d_->bind();
			shader_3d_->setUniform("modelview", camera.viewMatrix());
			shader_3d_->setUniform("projection", camera.projectionMatrix());
			shader_3d_->setUniform("fullbright", true);
			shader_3d_->setUniform("fog_density", 0.0f);
			shader_3d_->setUniform("colour", hl_colour.ampf(1.0f, 1.0f, 1.0f, render_3d_hilight == 2 ? 0.15f : 0.25f));

			// Setup GL state
			glEnable(GL_DEPTH_TEST);

			// Draw filled polygon(s)
			gl::Texture::bind(gl::Texture::whiteTexture());
			gl::bindVAO(vertex_buffer->vao());
			highlight_fill_->bind();
			gl::drawElements(gl::Primitive::Triangles, highlight_fill_->size(), GL_UNSIGNED_INT);
			gl::bindEBO(0);
			gl::bindVAO(0);
		}
	}

	// Reset GL state
	gl::setBlend(gl::Blend::Normal);
	glDisable(GL_DEPTH_TEST);
}

// -----------------------------------------------------------------------------
// Updates the selection overlay with the given [seletion]
// -----------------------------------------------------------------------------
void MapRenderer3D::updateSelection(const ItemSelection& selection)
{
	populateSelectionOverlay(selection_overlay_, selection.selectedItems());
}

// -----------------------------------------------------------------------------
// Populates the given selection [overlay] with the vertex/index data for the
// given list of [items]
// -----------------------------------------------------------------------------
void MapRenderer3D::populateSelectionOverlay(SelectionOverlay3D& overlay, const vector<Item>& items) const
{
	if (items.empty())
	{
		// No selection, clear buffers
		overlay.outline.reset();
		overlay.fill_flats.reset();
		overlay.fill_quads.reset();
		return;
	}

	// (Re-)Create buffers
	overlay.outline = std::make_unique<gl::LineBuffer>();
	overlay.outline->setWidthMult(1.5f);
	overlay.fill_flats = std::make_unique<gl::IndexBuffer>();
	overlay.fill_quads = std::make_unique<gl::IndexBuffer>();

	// Update buffers
	overlay.outline->buffer().clear();
	vector<GLuint> indices_flats;
	vector<GLuint> indices_quads;
	for (const auto& item : items)
	{
		auto base_type = baseItemType(item.type);
		if (base_type == ItemType::Sector)
		{
			addFlatOutline(item, *overlay.outline, 1.0f);
			addItemFlatIndices(item, indices_flats);
		}
		else if (base_type == ItemType::Side)
		{
			addQuadOutline(item, *overlay.outline, 1.0f);
			addItemQuadIndices(item, indices_quads);
		}
	}
	overlay.outline->push();
	overlay.fill_flats->upload(indices_flats);
	overlay.fill_quads->upload(indices_quads);
}

// -----------------------------------------------------------------------------
// Renders a selection [overlay] using the given [camera] and [view], with the
// specified [colour] and [alpha] transparency
// -----------------------------------------------------------------------------
void MapRenderer3D::renderSelectionOverlay(
	const gl::Camera&         camera,
	const gl::View&           view,
	const SelectionOverlay3D& overlay,
	glm::vec4                 colour,
	float                     alpha) const
{
	if (!overlay.outline)
		return;

	// Setup colour
	colour.a *= alpha;

	// Draw outline
	overlay.outline->draw(camera, view.size(), colour);

	// Draw fill

	// Setup shader
	shader_3d_->bind();
	shader_3d_->setUniform("modelview", camera.viewMatrix());
	shader_3d_->setUniform("projection", camera.projectionMatrix());
	shader_3d_->setUniform("fullbright", true);
	shader_3d_->setUniform("fog_density", 0.0f);
	colour.a *= 0.15f;
	shader_3d_->setUniform("colour", colour);

	// Setup GL state
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Draw filled flats
	gl::Texture::bind(gl::Texture::whiteTexture());
	gl::bindVAO(vb_flats_->vao());
	overlay.fill_flats->bind();
	gl::drawElements(gl::Primitive::Triangles, overlay.fill_flats->size(), GL_UNSIGNED_INT);

	// Draw filled quads
	gl::bindVAO(vb_quads_->vao());
	overlay.fill_quads->bind();
	gl::drawElements(gl::Primitive::Triangles, overlay.fill_quads->size(), GL_UNSIGNED_INT);
	gl::bindEBO(0);
	gl::bindVAO(0);


	// Reset GL state
	glDepthFunc(GL_LESS);
	glDisable(GL_DEPTH_TEST);
}

// -----------------------------------------------------------------------------
// Renders the current selection overlay (if any) using the given [camera] and
// [view]
// -----------------------------------------------------------------------------
void MapRenderer3D::renderSelection(const gl::Camera& camera, const gl::View& view) const
{
	if (!selection_enabled_)
		return;

	// Render selection overlay in selection colour
	auto& def = colourconfig::colDef("map_3d_selection");
	gl::setBlend(def.blendMode());
	renderSelectionOverlay(camera, view, selection_overlay_, def.colour);
	gl::setBlend(gl::Blend::Normal);
}

// -----------------------------------------------------------------------------
// Clears all geometry data (flats/quads) from the renderer
// -----------------------------------------------------------------------------
void MapRenderer3D::clearData()
{
	// Flats
	vb_flats_->buffer().clear();
	sector_flats_.clear();
	flat_groups_.clear();

	// Walls
	vb_quads_->buffer().clear();
	line_quads_.clear();
	quad_groups_.clear();

	// Selection/Highlight
	highlight_lines_.reset();
	highlight_fill_.reset();
}

// -----------------------------------------------------------------------------
// Returns the size of the flats vertex buffer in bytes
// -----------------------------------------------------------------------------
unsigned MapRenderer3D::flatsBufferSize() const
{
	return vb_flats_->buffer().size() * sizeof(MGVertex);
}

// -----------------------------------------------------------------------------
// Returns the size of the quads vertex buffer in bytes
// -----------------------------------------------------------------------------
unsigned MapRenderer3D::quadsBufferSize() const
{
	return vb_quads_->buffer().size() * sizeof(MGVertex);
}

// -----------------------------------------------------------------------------
// Renders sky flats and quads (if any) to fill the depth buffer where the sky
// is visible
// -----------------------------------------------------------------------------
void MapRenderer3D::renderSkyFlatsQuads() const
{
	gl::Texture::bind(gl::Texture::whiteTexture());
	shader_3d_->setUniform("colour", glm::vec4(0.0f));

	// Render sky flats
	gl::bindVAO(vb_flats_->vao());
	for (auto& group : flat_groups_)
	{
		if (group.render_pass != RenderPass::Sky)
			continue;

		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	// Render sky quads
	gl::bindVAO(vb_quads_->vao());
	for (auto& group : quad_groups_)
	{
		if (group.render_pass != RenderPass::Sky)
			continue;

		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	shader_3d_->setUniform("colour", glm::vec4(1.0f));

	gl::bindEBO(0);
	gl::bindVAO(0);
}

// -----------------------------------------------------------------------------
// Renders the given [groups] of geometry from the specified [buffer] using the
// given [shader].
// Only groups that match the specified render [pass] will be rendered
// -----------------------------------------------------------------------------
void MapRenderer3D::renderGroups(
	const MapGeometryBuffer3D& buffer,
	const vector<RenderGroup>& groups,
	const gl::Shader&          shader,
	RenderPass                 pass)
{
	gl::bindVAO(buffer.vao());

	for (auto& group : groups)
	{
		// Ensure group is part of the correct render pass
		if (group.render_pass != pass)
			continue;

		// Setup blending if needed
		if (pass == RenderPass::Transparent)
		{
			if (group.trans_additive)
				gl::setBlend(gl::Blend::Additive);
			else
				gl::setBlend(gl::Blend::Normal);
		}

		shader.setUniform("colour", group.colour);
		gl::Texture::bind(group.texture);
		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	gl::bindEBO(0);
	gl::bindVAO(0);
}
