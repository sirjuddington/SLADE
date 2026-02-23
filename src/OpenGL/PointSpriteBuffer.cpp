
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PointSpriteBuffer.cpp
// Description: PointSpriteBuffer class - buffer for rendering point sprites
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
#include "PointSpriteBuffer.h"
#include "Shader.h"
#include "View.h"

using namespace slade;
using namespace gl;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
unique_ptr<Shader> shader_ps_textured;
unique_ptr<Shader> shader_ps_circle;
unique_ptr<Shader> shader_ps_circle_outline;
unique_ptr<Shader> shader_ps_rsquare;
unique_ptr<Shader> shader_ps_rsquare_outline;
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Loads the shaders used for point sprites
// -----------------------------------------------------------------------------
void loadShaders()
{
	shader_ps_textured = std::make_unique<Shader>("ps_textured");
	shader_ps_textured->define("TEXTURED");
	shader_ps_textured->loadResourceEntries("point_sprite.vert", "default2d.frag");

	shader_ps_circle = std::make_unique<Shader>("ps_circle");
	shader_ps_circle->loadResourceEntries("point_sprite.vert", "circle.frag");

	shader_ps_circle_outline = std::make_unique<Shader>("ps_circle_outline");
	shader_ps_circle_outline->define("OUTLINE");
	shader_ps_circle_outline->loadResourceEntries("point_sprite.vert", "circle.frag");

	shader_ps_rsquare = std::make_unique<Shader>("ps_rsquare");
	shader_ps_rsquare->loadResourceEntries("point_sprite.vert", "rounded_square.frag");

	shader_ps_rsquare_outline = std::make_unique<Shader>("ps_rsquare_outline");
	shader_ps_rsquare_outline->define("OUTLINE");
	shader_ps_rsquare_outline->loadResourceEntries("point_sprite.vert", "rounded_square.frag");
}

// -----------------------------------------------------------------------------
// Returns the shader for rendering point sprites of the specified [type]
// -----------------------------------------------------------------------------
Shader* pointSpriteShader(PointSpriteType type)
{
	if (!shader_ps_textured)
		loadShaders();

	switch (type)
	{
	case PointSpriteType::Textured:             return shader_ps_textured.get();
	case PointSpriteType::Circle:               return shader_ps_circle.get();
	case PointSpriteType::CircleOutline:        return shader_ps_circle_outline.get();
	case PointSpriteType::RoundedSquare:        return shader_ps_rsquare.get();
	case PointSpriteType::RoundedSquareOutline: return shader_ps_rsquare_outline.get();
	default:                                    return shader_ps_textured.get();
	}
}
} // namespace


// -----------------------------------------------------------------------------
//
// PointSpriteBuffer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PointSpriteBuffer class destructor
// -----------------------------------------------------------------------------
PointSpriteBuffer::~PointSpriteBuffer()
{
	gl::deleteVAO(vao_);
}

// -----------------------------------------------------------------------------
// Adds a point sprite at [position] with [radius] to the buffer
// -----------------------------------------------------------------------------
void PointSpriteBuffer::add(const glm::vec2& position, float radius)
{
	sprites_.emplace_back(position, radius);
}

// -----------------------------------------------------------------------------
// Adds multiple point sprites at [positions] with [radius] to the buffer
// -----------------------------------------------------------------------------
void PointSpriteBuffer::add(const vector<glm::vec2>& positions, float radius)
{
	for (const auto& pos : positions)
		sprites_.emplace_back(pos, radius);
}

// -----------------------------------------------------------------------------
// Uploads point sprites to the GPU and clears the instance list
// -----------------------------------------------------------------------------
void PointSpriteBuffer::push()
{
	if (!vao_)
		initVAO();

	buffer_.upload(sprites_);
	sprites_.clear();
}

// -----------------------------------------------------------------------------
// Draws the point sprites using the specified [view] and [type].
// If [count] is non-zero, only draw the first [count] sprites in the buffer.
// -----------------------------------------------------------------------------
void PointSpriteBuffer::draw(PointSpriteType type, const View* view, unsigned count) const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_.empty())
		return;

	// Check valid parameters
	if (count == 0)
		count = buffer_.size();
	if (count > buffer_.size())
		return;

	// Setup shader
	auto shader = pointSpriteShader(type);
	if (view)
		view->setupShader(*shader);
	shader->setUniform("point_radius", radius_);
	shader->setUniform("colour", colour_);
	if (type == PointSpriteType::CircleOutline || type == PointSpriteType::RoundedSquareOutline)
	{
		shader->setUniform("outline_width", outline_width_);
		shader->setUniform("fill_opacity", fill_opacity_);
	}

	// Draw
	gl::bindVAO(vao_);
	gl::drawArraysInstanced(Primitive::Triangles, 0, 6, count);
	gl::bindVAO(0);
}

// -----------------------------------------------------------------------------
// Initializes the VAO for rendering point sprites
// -----------------------------------------------------------------------------
void PointSpriteBuffer::initVAO()
{
	vao_ = createVAO();
	bindVAO(vao_);

	// -------------------------------------------------------------------------

	// Create+init square buffer
	buffer_square_ = std::make_unique<Buffer<glm::vec2>>();
	buffer_square_->bind();
	buffer_square_->upload(
		vector<glm::vec2>{
			{ -1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } });

	// Setup vertex attributes for square

	// Position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// -------------------------------------------------------------------------

	// Setup vertex attributes for sprite instances

	buffer_.bind();

	// Position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// Radius
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	gl::bindVAO(0);
}
