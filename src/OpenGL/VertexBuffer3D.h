#pragma once

#include "VertexBufferBase.h"

namespace slade::gl
{
struct Vertex3D
{
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec4 colour;
	glm::vec3 normal;

	Vertex3D() = default;
	Vertex3D(glm::vec3 position, glm::vec2 uv) : position{ position }, uv{ uv } {}
	Vertex3D(glm::vec3 position, glm::vec2 uv, glm::vec4 colour) : position{ position }, uv{ uv }, colour{ colour } {}
	Vertex3D(glm::vec3 position, glm::vec2 uv, glm::vec4 colour, glm::vec3 normal) :
		position{ position },
		uv{ uv },
		colour{ colour },
		normal{ normal }
	{
	}
};

class VertexBuffer3D : public VertexBufferBase<Vertex3D>
{
public:
	VertexBuffer3D() = default;

	void add(const glm::vec3& position, const glm::vec2& uv = glm::vec2{ 0.0f });
	void addQuad(const Vertex3D& tl, const Vertex3D& tr, const Vertex3D& bl, const Vertex3D& br);

private:
	void initVAO() override;
};
} // namespace slade::gl
