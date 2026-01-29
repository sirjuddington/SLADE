
#include "Main.h"
#include "MapRenderer3D.h"
#include "App.h"
#include "Flat3D.h"
#include "MapGeometry.h"
#include "MapGeometryBuffer3D.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/Shader.h"
#include "Quad3D.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/MapSpecials/MapSpecials.h"
#include "SLADEMap/SLADEMap.h"
#include "Skybox.h"
#include "Utility/Vector.h"

using namespace slade;


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
// MapRenderer3D Class Functions
//
// -----------------------------------------------------------------------------

MapRenderer3D::MapRenderer3D(SLADEMap* map) : map_{ map }
{
	vb_flats_ = std::make_unique<mapeditor::MapGeometryBuffer3D>();
	vb_quads_ = std::make_unique<mapeditor::MapGeometryBuffer3D>();
	skybox_   = std::make_unique<mapeditor::Skybox>();
}

MapRenderer3D::~MapRenderer3D() = default;

void MapRenderer3D::enableHilight(bool enable) {}
void MapRenderer3D::enableSelection(bool enable) {}

void MapRenderer3D::setSkyTexture(string_view tex1, string_view tex2) const
{
	skybox_->setSkyTextures(tex1, tex2);
}

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
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glAlphaFunc(GL_GREATER, 0.0f);

	// Render skybox first (before depth buffer is populated)
	if (render_3d_sky)
		skybox_->render(camera);

	// Set ModelView/Projection matrix uniforms from camera
	shader_3d_->setUniform("modelview", camera.viewMatrix());
	shader_3d_->setUniform("projection", camera.projectionMatrix());
	shader_3d_alphatest_->setUniform("modelview", camera.viewMatrix());
	shader_3d_alphatest_->setUniform("projection", camera.projectionMatrix());

	// Setup shader uniforms
	shader_3d_->setUniform("fullbright", fullbright_);
	shader_3d_->setUniform("fog_density", fog_ ? render_fog_density : 0.0f);
	shader_3d_alphatest_->setUniform("fullbright", fullbright_);
	shader_3d_alphatest_->setUniform("fog_density", fog_ ? render_fog_density : 0.0f);

	// Update flats and walls
	updateFlats();
	updateWalls();

	// Render sky flats/quads first if needed
	shader_3d_->bind();
	if (render_3d_sky)
		renderSkyFlatsQuads();

	// First pass, render solid flats/walls
	renderFlats(*shader_3d_, 0);
	renderWalls(*shader_3d_, 0);

	// Second pass, render alpha-tested flats/walls
	shader_3d_alphatest_->bind();
	renderFlats(*shader_3d_alphatest_, 1);
	renderWalls(*shader_3d_alphatest_, 1);

	// Third pass, render transparent flats/walls
	shader_3d_->bind();
	glDepthMask(GL_FALSE);
	renderFlats(*shader_3d_, 2);
	renderWalls(*shader_3d_, 2);

	// Cleanup gl state
	glDepthMask(GL_TRUE);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void MapRenderer3D::clearData()
{
	// Flats
	vb_flats_->buffer().clear();
	sector_flats_.clear();
	flat_groups_.clear();

	// Walls
	vb_quads_->buffer().clear();
	quads_.clear();
	quad_groups_.clear();
}

unsigned MapRenderer3D::flatsBufferSize() const
{
	return vb_flats_->buffer().size() * sizeof(mapeditor::MGVertex);
}

unsigned MapRenderer3D::quadsBufferSize() const
{
	return vb_quads_->buffer().size() * sizeof(mapeditor::MGVertex);
}

