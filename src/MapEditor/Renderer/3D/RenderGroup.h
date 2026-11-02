#pragma once

#include "RenderPass.h"

namespace slade::gl
{
class IndexBuffer;
} // namespace slade::gl

namespace slade::mapeditor
{
// Groups same-texture/material geometry into a single draw call
struct RenderGroup
{
	unsigned                    texture;
	glm::vec4                   colour;
	glm::vec3                   fade_colour;
	unique_ptr<gl::IndexBuffer> index_buffer;
	RenderPass                  render_pass;
	bool                        trans_additive = false;
};
} // namespace slade::mapeditor
