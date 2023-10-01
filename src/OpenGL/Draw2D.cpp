
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
#include "General/ColourConfiguration.h"
#include "LineBuffer.h"
#include "PointSpriteBuffer.h"
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
glm::mat4 identity_matrix(1.f);
glm::vec4 col_white{ 1.0f };

// Buffers
unique_ptr<LineBuffer>        line_buffer;
unique_ptr<VertexBuffer2D>    vertex_buffer;
unique_ptr<PointSpriteBuffer> ps_buffer;

// Shaders
unique_ptr<Shader> shader_2d;
unique_ptr<Shader> shader_2d_notex;
unique_ptr<Shader> shader_line_stipple;

// Fonts & text rendering
struct FontDef
{
	string    face;
	float     softness = 0.015f;
	dtx_font* handle   = nullptr;
	float     yoff     = 0.08f;
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
CVAR(Bool, hud_statusbar, 1, CVar::Flag::Save)
CVAR(Bool, hud_center, 1, CVar::Flag::Save)
CVAR(Bool, hud_wide, 0, CVar::Flag::Save)
CVAR(Bool, hud_bob, 0, CVar::Flag::Save)


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
	unsigned tex;
	if (!pixmap->udata)
	{
		tex = gl::Texture::create(TexFilter::Linear, false);
		gl::Texture::loadAlphaData(tex, pixmap->pixels, pixmap->width, pixmap->height);
		pixmap->udata = new unsigned{ tex };
	}
	else
		tex = *static_cast<unsigned*>(pixmap->udata);

	glm::vec4 colour{ 1.0f };
	for (int i = 0; i < vcount; ++i)
	{
		vb_text.add({ text_offset.x + v->x * text_scale, text_offset.y - v->y * text_scale }, colour, { v->s, v->t });
		++v;
	}
	vb_text.push();

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

void drawPointSprites(const Context& dc)
{
	// Blending
	if (dc.blend != Blend::Ignore)
		gl::setBlend(dc.blend);

	// Texture
	if (dc.pointsprite_type == PointSpriteType::Textured)
	{
		glEnable(GL_TEXTURE_2D);
		gl::Texture::bind(dc.texture);
	}

	// Set buffer options
	ps_buffer->setColour(dc.colour.asVec4());
	ps_buffer->setPointRadius(dc.pointsprite_radius);
	ps_buffer->setOutlineWidth(dc.pointsprite_outline_width);
	ps_buffer->setFillOpacity(dc.pointsprite_fill_opacity);

	// Draw
	ps_buffer->draw(dc.pointsprite_type, dc.view);
}
} // namespace slade::gl::draw2d


Vec2f draw2d::Context::viewSize() const
{
	return view ? Vec2f{ static_cast<float>(view->size().x), static_cast<float>(view->size().y) } : Vec2f{};
}

void draw2d::Context::translate(float x, float y)
{
	model_matrix = glm::translate(model_matrix, { x, y, 0.0f });
}

void draw2d::Context::scale(float x, float y)
{
	model_matrix = glm::scale(model_matrix, { x, y, 1.0f });
}

void draw2d::Context::setColourFromConfig(const string& def_name, float alpha, bool blend)
{
	auto def = colourconfig::colDef(def_name);
	colour   = def.colour;
	colour.a *= alpha;
	if (blend)
		this->blend = def.blendMode();
}

float draw2d::Context::textLineHeight() const
{
	return draw2d::lineHeight(font, text_size);
}

Vec2f draw2d::Context::textExtents(const string& text) const
{
	return draw2d::textExtents(text, font, text_size);
}

