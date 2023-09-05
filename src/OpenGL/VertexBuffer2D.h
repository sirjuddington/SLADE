#pragma once

#include "Buffer.h"

namespace slade::gl
{
class View;
class Shader;

class VertexBuffer2D
{
public:
	struct Vertex
	{
		glm::vec2 position;
		glm::vec4 colour;
		glm::vec2 tex_coord;

		Vertex(glm::vec2 position) : position{ position }, colour{}, tex_coord{} {}
		Vertex(glm::vec2 position, glm::vec4 colour) : position{ position }, colour{ colour }, tex_coord{} {}
		Vertex(glm::vec2 position, glm::vec4 colour, glm::vec2 tex_coord) :
			position{ position }, colour{ colour }, tex_coord{ tex_coord }
		{
		}
	};

	VertexBuffer2D() = default;
	~VertexBuffer2D();

	const Buffer<Vertex>& buffer() const { return buffer_; }
	Buffer<Vertex>&       buffer() { return buffer_; }

	unsigned queueSize() const { return vertices_.size(); }

	void add(const Vertex& vertex);
	void add(const vector<Vertex>& vertices);
	void add(const glm::vec2& pos, const glm::vec4& colour, const glm::vec2& tex_coord);
	void addQuadTriangles(
		glm::vec2 tl,
		glm::vec2 br,
		glm::vec4 colour = glm::vec4{ 1.0f },
		glm::vec2 uv_tl  = glm::vec2{ 0.0f },
		glm::vec2 uv_br  = glm::vec2{ 1.0f });

	void push();

	void draw(
		Primitive     primitive = Primitive::Triangles,
		const Shader* shader    = nullptr,
		const View*   view      = nullptr,
		unsigned      first     = 0,
		unsigned      count     = 0) const;

	static const VertexBuffer2D& unitSquare();

private:
	vector<Vertex> vertices_;
	unsigned       vao_ = 0;
	Buffer<Vertex> buffer_;
};
} // namespace slade::gl
