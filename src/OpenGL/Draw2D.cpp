
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Draw2D.cpp
// Description: Various OpenGL 2D drawing functions and related classes
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------

// FONT HANDLING
// Load each font face in 36px size (our 'default' is 18px, so 2x that)
// Each font face has custom smoothness value
// When drawing with the font, calculate scale required to get desired pixel size
// Smoothness value is scaled inverse, eg.
// 18px = 0.5x scale, 2x smoothness


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Draw2D.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "GLTexture.h"
#include "LineBuffer.h"
#include "Shader.h"
#include "VertexBuffer2D.h"
#include "View.h"
#include "thirdparty/libdrawtext/drawtext.h"
#include <glm/ext/matrix_transform.hpp>

using namespace slade;
using namespace gl;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
glm::mat4      identity_matrix(1.f);
gl::LineBuffer lb_lines;

// Fonts & text rendering
struct FontDef
{
	string    face;
	float     softness = 0.015f;
	dtx_font* handle   = nullptr;
};
constexpr int      FONT_SIZE_BASE = 36;
vector<FontDef>    fonts          = { { "FiraSans-Regular", 0.018f, nullptr },          // Normal
									  { "FiraSans-Bold", 0.018f, nullptr },             // Bold
									  { "FiraSansCondensed-Regular", 0.018f, nullptr }, // Condensed
									  { "FiraSansCondensed-Bold", 0.018f, nullptr },    // CondensedBold
									  { "FiraMono-Medium", 0.02f, nullptr },            // Monospace
									  { "FiraMono-Bold", 0.02f, nullptr } };            // MonospaceBold
gl::VertexBuffer2D vb_text;
bool               text_draw_init = false;
glm::vec2          text_offset;
float              text_scale = 1.0f;
} // namespace


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, hud_statusbar)
EXTERN_CVAR(Bool, hud_center)
EXTERN_CVAR(Bool, hud_wide)
EXTERN_CVAR(Bool, hud_bob)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
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

bool loadFont(FontDef& font)
{
	auto entry = app::archiveManager().programResourceArchive()->entryAtPath(fmt::format("fonts/{}.gm", font.face));
	if (!entry)
	{
		log::error("Font {} does not exist in slade.pk3", font.face);
		return false;
	}

	font.handle = dtx_open_font_glyphmap_mem(const_cast<uint8_t*>(entry->rawData()), entry->size());

	if (!font.handle)
	{
		log::error("Error loading font {} glyphmap", font.face);
		return false;
	}

	return true;
}

FontDef* getFont(Font font)
{
	auto* fdef = &fonts[static_cast<int>(font)];

	if (!fdef->handle)
	{
		if (!loadFont(*fdef))
			return nullptr;
	}

	return fdef;
}

inline float fontScale(int size)
{
	constexpr auto font_size_base = static_cast<float>(FONT_SIZE_BASE);
	return static_cast<float>(size) / font_size_base;
}

void drawTextCustomCallback(dtx_vertex* v, int vcount, dtx_pixmap* pixmap, void* cls)
{
	// Generate texture if needed
	auto tex = reinterpret_cast<unsigned>(pixmap->udata);
	if (tex == 0)
	{
		tex = gl::Texture::create(TexFilter::Linear, false);
		gl::Texture::loadAlphaData(tex, pixmap->pixels, pixmap->width, pixmap->height);
		pixmap->udata = reinterpret_cast<void*>(tex);
	}

	glm::vec4 colour{ 1.0f };
	vb_text.clear();
	for (int i = 0; i < vcount; ++i)
	{
		vb_text.add({ text_offset.x + v->x * text_scale, text_offset.y - v->y * text_scale }, colour, { v->s, v->t });
		++v;
	}

	glEnable(GL_TEXTURE_2D);
	gl::Texture::bind(tex);
	gl::setBlend(gl::Blend::Normal);

	vb_text.draw();
}

void initTextDrawing()
{
	dtx_set(DTX_PADDING, 64);

	// Register custom callback for rendering text via libdrawtext
	dtx_target_user(drawTextCustomCallback, nullptr);

	text_draw_init = true;
}
} // namespace slade::gl::draw2d


