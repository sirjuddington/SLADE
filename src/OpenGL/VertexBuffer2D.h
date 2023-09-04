#pragma once

#include "Buffer.h"
#include "OpenGL.h"

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

	~VertexBuffer2D() { gl::deleteVAO(vao_); }

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

	const Buffer<Vertex>& buffer() const { return buffer_; }
	Buffer<Vertex>&       buffer() { return buffer_; }

	bool isUploaded() const { return buffer_.size() > 0; }

	void add(const Vertex& vertex);
	void add(const vector<Vertex>& vertices);
	void add(const glm::vec2& pos, const glm::vec4& colour, const glm::vec2& tex_coord)
	{
		add(Vertex{ pos, colour, tex_coord });
	}
	void addQuadTriangles(
		glm::vec2 tl,
		glm::vec2 br,
		glm::vec4 colour = glm::vec4{ 1.0f },
		glm::vec2 uv_tl = glm::vec2{ 0.0f },
		glm::vec2 uv_br = glm::vec2{ 1.0f });

	void update(unsigned offset, const vector<Vertex>& vertices) const { buffer_.update(offset, vertices); }
	void upload() const;
	void upload(bool keep_data);

	void draw(
		Primitive     primitive = Primitive::Triangles,
		const Shader* shader    = nullptr,
		const View*   view      = nullptr,
		unsigned      first     = 0,
		unsigned      count     = 0) const;

	static const VertexBuffer2D& unitSquare();

private:
	vector<Vertex>         vertices_;
	mutable unsigned       vao_ = 0;
	mutable Buffer<Vertex> buffer_;
	mutable bool           vertices_updated_ = false; // True if we need to update the VBO next draw

	void initVAO() const;
};
} // namespace slade::gl
