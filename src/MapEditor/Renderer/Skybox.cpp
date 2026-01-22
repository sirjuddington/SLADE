
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Skybox.cpp
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
#include "Skybox.h"
#include "Geometry/Rect.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "OpenGL/Camera.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/Shader.h"
#include "OpenGL/VertexBuffer3D.h"
#include <glm/ext/matrix_transform.hpp>

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Skybox Class Functions
//
// -----------------------------------------------------------------------------

Skybox::Skybox()
{
	vertex_buffer_ = std::make_unique<gl::VertexBuffer3D>();

	// Generate sky circle positions
	double rot = 0;
	for (auto& pos : sky_circle_)
	{
		pos.x = sin(rot);
		pos.y = -cos(rot);
		rot -= (3.1415926535897932384626433832795 * 2) / 32.0;
	}
}

void Skybox::setSkyTextures(string_view tex1, string_view tex2)
{
	skytex1_ = tex1;
	skytex2_ = tex2;

	// Clear vertex buffer to force rebuild
	vertex_buffer_->buffer().clear();
}

void Skybox::render(const gl::Camera& camera, const gl::Shader& shader)
{
	// Build vertex buffer if needed
	if (vertex_buffer_->buffer().empty())
		buildVertexBuffer();

	// Setup GL state for skybox rendering
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Create model matrix to position skybox around camera
	// Translate to camera position (a bit below to center the skybox better)
	auto cam_pos = camera.position();
	auto model   = glm::translate(glm::mat4(1.0f), glm::vec3(cam_pos.x, cam_pos.y, cam_pos.z - 10.0f));
	auto mvp     = camera.projectionMatrix() * camera.viewMatrix() * model;
	shader.setUniform("mvp", mvp);

	// Render top cap
	gl::Texture::bind(gl::Texture::whiteTexture());
	shader.setUniform("colour", skycol_top_);
	vertex_buffer_->drawPartial(vertex_index_caps_, 6);

	// Render bottom cap
	shader.setUniform("colour", skycol_bottom_);
	vertex_buffer_->drawPartial(vertex_index_caps_ + 6, 6);

	// Render skybox sides
	shader.setUniform("colour", glm::vec4(1.0f));
	gl::Texture::bind(sky_tex_id_);
	vertex_buffer_->draw(gl::Primitive::Triangles, nullptr, nullptr, 0, vertex_index_caps_);

	// Restore GL state
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
}

void Skybox::buildSkySlice(float top, float bottom, float alpha_top, float alpha_bottom, float size, float tx, float ty)
{
	auto tc_x  = 0.0f;
	auto tc_y1 = (-top + 1.0f) * (ty * 0.5f);
	auto tc_y2 = (-bottom + 1.0f) * (ty * 0.5f);

	// Generate quads for each segment
	for (unsigned a = 0; a < 32; ++a)
	{
		vertex_buffer_->addQuad(
			// TL
			{ glm::vec3(sky_circle_[a].x * size, -sky_circle_[a].y * size, top * size),
			  glm::vec2(tc_x, tc_y1),
			  glm::vec4(1.0f, 1.0f, 1.0f, alpha_top),
			  glm::vec3(0.0f, 0.0f, 1.0f) },
			// TR
			{ glm::vec3(sky_circle_[(a + 1) % 32].x * size, -sky_circle_[(a + 1) % 32].y * size, top * size),
			  glm::vec2(tc_x + tx, tc_y1),
			  glm::vec4(1.0f, 1.0f, 1.0f, alpha_top),
			  glm::vec3(0.0f, 0.0f, 1.0f) },
			// BL
			{ glm::vec3(sky_circle_[a].x * size, -sky_circle_[a].y * size, bottom * size),
			  glm::vec2(tc_x, tc_y2),
			  glm::vec4(1.0f, 1.0f, 1.0f, alpha_bottom),
			  glm::vec3(0.0f, 0.0f, 1.0f) },
			// BR
			{ glm::vec3(sky_circle_[(a + 1) % 32].x * size, -sky_circle_[(a + 1) % 32].y * size, bottom * size),
			  glm::vec2(tc_x + tx, tc_y2),
			  glm::vec4(1.0f, 1.0f, 1.0f, alpha_bottom),
			  glm::vec3(0.0f, 0.0f, 1.0f) });

		tc_x += tx;
	}
}

void Skybox::buildVertexBuffer()
{
	vertex_buffer_->buffer().clear();

	// Get sky texture
	if (!skytex2_.empty())
		sky_tex_id_ = textureManager().texture(skytex2_, false).gl_id;
	else
		sky_tex_id_ = textureManager().texture(skytex1_, false).gl_id;

	if (sky_tex_id_ == 0)
		return;

	// Get average colour for top/bottom caps
	auto& tex_info = gl::Texture::info(sky_tex_id_);
	int   theight  = tex_info.size.y * 0.4;
	skycol_top_    = gl::Texture::averageColour(sky_tex_id_, { 0, 0, tex_info.size.x, theight });
	skycol_bottom_ = gl::Texture::averageColour(
		sky_tex_id_, { 0, tex_info.size.y - theight, tex_info.size.x, tex_info.size.y });

	// Check for odd sky sizes
	float tx = 0.125f;
	float ty = 2.0f;
	if (tex_info.size.x > 256)
		tx = 0.125f / (static_cast<float>(tex_info.size.x) / 256.0f);
	if (tex_info.size.y > 128)
		ty = 1.0f;

	// Build sky slices
	float size = 64.0f;
	buildSkySlice(1.0f, 0.5f, 0.0f, 1.0f, size, tx, ty);   // Top
	buildSkySlice(0.5f, -0.5f, 1.0f, 1.0f, size, tx, ty);  // Middle
	buildSkySlice(-0.5f, -1.0f, 1.0f, 0.0f, size, tx, ty); // Bottom

	// Add caps
	vertex_index_caps_ = vertex_buffer_->queueSize();
	vertex_buffer_->addQuad( // Top
		gl::Vertex3D{ { -size * 10.0f, -size * 10.0f, size }, {}, glm::vec4{ 1.0f } },
		gl::Vertex3D{ { size * 10.0f, -size * 10.0f, size }, {}, glm::vec4{ 1.0f } },
		gl::Vertex3D{ { -size * 10.0f, size * 10.0f, size }, {}, glm::vec4{ 1.0f } },
		gl::Vertex3D{ { size * 10.0f, size * 10.0f, size }, {}, glm::vec4{ 1.0f } });
	vertex_buffer_->addQuad( // Bottom
		gl::Vertex3D{ { -size * 10.0f, -size * 10.0f, -size }, {}, glm::vec4{ 1.0f } },
		gl::Vertex3D{ { size * 10.0f, -size * 10.0f, -size }, {}, glm::vec4{ 1.0f } },
		gl::Vertex3D{ { -size * 10.0f, size * 10.0f, -size }, {}, glm::vec4{ 1.0f } },
		gl::Vertex3D{ { size * 10.0f, size * 10.0f, -size }, {}, glm::vec4{ 1.0f } });

	vertex_buffer_->push();
}
