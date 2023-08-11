
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

void setupTextShader(const Shader& shader, const Context& dc, const ColRGBA& colour, float softness)
{
	shader.bind();
	shader.setUniform("colour", colour.asVec4());
	shader.setUniform("mvp", dc.view ? dc.view->mvpMatrix(dc.model_matrix) : dc.model_matrix);
	shader.setUniform("softness", softness);
	if (dc.view)
		shader.setUniform("viewport_size", glm::vec2(dc.view->size().x, dc.view->size().y));
	if (dc.text_style == TextStyle::Outline)
		shader.setUniform("outline_colour", dc.outline_colour.asVec4());
}
} // namespace slade::gl::draw2d


void draw2d::Context::translate(float x, float y)
{
	model_matrix = glm::translate(model_matrix, { x, y, 0.0f });
}

void draw2d::Context::scale(float x, float y)
{
	model_matrix = glm::scale(model_matrix, { x, y, 1.0f });
}

float draw2d::Context::textLineHeight() const
{
	return draw2d::lineHeight(font, text_size);
}

Vec2f draw2d::Context::textExtents(const string& text) const
{
	return draw2d::textExtents(text, font, text_size);
}

void draw2d::Context::drawRect(const Rectf& rect) const
{
	const auto& shader = defaultShader(texture > 0);
	shader.bind();

	// Texture
	if (texture > 0)
	{
		glEnable(GL_TEXTURE_2D);
		Texture::bind(texture);
	}
	else
		glDisable(GL_TEXTURE_2D);

	// Colour
	shader.setUniform("colour", colour.asVec4());

	// Blending
	if (blend != Blend::Ignore)
		gl::setBlend(blend);

	auto model = glm::translate(model_matrix, { rect.tl.x, rect.tl.y, 0.f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.f });
	shader.setUniform("mvp", view ? view->mvpMatrix(model) : model);

	VertexBuffer2D::unitSquare().draw(Primitive::Quads);
}

void draw2d::Context::drawRectOutline(const Rectf& rect) const
{
	auto& shader = defaultShader(false);

	// Texture
	glDisable(GL_TEXTURE_2D);

	// Colour
	shader.setUniform("colour", colour.asVec4());

	// Blending
	if (blend != Blend::Ignore)
		gl::setBlend(blend);

	auto model = glm::translate(model_matrix, { rect.tl.x, rect.tl.y, 0.f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.f });
	shader.setUniform("mvp", view ? view->mvpMatrix(model) : model);

	glLineWidth(line_thickness);

	VertexBuffer2D::unitSquare().draw(Primitive::LineLoop);
}

void draw2d::Context::drawLines(const vector<Rectf>& lines) const
{
	// Build line buffer
	lb_lines.clear();
	auto col = colour.asVec4();
	for (const auto& line : lines)
		lb_lines.add2d(line.x1(), line.y1(), line.x2(), line.y2(), col, line_thickness);
	lb_lines.setAaRadius(line_aa_radius, line_aa_radius);

	// Blending
	if (blend != Blend::Ignore)
		gl::setBlend(blend);

	lb_lines.draw(view, glm::vec4{ 1.0f }, model_matrix);
}

void draw2d::Context::drawText(const string& text, const Vec2f& pos) const
{
	// TODO: Improve DropShadow rendering
	// Using just the shader doesn't really allow for the shadow to be offset much,
	// so it isn't very visible for small text.
	// Might need to do something like render the text twice, once for the shadow and once normally on top
	// (we have a vertex buffer for the string so it'd just be 1 extra draw call)

	if (!text_draw_init)
		initTextDrawing();

	// Get+setup font
	auto fdef = getFont(font);
	if (!fdef || !fdef->handle)
		return;
	dtx_use_font(fdef->handle, FONT_SIZE_BASE);

	// Determine offset
	auto line_height = (dtx_baseline() - dtx_line_height()) * fontScale(text_size);
	switch (text_alignment)
	{
	case Align::Right:
	{
		auto size     = textExtents(text);
		text_offset.x = pos.x - size.x;
		text_offset.y = pos.y + line_height;
		break;
	}
	case Align::Center:
	{
		auto size     = textExtents(text);
		text_offset.x = pos.x - size.x * 0.5f;
		text_offset.y = pos.y + line_height;
		break;
	}
	default:
		text_offset.x = pos.x;
		text_offset.y = pos.y + line_height;
		break;
	}

	// Sharpen softness value if the view is zoomed in
	auto scale      = fontScale(text_size);
	auto full_scale = scale;
	if (view && view->scale().x > 1)
		full_scale *= view->scale().x;
	text_scale = scale;

	// Create shaders if needed
	static Shader shader_text("text");
	static Shader shader_text_outline("text_outline");
	if (!shader_text.isValid())
	{
		shader_text.loadResourceEntries("default2d.vert", "text.frag");
		shader_text_outline.loadResourceEntries("default2d.vert", "text_outline.frag");
	}

	// Draw drop shadow if needed
	const Shader* shader = nullptr;
	if (text_dropshadow)
	{
		shader = &shader_text;
		setupTextShader(*shader, *this, text_dropshadow_colour, 0.1f);
		auto prev_offset = text_offset;
		text_offset.x += 2;
		text_offset.y += 2;
		dtx_string(text.c_str());
		text_offset = prev_offset;
	}

	// Setup shader
	switch (text_style)
	{
	case TextStyle::Normal: shader = &shader_text; break;
	case TextStyle::Outline: shader = &shader_text_outline; break;
	default: shader = &shader_text; break;
	}
	setupTextShader(*shader, *this, colour, fdef->softness / full_scale);

	// Draw the text
	dtx_string(text.c_str());
}

