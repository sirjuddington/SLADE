
#include "Main.h"
#include "VertexBuffer2D.h"
#include "View.h"

using namespace slade;
using namespace gl;


void VertexBuffer2D::add(const Vertex& vertex)
{
	vertices_.push_back(vertex);
	vertices_updated_ = true;
}

void VertexBuffer2D::add(const vector<Vertex>& vertices)
{
	for (const auto& vertex : vertices)
		vertices_.push_back(vertex);

	vertices_updated_ = true;
}

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

void VertexBuffer2D::upload() const
{
	buffer_.upload(vertices_);
	vertices_updated_ = false;
}

void VertexBuffer2D::upload(bool keep_data)
{
	buffer_.upload(vertices_);
	vertices_updated_ = false;

	if (!keep_data)
		vertices_.clear();
}

void VertexBuffer2D::draw(Primitive primitive, const Shader* shader, const View* view, unsigned first, unsigned count)
	const
{
	// Init/update gl objects
	if (!vao_)
		initVAO();
	else if (vertices_updated_)
		upload();

	if (buffer_.size() == 0)
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
	glDrawArrays(static_cast<GLenum>(primitive), first, count);
	gl::bindVAO(0);
}

const VertexBuffer2D& VertexBuffer2D::unitSquare()
{
	static VertexBuffer2D vb_square;
	if (vb_square.buffer_.size() == 0)
	{
		vb_square.add({ { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f } });
		vb_square.add({ { 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 1.f } });
		vb_square.add({ { 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f } });
		vb_square.add({ { 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 0.f } });
		vb_square.upload();
	}

	return vb_square;
}

void VertexBuffer2D::initVAO() const
{
	vao_ = gl::createVAO();
	gl::bindVAO(vao_);

	buffer_.bind();

	// Position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Colour (RGBA)
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Texture Coordinates
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	gl::bindVAO(0);
}
