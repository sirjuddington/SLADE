
#include "Main.h"
#include "LineBuffer.h"
#include "Shader.h"
#include "Utility/MathStuff.h"
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
} // namespace

void LineBuffer::add(const Line& line)
{
	lines_.push_back(line);
	lines_updated_ = true;
}

void LineBuffer::add(const vector<Line>& lines)
{
	for (const auto& line : lines)
		lines_.push_back(line);

	lines_updated_ = true;
}

void LineBuffer::addArrow(
	const Rectf& line,
	glm::vec4    colour,
	float        width,
	float        arrowhead_length,
	float        arrowhead_angle,
	bool         arrowhead_both)
{
	Vec2f vector  = line.br - line.tl;
	auto  angle   = atan2(-vector.y, vector.x);
	auto  ang_rad = math::degToRad(arrowhead_angle);

	// Line end arrowhead
	Vec2f a1r;
	Vec2f a1l = a1r = line.br;
	a1l.x += arrowhead_length * sin(angle - ang_rad);
	a1l.y += arrowhead_length * cos(angle - ang_rad);
	a1r.x -= arrowhead_length * sin(angle + ang_rad);
	a1r.y -= arrowhead_length * cos(angle + ang_rad);
	add2d(line.tl.x, line.tl.y, line.br.x, line.br.y, colour, width);
	add2d(line.br.x, line.br.y, a1l.x, a1l.y, colour, width);
	add2d(line.br.x, line.br.y, a1r.x, a1r.y, colour, width);

	if (arrowhead_both)
	{
		// Line start arrowhead
		vector = line.tl - line.br;
		angle  = atan2(-vector.y, vector.x);

		Vec2f a2r;
		Vec2f a2l = a2r = line.tl;
		a2l.x += arrowhead_length * sin(angle - ang_rad);
		a2l.y += arrowhead_length * cos(angle - ang_rad);
		a2r.x -= arrowhead_length * sin(angle + ang_rad);
		a2r.y -= arrowhead_length * cos(angle + ang_rad);
		add2d(line.tl.x, line.tl.y, a2l.x, a2l.y, colour, width);
		add2d(line.tl.x, line.tl.y, a2r.x, a2r.y, colour, width);
	}
}

void LineBuffer::upload()
{
	if (!vao_)
		initVAO();
	else
		updateVBO();

	lines_.clear();
}

void LineBuffer::draw(const View* view, const glm::vec4& colour, const glm::mat4& model) const
{
	// Init/update gl objects
	if (!vao_)
		initVAO();
	else if (lines_updated_)
		updateVBO();

	if (lines_uploaded_ == 0)
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
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr, lines_uploaded_);
	gl::bindVAO(0);
}

const Shader& LineBuffer::shader()
{
	if (!shader_lines.isValid())
		initShader();

	return shader_lines;
}

void LineBuffer::initVAO() const
{
	vao_ = gl::createVAO();
	gl::bindVAO(vao_);

	updateVBO();

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
		vbo_quad = gl::createVBO();
		gl::bindVBO(vbo_quad);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

		ebo_quad = gl::createVBO();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_quad);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
	}
	else
	{
		gl::bindVBO(vbo_quad);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_quad);
	}

	// Instanced quad vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	gl::bindVAO(0);
}

void LineBuffer::updateVBO() const
{
	if (vbo_ == 0)
		vbo_ = gl::createVBO();

	gl::bindVBO(vbo_);

	// Only allocate new buffer if we are uploading more data than the buffer currently holds
	if (lines_.size() > lines_uploaded_)
		glBufferData(GL_ARRAY_BUFFER, lines_.size() * sizeof(Line), lines_.data(), GL_STATIC_DRAW);
	else
		glBufferSubData(GL_ARRAY_BUFFER, 0, lines_.size() * sizeof(Line), lines_.data());

	lines_updated_  = false;
	lines_uploaded_ = lines_.size();
}
