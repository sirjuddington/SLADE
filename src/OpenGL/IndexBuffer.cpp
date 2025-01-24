
#include "Main.h"
#include "IndexBuffer.h"
#include "OpenGL.h"

using namespace slade;
using namespace gl;

IndexBuffer::~IndexBuffer()
{
	if (ebo_ > 0)
		deleteEBO(ebo_);
}

bool IndexBuffer::bind()
{
	if (!getContext())
		return false;

	if (ebo_ == 0)
		ebo_ = createBuffer();

	bindEBO(ebo_);

	return ebo_ != 0;
}

bool IndexBuffer::update(unsigned offset, const vector<uint32_t>& data)
{
	if (!getContext())
		return false;

	if (offset >= index_count_ || offset + data.size() > index_count_)
		return false;

	if (bind())
	{
		glBufferSubData(
			GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(uint32_t), data.size() * sizeof(uint32_t), data.data());
		return true;
	}

	return false;
}

bool IndexBuffer::upload(const vector<uint32_t>& data)
{
	if (!getContext())
		return false;

	if (!bind())
		return false;

	// Only allocate new buffer if we are uploading more data than the buffer currently holds
	if (data.size() > index_count_)
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.size() * sizeof(uint32_t), data.data(), GL_STATIC_DRAW);
	else
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, data.size() * sizeof(uint32_t), data.data());

	index_count_ = data.size();

	return true;
}
