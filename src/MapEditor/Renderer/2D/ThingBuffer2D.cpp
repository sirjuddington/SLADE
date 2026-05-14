
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ThingBuffer2D.cpp
// Description: ThingBuffer2D class - buffer for rendering things in the 2d map
//              editor view
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
#include "ThingBuffer2D.h"
#include "Game/ThingType.h"
#include "Geometry/Geometry.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/Shader.h"
#include "OpenGL/View.h"

using namespace slade;
using namespace gl;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
enum class TexType
{
	None   = 0,
	Icon   = 1,
	Sprite = 2
};

struct ThingShader
{
	unique_ptr<Shader> shader;
	bool               square      = false;
	TexType            tex_type    = TexType::None;
	bool               direction   = false;
	bool               sprite_only = false;
};
vector<ThingShader> thing_shaders;
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns a shader for rendering things with the specified properties:
// [square] - if true, shader will render things with a square background
// instead of round
// [textured] - the type of texture to use for things (none, icon or sprite)
// [direction] - if true, shader will render an arrow indicating thing direction
// [sprite_only] - if true, only render the sprite (at actual size)
// -----------------------------------------------------------------------------
Shader* thingShader(bool square, TexType textured, bool direction, bool sprite_only = false)
{
	// Check if already loaded
	for (const auto& shader : thing_shaders)
	{
		if (shader.square == square
			&& shader.tex_type == textured
			&& shader.direction == direction
			&& shader.sprite_only == sprite_only)
			return shader.shader.get();
	}

	// Build shader name
	string shader_name = "thing2d";
	if (sprite_only)
		shader_name += "_sprite_only";
	else if (square)
		shader_name += "_square";
	else
		shader_name += "_round";
	if (textured == TexType::Icon)
		shader_name += "_icon";
	else if (textured == TexType::Sprite)
		shader_name += "_sprite";
	if (direction)
		shader_name += "_arrow";

	// Load the shader with requested properties
	thing_shaders.push_back({ std::make_unique<Shader>(shader_name), square, textured, direction, sprite_only });
	auto shader = thing_shaders.back().shader.get();
	if (sprite_only)
	{
		if (direction)
			shader->define("ARROW");
		shader->loadResourceEntries("thing2d_spriteonly.vert", "thing2d_spriteonly.frag");
	}
	else
	{
		if (square)
			shader->define("SQUARE");
		if (textured == TexType::Icon)
			shader->define("ICON");
		else if (textured == TexType::Sprite)
			shader->define("SPRITE");
		if (direction)
			shader->define("ARROW");
		shader->loadResourceEntries("thing2d.vert", "thing2d.frag");
	}

	return shader;
}
} // namespace


// -----------------------------------------------------------------------------
//
// ThingBuffer2D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ThingBuffer2D class constructor
// -----------------------------------------------------------------------------
ThingBuffer2D::ThingBuffer2D()
{
	buffer_things_ = std::make_unique<Buffer<ThingInstance>>();
}

// -----------------------------------------------------------------------------
// Sets up the buffer for a thing type, using the type's properties
// -----------------------------------------------------------------------------
void ThingBuffer2D::setup(const game::ThingType& type)
{
	colour_         = type.colour();
	radius_         = type.radius();
	arrow_          = type.angled();
	shrink_on_zoom_ = type.shrinkOnZoom();
}

// -----------------------------------------------------------------------------
// Sets the texture to use for things in this buffer. If [sprite] is true, the
// texture is treated as a sprite and is adjusted to fit within the thing's
// radius
// -----------------------------------------------------------------------------
void ThingBuffer2D::setTexture(unsigned texture, bool sprite)
{
	texture_ = texture;
	sprite_  = sprite;

	if (sprite_)
	{
		const auto& tex = Texture::info(texture_);

		sprite_size_ = glm::vec2{ static_cast<float>(tex.size.x), static_cast<float>(tex.size.y) };

		if (tex.size.x > tex.size.y)
		{
			tex_size_.x = 1.0f;
			tex_size_.y = static_cast<float>(tex.size.y) / static_cast<float>(tex.size.x);
		}
		else
		{
			tex_size_.y = 1.0f;
			tex_size_.x = static_cast<float>(tex.size.x) / static_cast<float>(tex.size.y);
		}

		tex_size_.x = 1.0f / (tex_size_.x * 0.75f);
		tex_size_.y = 1.0f / (tex_size_.y * 0.75f);
	}
	else
	{
		tex_size_ = glm::vec2{ 1.0f };
		sprite_size_ = { -1.0f, -1.0f };
	}
}