void MapRenderer3D::updateFlats()
{
	using FlatFlags = mapeditor::Flat3D::Flags;

	// Clear flats to be rebuilt if map geometry has been updated
	if (map_->geometryUpdated() > flats_updated_)
	{
		vb_flats_->buffer().clear();
		sector_flats_.clear();
		flat_groups_.clear();
	}

	// Generate flats if needed
	if (sector_flats_.empty())
	{
		unsigned vertex_index = 0;
		for (auto* sector : map_->sectors())
		{
			auto [flats, vertices] = mapeditor::generateSectorFlats(*sector, vertex_index);

			sector_flats_.push_back(
				{ .sector               = sector,
				  .flats                = flats,
				  .vertex_buffer_offset = vertex_index,
				  .updated_time         = app::runTimer() });

			vb_flats_->addVertices(vertices);
			vertex_index += vertices.size();
		}

		vb_flats_->push();
		flats_updated_ = app::runTimer();
		flat_groups_.clear();
	}
	else if (
		flats_updated_ < map_->typeLastUpdated(map::ObjectType::Sector)
		|| flats_updated_ < map_->mapSpecials().specialsLastUpdated()
		|| flats_updated_ < map_->sectorRenderInfoUpdated())
	{
		// Check for sectors that need an update
		bool updated = false;
		for (auto& sf : sector_flats_)
		{
			if (sf.updated_time >= sf.sector->modifiedTime() && sf.updated_time >= sf.sector->renderInfoLastUpdated())
				continue;

			// Build new flats/vertices
			auto flats_count               = sf.flats.size();
			auto [new_flats, new_vertices] = mapeditor::generateSectorFlats(*sf.sector, sf.vertex_buffer_offset);
			sf.flats                       = new_flats;

			// Update vertex buffer
			if (new_flats.size() <= flats_count)
			{
				// Same or fewer flats, just update existing vertex data
				vb_flats_->buffer().update(sf.vertex_buffer_offset, new_vertices);
			}
			else
			{
				// More flats than before, need to re-upload entire buffer

				// TODO: This will result in gaps in the buffer, either compact
				//       the buffer here or implement some way to track and
				//       reuse freed space later

				sf.vertex_buffer_offset = vb_flats_->buffer().size();
				vb_flats_->pull();                    // Pull data from GPU
				vb_flats_->addVertices(new_vertices); // Add new vertex data
				vb_flats_->push();                    // Push data back to GPU
			}

			// Set updated
			updated         = true;
			sf.updated_time = app::runTimer();
		}

		// Clear flat groups to be rebuilt if any flats were updated
		if (updated)
		{
			flats_updated_ = app::runTimer();
			flat_groups_.clear();
		}
	}

	// Generate flat groups if needed
	if (flat_groups_.empty())
	{
		// Build list of flats to process
		struct FlatToProcess
		{
			mapeditor::Flat3D* flat;
			bool               processed;
		};
		vector<FlatToProcess> flats_to_process;
		for (auto& sf : sector_flats_)
			for (auto& flat : sf.flats)
				flats_to_process.push_back({ .flat = &flat, .processed = false });

		for (auto& fp1 : flats_to_process)
		{
			if (fp1.processed)
				continue;

			// Build list of vertex indices for flats with matching texture/flags/etc
			vector<GLuint> indices;
			for (auto& fp2 : flats_to_process)
			{
				// Check for match
				if (fp2.processed || *fp1.flat != *fp2.flat)
					continue;

				// Add indices
				auto vi = fp2.flat->vertex_offset;
				while (vi < fp2.flat->vertex_offset + fp2.flat->sector->polygonVertices().size())
					indices.push_back(vi++);

				fp2.processed = true;
			}

			// Determine transparency type
			auto transparency = RenderGroup::Transparency::None;
			if (fp1.flat->hasFlag(FlatFlags::Additive))
				transparency = RenderGroup::Transparency::Additive;
			else if (fp1.flat->colour.a < 1.0f)
				transparency = RenderGroup::Transparency::Normal;

			// Add flat group for texture
			flat_groups_.push_back(
				{ .texture      = fp1.flat->texture,
				  .colour       = fp1.flat->colour,
				  .index_buffer = std::make_unique<gl::IndexBuffer>(),
				  .alpha_test   = transparency == RenderGroup::Transparency::None
								&& fp1.flat->hasFlag(FlatFlags::ExtraFloor),
				  .sky         = fp1.flat->hasFlag(FlatFlags::Sky),
				  .transparent = transparency });
			flat_groups_.back().index_buffer->upload(indices);

			fp1.processed = true;
		}
	}
}

