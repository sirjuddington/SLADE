#pragma once

#include "Buffer.h"
#include "OpenGL/View.h"
#include "Utility/Vector.h"

namespace slade::gl
{
class View;
class Shader;

template<typename T> class VertexBufferBase
{
public:
	VertexBufferBase() = default;
	virtual ~VertexBufferBase() { deleteVAO(vao_); }

	unsigned         vao() const { return vao_; }
	const Buffer<T>& buffer() const { return buffer_; }
	Buffer<T>&       buffer() { return buffer_; }
	unsigned         queueSize() const { return vertices_.size(); }

	void addVertex(const T& vertex) { vertices_.push_back(vertex); }
	void addVertices(const vector<T>& vertices) { vectorConcat(vertices_, vertices); }

	void push()
	{
		if (!getContext())
			return;

		// Init VAO if needed
		if (!vao_)
			initVAO();

		buffer_.upload(vertices_);
		vertices_.clear();
	}

	bool pull()
	{
		if (!getContext() || buffer_.empty())
			return false;

		vertices_ = buffer_.download();
		return true;
	}

	void draw(
		Primitive     primitive = Primitive::Triangles,
		const Shader* shader    = nullptr,
		const View*   view      = nullptr,
		unsigned      first     = 0,
		unsigned      count     = 0) const
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

	void drawPartial(
		unsigned      first,
		unsigned      count,
		Primitive     primitive = Primitive::Triangles,
		const Shader* shader    = nullptr,
		const View*   view      = nullptr) const
	{
		draw(primitive, shader, view, first, count);
	}

protected:
	vector<T> vertices_;
	Buffer<T> buffer_;
	unsigned  vao_ = 0;

	virtual void initVAO() = 0;
};
} // namespace slade::gl
