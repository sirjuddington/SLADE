#pragma once

#include "Buffer.h"

namespace slade::gl
{
class Shader;
class View;

struct Vertex3D
{
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec4 colour;
	glm::vec3 normal;

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

class VertexBuffer3D
{
public:
	VertexBuffer3D() = default;
	~VertexBuffer3D();

	unsigned                vao() const { return vao_; }
	const Buffer<Vertex3D>& buffer() const { return buffer_; }
	Buffer<Vertex3D>&       buffer() { return buffer_; }

	unsigned queueSize() const { return vertices_.size(); }

	void add(const Vertex3D& vertex);
	void add(const vector<Vertex3D>& vertices);
	void add(const glm::vec3& position, const glm::vec2& uv = glm::vec2{ 0.0f });

	void push();

	void draw(
		Primitive     primitive = Primitive::Triangles,
		const Shader* shader    = nullptr,
		const View*   view      = nullptr,
		unsigned      first     = 0,
		unsigned      count     = 0) const;

private:
	vector<Vertex3D> vertices_;
	Buffer<Vertex3D> buffer_;
	unsigned         vao_ = 0;
};
} // namespace slade::gl
