
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
#include "App.h"
#include "FlatRenderer3D.h"
#include "General/ColourConfiguration.h"
#include "Geometry/Ray.h"
#include "MapEditor/Item.h"
#include "MapEditor/ItemSelection.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/Renderer/MCAnimations.h"
#include "MapEditor/Renderer/Overlays/LoadingOverlay.h"
#include "MapEditor/Renderer/Renderer.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/LineBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer3D.h"
#include "RenderPass.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/MapSpecials/PointLights.h"
#include "SLADEMap/SLADEMap.h"
#include "SLADEMap/Types.h"
#include "Skybox.h"
#include "ThingRenderer3D.h"
#include "WallRenderer3D.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Float, map3d_max_render_dist, 0, CVar::Flag::Save)
CVAR(Float, map3d_max_thing_dist, 0, CVar::Flag::Save)
CVAR(Bool, map3d_render_sky, true, CVar::Flag::Save)
CVAR(Float, map3d_brightness, 1, CVar::Flag::Save)
CVAR(Float, map3d_fog_density, 1, CVar::Flag::Save)
CVAR(Bool, map3d_fake_contrast, true, CVar::Flag::Save)
CVAR(Bool, map3d_highlight_enabled, true, CVar::Flag::Save)
CVAR(Bool, map3d_highlight_fill, true, CVar::Flag::Save)
CVAR(Bool, map3d_highlight_outline, false, CVar::Flag::Save)
CVAR(Bool, map3d_lights_enabled, true, CVar::Flag::Save)
CVAR(Int, map3d_lights_max, 128, CVar::Flag::Save)
CVAR(Float, map3d_lights_intensity, 1.0f, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns a list of point lights that are visible in the given [camera]'s
// frustum, sorted by distance
// -----------------------------------------------------------------------------
vector<map::PointLight> visiblePointLights(const SLADEMap& map, const gl::Camera& camera)
{
	vector<map::PointLight> visible_lights;

	// Get point lights from map specials
	for (const auto& pl : map.mapSpecials().pointLights())
	{
		// Check against camera frustum
		if (camera.sphereInFrustum(pl.position, pl.radius * 2))
			visible_lights.push_back(pl);
	}

	// Sort by distance from camera
	std::ranges::sort(
		visible_lights,
		[&camera](const map::PointLight& lhs, const map::PointLight& rhs)
		{ return glm::distance(lhs.position, camera.position()) < glm::distance(rhs.position, camera.position()); });

	return visible_lights;
}

// -----------------------------------------------------------------------------
// Sets up common shader uniforms for the given 3d geometry [shader]
// -----------------------------------------------------------------------------
void setupShaderUniforms(
	const gl::Shader& shader,
	const gl::Camera& camera,
	bool              fullbright,
	bool              fog,
	bool              sprite_shader,
	int               num_point_lights = 0)
{
	// Set ModelView/Projection matrix uniforms from camera
	shader.setUniform("modelview", camera.viewMatrix());
	shader.setUniform("projection", camera.projectionMatrix());

	// Set option uniforms
	shader.setUniform("fullbright", fullbright);
	shader.setUniform("fog_density", fog ? map3d_fog_density : 0.0f);
	shader.setUniform("brightness_mult", map3d_brightness.value);
	shader.setUniform("max_dist", map3d_max_render_dist > 0.0f ? map3d_max_render_dist : 40000.0f);
	shader.setUniform("colour", glm::vec4(1.0f));
	if (sprite_shader)
	{
		// Uniforms for sprite shader
		shader.setUniform("cam_right", static_cast<Vec3f>(camera.strafeVector()));
	}
	else
	{
		// Uniforms for non-sprite shaders
		shader.setUniform("fake_contrast", map3d_fake_contrast);
	}

	// Set point light count
	shader.setUniform("num_point_lights", num_point_lights);
	if (num_point_lights > 0)
		shader.setUniform("point_light_intensity", map3d_lights_intensity.value);
}

// -----------------------------------------------------------------------------
// Calculates a ray from the [camera] through the cursor position [cursor_pos]
// in world space
// -----------------------------------------------------------------------------
glm::vec3 calculateCursorRay(const gl::Camera& camera, const gl::View& view, const Vec2i& cursor_pos)
{
	// Convert cursor position to NDC (-1 to 1 range)
	auto viewport_size = view.size();
	auto ndc_x         = (2.0f * cursor_pos.x) / viewport_size.x - 1.0f;
	auto ndc_y         = 1.0f - (2.0f * cursor_pos.y) / viewport_size.y; // Flip Y

	// Create NDC points for near and far plane
	glm::vec4 ray_clip_near(ndc_x, ndc_y, -1.0f, 1.0f);
	glm::vec4 ray_clip_far(ndc_x, ndc_y, 1.0f, 1.0f);

	// Get inverse matrices
	auto inv_projection = glm::inverse(camera.projectionMatrix());
	auto inv_view       = glm::inverse(camera.viewMatrix());

	// Transform to view space
	auto ray_view_near = inv_projection * ray_clip_near;
	auto ray_view_far  = inv_projection * ray_clip_far;
	ray_view_near /= ray_view_near.w;
	ray_view_far /= ray_view_far.w;

	// Then to world space
	auto ray_world_near = inv_view * ray_view_near;
	auto ray_world_far  = inv_view * ray_view_far;

	// Calculate ray direction
	return normalize(
		glm::vec3(
			ray_world_far.x - ray_world_near.x,
			ray_world_far.y - ray_world_near.y,
			ray_world_far.z - ray_world_near.z));
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
MapRenderer3D::MapRenderer3D(MapEditContext* context, Renderer* renderer) :
	map_{ &context->map() },
	context_{ context },
	renderer_{ renderer }
{
	skybox_         = std::make_unique<Skybox>();
	thing_renderer_ = std::make_unique<ThingRenderer3D>(this);
	wall_renderer_  = std::make_unique<WallRenderer3D>(this);
	flat_renderer_  = std::make_unique<FlatRenderer3D>(this);
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
void MapRenderer3D::render(const gl::Camera& camera, const gl::View& view)
{
	// Clear to black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Create shaders if needed
	if (!shader_3d_)
	{
		auto max_point_lights = fmt::format("{}", MAX_POINT_LIGHTS);

		shader_3d_ = std::make_unique<gl::Shader>("map_3d");
		shader_3d_->define("MAX_POINT_LIGHTS", max_point_lights);
		shader_3d_->loadResourceEntries("map_geometry3d.vert", "map_geometry3d.frag");
		shader_3d_->bindUniformBlock("PointLightsBlock", 0);

		shader_3d_alphatest_ = std::make_unique<gl::Shader>("map_3d_alphatest");
		shader_3d_alphatest_->define("ALPHA_TEST");
		shader_3d_alphatest_->define("MAX_POINT_LIGHTS", max_point_lights);
		shader_3d_alphatest_->loadResourceEntries("map_geometry3d.vert", "map_geometry3d.frag");
		shader_3d_alphatest_->bindUniformBlock("PointLightsBlock", 0);

		shader_3d_sprite_ = std::make_unique<gl::Shader>("map_3d_sprite");
		shader_3d_sprite_->define("ALPHA_TEST");
		shader_3d_sprite_->define("SPRITE");
		shader_3d_sprite_->define("MAX_POINT_LIGHTS", max_point_lights);
		shader_3d_sprite_->loadResourceEntries("map_sprite3d.vert", "map_geometry3d.frag");
		shader_3d_sprite_->bindUniformBlock("PointLightsBlock", 0);

		shader_3d_icon_ = std::make_unique<gl::Shader>("map_3d_icon");
		shader_3d_icon_->define("CIRCLE_MASK");
		shader_3d_icon_->define("MAX_POINT_LIGHTS", max_point_lights);
		shader_3d_icon_->loadResourceEntries("map_sprite3d.vert", "map_geometry3d.frag");
		shader_3d_icon_->bindUniformBlock("PointLightsBlock", 0);
	}

	// Setup GL stuff
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GEQUAL);
	glClearDepth(0.0f);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	gl::setBlend(gl::Blend::Normal);

	// Build and upload the point lights UBO
	auto point_lights = visiblePointLights(*map_, camera);
	auto num_lights   = 0;
	if (!point_lights.empty() && map3d_lights_enabled)
	{
		num_lights = std::min(map3d_lights_max.value, static_cast<int>(point_lights.size()));
		PointLightsUBOData ubo_data{};
		for (int i = 0; i < num_lights; ++i)
		{
			const auto& pl                   = point_lights[i];
			ubo_data.lights[i].position_type = glm::vec4(glm::vec3(pl.position), static_cast<float>(pl.type));
			ubo_data.lights[i].colour_radius = glm::vec4(
				pl.r / 255.0f, pl.g / 255.0f, pl.b / 255.0f, static_cast<float>(pl.radius * 2));
		}
		point_lights_ubo_.upload(ubo_data);
		point_lights_ubo_.bindTo(0);
	}

	// Setup per-shader uniforms
	setupShaderUniforms(*shader_3d_, camera, fullbright_, fog_, false, num_lights);
	setupShaderUniforms(*shader_3d_alphatest_, camera, fullbright_, fog_, false, num_lights);
	setupShaderUniforms(*shader_3d_sprite_, camera, fullbright_, fog_, true, num_lights);
	setupShaderUniforms(*shader_3d_icon_, camera, fullbright_, fog_, true); // No lights on icons

	// Update visibility if needed
	if (map3d_max_render_dist > 0.0f)
	{
		flat_renderer_->updateVisibility(camera, map3d_max_render_dist);
		wall_renderer_->updateVisibility(camera, map3d_max_render_dist);
		thing_renderer_->updateVisibility(camera, map3d_max_render_dist);
	}
	else
		thing_renderer_->updateVisibility(camera, 0.0f);

	// Update flats and walls
	auto update_done = flat_renderer_->update(map3d_max_render_dist > 0.0f);
	update_done &= wall_renderer_->update(map3d_max_render_dist > 0.0f);
	update_done &= thing_renderer_->update(map3d_max_render_dist > 0.0f);
	if (update_done)
		context_->closeLoadingOverlay();
	else
	{
		auto total_progress = (flat_renderer_->updateProgress() + wall_renderer_->updateProgress()
							   + thing_renderer_->updateProgress())
							  / 3.0f;
		int progress_percent = static_cast<int>(total_progress * 100.0f);
		context_->loadingOverlay().setMessage(fmt::format("Building 3D geometry ({}%)...", progress_percent));
	}

	// Render sky first if needed
	shader_3d_->bind();
	if (map3d_render_sky)
	{
		// Stencil-mask the skybox to only sky flats/quads
		glEnable(GL_STENCIL_TEST);
		glClearStencil(0);
		glStencilMask(0xFF);
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		glDepthMask(GL_TRUE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		renderSkyFlatsQuads();
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glStencilMask(0x00);
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		skybox_->render(camera);

		glDisable(GL_STENCIL_TEST);
	}

	// First pass, render solid flats/walls
	flat_renderer_->render(*shader_3d_, RenderPass::Normal); // Flats
	wall_renderer_->render(*shader_3d_, RenderPass::Normal); // Walls
	if (!map3d_render_sky)
	{
		// Not rendering the skybox, render sky walls/flats as normal
		flat_renderer_->render(*shader_3d_, RenderPass::Sky); // Flats
		wall_renderer_->render(*shader_3d_, RenderPass::Sky); // Walls
	}

	// Second pass, render masked flats/walls and sprites
	shader_3d_alphatest_->bind();
	flat_renderer_->render(*shader_3d_alphatest_, RenderPass::Masked); // Flats
	wall_renderer_->render(*shader_3d_alphatest_, RenderPass::Masked); // Walls
	thing_renderer_->renderSprites(*shader_3d_sprite_, false);         // Sprites
	thing_renderer_->renderSprites(*shader_3d_icon_, true);            // Icons

	// Third pass, render transparent flats/walls (TODO: transparent sprites)
	shader_3d_->bind();
	glDepthMask(GL_FALSE);
	flat_renderer_->render(*shader_3d_, RenderPass::Transparent); // Flats
	wall_renderer_->render(*shader_3d_, RenderPass::Transparent); // Walls

	// Render thing boxes
	thing_renderer_->renderThingBoxes(camera, view, map3d_max_render_dist > 0.0f ? map3d_max_render_dist : 40000.0f);

	// Cleanup gl state
	glDepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

// -----------------------------------------------------------------------------
// Renders a highlight for the given [item] using the specified [camera] and
// [view]
// ----------------------------------------------------------------------------
void MapRenderer3D::renderHighlight(const Item& item, const gl::Camera& camera, const gl::View& view, float alpha)
{
	if (!highlight_enabled_ || !map3d_highlight_enabled || item.index < 0)
		return;

	// Determine item type and highlight colour
	auto& def       = colourconfig::colDef("map_3d_hilight");
	auto  hl_colour = def.colour.ampf(1.0f, 1.0f, 1.0f, alpha);
	auto  base_type = baseItemType(item.type);

	// Pass to thing renderer if thing
	if (base_type == ItemType::Thing)
	{
		gl::setBlend(def.blendMode());
		thing_renderer_->renderHighlight(item, camera, view, hl_colour, map3d_highlight_outline, map3d_highlight_fill);
		return;
	}

	// Create buffers if needed
	if (!highlight_lines_)
	{
		highlight_lines_ = std::make_unique<gl::LineBuffer>();
		highlight_lines_->setWidthMult(2.0f);

		highlight_fill_ = std::make_unique<gl::IndexBuffer>();
	}

	// Outline
	if (map3d_highlight_outline)
	{
		// Update line buffer
		highlight_lines_->buffer().clear();
		if (base_type == ItemType::Sector)
			flat_renderer_->addOutline(item, *highlight_lines_, 1.0f);
		else if (base_type == ItemType::Side)
			wall_renderer_->addOutline(item, *highlight_lines_, 1.0f);
		highlight_lines_->push();

		// Draw outline
		gl::setBlend(def.blendMode());
		highlight_lines_->draw(camera, view.size(), hl_colour);
	}

	// Fill (Wall/Flat)
	if (base_type != ItemType::Thing && map3d_highlight_fill)
	{
		// Setup fill buffer
		unsigned       fill_vao = 0;
		vector<GLuint> indices;
		if (base_type == ItemType::Sector)
		{
			flat_renderer_->addItemIndices(item, indices);
			fill_vao = flat_renderer_->vao();
		}
		else if (base_type == ItemType::Side)
		{
			wall_renderer_->addItemIndices(item, indices);
			fill_vao = wall_renderer_->vao();
		}

		// Draw fill (if any)
		if (fill_vao && !indices.empty())
		{
			highlight_fill_->upload(indices);

			// Setup shader
			shader_3d_->bind();
			shader_3d_->setUniform("modelview", camera.viewMatrix());
			shader_3d_->setUniform("projection", camera.projectionMatrix());
			shader_3d_->setUniform("fullbright", true);
			shader_3d_->setUniform("fog_density", 0.0f);
			shader_3d_->setUniform("colour", hl_colour.ampf(1.0f, 1.0f, 1.0f, 0.25f));
			shader_3d_->setUniform("fake_contrast", false);
			shader_3d_->setUniform("num_point_lights", 0);

			// Setup GL state
			glEnable(GL_DEPTH_TEST);

			// Draw filled polygon(s)
			gl::Texture::bind(gl::Texture::whiteTexture());
			gl::bindVAO(fill_vao);
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
	selection_overlay_updated_ = app::runTimer();
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
		overlay.fill_things.reset();
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
			flat_renderer_->addOutline(item, *overlay.outline, 1.0f);
			flat_renderer_->addItemIndices(item, indices_flats);
		}
		else if (base_type == ItemType::Side)
		{
			wall_renderer_->addOutline(item, *overlay.outline, 1.0f);
			wall_renderer_->addItemIndices(item, indices_quads);
		}
		else if (base_type == ItemType::Thing)
		{
			if (!overlay.fill_things)
				overlay.fill_things = std::make_unique<gl::VertexBuffer3D>();

			thing_renderer_->addToSelectionOverlay(overlay, item);
		}
	}
	overlay.outline->push();
	overlay.fill_flats->upload(indices_flats);
	overlay.fill_quads->upload(indices_quads);
	if (overlay.fill_things)
		overlay.fill_things->push();
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
	colour.a *= 0.25f;
	shader_3d_->setUniform("colour", colour);
	shader_3d_->setUniform("num_point_lights", 0);

	// Setup GL state
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Draw filled flats
	gl::Texture::bind(gl::Texture::whiteTexture());
	gl::bindVAO(flat_renderer_->vao());
	overlay.fill_flats->bind();
	gl::drawElements(gl::Primitive::Triangles, overlay.fill_flats->size(), GL_UNSIGNED_INT);

	// Draw filled quads
	gl::bindVAO(wall_renderer_->vao());
	overlay.fill_quads->bind();
	gl::drawElements(gl::Primitive::Triangles, overlay.fill_quads->size(), GL_UNSIGNED_INT);
	gl::bindEBO(0);
	gl::bindVAO(0);

	// Draw filled things
	if (overlay.fill_things)
		overlay.fill_things->draw();


	// Reset GL state
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

// -----------------------------------------------------------------------------
// Renders the current selection overlay (if any) using the given [camera] and
// [view]
// -----------------------------------------------------------------------------
void MapRenderer3D::renderSelection(const gl::Camera& camera, const gl::View& view)
{
	if (!selection_enabled_)
		return;

	// Refresh selection if anything has been updated since the last selection
	// overlay update
	if (map_->typeLastUpdated(map::ObjectType::Thing) > selection_overlay_updated_
		|| map_->geometryUpdated() > selection_overlay_updated_
		|| map_->sectorRenderInfoUpdated() > selection_overlay_updated_)
		updateSelection(context_->selection());

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
	flat_renderer_->clear();
	wall_renderer_->clear();

	// Things
	thing_renderer_->clear();

	// Selection/Highlight
	highlight_lines_.reset();
	highlight_fill_.reset();
}

// -----------------------------------------------------------------------------
// Finds the map item at the cursor position [cursor_pos] in the given [camera]
// and [view]
// -----------------------------------------------------------------------------
Item MapRenderer3D::findHighlightedItem(const gl::Camera& camera, const gl::View& view, const Vec2i& cursor_pos) const
{
	Ray ray{ camera.position(), calculateCursorRay(camera, view, cursor_pos) };

	float min_dist = 9999999.f;
	Item  current;

	// Check walls
	if (auto [wall, dist] = wall_renderer_->nearestIntersectingWall(ray, min_dist); dist >= 0)
	{
		current  = wall;
		min_dist = dist;
	}

	// Check flats
	if (auto [flat, dist] = flat_renderer_->nearestIntersectingFlat(ray, min_dist); dist >= 0)
	{
		current  = flat;
		min_dist = dist;
	}

	// Check things
	if (auto closest_thing = thing_renderer_->nearestIntersectingThing(camera, ray, min_dist); closest_thing)
		current = *closest_thing;

	return current;
}

// -----------------------------------------------------------------------------
// Returns the size of the flats vertex buffer in bytes
// -----------------------------------------------------------------------------
unsigned MapRenderer3D::flatsBufferSize() const
{
	return flat_renderer_->bufferSize();
}

// -----------------------------------------------------------------------------
// Returns the size of the quads vertex buffer in bytes
// -----------------------------------------------------------------------------
unsigned MapRenderer3D::quadsBufferSize() const
{
	return wall_renderer_->bufferSize();
}

// -----------------------------------------------------------------------------
// Renders sky flats and quads (if any) to fill the depth buffer where the sky
// is visible
// -----------------------------------------------------------------------------
void MapRenderer3D::renderSkyFlatsQuads() const
{
	gl::Texture::bind(gl::Texture::whiteTexture());
	shader_3d_->setUniform("colour", glm::vec4(0.0f));

	flat_renderer_->renderSkyGeometry();
	wall_renderer_->renderSkyGeometry();

	shader_3d_->setUniform("colour", glm::vec4(1.0f));

	gl::bindEBO(0);
	gl::bindVAO(0);
}
