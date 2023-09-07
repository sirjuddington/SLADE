#pragma once

#include "Buffer.h"

namespace slade::gl
{
class Shader;
class View;

class VertexBuffer3D
{
public:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec4 colour;

		Vertex(glm::vec3 position, glm::vec2 uv, glm::vec3 normal, glm::vec4 colour) :
			position{ position }, uv{ uv }, normal{ normal }, colour{ colour }
		{
		}
	};

	VertexBuffer3D() = default;
	~VertexBuffer3D();

	const Buffer<Vertex>& buffer() const { return buffer_; }
	Buffer<Vertex>&       buffer() { return buffer_; }

	unsigned queueSize() const { return vertices_.size(); }

	void add(const Vertex& vertex);
	void add(const vector<Vertex>& vertices);
	void add(
		const glm::vec3& position,
		const glm::vec2& uv     = glm::vec2{ 0.0f },
		const glm::vec3& normal = glm::vec3{ 0.0f },
		const glm::vec4& colour = glm::vec4{ 1.0f });

	void push();

	void draw(
		Primitive     primitive = Primitive::Triangles,
		const Shader* shader    = nullptr,
		const View*   view      = nullptr,
		unsigned      first     = 0,
		unsigned      count     = 0) const;

private:
	vector<Vertex> vertices_;
	Buffer<Vertex> buffer_;
	unsigned       vao_;
};
} // namespace slade::gl
