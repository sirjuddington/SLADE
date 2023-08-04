
#include "Main.h"
#include "Draw2D.h"
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
glm::mat4      identity_matrix(1.f);
gl::LineBuffer lb_lines;
} // namespace
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
		shader_2d.define("TEXTURED");
		shader_2d.loadResourceEntries("default2d.vert", "default2d.frag");
		shader_2d_notex.loadResourceEntries("default2d.vert", "default2d.frag");
	}

	return textured ? shader_2d : shader_2d_notex;
}

const Shader& draw2d::linesShader()
{
	static Shader shader_lines("default2d_lines");

	if (!shader_lines.isValid())
	{
		shader_lines.define("THICK_LINES");
		shader_lines.loadResourceEntries("default2d.vert", "default2d.frag");
	}

	return shader_lines;
}

const Shader& draw2d::pointSpriteShader(PointSprite type)
{
	static Shader shader_psprite_tex("default2d_pointsprite_tex");
	static Shader shader_psprite_circle("default2d_pointsprite_circle");

	if (!shader_psprite_tex.isValid())
	{
		// PointSprite::Textured
		shader_psprite_tex.define("GEOMETRY_SHADER");
		shader_psprite_tex.define("TEXTURED");
		shader_psprite_tex.loadResourceEntries("default2d.vert", "default2d.frag", "point_sprite.geom");

		// PointSprite::Circle
		shader_psprite_circle.define("GEOMETRY_SHADER");
		shader_psprite_circle.loadResourceEntries("default2d.vert", "circle.frag", "point_sprite.geom");
	}

	switch (type)
	{
	case PointSprite::Textured: return shader_psprite_tex;
	case PointSprite::Circle: return shader_psprite_circle;
	default: return shader_psprite_tex;
	}
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

void draw2d::drawLines(const vector<Rectf>& lines, const RenderOptions& opt, const View* view)
{
	lb_lines.clear();
	auto colour = opt.colour.asVec4();
	for (const auto& line : lines)
		lb_lines.add2d(line.x1(), line.y1(), line.x2(), line.y2(), colour, opt.line_thickness);
	lb_lines.setAaRadius(opt.line_aa_radius, opt.line_aa_radius);

	lb_lines.draw(view);
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
	lb_hud->draw(view);
}
