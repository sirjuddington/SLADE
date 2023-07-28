
#include "Main.h"
#include "Draw2D.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "GLTexture.h"
#include "LineBuffer.h"
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
EXTERN_CVAR(Bool, hud_statusbar)
EXTERN_CVAR(Bool, hud_center)
EXTERN_CVAR(Bool, hud_wide)
EXTERN_CVAR(Bool, hud_bob)

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
	else
		glDisable(GL_TEXTURE_2D);

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

const Shader& draw2d::pointSpriteShader(PointSprite type)
{
	static Shader shader_psprite("default2d_pointsprite");

	if (!shader_psprite.isValid())
	{
		auto program_resource = app::archiveManager().programResourceArchive();
		if (program_resource)
		{
			auto fragment_shader = "shaders/default2d.frag";

			if (type == PointSprite::Circle)
				fragment_shader = "shaders/circle.frag";

			auto entry_vert = program_resource->entryAtPath("shaders/default2d.vert");
			auto entry_geom = program_resource->entryAtPath("shaders/point_sprite.geom");
			auto entry_frag = program_resource->entryAtPath(fragment_shader);
			if (entry_vert && entry_frag && entry_geom)
			{
				if (type == PointSprite::Textured)
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

void draw2d::drawRect(Rectf rect, const RenderOptions& opt, const View* view)
{
	auto& shader = setOptions(opt);

	auto model = glm::translate(identity_matrix, { rect.tl.x, rect.tl.y, 0.f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.f });
	shader.setUniform("mvp", view ? view->mvpMatrix(model) : model);

	VertexBuffer2D::unitSquare().draw(Primitive::Quads);
}

void draw2d::drawRectOutline(Rectf rect, const RenderOptions& opt, const View* view)
{
	auto& shader = setOptions(opt);

	auto model = glm::translate(identity_matrix, { rect.tl.x, rect.tl.y, 0.f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.f });
	shader.setUniform("mvp", view ? view->mvpMatrix(model) : model);

	glLineWidth(opt.line_thickness);

	VertexBuffer2D::unitSquare().draw(Primitive::LineLoop);
}

void draw2d::drawHud(const View* view)
{
	static unique_ptr<LineBuffer> lb_hud;
	static bool                   hud_wide_prev, hud_statusbar_prev, hud_center_prev, hud_bob_prev;

	// Create hud line buffer when called for the first time
	if (!lb_hud)
	{
		lb_hud        = std::make_unique<LineBuffer>();
		hud_wide_prev = !hud_wide; // Force buffer rebuild
	}

	// Rebuild line buffer if hud drawing options changed
	if (hud_wide != hud_wide_prev || hud_statusbar != hud_statusbar_prev || hud_center != hud_center_prev
		|| hud_bob != hud_bob_prev)
	{
		lb_hud->clear();

		glm::vec4 colour{ 0.0f, 0.0f, 0.0f, 1.0f };

		// (320/354)x200 screen outline
		float right = hud_wide ? 337.0f : 320.0f;
		float left  = hud_wide ? -17.0f : 0.0f;
		lb_hud->add2d(left, 0.0f, left, 200.0f, colour, 1.5f);
		lb_hud->add2d(left, 200.0f, right, 200.0f, colour, 1.5f);
		lb_hud->add2d(right, 200.0f, right, 0.0f, colour, 1.5f);
		lb_hud->add2d(right, 0.0f, left, 0.0f, colour, 1.5f);

		// Statusbar line(s)
		colour.a = 0.5f;
		if (hud_statusbar)
		{
			lb_hud->add2d(left, 168.0f, right, 168.0f, colour); // Doom's status bar: 32 pixels tall
			lb_hud->add2d(left, 162.0f, right, 162.0f, colour); // Hexen: 38 pixels
			lb_hud->add2d(left, 158.0f, right, 158.0f, colour); // Heretic: 42 pixels
		}

		// Center lines
		if (hud_center)
		{
			lb_hud->add2d(left, 100.0f, right, 100.0f, colour);
			lb_hud->add2d(160.0f, 0.0f, 160.0f, 200.0f, colour);
		}

		// Normal screen edge guides if widescreen
		if (hud_wide)
		{
			lb_hud->add2d(0.0f, 0.0f, 0.0f, 200.0f, colour);
			lb_hud->add2d(320.0f, 0.0f, 320.0f, 200.0f, colour);
		}

		// Weapon bobbing guides
		if (hud_bob)
		{
			lb_hud->add2d(left - 16.0f, -16.0f, left - 16.0f, 216.0f, colour, 0.8f);
			lb_hud->add2d(left - 16.0f, 216.0f, right + 16.0f, 216.0f, colour, 0.8f);
			lb_hud->add2d(right + 16.0f, 216.0f, right + 16.0f, -16.0f, colour, 0.8f);
			lb_hud->add2d(right + 16.0f, -16.0f, left - 16.0f, -16.0f, colour, 0.8f);
		}

		hud_wide_prev      = hud_wide;
		hud_statusbar_prev = hud_statusbar;
		hud_center_prev    = hud_center;
		hud_bob_prev       = hud_bob;
	}

	// Draw the hud lines
	const auto& shader = lb_hud->shader();
	if (view)
		view->setupShader(shader);
	lb_hud->draw();
}
