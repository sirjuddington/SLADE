
#include "Main.h"

#include "App.h"
#include "LineBuffer.h"
#include "Shader.h"
#include "Archive/ArchiveManager.h"

using namespace slade;
using namespace gl;

namespace
{
unsigned vbo_quad = 0;
unsigned ebo_quad = 0;
float    quad_vertices[] = { 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, -1.0, 0.0 };
uint16_t quad_indices[]  = { 0, 1, 2, 0, 2, 3 };
Shader   shader_lines{ "lines" };
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

void LineBuffer::draw() const
{
	if (lines_.empty())
		return;

	if (!vao_)
		initVAO();

	if (lines_updated_)
		updateVBO();

	gl::bindVAO(vao_);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr, lines_.size());
	gl::bindVAO(0);
}

const Shader& LineBuffer::shader(float aa_radius)
{
	if (!shader_lines.isValid())
	{
		auto program_resource = app::archiveManager().programResourceArchive();
		if (program_resource)
		{
			auto entry_vert = program_resource->entryAtPath("shaders/lines.vert");
			auto entry_frag = program_resource->entryAtPath("shaders/lines.frag");
			if (entry_vert && entry_frag)
				shader_lines.load(entry_vert->data().asString(), entry_frag->data().asString());
			else
				log::error("Unable to find line drawing shaders in the program resource!");
		}
	}

	shader_lines.bind();
	shader_lines.setUniform("aa_radius", glm::vec2{ aa_radius, aa_radius });

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
	glBufferData(GL_ARRAY_BUFFER, lines_.size() * sizeof(Line), lines_.data(), GL_STATIC_DRAW);

	lines_updated_ = false;
}