void draw2d::Context::setupToDraw(const Shader& shader, bool mvp) const
{
	shader.bind();

	// Texture
	Texture::bind(texture);

	// Colour
	shader.setUniform("colour", colour.asVec4());

	// Blending
	if (blend != Blend::Ignore)
		gl::setBlend(blend);

	// MVP matrix
	if (mvp)
		shader.setUniform("mvp", view ? view->mvpMatrix(model_matrix) : model_matrix);
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
	static LineBuffer lb_rect;

	if (lb_rect.buffer().empty())
	{
		lb_rect.add2d(0.0f, 0.0f, 0.0f, 1.0f, col_white);
		lb_rect.add2d(0.0f, 1.0f, 1.0f, 1.0f, col_white);
		lb_rect.add2d(1.0f, 1.0f, 1.0f, 0.0f, col_white);
		lb_rect.add2d(1.0f, 0.0f, 0.0f, 0.0f, col_white);
		lb_rect.push();
	}

	auto model = glm::translate(model_matrix, { rect.tl.x, rect.tl.y, 0.0f });
	model      = glm::scale(model, { rect.width(), rect.height(), 1.0f });

	lb_rect.setWidthMult(line_thickness);
	if (line_thickness != 1.0f)
		lb_rect.setAaRadius(line_aa_radius, line_aa_radius);
	else
		lb_rect.setAaRadius(0.0f, 0.0f);
	lb_rect.draw(view, colour.asVec4(), model);
}

void draw2d::Context::drawLines(const vector<Rectf>& lines) const
{
	if (!line_buffer)
		line_buffer = std::make_unique<LineBuffer>();

	// Build line buffer
	auto col = colour.asVec4();
	for (const auto& line : lines)
	{
		if (line_arrow_length > 0.0f)
			line_buffer->addArrow(line, col, line_thickness, line_arrow_length, line_arrow_angle);
		else
			line_buffer->add2d(line.x1(), line.y1(), line.x2(), line.y2(), col, line_thickness);
	}
	line_buffer->push();
	line_buffer->setAaRadius(line_aa_radius, line_aa_radius);

	// Blending
	if (blend != Blend::Ignore)
		gl::setBlend(blend);

	line_buffer->draw(view, glm::vec4{ 1.0f }, model_matrix);
}

void draw2d::Context::drawPointSprites(const vector<Vec2f>& points) const
{
	// Create buffer if needed
	if (!ps_buffer)
		ps_buffer = std::make_unique<PointSpriteBuffer>();

	// Populate buffer
	// TODO: Use glm::vec2 as input or add direct conversion from Vec2f to glm::vec2
	for (const auto& point : points)
		ps_buffer->add({ point.x, point.y });
	ps_buffer->push();

	draw2d::drawPointSprites(*this);
}
void draw2d::Context::drawPointSprites(const vector<Vec2d>& points) const
{
	// Create buffer if needed
	if (!ps_buffer)
		ps_buffer = std::make_unique<PointSpriteBuffer>();

	// Populate buffer
	// TODO: Use glm::vec2 as input or add direct conversion from Vec2f to glm::vec2
	for (const auto& point : points)
		ps_buffer->add({ point.x, point.y });
	ps_buffer->push();

	draw2d::drawPointSprites(*this);
}

