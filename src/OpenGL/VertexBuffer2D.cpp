
#include "Main.h"
#include "VertexBuffer2D.h"

using namespace OpenGL;

VertexBuffer2D::~VertexBuffer2D()
{
	OpenGL::deleteVBO(vbo_);
	OpenGL::deleteVAO(vao_);
}

void VertexBuffer2D::add(const Vertex2D& vertex)
{
	vertices_.push_back(vertex);
	vertices_updated_ = true;
}

void VertexBuffer2D::add(const vector<Vertex2D>& vertices)
{
	for (const auto& vertex : vertices)
		vertices_.push_back(vertex);

	vertices_updated_ = true;
}

void VertexBuffer2D::draw(Primitive primitive) const
{
	if (vertices_.empty())
		return;

	if (!vao_)
		initVAO();

	if (vertices_updated_)
		updateVBO();

	OpenGL::bindVAO(vao_);
	glDrawArrays(static_cast<GLenum>(primitive), 0, vertices_.size());
	OpenGL::bindVAO(0);
}

const VertexBuffer2D& VertexBuffer2D::unitSquare()
{
	static VertexBuffer2D vb_square;
	if (vb_square.empty())
	{
		vb_square.add({ { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f } });
		vb_square.add({ { 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 1.f } });
		vb_square.add({ { 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f } });
		vb_square.add({ { 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 0.f } });
	}

	return vb_square;
}

void VertexBuffer2D::initVAO() const
{
	vao_ = OpenGL::createVAO();
	OpenGL::bindVAO(vao_);

	updateVBO();

	// Position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Colour (RGBA)
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Texture Coordinates
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	OpenGL::bindVAO(0);
}

void VertexBuffer2D::updateVBO() const
{
	if (vbo_ == 0)
		vbo_ = OpenGL::createVBO();

	OpenGL::bindVBO(vbo_);
	glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex2D), vertices_.data(), GL_STATIC_DRAW);

	vertices_updated_ = false;
}
