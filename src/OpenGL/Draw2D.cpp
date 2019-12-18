
#include "Main.h"
#include "Draw2D.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "GLTexture.h"
#include "Shader.h"
#include "UI/Canvas/GLCanvas.h"
#include "VertexBuffer2D.h"
#include <glm/ext/matrix_transform.hpp>

using namespace OpenGL;

namespace
{
glm::mat4 identity_matrix(1.f);
}

namespace OpenGL::Draw2D
{
const Shader& setOptions(const RenderOptions& opt)
{
	const auto& shader = defaultShader(opt.texture > 0);
	shader.bind();

	// Texture
	if (opt.texture > 0)
	{
		glEnable(GL_TEXTURE_2D);
		Texture::bind(opt.texture);
	}

	// Colour
	shader.setUniform("colour", glm::vec4(opt.colour.fr(), opt.colour.fg(), opt.colour.fb(), opt.colour.fa()));

	// Blending
	if (opt.blend != Blend::Ignore)
		OpenGL::setBlend(opt.blend);

	return shader;
}
} // namespace OpenGL::Draw2D


const Shader& OpenGL::Draw2D::defaultShader(bool textured)
{
	static Shader shader_2d("default2d");
	static Shader shader_2d_notex("default2d_notex");

	if (!shader_2d.isValid())
	{
		string shader_vert, shader_frag, shader_frag_notex;

		auto program_resource = App::archiveManager().programResourceArchive();
		if (program_resource)
		{
			auto entry_vert       = program_resource->entryAtPath("shaders/default2d.vert");
			auto entry_frag       = program_resource->entryAtPath("shaders/default2d.frag");
			auto entry_frag_notex = program_resource->entryAtPath("shaders/default2d_notex.frag");
			if (entry_vert && entry_frag && entry_frag_notex)
			{
				shader_vert.assign(reinterpret_cast<const char*>(entry_vert->rawData()), entry_vert->size());
				shader_frag.assign(reinterpret_cast<const char*>(entry_frag->rawData()), entry_frag->size());
				shader_frag_notex.assign(
					reinterpret_cast<const char*>(entry_frag_notex->rawData()), entry_frag_notex->size());
				shader_2d.load(shader_vert, shader_frag);
				shader_2d_notex.load(shader_vert, shader_frag_notex);
			}
			else
				Log::error("Unable to find default 2d shaders in the program resource!");
		}
	}

	return textured ? shader_2d : shader_2d_notex;
}

void OpenGL::Draw2D::setupFor2D(const Shader& shader, const GLCanvas& canvas)
{
	shader.bind();
	shader.setUniform("projection", canvas.orthoProjectionMatrix());
	shader.setUniform("model", glm::mat4(1.f));
	shader.setUniform("colour", glm::vec4(1.f, 1.f, 1.f, 1.f));
}

void Draw2D::drawRect(Rectf rect, const RenderOptions& opt)
{
	auto& shader = setOptions(opt);

	auto model = glm::translate(identity_matrix, { rect.tl.x, rect.tl.y, 0.f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.f });
	shader.setUniform("model", model);

	VertexBuffer2D::unitSquare().draw(Primitive::Quads);

	shader.setUniform("model", identity_matrix);
}

void OpenGL::Draw2D::drawRectOutline(Rectf rect, const RenderOptions& opt)
{
	auto& shader = setOptions(opt);

	auto model = glm::translate(identity_matrix, { rect.tl.x, rect.tl.y, 0.f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.f });
	shader.setUniform("model", model);

	glLineWidth(opt.line_thickness); // TODO: Geometry shader for thickness > 1.f

	VertexBuffer2D::unitSquare().draw(Primitive::LineLoop);

	shader.setUniform("model", identity_matrix);
}