void draw2d::Context::drawHud() const
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

		glm::vec4 col{ 0.0f, 0.0f, 0.0f, 1.0f };

		// (320/354)x200 screen outline
		float right = hud_wide ? 337.0f : 320.0f;
		float left  = hud_wide ? -17.0f : 0.0f;
		lb_hud->add2d(left, 0.0f, left, 200.0f, col, 1.5f);
		lb_hud->add2d(left, 200.0f, right, 200.0f, col, 1.5f);
		lb_hud->add2d(right, 200.0f, right, 0.0f, col, 1.5f);
		lb_hud->add2d(right, 0.0f, left, 0.0f, col, 1.5f);

		// Statusbar line(s)
		col.a = 0.5f;
		if (hud_statusbar)
		{
			lb_hud->add2d(left, 168.0f, right, 168.0f, col); // Doom's status bar: 32 pixels tall
			lb_hud->add2d(left, 162.0f, right, 162.0f, col); // Hexen: 38 pixels
			lb_hud->add2d(left, 158.0f, right, 158.0f, col); // Heretic: 42 pixels
		}

		// Center lines
		if (hud_center)
		{
			lb_hud->add2d(left, 100.0f, right, 100.0f, col);
			lb_hud->add2d(160.0f, 0.0f, 160.0f, 200.0f, col);
		}

		// Normal screen edge guides if widescreen
		if (hud_wide)
		{
			lb_hud->add2d(0.0f, 0.0f, 0.0f, 200.0f, col);
			lb_hud->add2d(320.0f, 0.0f, 320.0f, 200.0f, col);
		}

		// Weapon bobbing guides
		if (hud_bob)
		{
			lb_hud->add2d(left - 16.0f, -16.0f, left - 16.0f, 216.0f, col, 0.8f);
			lb_hud->add2d(left - 16.0f, 216.0f, right + 16.0f, 216.0f, col, 0.8f);
			lb_hud->add2d(right + 16.0f, 216.0f, right + 16.0f, -16.0f, col, 0.8f);
			lb_hud->add2d(right + 16.0f, -16.0f, left - 16.0f, -16.0f, col, 0.8f);
		}

		hud_wide_prev      = hud_wide;
		hud_statusbar_prev = hud_statusbar;
		hud_center_prev    = hud_center;
		hud_bob_prev       = hud_bob;
	}

	// Draw the hud lines
	lb_hud->draw(view, glm::vec4{ 1.0f }, model_matrix);
}





// -----------------------------------------------------------------------------
//
// TextBox Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextBox class constructor
// -----------------------------------------------------------------------------
draw2d::TextBox::TextBox(string_view text, float width, Font font, int font_size, float line_height) :
	font_{ font }, font_size_{ font_size }, width_{ width }, line_height_{ line_height }
{
	setText(text);
}

// -----------------------------------------------------------------------------
// Splits [text] into separate lines (split by newlines), also performs further
// splitting to word wrap the text within the box
// -----------------------------------------------------------------------------
void draw2d::TextBox::split(string_view text)
{
	// Clear current text lines
	lines_.clear();

	// Do nothing for empty string
	if (text.empty())
		return;

	// Split at newlines
	auto split = strutil::splitV(text, '\n');
	for (auto& line : split)
		lines_.emplace_back(line);

	// Don't bother wrapping if width is really small
	if (width_ < 32)
		return;

	// Word wrap
	unsigned line = 0;
	while (line < lines_.size())
	{
		// Ignore empty or single-character line
		if (lines_[line].length() < 2)
		{
			line++;
			continue;
		}

		// Get line width
		auto width = textExtents(lines_[line], font_, font_size_).x;

		// Continue to next line if within box
		if (width < width_)
		{
			line++;
			continue;
		}

		// Halve length until it fits in the box
		unsigned c = lines_[line].length() - 1;
		while (width >= width_)
		{
			if (c <= 1)
				break;

			c *= 0.5;
			width = textExtents(lines_[line].substr(0, c), font_, font_size_).x;
		}

		// Increment length until it doesn't fit
		while (width < width_)
		{
			c++;
			width = textExtents(lines_[line].substr(0, c), font_, font_size_).x;
		}
		c--;

		// Find previous space
		int sc = c;
		while (sc >= 0)
		{
			if (lines_[line][sc] == ' ')
				break;
			sc--;
		}
		if (sc <= 0)
			sc = c;
		else
			sc++;

		// Split line
		auto nl      = lines_[line].substr(sc);
		lines_[line] = lines_[line].substr(0, sc);
		lines_.insert(lines_.begin() + line + 1, nl);
		line++;
	}

	// Update height
	height_ = lines_.size() * (lineHeight(font_, font_size_) * line_height_);
}

// -----------------------------------------------------------------------------
// Sets the text box text
// -----------------------------------------------------------------------------
void draw2d::TextBox::setText(string_view text)
{
	text_ = text;
	split(text);
}

// -----------------------------------------------------------------------------
// Sets the text box width
// -----------------------------------------------------------------------------
void draw2d::TextBox::setWidth(float width)
{
	width_ = width;
	split(text_);
}

// -----------------------------------------------------------------------------
// Draws the text box
// -----------------------------------------------------------------------------
void draw2d::TextBox::draw(Vec2f& pos, Context& dc) const
{
	auto prev_font = dc.font;
	auto prev_size = dc.text_size;

	for (const auto& line : lines_)
	{
		dc.drawText(line, pos);
		pos.y += dc.textLineHeight() * line_height_;
	}

	dc.font      = prev_font;
	dc.text_size = prev_size;
}






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