void draw2d::Context::drawText(const string& text, const Vec2f& pos) const
{
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
	text_offset.y += textLineHeight() * fdef->yoff;

	// Reduce softness value if the view is zoomed in
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
	const Shader* shader;
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

void draw2d::Context::drawTextureTiled(const Rectf& rect) const
{
	// Ignore empty texture
	if (!Texture::isLoaded(texture))
		return;

	// Calculate texture coordinates
	auto&  tex_info = Texture::info(texture);
	double tex_x    = rect.width() / (double)tex_info.size.x;
	double tex_y    = rect.height() / (double)tex_info.size.y;

	// Setup vertex buffer
	if (!vertex_buffer)
		vertex_buffer = std::make_unique<VertexBuffer2D>();
	vertex_buffer->add({ rect.tl.x, rect.tl.y }, col_white, { 0.0f, 0.0f });
	vertex_buffer->add({ rect.tl.x, rect.br.y }, col_white, { 0.0f, tex_y });
	vertex_buffer->add({ rect.br.x, rect.br.y }, col_white, { tex_x, tex_y });
	vertex_buffer->add({ rect.br.x, rect.tl.y }, col_white, { tex_x, 0.0f });
	vertex_buffer->push();

	// Bind the texture
	glEnable(GL_TEXTURE_2D);
	Texture::bind(texture);

	// Colour
	const auto& shader = defaultShader(true);
	shader.bind();
	shader.setUniform("colour", colour.asVec4());

	// Blending
	if (blend != Blend::Ignore)
		gl::setBlend(blend);

	// MVP matrix
	shader.setUniform("mvp", view ? view->mvpMatrix(model_matrix) : model_matrix);

	// Draw
	vertex_buffer->draw(Primitive::Quads);
}

void draw2d::Context::drawTextureWithin(const Rectf& rect, float pad, float max_scale) const
{
	// Ignore empty texture
	if (!Texture::isLoaded(texture))
		return;

	auto width  = rect.x2() - rect.x1();
	auto height = rect.y2() - rect.y1();

	// Get image dimensions
	auto& tex_info = Texture::info(texture);
	auto  x_dim    = static_cast<float>(tex_info.size.x);
	auto  y_dim    = static_cast<float>(tex_info.size.y);

	// Get max scale for x and y (including padding)
	auto x_scale = (width - pad) / x_dim;
	auto y_scale = (height - pad) / y_dim;

	// Set scale to smallest of the 2 (so that none of the texture will be clipped)
	auto scale = std::min<float>(x_scale, y_scale);

	// Clamp scale to maximum desired scale
	if (scale > max_scale)
		scale = max_scale;

	// Now draw the texture
	drawRect({ rect.x1() + width * 0.5f, rect.y1() + height * 0.5f, x_dim * scale, y_dim * scale, true });
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

		lb_hud->push();

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
// Returns the height of the text box based on the font, text and max width
// -----------------------------------------------------------------------------
float draw2d::TextBox::height()
{
	if (lines_.empty())
		split(text_);

	return height_;
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
	if (text_ == text)
		return;

	text_ = text;
	lines_.clear();
}

// -----------------------------------------------------------------------------
// Sets the text box width
// -----------------------------------------------------------------------------
void draw2d::TextBox::setWidth(float width)
{
	if (width_ == width)
		return;

	width_ = width;
	lines_.clear();
}

// -----------------------------------------------------------------------------
// Sets the text box font (+size)
// -----------------------------------------------------------------------------
void draw2d::TextBox::setFont(Font font, int size)
{
	if (font_ == font && font_size_ == size)
		return;

	font_      = font;
	font_size_ = size;
	lines_.clear();
}

// -----------------------------------------------------------------------------
// Draws the text box
// -----------------------------------------------------------------------------
// #define TEST_TB
void draw2d::TextBox::draw(const Vec2f& pos, Context& dc)
{
	if (lines_.empty())
		split(text_);

	auto prev_font = dc.font;
	auto prev_size = dc.text_size;
	auto cpos      = pos;

#ifdef TEST_TB
	auto prev_alpha   = dc.colour.a;
	auto prev_lt      = dc.line_thickness;
	dc.line_thickness = 1.0f;
#endif

	dc.font      = font_;
	dc.text_size = font_size_;

	for (const auto& line : lines_)
	{
		dc.drawText(line, cpos);
		cpos.y += dc.textLineHeight() * line_height_;

#ifdef TEST_TB
		dc.colour.a *= 0.5;
		if (&line != &lines_.back())
			dc.drawLines({ { pos.x, cpos.y, pos.x + width_, cpos.y } });
		dc.colour.a = prev_alpha;
#endif
	}

	dc.font      = prev_font;
	dc.text_size = prev_size;

#ifdef TEST_TB
	dc.colour.a *= 0.5;
	dc.drawRectOutline({ pos.x, pos.y, width_, height_, false });
	dc.colour.a       = prev_alpha;
	dc.line_thickness = prev_lt;
#endif
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
	if (!shader_2d)
	{
		shader_2d       = std::make_unique<Shader>("default2d");
		shader_2d_notex = std::make_unique<Shader>("default2d_notex");

		shader_2d->define("TEXTURED");
		shader_2d->loadResourceEntries("default2d.vert", "default2d.frag");
		shader_2d_notex->loadResourceEntries("default2d.vert", "default2d.frag");
	}

	return textured ? *shader_2d : *shader_2d_notex;
}

const Shader& draw2d::lineStippleShader(uint16_t pattern, float factor)
{
	if (!shader_line_stipple)
	{
		shader_line_stipple = std::make_unique<Shader>("default2d_line_stipple");
		shader_line_stipple->define("LINE_STIPPLE");
		shader_line_stipple->loadResourceEntries("default2d.vert", "default2d.frag");
	}

	shader_line_stipple->bind();
	shader_line_stipple->setUniform("stipple_pattern", pattern);
	shader_line_stipple->setUniform("stipple_factor", factor);

	return *shader_line_stipple;
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
