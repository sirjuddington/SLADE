
#include "Main.h"

#include "App.h"
#include "Flat3D.h"
#include "MapGeometry.h"
#include "MapRenderer3D.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer3D.h"
#include "OpenGL/View.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObjectList/SectorList.h"
#include "SLADEMap/SLADEMap.h"
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

MapRenderer3D::MapRenderer3D(SLADEMap* map) : map_{ map }, vb_flats_{ new gl::VertexBuffer3D } {}
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

void MapRenderer3D::render(const gl::Camera& camera)
{
	// Create default 3d shader if needed
	if (!shader_3d_)
	{
		shader_3d_ = std::make_unique<gl::Shader>("map_3d");
		shader_3d_->loadResourceEntries("default3d.vert", "default3d.frag");
	}

	// Set ModelViewProjection matrix uniform from camera
	shader_3d_->setUniform("mvp", camera.projectionMatrix() * camera.viewMatrix());

	// Setup GL stuff
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glAlphaFunc(GL_GREATER, 0.0f);

	renderFlats();

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
	return vb_flats_->buffer().size() * (sizeof(gl::Vertex3D) + sizeof(glm::vec4));
}

void MapRenderer3D::renderFlats()
{
	// Clear flats to be rebuilt if map geometry has been updated
	if (map_->geometryUpdated() > flats_updated_)
	{
		vb_flats_->buffer().clear();
		flats_.clear();
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

	// Render flats
	for (auto& flat : flats_)
	{
		if (flat.updated_time < flat.sector->modifiedTime())
		{
			vector<gl::Vertex3D> vertices;
			mapeditor::updateFlat(flat, vertices);
			vb_flats_->buffer().update(flat.vertex_offset, vertices);
		}

		gl::Texture::bind(flat.texture);
		shader_3d_->setUniform("colour", flat.colour);
		vb_flats_->draw(gl::Primitive::Triangles, nullptr, nullptr, flat.vertex_offset, flat.vertex_count);
	}
}
