#pragma once

#include "OpenGL.h"

namespace slade::gl
{
class IndexBuffer
{
public:
	IndexBuffer() = default;
	~IndexBuffer();

	unsigned ebo() const { return ebo_; }
	unsigned size() const { return index_count_; }
	bool     empty() const { return index_count_ == 0; }

	bool bind();
	bool update(unsigned offset, const vector<GLuint>& data);
	bool upload(const vector<GLuint>& data);

private:
	unsigned ebo_;
	unsigned index_count_ = 0;
};
} // namespace slade::gl
