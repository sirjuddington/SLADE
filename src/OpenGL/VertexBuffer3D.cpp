
#include "Main.h"
#include "VertexBuffer3D.h"
#include "Utility/Vector.h"
#include "View.h"

using namespace slade;
using namespace gl;


namespace
{
unsigned initVAO(Buffer<Vertex3D>& buffer)
{
	auto vao = createVAO();
	bindVAO(vao);

	buffer.bind();

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

	return vao;
}
} // namespace


VertexBuffer3D::~VertexBuffer3D()
{
	deleteVAO(vao_);
}

void VertexBuffer3D::add(const Vertex3D& vertex)
{
	vertices_.push_back(vertex);
}

void VertexBuffer3D::add(const vector<Vertex3D>& vertices)
{
	vectorConcat(vertices_, vertices);
}

void VertexBuffer3D::add(const glm::vec3& position, const glm::vec2& uv)
{
	vertices_.emplace_back(position, uv);
}

void VertexBuffer3D::addQuad(const Vertex3D& tl, const Vertex3D& tr, const Vertex3D& bl, const Vertex3D& br)
{
	vertices_.push_back(tl);
	vertices_.push_back(bl);
	vertices_.push_back(br);
	vertices_.push_back(tl);
	vertices_.push_back(br);
	vertices_.push_back(tr);
}

void VertexBuffer3D::push()
{
	if (!getContext())
		return;

	// Init VAO if needed
	if (!vao_)
		vao_ = initVAO(buffer_);

	buffer_.upload(vertices_);
	vertices_.clear();
}

void VertexBuffer3D::draw(Primitive primitive, const Shader* shader, const View* view, unsigned first, unsigned count)
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
	bindVAO(vao_);
	drawArrays(primitive, first, count);
	bindVAO(0);
}
