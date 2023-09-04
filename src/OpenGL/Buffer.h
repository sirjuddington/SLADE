#pragma once

#include "OpenGL.h"

namespace slade::gl
{
template<typename T> class Buffer
{
public:
	Buffer() = default;
	~Buffer() { gl::deleteVBO(vbo_); }

	unsigned vbo() const { return vbo_; }
	unsigned size() const { return data_uploaded_; }

	// This doesn't actually clear the buffer on the gpu, just use it to indicate the buffer needs re-uploading
	void clear() { data_uploaded_ = 0; }

	bool bind()
	{
		if (vbo_ == 0)
			vbo_ = gl::createVBO();

		gl::bindVBO(vbo_);

		return vbo_ != 0;
	}

	bool update(unsigned offset, const vector<T>& data)
	{
		if (offset >= data_uploaded_ || offset + data.size() > data_uploaded_)
			return false;

		if (bind())
		{
			glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(T), data.size() * sizeof(T), data.data());
			return true;
		}

		return false;
	}

	bool upload(const vector<T>& data)
	{
		if (!bind())
			return false;

		// Only allocate new buffer if we are uploading more data than the buffer currently holds
		if (data.size() > data_uploaded_)
			glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
		else
			glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(T), data.data());

		data_uploaded_ = data.size();

		return true;
	}

private:
	unsigned vbo_           = 0;
	unsigned data_uploaded_ = 0; // No. of items currently 'uploaded' to the GL buffer
};
} // namespace slade::gl
