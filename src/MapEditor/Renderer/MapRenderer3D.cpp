
#include "Main.h"
#include "MapRenderer3D.h"
#include "App.h"
#include "Flat3D.h"
#include "MapGeometry.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/IndexBuffer.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer3D.h"
#include "Quad3D.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObjectList/LineList.h"
#include "SLADEMap/MapObjectList/SectorList.h"
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
CVAR(Bool, render_fog_quality, true, CVar::Flag::Save)
CVAR(Bool, render_max_dist_adaptive, false, CVar::Flag::Save)
CVAR(Int, render_adaptive_ms, 15, CVar::Flag::Save)
CVAR(Bool, render_3d_sky, true, CVar::Flag::Save)
CVAR(Int, render_3d_things, 1, CVar::Flag::Save)
CVAR(Int, render_3d_things_style, 1, CVar::Flag::Save)
CVAR(Int, render_3d_hilight, 1, CVar::Flag::Save)
CVAR(Float, render_3d_brightness, 1, CVar::Flag::Save)
CVAR(Float, render_fog_distance, 1500, CVar::Flag::Save)
CVAR(Bool, render_fog_new_formula, true, CVar::Flag::Save)
CVAR(Bool, render_shade_orthogonal_lines, true, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// MapRenderer3D Class Functions
//
// -----------------------------------------------------------------------------

MapRenderer3D::MapRenderer3D(SLADEMap* map) : map_{ map }
{
	vb_flats_ = std::make_unique<gl::VertexBuffer3D>();
	vb_quads_ = std::make_unique<gl::VertexBuffer3D>();
	skybox_   = std::make_unique<mapeditor::Skybox>();
}

MapRenderer3D::~MapRenderer3D() = default;

bool MapRenderer3D::fogEnabled() const
{
	return false;
}

bool MapRenderer3D::fullbrightEnabled() const
{
	return false;
}

void MapRenderer3D::enableHilight(bool enable) {}
void MapRenderer3D::enableSelection(bool enable) {}
void MapRenderer3D::enableFog(bool enable) {}
void MapRenderer3D::enableFullbright(bool enable) {}

void MapRenderer3D::setSkyTexture(string_view tex1, string_view tex2)
{
	skybox_->setSkyTextures(tex1, tex2);
}

void MapRenderer3D::render(const gl::Camera& camera)
{
	// Create shaders if needed
	if (!shader_3d_)
	{
		shader_3d_ = std::make_unique<gl::Shader>("map_3d");
		shader_3d_->define("TEXTURED");
		shader_3d_->loadResourceEntries("default3d.vert", "default3d.frag");
	}
	if (!shader_3d_alphatest_)
	{
		shader_3d_alphatest_ = std::make_unique<gl::Shader>("map_3d_alphatest");
		shader_3d_alphatest_->define("TEXTURED");
		shader_3d_alphatest_->define("ALPHA_TEST");
		shader_3d_alphatest_->loadResourceEntries("default3d.vert", "default3d.frag");
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
		skybox_->render(camera, *shader_3d_);

	// Set ModelViewProjection matrix uniform from camera
	auto mvp = camera.projectionMatrix() * camera.viewMatrix();
	shader_3d_->setUniform("mvp", mvp);
	shader_3d_alphatest_->setUniform("mvp", mvp);

	// Update flats and walls
	updateFlats();
	updateWalls();

	// Render sky flats/quads first if needed
	shader_3d_->bind();
	if (render_3d_sky)
		renderSkyFlatsQuads();

	// Render flats and walls (normal first, then alpha-tested)
	renderFlats(false);
	renderWalls(false);
	shader_3d_alphatest_->bind();
	renderFlats(true);
	renderWalls(true);

	// Cleanup gl state
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void MapRenderer3D::clearData()
{
	vb_flats_->buffer().clear();
	flats_.clear();
}

unsigned MapRenderer3D::flatsBufferSize() const
{
	return vb_flats_->buffer().size() * sizeof(gl::Vertex3D);
}

unsigned MapRenderer3D::quadsBufferSize() const
{
	return vb_quads_->buffer().size() * sizeof(gl::Vertex3D);
}

void MapRenderer3D::updateFlats()
{
	// Clear flats to be rebuilt if map geometry has been updated
	if (map_->geometryUpdated() > flats_updated_)
	{
		vb_flats_->buffer().clear();
		flats_.clear();
		flat_groups_.clear();
	}

	// Generate flats if needed
	if (flats_.empty())
	{
		unsigned vertex_index = 0;
		for (auto* sector : map_->sectors())
		{
			auto [flats, vertices] = mapeditor::generateSectorFlats(*sector, vertex_index);

			vectorConcat(flats_, flats);
			vb_flats_->add(vertices);
			vertex_index += vertices.size();
		}

		vb_flats_->push();
		flats_updated_ = app::runTimer();
	}
	else if (flats_updated_ < map_->typeLastUpdated(map::ObjectType::Sector))
	{
		// Check for flats that need an update
		bool updated = false;
		for (auto& flat : flats_)
		{
			if (flat.updated_time < flat.sector->modifiedTime())
			{
				vector<gl::Vertex3D> vertices;
				updateFlat(flat, vertices);
				vb_flats_->buffer().update(flat.vertex_offset, vertices);
				updated = true;
			}
		}

		// Rebuild flat groups if any flats were updated
		if (updated)
		{
			flats_updated_ = app::runTimer();
			flat_groups_.clear();
		}
	}

	// Generate flat groups if needed
	if (flat_groups_.empty())
	{
		vector<uint8_t> flats_processed(flats_.size());

		for (unsigned i = 0; i < flats_.size(); ++i)
		{
			if (flats_processed[i])
				continue;

			// Build list of vertex indices for flats with this texture+flags
			vector<GLuint> indices;
			for (unsigned f = i; f < flats_.size(); ++f)
			{
				// Check texture match
				auto& flat = flats_[f];
				if (flats_processed[f] || flat.texture != flats_[i].texture)
					continue;

				// Check flags
				if (flat.flags != flats_[i].flags)
					continue;

				// Add indices
				auto vi = flat.vertex_offset;
				while (vi < flat.vertex_offset + flat.sector->polygonVertices().size())
					indices.push_back(vi++);

				flats_processed[f] = 1;
			}

			// Add flat group for texture
			flat_groups_.emplace_back();
			flat_groups_.back().texture      = flats_[i].texture;
			flat_groups_.back().sky          = flats_[i].hasFlag(mapeditor::Flat3D::Flags::Sky);
			flat_groups_.back().alpha_test   = flats_[i].hasFlag(mapeditor::Flat3D::Flags::ExtraFloor);
			flat_groups_.back().index_buffer = std::make_unique<gl::IndexBuffer>();
			flat_groups_.back().index_buffer->upload(indices);

			flats_processed[i] = 1;
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
			vb_quads_->add(vertices);
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

			// Build list of vertex indices for quads with this texture+flags
			vector<GLuint> indices;
			for (unsigned f = i; f < quads_.size(); ++f)
			{
				// Check texture match
				auto& quad = quads_[f];
				if (quads_processed[f] || quad.texture != quads_[i].texture)
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

			// Add quad group for texture
			quad_groups_.emplace_back();
			quad_groups_.back().texture      = quads_[i].texture;
			quad_groups_.back().alpha_test   = quads_[i].hasFlag(QuadFlags::MidTexture);
			quad_groups_.back().sky          = quads_[i].hasFlag(QuadFlags::Sky);
			quad_groups_.back().index_buffer = std::make_unique<gl::IndexBuffer>();
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

void MapRenderer3D::renderFlats(bool alpha_tested) const
{
	gl::bindVAO(vb_flats_->vao());

	for (auto& group : flat_groups_)
	{
		if (group.alpha_test != alpha_tested)
			continue;

		// Ignore sky flats if sky rendering is enabled
		if (render_3d_sky && group.sky)
			continue;

		gl::Texture::bind(group.texture);
		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	gl::bindEBO(0);
	gl::bindVAO(0);
}

void MapRenderer3D::renderWalls(bool alpha_tested) const
{
	gl::bindVAO(vb_quads_->vao());

	// First render non-alpha-tested quads
	for (auto& group : quad_groups_)
	{
		if (group.alpha_test != alpha_tested)
			continue;

		// Ignore sky quads if sky rendering is enabled
		if (render_3d_sky && group.sky)
			continue;

		gl::Texture::bind(group.texture);
		group.index_buffer->bind();
		gl::drawElements(gl::Primitive::Triangles, group.index_buffer->size(), GL_UNSIGNED_INT);
	}

	gl::bindEBO(0);
	gl::bindVAO(0);
}