float draw2d::lineHeight(Font font, int size)
{
	auto fdef = getFont(font);
	if (fdef)
	{
		dtx_use_font(fdef->handle, FONT_SIZE_BASE);
		return dtx_line_height() * fontScale(size);
	}

	return 0.0f;
}

Vec2f draw2d::textExtents(const string& text, Font font, int size)
{
	auto fdef = getFont(font);
	if (fdef)
	{
		dtx_box box;
		auto    scale = fontScale(size);

		dtx_use_font(fdef->handle, FONT_SIZE_BASE);
		dtx_string_box(text.c_str(), &box);
		return { (box.width - box.x) * scale, box.height * scale };
	}

	return {};
}

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

void draw2d::drawText(const string& text, const Vec2f& pos, const TextOptions& opt, const View* view)
{
	// TODO: Improve DropShadow rendering
	// Using just the shader doesn't really allow for the shadow to be offset much,
	// so it isn't very visible for small text.
	// Might need to do something like render the text twice, once for the shadow and once normally on top
	// (we have a vertex buffer for the string so it'd just be 1 extra draw call)

	if (!text_draw_init)
		initTextDrawing();

	// Get+setup font
	auto font = getFont(opt.font);
	if (!font)
		return;

	// Determine offset
	switch (opt.alignment)
	{
	case Align::Right:
	{
		auto size     = textExtents(text, opt.font, opt.size);
		text_offset.x = pos.x - size.x;
		text_offset.y = pos.y;
		break;
	}
	case Align::Center:
	{
		auto size     = textExtents(text, opt.font, opt.size);
		text_offset.x = pos.x - size.x * 0.5f;
		text_offset.y = pos.y;
		break;
	}
	default:
		text_offset.x = pos.x;
		text_offset.y = pos.y;
		break;
	}

	// Sharpen softness value if the view is zoomed in
	auto scale      = fontScale(opt.size);
	auto full_scale = scale;
	if (view && view->scale().x > 1)
		full_scale *= view->scale().x;

	// Create shaders if needed
	static Shader shader_text("text");
	static Shader shader_text_outline("text_outline");
	static Shader shader_text_dropshadow("text_dropshadow");
	if (!shader_text.isValid())
	{
		shader_text.loadResourceEntries("default2d.vert", "text.frag");
		shader_text_outline.loadResourceEntries("default2d.vert", "text_outline.frag");
		shader_text_dropshadow.loadResourceEntries("default2d.vert", "text_dropshadow.frag");
	}

	// Setup shader
	const Shader* shader = nullptr;
	switch (opt.style)
	{
	case TextStyle::Normal: shader = &shader_text; break;
	case TextStyle::Outline: shader = &shader_text_outline; break;
	case TextStyle::DropShadow: shader = &shader_text_dropshadow; break;
	}
	shader->bind();
	shader->setUniform("colour", glm::vec4{ 1.0f });
	shader->setUniform("mvp", view ? view->mvpMatrix(identity_matrix) : identity_matrix);
	shader->setUniform("softness", font->softness / full_scale);
	if (view)
		shader->setUniform("viewport_size", glm::vec2(view->size().x, view->size().y));
	if (opt.style == TextStyle::Outline)
		shader->setUniform("outline_colour", opt.outline_shadow_colour.asVec4());
	if (opt.style == TextStyle::DropShadow)
		shader->setUniform("shadow_colour", opt.outline_shadow_colour.asVec4());

	// Draw the text
	text_scale = scale;
	dtx_use_font(font->handle, FONT_SIZE_BASE);
	dtx_string(text.c_str());
}



#include "General/Console.h"

CONSOLE_COMMAND(gen_glyphmap, 1, false)
{
	dtx_set(DTX_PADDING, 64);

	auto font = dtx_open_font(args[0].c_str(), 0);
	if (!font)
	{
		log::error("Unable to open font file");
		return;
	}

	auto mult = 8;
	dtx_prepare_range(font, FONT_SIZE_BASE * mult, 32, 127);
	dtx_calc_font_distfield(font, 1, mult);

	auto gm_fn   = fmt::format("{}.gm", strutil::Path::fileNameOf(args[0], false));
	auto gm_path = app::path(gm_fn, app::Dir::Data);
	dtx_save_glyphmap(gm_path.c_str(), dtx_get_glyphmap(font, 0));
}
