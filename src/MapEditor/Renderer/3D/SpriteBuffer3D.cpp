
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SpriteBuffer3D.cpp
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
#include "SpriteBuffer3D.h"

using namespace slade;
using namespace mapeditor;



SpriteBuffer3D::SpriteBuffer3D()
{
	buffer_sprites_ = std::make_unique<gl::Buffer<Instance>>();
}

SpriteBuffer3D::~SpriteBuffer3D()
{
	gl::deleteVAO(vao_);
}

void SpriteBuffer3D::add(const glm::vec3& position, float brightness, const glm::vec4& colour)
{
	sprites_.emplace_back(position, brightness, colour);
}

void SpriteBuffer3D::push()
{
	if (!vao_)
		initVAO();

	buffer_sprites_->upload(sprites_);
	sprites_.clear();
}

void SpriteBuffer3D::draw() const
{
	if (!vao_ || buffer_sprites_->empty())
		return;

	gl::bindVAO(vao_);
	gl::drawArraysInstanced(gl::Primitive::Triangles, 0, 6, buffer_sprites_->size());
	gl::bindVAO(0);
}

void SpriteBuffer3D::initVAO()
{
	vao_ = gl::createVAO();
	gl::bindVAO(vao_);

	// -------------------------------------------------------------------------

	// Create+init square buffer
	buffer_square_ = std::make_unique<gl::Buffer<glm::vec2>>();
	buffer_square_->bind();
	buffer_square_->upload(
		vector<glm::vec2>{
			{ -0.5f, 0.0f }, { -0.5f, 1.0f }, { 0.5f, 1.0f }, { 0.5f, 1.0f }, { 0.5f, 0.0f }, { -0.5f, 0.0f } });

	// Setup vertex attributes for square

	// Position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// -------------------------------------------------------------------------

	// Create thing instance buffer if needed
	if (!buffer_sprites_)
		buffer_sprites_ = std::make_unique<gl::Buffer<Instance>>();
	buffer_sprites_->bind();

	// Setup vertex attributes for thing instances

	// Position
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// Brightness
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	// Colour
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	// -------------------------------------------------------------------------

	gl::bindVAO(0);
}
