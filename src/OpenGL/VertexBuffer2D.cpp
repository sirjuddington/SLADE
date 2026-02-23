
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    VertexBuffer2D.cpp
// Description: VertexBuffer2D class - buffer for rendering 2D primitives
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
#include "VertexBuffer2D.h"
#include "IndexBuffer.h"
#include "Utility/Vector.h"
#include "View.h"

using namespace slade;
using namespace gl;


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Initializes a VAO for the given vertex buffer and returns its ID
// -----------------------------------------------------------------------------
unsigned initVAO(Buffer<Vertex2D>& buffer)
{
	auto vao = createVAO();
	bindVAO(vao);

	buffer.bind();

	// Position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Colour (RGBA)
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Texture Coordinates
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	bindVAO(0);

	return vao;
}
} // namespace


// -----------------------------------------------------------------------------
//
// VertexBuffer2D Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// VertexBuffer2D class destructor
// -----------------------------------------------------------------------------
VertexBuffer2D::~VertexBuffer2D()
{
	deleteVAO(vao_);
}

// -----------------------------------------------------------------------------
// Adds [vertex] to the buffer
// -----------------------------------------------------------------------------
void VertexBuffer2D::add(const Vertex2D& vertex)
{
	vertices_.push_back(vertex);
}

// -----------------------------------------------------------------------------
// Adds [vertices] to the buffer
// -----------------------------------------------------------------------------
void VertexBuffer2D::add(const vector<Vertex2D>& vertices)
{
	vectorConcat(vertices_, vertices);
}

// -----------------------------------------------------------------------------
// Adds a vertex at [pos] with [colour] and [tex_coord] to the buffer
// -----------------------------------------------------------------------------
void VertexBuffer2D::add(const glm::vec2& pos, const glm::vec4& colour, const glm::vec2& tex_coord)
{
	add(Vertex2D{ pos, colour, tex_coord });
}

// -----------------------------------------------------------------------------
// Adds two triangles forming a quad with top-left corner at [tl] and
// bottom-right corner at [br], with [colour] and UV coordinates from [uv_tl] to
// [uv_br]
// -----------------------------------------------------------------------------
void VertexBuffer2D::addQuadTriangles(glm::vec2 tl, glm::vec2 br, glm::vec4 colour, glm::vec2 uv_tl, glm::vec2 uv_br)
{
	// Bottom-Left triangle
	vertices_.emplace_back(tl, colour, uv_tl);
	vertices_.emplace_back(glm::vec2{ tl.x, br.y }, colour, glm::vec2{ uv_tl.x, uv_br.y });
	vertices_.emplace_back(br, colour, uv_br);

	// Top-Right triangle
	vertices_.emplace_back(tl, colour, uv_tl);
	vertices_.emplace_back(br, colour, uv_br);
	vertices_.emplace_back(glm::vec2{ br.x, tl.y }, colour, glm::vec2{ uv_br.x, uv_tl.y });
}

// -----------------------------------------------------------------------------
// Uploads vertices to the GPU and clears the vertex list
// -----------------------------------------------------------------------------
void VertexBuffer2D::push()
{
	if (!getContext())
		return;

	// Init VAO if needed
	if (!vao_)
		vao_ = initVAO(buffer_);

	buffer_.upload(vertices_);
	vertices_.clear();
}

// -----------------------------------------------------------------------------
// Draws the vertices in this buffer as [primitive] using the specified [shader]
// and [view] if given.
// If [count] is non-zero, only draw the first [count] vertices in the buffer.
// -----------------------------------------------------------------------------
void VertexBuffer2D::draw(Primitive primitive, const Shader* shader, const View* view, unsigned first, unsigned count)
	const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_.empty())
		return;

	// Check valid parameters
	if (count == 0)
		count = buffer_.size() - first;
	if (first >= buffer_.size() || first + count > buffer_.size())
		return;

	// Setup shader/view if given
	if (shader && view)
		view->setupShader(*shader);

	// Draw
	gl::bindVAO(vao_);
	gl::drawArrays(primitive, first, count);
	gl::bindVAO(0);
}

// -----------------------------------------------------------------------------
// Draws the vertices in this buffer as [primitive] using indexed drawing with
// the given [index_buffer], and using the specified [shader] and [view] if
// given.
// If [count] is non-zero, only draw the first [count] indices in the index
// buffer.
// -----------------------------------------------------------------------------
void VertexBuffer2D::drawElements(
	IndexBuffer&  index_buffer,
	Primitive     primitive,
	const Shader* shader,
	const View*   view) const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_.empty() || index_buffer.empty())
		return;

	// Setup shader/view if given
	if (shader && view)
		view->setupShader(*shader);

	// Draw
	bindVAO(vao_);
	index_buffer.bind();
	gl::drawElements(primitive, index_buffer.size(), GL_UNSIGNED_INT);
	bindEBO(0);
	bindVAO(0);
}

// -----------------------------------------------------------------------------
// Returns a vertex buffer containing a single quad covering the area from
// (0, 0) to (1, 1).
// Note that this is a quad (4 vertices) not triangles
// -----------------------------------------------------------------------------
const VertexBuffer2D& VertexBuffer2D::unitSquare()
{
	static VertexBuffer2D vb_square;
	if (vb_square.buffer_.empty())
	{
		vb_square.add({ { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f } });
		vb_square.add({ { 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 1.f } });
		vb_square.add({ { 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f } });
		vb_square.add({ { 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 0.f } });
		vb_square.push();
	}

	return vb_square;
}
