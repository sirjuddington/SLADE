
#include "Main.h"
#include "VertexBuffer3D.h"

using namespace slade;
using namespace gl;


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
