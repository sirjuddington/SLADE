
#include "Main.h"
#include "LineBuffer.h"
#include "Shader.h"
#include "Utility/MathStuff.h"
#include "Utility/Vector.h"
#include "View.h"

using namespace slade;
using namespace gl;

namespace
{
unsigned vbo_quad        = 0;
unsigned ebo_quad        = 0;
float    quad_vertices[] = { 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, -1.0, 0.0 };
uint16_t quad_indices[]  = { 0, 1, 2, 0, 2, 3 };
Shader   shader_lines{ "lines" };
Shader   shader_lines_dashed{ "lines_dashed" };
} // namespace


namespace
{
void initShader()
{
	shader_lines.loadResourceEntries("lines.vert", "lines.frag");
	shader_lines_dashed.define("DASHED_LINES");
	shader_lines_dashed.loadResourceEntries("lines.vert", "lines.frag");
}

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

LineBuffer::~LineBuffer()
{
	deleteVAO(vao_);
}

void LineBuffer::add(const Line& line)
{
	lines_.push_back(line);
}

void LineBuffer::add(const vector<Line>& lines)
{
	vectorConcat(lines_, lines);
}

void LineBuffer::addArrow(
	const Rectf& line,
	glm::vec4    colour,
	float        width,
	float        arrowhead_length,
	float        arrowhead_angle,
	bool         arrowhead_both)
{
	for (const auto& l : math::arrowLines(line, arrowhead_length, arrowhead_angle, arrowhead_both))
		add2d(l.x1(), l.y1(), l.x2(), l.y2(), colour, width);
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

	gl::bindVAO(vao_);
	gl::drawElementsInstanced(Primitive::Triangles, 6, GL_UNSIGNED_SHORT, buffer_.size());
	gl::bindVAO(0);
}

const Shader& LineBuffer::shader()
{
	if (!shader_lines.isValid())
		initShader();

	return shader_lines;
}
