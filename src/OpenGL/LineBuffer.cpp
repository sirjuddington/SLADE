
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LineBuffer.cpp
// Description: LineBuffer class - buffer for rendering thick antialiased lines
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
#include "LineBuffer.h"
#include "Camera.h"
#include "Geometry/Geometry.h"
#include "Geometry/Plane.h"
#include "Shader.h"
#include "Utility/MathStuff.h"
#include "Utility/Vector.h"
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
unsigned vbo_quad        = 0;
unsigned ebo_quad        = 0;
float    quad_vertices[] = { 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, -1.0, 0.0 };
uint16_t quad_indices[]  = { 0, 1, 2, 0, 2, 3 };
Shader   shader_lines{ "lines" };
Shader   shader_lines_dashed{ "lines_dashed" };
Shader   shader_lines_map3d{ "lines_map3d" };
Shader   shader_lines_map3d_dashed{ "lines_map3d_dashed" };
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Initializes the line shader(s)
// -----------------------------------------------------------------------------
void initShader()
{
	shader_lines.loadResourceEntries("lines.vert", "lines.frag");
	shader_lines_dashed.define("DASHED_LINES");
	shader_lines_dashed.loadResourceEntries("lines.vert", "lines.frag");
	shader_lines_map3d.define("DISTANCE_FADE");
	shader_lines_map3d.loadResourceEntries("lines.vert", "lines.frag");
	shader_lines_map3d_dashed.define("DISTANCE_FADE");
	shader_lines_map3d_dashed.define("DASHED_LINES");
	shader_lines_map3d_dashed.loadResourceEntries("lines.vert", "lines.frag");
}

// -----------------------------------------------------------------------------
// Initializes the line [buffer]'s VAO for rendering lines with the line shader
// -----------------------------------------------------------------------------
unsigned initVAO(Buffer<LineBuffer::Line>& buffer)
{
	auto vao = createVAO();
	bindVAO(vao);

	buffer.bind();

	// Vertex1 Position + Width (vec4 X,Y,Z,Width)
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// Vertex1 Colour (vec4 R,G,B,A)
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(4 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	// Vertex2 Position + Width (vec4 X,Y,Z,Width)
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	// Vertex2 Colour (vec4 R,G,B,A)
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(12 * sizeof(float)));
	glEnableVertexAttribArray(4);
	glVertexAttribDivisor(4, 1);


	// Setup instanced quad
	if (vbo_quad == 0)
	{
		vbo_quad = createBuffer();
		bindVBO(vbo_quad);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

		ebo_quad = createBuffer();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_quad);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
	}
	else
	{
		bindVBO(vbo_quad);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_quad);
	}

	// Instanced quad vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	bindVAO(0);

	return vao;
}
} // namespace


// -----------------------------------------------------------------------------
//
// LineBuffer Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// LineBuffer class destructor
// -----------------------------------------------------------------------------
LineBuffer::~LineBuffer()
{
	deleteVAO(vao_);
}

// -----------------------------------------------------------------------------
// Adds a [line] to the buffer
// -----------------------------------------------------------------------------
void LineBuffer::add(const Line& line)
{
	lines_.push_back(line);
}

// -----------------------------------------------------------------------------
// Adds [lines] to the buffer
// -----------------------------------------------------------------------------
void LineBuffer::add(const vector<Line>& lines)
{
	vectorConcat(lines_, lines);
}

// -----------------------------------------------------------------------------
// Adds a 2D line from [start] to [end] with [colour] and [width]
// -----------------------------------------------------------------------------
void LineBuffer::add3d(glm::vec2 start, glm::vec2 end, const Plane& plane, glm::vec4 colour, float width)
{
	lines_.push_back(
		Line{ .v1_pos_width = { start.x, start.y, plane.heightAt(start.x, start.y), width },
			  .v1_colour    = colour,
			  .v2_pos_width = { end.x, end.y, plane.heightAt(end.x, end.y), width },
			  .v2_colour    = colour });
}

// -----------------------------------------------------------------------------
// Adds a 2D [line]  with an arrowhead to the buffer with [colour] and [width].
// The arrowhead is added to the end of the line, or both ends if
// [arrowhead_both] is true
// -----------------------------------------------------------------------------
void LineBuffer::addArrow(
	const Rectf& line,
	glm::vec4    colour,
	float        width,
	float        arrowhead_length,
	float        arrowhead_angle,
	bool         arrowhead_both)
{
	for (const auto& l : geometry::arrowLines(line, arrowhead_length, arrowhead_angle, arrowhead_both))
		add2d(l.x1(), l.y1(), l.x2(), l.y2(), colour, width);
}

