#pragma once

#include "OpenGL.h"

namespace slade::gl
{
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

	~VertexBuffer2D()
	{
		gl::deleteVBO(vbo_);
		gl::deleteVAO(vao_);
	}

	// Vector-like access to vertices
	unsigned      size() const { return vertices_.size(); }
	void          clear() { vertices_.clear(); }
	bool          empty() const { return vertices_.empty(); }
	const Vertex& operator[](unsigned index) const { return vertices_[index]; }
	Vertex&       operator[](unsigned index)
	{
		vertices_updated_ = true;
		return vertices_[index];
	}

	void add(const Vertex& vertex);
	void add(const vector<Vertex>& vertices);
	void add(const glm::vec2& pos, const glm::vec4& colour, const glm::vec2& tex_coord)
	{
		add(Vertex{ pos, colour, tex_coord });
	}

	void draw(Primitive primitive = Primitive::Triangles) const;

	static const VertexBuffer2D& unitSquare();

private:
	vector<Vertex>   vertices_;
	mutable unsigned vao_              = 0;
	mutable unsigned vbo_              = 0;
	mutable bool     vertices_updated_ = false;

	void initVAO() const;
	void updateVBO() const;
};
} // namespace slade::gl
