#pragma once

#include "OpenGL.h"
#include "Structs.h"

namespace OpenGL
{
class VertexBuffer2D
{
public:
	VertexBuffer2D() = default;
	~VertexBuffer2D();

	// Vector-like access to vertices
	unsigned        size() const { return vertices_.size(); }
	void            clear() { vertices_.clear(); }
	bool            empty() const { return vertices_.empty(); }
	const Vertex2D& operator[](unsigned index) const { return vertices_[index]; }
	Vertex2D&       operator[](unsigned index)
	{
		vertices_updated_ = true;
		return vertices_[index];
	}

	void add(const Vertex2D& vertex);
	void add(const vector<Vertex2D>& vertices);

	void draw(Primitive primitive = Primitive::Triangles) const;

	static const VertexBuffer2D& unitSquare();

private:
	vector<Vertex2D> vertices_;
	mutable unsigned vao_              = 0;
	mutable unsigned vbo_              = 0;
	mutable bool     vertices_updated_ = false;

	void initVAO() const;
	void updateVBO() const;
};
} // namespace OpenGL