void MapRenderer3D::updateWalls()
{
	using QuadFlags = mapeditor::Quad3D::Flags;

	// Clear walls to be rebuilt if map geometry has been updated
	if (map_->geometryUpdated() > quads_updated_)
	{
		vb_quads_->buffer().clear();
		quads_.clear();
		quad_groups_.clear();
	}

	// Generate wall quads if needed
	if (quads_.empty())
	{
		unsigned vertex_index = 0;
		for (auto* line : map_->lines())
		{
			auto [quads, vertices] = mapeditor::generateLineQuads(*line, vertex_index);
			vectorConcat(quads_, quads);
			vb_quads_->addVertices(vertices);
			vertex_index += vertices.size();
		}
		vb_quads_->push();
		quads_updated_ = app::runTimer();
	}

	// Generate quad groups if needed
	if (quad_groups_.empty())
	{
		vector<uint8_t> quads_processed(quads_.size());

		for (unsigned i = 0; i < quads_.size(); ++i)
		{
			if (quads_processed[i])
				continue;

			// Build list of vertex indices for quads with this texture+colour+flags
			vector<GLuint> indices;
			for (unsigned f = i; f < quads_.size(); ++f)
			{
				// Check texture match
				auto& quad = quads_[f];
				if (quads_processed[f] || quad.texture != quads_[i].texture)
					continue;

				// Check colour match
				if (quad.colour != quads_[i].colour)
					continue;

				// Check flags
				if (quad.flags != quads_[i].flags)
					continue;

				// Add indices
				auto vi = quad.vertex_offset;
				while (vi < quad.vertex_offset + 6)
					indices.push_back(vi++);

				quads_processed[f] = 1;
			}

			// Determine transparency type
			auto transparency = RenderGroup::Transparency::None;
			if (quads_[i].hasFlag(QuadFlags::Additive))
				transparency = RenderGroup::Transparency::Additive;
			else if (quads_[i].colour.a < 1.0f)
				transparency = RenderGroup::Transparency::Normal;

			// Add quad group for texture
			quad_groups_.push_back(
				{ .texture      = quads_[i].texture,
				  .colour       = quads_[i].colour,
				  .index_buffer = std::make_unique<gl::IndexBuffer>(),
				  .alpha_test   = transparency == RenderGroup::Transparency::None
								&& quads_[i].hasFlag(QuadFlags::MidTexture),
				  .sky         = quads_[i].hasFlag(QuadFlags::Sky),
				  .transparent = transparency });
			quad_groups_.back().index_buffer->upload(indices);

			quads_processed[i] = 1;
		}
	}
}

void MapRenderer3D::renderSkyFlatsQuads() const
{
	gl::Texture::bind(gl::Texture::whiteTexture());
	shader_3d_->setUniform("colour", glm::vec4(0.0f));

	// Render sky flats
	gl::bindVAO(vb_flats_->vao());
	for (auto& group : flat_groups_)
	{
		if (!group.sky)
			continue;

		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	// Render sky quads
	gl::bindVAO(vb_quads_->vao());
	for (auto& group : quad_groups_)
	{
		if (!group.sky)
			continue;

		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	shader_3d_->setUniform("colour", glm::vec4(1.0f));

	gl::bindEBO(0);
	gl::bindVAO(0);
}

void MapRenderer3D::renderFlats(gl::Shader& shader, int pass) const
{
	gl::bindVAO(vb_flats_->vao());

	for (auto& group : flat_groups_)
	{
		// Ensure group is part of the correct render pass
		if (group.transparent != RenderGroup::Transparency::None && pass != 2)
			continue;
		if (group.alpha_test && pass == 0)
			continue;

		// Ignore sky flats if sky rendering is enabled
		if (render_3d_sky && group.sky)
			continue;

		// Setup blending if needed
		if (pass == 2)
		{
			if (group.transparent == RenderGroup::Transparency::Additive)
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

void MapRenderer3D::renderWalls(gl::Shader& shader, int pass) const
{
	gl::bindVAO(vb_quads_->vao());

	// First render non-alpha-tested quads
	for (auto& group : quad_groups_)
	{
		// Ensure group is part of the correct render pass
		if (group.transparent != RenderGroup::Transparency::None && pass != 2)
			continue;
		if (group.alpha_test && pass == 0)
			continue;

		// Ignore sky quads if sky rendering is enabled
		if (render_3d_sky && group.sky)
			continue;

		// Setup blending if needed
		if (pass == 2)
		{
			if (group.transparent == RenderGroup::Transparency::Additive)
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