// -----------------------------------------------------------------------------
// Adds a thing instance to the buffer with the specified properties
// -----------------------------------------------------------------------------
void ThingBuffer2D::add(float x, float y, float angle, float alpha)
{
	auto dir = geometry::vectorAngle(geometry::degToRad(angle));
	things_.emplace_back(glm::vec2{ x, y }, glm::vec2{ dir.x, dir.y }, alpha);
}

// -----------------------------------------------------------------------------
// Uploads thing instances to the GPU and clears the instance list
// -----------------------------------------------------------------------------
void ThingBuffer2D::push()
{
	if (!vao_)
		initVAO();

	buffer_things_->upload(things_);
	things_.clear();
}

// -----------------------------------------------------------------------------
// Draws the things in this buffer using the specified [view] and [colour].
// If [square] is true, things will be drawn with a square background instead of
// round.
// If [force_arrow] is true, things will be drawn with an arrow indicating their
// direction, regardless of the thing type's angled property
// -----------------------------------------------------------------------------
void ThingBuffer2D::draw(const View* view, const glm::vec4& colour, bool square, bool sprite_only, bool force_arrow)
	const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_things_->empty())
		return;

	// Setup shader
	Shader* shader;
	if (sprite_only && sprite_size_.x >= 0.0f)
	{
		const bool draw_arrow = arrow_ || force_arrow;
		shader                = thingShader(false, TexType::Sprite, draw_arrow, true);
		shader->bind();
		if (view)
			view->setupShader(*shader);
		shader->setUniform("tex_size", sprite_size_);
		shader->setUniform("colour", colour_ * colour);
		if (draw_arrow)
		{
			shader->setUniform("radius", std::max({ 32.0f, sprite_size_.x, sprite_size_.y }));
			shader->setUniform("shadow_opacity", shadow_opacity_);
		}
	}
	else
	{
		// Determine radius
		auto radius = radius_;
		if (shrink_on_zoom_ && view)
		{
			if (view->scale().x > 1.0)
				radius /= view->scale().x;
		}

		shader = thingShader(
			square, texture_ > 0 ? (sprite_ ? TexType::Sprite : TexType::Icon) : TexType::None, arrow_ || force_arrow);
		shader->bind();
		if (view)
			view->setupShader(*shader);
		shader->setUniform("colour", colour_ * colour);
		shader->setUniform("radius", radius);
		if (sprite_ && !square)
			shader->setUniform("tex_size", tex_size_ * 1.2f); // Smaller sprite if round background
		else
			shader->setUniform("tex_size", tex_size_);
		shader->setUniform("shadow_opacity", shadow_opacity_);
	}

	gl::bindVAO(vao_);
	gl::Texture::bind(texture_);

	gl::drawArraysInstanced(Primitive::Triangles, 0, 6, buffer_things_->size());

	gl::bindVAO(0);
}

// -----------------------------------------------------------------------------
// Initializes the VAO for this buffer, setting up vertex attributes for the
// square and thing instance data
// -----------------------------------------------------------------------------
void ThingBuffer2D::initVAO()
{
	vao_ = gl::createVAO();
	gl::bindVAO(vao_);

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

	// Create thing instance buffer if needed
	if (!buffer_things_)
		buffer_things_ = std::make_unique<Buffer<ThingInstance>>();
	buffer_things_->bind();

	// Setup vertex attributes for thing instances

	// Position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// Direction
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	// Alpha
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	// -------------------------------------------------------------------------

	gl::bindVAO(0);
}
