#pragma once

#include "App.h"
#include "OpenGL.h"

namespace slade::gl
{
template<typename T>
class UniformBuffer
{
public:
	UniformBuffer() = default;
	~UniformBuffer()
	{
		if (ubo_ > 0 && !app::isExiting())
			deleteBuffer(ubo_);
	}

	// Non-copyable
	UniformBuffer(const UniformBuffer&)            = delete;
	UniformBuffer& operator=(const UniformBuffer&) = delete;

	// Movable
	UniformBuffer(UniformBuffer&& other) noexcept : ubo_{ other.ubo_ }, uploaded_{ other.uploaded_ }
	{
		other.ubo_      = 0;
		other.uploaded_ = false;
	}
	UniformBuffer& operator=(UniformBuffer&& other) noexcept
	{
		if (this != &other)
		{
			if (ubo_ > 0 && !app::isExiting())
				deleteBuffer(ubo_);
			ubo_            = other.ubo_;
			uploaded_       = other.uploaded_;
			other.ubo_      = 0;
			other.uploaded_ = false;
		}
		return *this;
	}

	unsigned id() const { return ubo_; }
	bool     isEmpty() const { return !uploaded_; }

	bool upload(const T& data)
	{
		if (!getContext())
			return false;

		if (ubo_ == 0)
			ubo_ = createBuffer();
		if (ubo_ == 0)
			return false;

		glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
		if (uploaded_)
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), &data);
		else
		{
			glBufferData(GL_UNIFORM_BUFFER, sizeof(T), &data, GL_DYNAMIC_DRAW);
			uploaded_ = true;
		}
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		return true;
	}

	bool bindTo(unsigned binding_point) const
	{
		if (!getContext() || ubo_ == 0)
			return false;

		glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, ubo_);
		return true;
	}

private:
	unsigned ubo_      = 0;
	bool     uploaded_ = false;
};
} // namespace slade::gl
