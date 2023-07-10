
#include "Main.h"
#include "Draw2D.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "GLTexture.h"
#include "Shader.h"
#include "UI/Canvas/GLCanvas.h"
#include "VertexBuffer2D.h"
#include <glm/ext/matrix_transform.hpp>

using namespace slade;
using namespace gl;

namespace
{
glm::mat4 identity_matrix(1.f);
}

namespace slade::gl::draw2d
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
		gl::setBlend(opt.blend);

	return shader;
}
} // namespace slade::gl::draw2d


const Shader& draw2d::defaultShader(bool textured)
{
	static Shader shader_2d("default2d");
	static Shader shader_2d_notex("default2d_notex");

	if (!shader_2d.isValid())
	{
		string shader_vert, shader_frag;

		auto program_resource = app::archiveManager().programResourceArchive();
		if (program_resource)
		{
			auto entry_vert = program_resource->entryAtPath("shaders/default2d.vert");
			auto entry_frag = program_resource->entryAtPath("shaders/default2d.frag");
			if (entry_vert && entry_frag)
			{
				shader_vert.assign(reinterpret_cast<const char*>(entry_vert->rawData()), entry_vert->size());
				shader_frag.assign(reinterpret_cast<const char*>(entry_frag->rawData()), entry_frag->size());
				shader_2d.define("TEXTURED");
				shader_2d.load(shader_vert, shader_frag);
				shader_2d_notex.load(shader_vert, shader_frag);
			}
			else
				log::error("Unable to find default 2d shaders in the program resource!");
		}
	}

	return textured ? shader_2d : shader_2d_notex;
}

const Shader& draw2d::linesShader()
{
	static Shader shader_lines("default2d_lines");

	if (!shader_lines.isValid())
	{
		auto program_resource = app::archiveManager().programResourceArchive();
		if (program_resource)
		{
			auto entry_vert = program_resource->entryAtPath("shaders/default2d.vert");
			auto entry_frag = program_resource->entryAtPath("shaders/default2d.frag");
			if (entry_vert && entry_frag)
			{
				shader_lines.define("THICK_LINES");
				shader_lines.load(entry_vert->data().asString(), entry_frag->data().asString());
			}
			else
				log::error("Unable to find default 2d shaders in the program resource!");
		}
	}

	return shader_lines;
}

const Shader& draw2d::pointSpriteShader()
{
	static Shader shader_psprite("default2d_pointsprite");

	if (!shader_psprite.isValid())
	{
		auto program_resource = app::archiveManager().programResourceArchive();
		if (program_resource)
		{
			auto entry_vert = program_resource->entryAtPath("shaders/default2d.vert");
			auto entry_geom = program_resource->entryAtPath("shaders/point_sprite.geom");
			auto entry_frag = program_resource->entryAtPath("shaders/default2d.frag");
			if (entry_vert && entry_frag && entry_geom)
			{
				shader_psprite.define("TEXTURED");
				shader_psprite.define("GEOMETRY_SHADER");
				shader_psprite.load(
					entry_vert->data().asString(), entry_frag->data().asString(), entry_geom->data().asString());
			}
			else
				log::error("Unable to find default 2d shaders in the program resource!");
		}
	}

	return shader_psprite;
}

void draw2d::drawRect(Rectf rect, const RenderOptions& opt)
{
	auto& shader = setOptions(opt);

	auto model = glm::translate(identity_matrix, { rect.tl.x, rect.tl.y, 0.f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.f });
	shader.setUniform("model", model);

	VertexBuffer2D::unitSquare().draw(Primitive::Quads);

	shader.setUniform("model", identity_matrix);
}

void draw2d::drawRectOutline(Rectf rect, const RenderOptions& opt)
{
	auto& shader = setOptions(opt);

	auto model = glm::translate(identity_matrix, { rect.tl.x, rect.tl.y, 0.f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.f });
	shader.setUniform("model", model);

	glLineWidth(opt.line_thickness); // TODO: Geometry shader for thickness > 1.f

	VertexBuffer2D::unitSquare().draw(Primitive::LineLoop);

	shader.setUniform("model", identity_matrix);
}
