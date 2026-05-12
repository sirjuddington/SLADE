#pragma once

#include "OpenGL/VertexBufferBase.h"

namespace slade::mapeditor
{
struct MGVertex
{
	glm::vec3 position;
	glm::vec2 uv;
	float     brightness = 1.0f;
	glm::vec3 normal;

	MGVertex() = default;
	MGVertex(glm::vec3 position, glm::vec2 uv) : position{ position }, uv{ uv } {}
	MGVertex(glm::vec3 position, glm::vec2 uv, float brightness) :
		position{ position },
		uv{ uv },
		brightness{ brightness }
	{
	}
	MGVertex(glm::vec3 position, glm::vec2 uv, float brightness, glm::vec3 normal) :
		position{ position },
		uv{ uv },
		brightness{ brightness },
		normal{ normal }
	{
	}
};

class MapGeometryBuffer3D : public gl::VertexBufferBase<MGVertex>
{
public:
	MapGeometryBuffer3D() = default;

private:
	void initVAO() override;
};
} // namespace slade::mapeditor