void LineBuffer::addArrow3d(
	glm::vec3 start,
	glm::vec3 end,
	glm::vec4 colour,
	float     width,
	float     arrowhead_length,
	float     arrowhead_angle,
	bool      arrowhead_both)
{
	// Add main line
	add3d(start, end, colour, width);

	// Calculate reverse direction vector
	auto direction = glm::normalize(start - end);
	if (glm::length(direction) < math::EPSILON)
		return; // Can't create an arrowhead for a 0-length line

	// Calculate two perpendicular vectors to the direction
	auto reference      = std::abs(direction.x) < 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	auto perpendicular1 = glm::normalize(glm::cross(direction, reference));
	auto perpendicular2 = glm::normalize(glm::cross(direction, perpendicular1));

	// Calculate arrowhead points
	float angle_rad      = geometry::degToRad(arrowhead_angle);
	float forward_scale  = arrowhead_length * std::cos(angle_rad);
	float side_scale     = arrowhead_length * std::sin(angle_rad);
	auto  arrowhead_base = end + direction * forward_scale;

	// Two lines along the first perpendicular axis
	auto arrow_p1a = arrowhead_base + perpendicular1 * side_scale;
	auto arrow_p1b = arrowhead_base - perpendicular1 * side_scale;

	// Two lines along the second perpendicular axis
	auto arrow_p2a = arrowhead_base + perpendicular2 * side_scale;
	auto arrow_p2b = arrowhead_base - perpendicular2 * side_scale;

	// Add the 4 arrowhead lines
	add3d(end, arrow_p1a, colour, width);
	add3d(end, arrow_p1b, colour, width);
	add3d(end, arrow_p2a, colour, width);
	add3d(end, arrow_p2b, colour, width);

	// Add additional arrowhead at the start if requested
	if (arrowhead_both)
	{
		auto direction_start      = glm::normalize(end - start);
		auto arrowhead_base_start = start + direction_start * forward_scale;

		// Recalculate perpendiculars
		auto arrow_s1a = arrowhead_base_start + perpendicular1 * side_scale;
		auto arrow_s1b = arrowhead_base_start - perpendicular1 * side_scale;
		auto arrow_s2a = arrowhead_base_start + perpendicular2 * side_scale;
		auto arrow_s2b = arrowhead_base_start - perpendicular2 * side_scale;

		add3d(start, arrow_s1a, colour, width);
		add3d(start, arrow_s1b, colour, width);
		add3d(start, arrow_s2a, colour, width);
		add3d(start, arrow_s2b, colour, width);
	}
}

void LineBuffer::push()
{
	if (!getContext())
		return;

	// Init VAO if needed
	if (!vao_)
		vao_ = initVAO(buffer_);

	buffer_.upload(lines_);
	lines_.clear();
}

bool LineBuffer::pull()
{
	if (!getContext() || buffer_.empty())
		return false;

	lines_ = buffer_.download();
	return true;
}

// -----------------------------------------------------------------------------
// Draws the lines in 2D using the specified [view] and [colour].
// If [model] is specified, it will be used as the model matrix for the view
// -----------------------------------------------------------------------------
void LineBuffer::draw(const View* view, const glm::vec4& colour, const glm::mat4& model) const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_.empty())
		return;

	// Setup shader for drawing
	if (!shader_lines.isValid())
		initShader();
	Shader& shader = dashed_ ? shader_lines_dashed : shader_lines;
	shader.bind();
	shader.setUniform("aa_radius", aa_radius_);
	shader.setUniform("line_width", width_mult_);
	shader.setUniform("colour", colour);
	if (dashed_)
	{
		shader.setUniform("dash_size", dash_size_);
		shader.setUniform("gap_size", dash_gap_size_);
	}
	if (view)
		view->setupShader(shader, model);

	bindVAO(vao_);
	drawElementsInstanced(Primitive::Triangles, 6, GL_UNSIGNED_SHORT, buffer_.size());
	bindVAO(0);
}

// -----------------------------------------------------------------------------
// Draws the lines in 3D using the given [camera], [viewport_size] and [colour].
// If [model] is specified, it will be used as the model matrix for the camera
// -----------------------------------------------------------------------------
void LineBuffer::draw(const Camera& camera, glm::vec2 viewport_size, const glm::vec4& colour, const glm::mat4& model)
	const
{
	if (!getContext())
		return;

	// Check we have anything to draw
	if (buffer_.empty())
		return;

	// Setup shader for drawing
	if (!shader_lines.isValid())
		initShader();
	Shader& shader = map_3d_ ? (dashed_ ? shader_lines_map3d_dashed : shader_lines_map3d)
							 : (dashed_ ? shader_lines_dashed : shader_lines);
	shader.bind();
	shader.setUniform("aa_radius", aa_radius_);
	shader.setUniform("line_width", width_mult_);
	shader.setUniform("colour", colour);
	if (dashed_)
	{
		shader.setUniform("dash_size", dash_size_);
		shader.setUniform("gap_size", dash_gap_size_);
	}

	// Setup camera matrix and view uniforms
	shader.setUniform("mvp", camera.projectionMatrix() * camera.viewMatrix() * model);
	shader.setUniform("viewport_size", viewport_size);
	if (map_3d_)
		shader.setUniform("max_dist", max_dist_);

	bindVAO(vao_);
	drawElementsInstanced(Primitive::Triangles, 6, GL_UNSIGNED_SHORT, buffer_.size());
	bindVAO(0);
}

// -----------------------------------------------------------------------------
// Returns the shader used for drawing lines with this buffer
// -----------------------------------------------------------------------------
const Shader& LineBuffer::shader()
{
	if (!shader_lines.isValid())
		initShader();

	return shader_lines;
}
