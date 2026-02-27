#pragma once

#include "OpenGL/Buffer.h"

namespace slade::gl
{
class Camera;
class View;
} // namespace slade::gl

namespace slade::mapeditor
{
class SpriteBuffer3D
{
public:
	SpriteBuffer3D();
	~SpriteBuffer3D() = default;

	void add(const glm::vec3& position, float brightness, const glm::vec4& colour = glm::vec4{ 1.0f });
	void push();
	void draw() const;

private:
	struct Instance
	{
		glm::vec3 position;
		float     brightness;
		glm::vec4 colour;

		Instance(const glm::vec3& pos, float brightness, const glm::vec4& colour = glm::vec4{ 1.0f }) :
			position{ pos },
			brightness{ brightness },
			colour{ colour }
		{
		}
	};

	vector<Instance>                  sprites_;
	unsigned                          vao_ = 0;
	unique_ptr<gl::Buffer<glm::vec2>> buffer_square_;
	unique_ptr<gl::Buffer<Instance>>  buffer_sprites_;

	void initVAO();
};
} // namespace slade::mapeditor
