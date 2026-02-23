
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    VertexBuffer3D.cpp
// Description: VertexBuffer3D class - buffer for rendering 3D primitives
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
#include "VertexBuffer3D.h"

using namespace slade;
using namespace gl;


// -----------------------------------------------------------------------------
//
// VertexBuffer3D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Adds a vertex at [position] with [uv] coordinates to the buffer
// -----------------------------------------------------------------------------
void VertexBuffer3D::add(const glm::vec3& position, const glm::vec2& uv)
{
	vertices_.emplace_back(position, uv);
}

// -----------------------------------------------------------------------------
// Adds a quad to the buffer with the given [tl],[tr],[bl],[br] vertices
// -----------------------------------------------------------------------------
void VertexBuffer3D::addQuad(const Vertex3D& tl, const Vertex3D& tr, const Vertex3D& bl, const Vertex3D& br)
{
	vertices_.push_back(tl);
	vertices_.push_back(bl);
	vertices_.push_back(br);
	vertices_.push_back(tl);
	vertices_.push_back(br);
	vertices_.push_back(tr);
}

// -----------------------------------------------------------------------------
// Initializes the VAO for this vertex buffer
// -----------------------------------------------------------------------------
void VertexBuffer3D::initVAO()
{
	vao_ = createVAO();
	bindVAO(vao_);

	buffer_.bind();

	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture Coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Colour
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Normal
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(3);

	bindVAO(0);
}
