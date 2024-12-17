
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Drawing.cpp
// Description: Various OpenGL drawing functions and related classes
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
#include "Drawing.h"
#include "App.h"
#include "GLTexture.h"
#include "General/UI.h"
#include "OpenGL.h"
#include "Utility/MathStuff.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, hud_statusbar, 1, CVar::Flag::Save)
CVAR(Bool, hud_center, 1, CVar::Flag::Save)
CVAR(Bool, hud_wide, 0, CVar::Flag::Save)
CVAR(Bool, hud_bob, 0, CVar::Flag::Save)
CVAR(Int, gl_font_size, 12, CVar::Flag::Save)

namespace slade::drawing
{
double  text_outline_width = 0;
ColRGBA outline_colour     = ColRGBA::BLACK;
}; // namespace slade::drawing


// -----------------------------------------------------------------------------
//
// Drawing Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the configured font size (scaled for DPI etc)
// -----------------------------------------------------------------------------
int drawing::fontSize()
{
	return ui::scalePx(gl_font_size);
}

// -----------------------------------------------------------------------------
// Draws a line from [start] to [end]
// -----------------------------------------------------------------------------
void drawing::drawLine(Vec2d start, Vec2d end)
{
	glBegin(GL_LINES);
	glVertex2d(start.x, start.y);
	glVertex2d(end.x, end.y);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a line from [x1,y1] to [x2,y2]
// -----------------------------------------------------------------------------
void drawing::drawLine(double x1, double y1, double x2, double y2)
{
	glBegin(GL_LINES);
	glVertex2d(x1, y1);
	glVertex2d(x2, y2);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a line from [start] to [end]
// -----------------------------------------------------------------------------
void drawing::drawLineTabbed(Vec2d start, Vec2d end, double tab, double tab_max)
{
	// Draw line
	glBegin(GL_LINES);
	glVertex2d(start.x, start.y);
	glVertex2d(end.x, end.y);
	glEnd();

	// Calculate midpoint
	Vec2d mid;
	mid.x = start.x + ((end.x - start.x) * 0.5);
	mid.y = start.y + ((end.y - start.y) * 0.5);

	// Calculate tab length
	double tablen = math::distance(start, end) * tab;
	if (tablen > tab_max)
		tablen = tab_max;
	if (tablen < 2)
		tablen = 2;

	// Calculate tab endpoint
	Vec2d invdir(-(end.y - start.y), end.x - start.x);
	invdir.normalize();

	// Draw tab
	glBegin(GL_LINES);
	glVertex2d(mid.x, mid.y);
	glVertex2d(mid.x - invdir.x * tablen, mid.y - invdir.y * tablen);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a line from [p1] to [p2] with an arrowhead at the [p1] end.
// If [twoway] is true, an arrowhead is also drawn at the [p2] end
// -----------------------------------------------------------------------------
void drawing::drawArrow(
	Vec2d          p1,
	Vec2d          p2,
	const ColRGBA& color,
	bool           twoway,
	double         arrowhead_angle,
	double         arrowhead_length)
{
	Vec2d  a1l, a1r, a2l, a2r;
	Vec2d  vector = p1 - p2;
	double angle  = atan2(-vector.y, vector.x);
	a1l = a1r = p1;
	a1l.x += arrowhead_length * sin(angle - arrowhead_angle);
	a1l.y += arrowhead_length * cos(angle - arrowhead_angle);
	a1r.x -= arrowhead_length * sin(angle + arrowhead_angle);
	a1r.y -= arrowhead_length * cos(angle + arrowhead_angle);
	if (twoway)
	{
		vector = p2 - p1;
		angle  = atan2(-vector.y, vector.x);
		a2l = a2r = p2;
		a2l.x += arrowhead_length * sin(angle - arrowhead_angle);
		a2l.y += arrowhead_length * cos(angle - arrowhead_angle);
		a2r.x -= arrowhead_length * sin(angle + arrowhead_angle);
		a2r.y -= arrowhead_length * cos(angle + arrowhead_angle);
	}
	gl::setColour(color);
	glBegin(GL_LINES);
	glVertex2d(p1.x, p1.y);
	glVertex2d(p2.x, p2.y);
	glVertex2d(p1.x, p1.y);
	glVertex2d(a1l.x, a1l.y);
	glVertex2d(p1.x, p1.y);
	glVertex2d(a1r.x, a1r.y);
	glEnd();
	if (twoway)
	{
		glBegin(GL_LINES);
		glVertex2d(p2.x, p2.y);
		glVertex2d(a2l.x, a2l.y);
		glEnd();
		glBegin(GL_LINES);
		glVertex2d(p2.x, p2.y);
		glVertex2d(a2r.x, a2r.y);
		glEnd();
	}
}

// -----------------------------------------------------------------------------
// Draws a rectangle from [tl] to [br]
// -----------------------------------------------------------------------------
void drawing::drawRect(Vec2d tl, Vec2d br)
{
	glBegin(GL_LINE_LOOP);
	glVertex2d(tl.x, tl.y);
	glVertex2d(tl.x, br.y);
	glVertex2d(br.x, br.y);
	glVertex2d(br.x, tl.y);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a rectangle from [x1,y1] to [x2,y2]
// -----------------------------------------------------------------------------
void drawing::drawRect(double x1, double y1, double x2, double y2)
{
	glBegin(GL_LINE_LOOP);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2);
	glVertex2d(x2, y2);
	glVertex2d(x2, y1);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a filled rectangle from [tl] to [br]
// -----------------------------------------------------------------------------
void drawing::drawFilledRect(Vec2d tl, Vec2d br)
{
	glBegin(GL_QUADS);
	glVertex2d(tl.x, tl.y);
	glVertex2d(tl.x, br.y);
	glVertex2d(br.x, br.y);
	glVertex2d(br.x, tl.y);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a filled rectangle from [x1,y1] to [x2,y2]
// -----------------------------------------------------------------------------
void drawing::drawFilledRect(double x1, double y1, double x2, double y2)
{
	glBegin(GL_QUADS);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2);
	glVertex2d(x2, y2);
	glVertex2d(x2, y1);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a filled rectangle with a border from [x1,y1] to [x2,y2]
// -----------------------------------------------------------------------------
void drawing::drawBorderedRect(Vec2d tl, Vec2d br, const ColRGBA& colour, const ColRGBA& border_colour)
{
	drawBorderedRect(tl.x, tl.y, br.x, br.y, colour, border_colour);
}

// -----------------------------------------------------------------------------
// Draws a filled rectangle with a border from [x1,y1] to [x2,y2]
// -----------------------------------------------------------------------------
void drawing::drawBorderedRect(
	double         x1,
	double         y1,
	double         x2,
	double         y2,
	const ColRGBA& colour,
	const ColRGBA& border_colour)
{
	// Rect
	gl::setColour(colour);
	glBegin(GL_QUADS);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2);
	glVertex2d(x2, y2);
	glVertex2d(x2, y1);
	glEnd();

	// Border
	gl::setColour(border_colour);
	glBegin(GL_LINE_LOOP);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2 - 1);
	glVertex2d(x2 - 1, y2 - 1);
	glVertex2d(x2 - 1, y1);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws an ellipse at [mid]
// -----------------------------------------------------------------------------
void drawing::drawEllipse(Vec2d mid, double radius_x, double radius_y, int sides, const ColRGBA& colour)
{
	// Set colour
	gl::setColour(colour);

	// Draw circle as line loop
	glBegin(GL_LINE_LOOP);
	double rot = 0;
	for (int a = 0; a < sides; a++)
	{
		glVertex2d(mid.x + sin(rot) * radius_x, mid.y - cos(rot) * radius_y);
		rot -= (3.1415926535897932384626433832795 * 2) / (double)sides;
	}
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a filled ellipse at [mid]
// -----------------------------------------------------------------------------
void drawing::drawFilledEllipse(Vec2d mid, double radius_x, double radius_y, int sides, const ColRGBA& colour)
{
	// Set colour
	gl::setColour(colour);

	// Draw circle as triangle fan
	glBegin(GL_TRIANGLE_FAN);
	glVertex2d(mid.x, mid.y);
	double rot = 0;
	for (int a = 0; a < sides + 1; a++)
	{
		glVertex2d(mid.x + sin(rot) * radius_x, mid.y - cos(rot) * radius_y);
		rot -= (3.1415926535897932384626433832795 * 2) / (double)sides;
	}
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a quad with the texture [id] at [x,y]
// -----------------------------------------------------------------------------
void drawing::drawTexture(unsigned id, double x, double y, bool flipx, bool flipy)
{
	// Ignore empty texture
	if (!gl::Texture::isLoaded(id))
		return;

	// Flipping?
	auto& tex_info = gl::Texture::info(id);
	if (flipx)
		x += tex_info.size.x;
	if (flipy)
		y += tex_info.size.y;

	// Bind the texture
	gl::Texture::bind(id);

	// Setup metrics
	double h = (double)tex_info.size.x;
	double v = (double)tex_info.size.y;
	if (flipx)
		h = -h;
	if (flipy)
		v = -v;

	// Translate to position
	glPushMatrix();
	glTranslated(x, y, 0);

	// Draw
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0);
	glVertex2d(0, 0);
	glTexCoord2d(0, 1);
	glVertex2d(0, v);
	glTexCoord2d(1, 1);
	glVertex2d(h, v);
	glTexCoord2d(1, 0);
	glVertex2d(h, 0);
	glEnd();

	glPopMatrix();
}

// -----------------------------------------------------------------------------
// Draws a quad of [width]x[height] with the texture [id] tiled proportionally
// -----------------------------------------------------------------------------
void drawing::drawTextureTiled(unsigned id, uint32_t width, uint32_t height)
{
	// Ignore empty texture
	if (!gl::Texture::isLoaded(id))
		return;

	// Bind the texture
	gl::Texture::bind(id);

	// Calculate texture coordinates
	auto&  tex_info = gl::Texture::info(id);
	double tex_x    = (double)width / (double)tex_info.size.x;
	double tex_y    = (double)height / (double)tex_info.size.y;

	// Draw
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0);
	glVertex2d(0, 0);
	glTexCoord2d(0, tex_y);
	glVertex2d(0, height);
	glTexCoord2d(tex_x, tex_y);
	glVertex2d(width, height);
	glTexCoord2d(tex_x, 0);
	glVertex2d(width, 0);
	glEnd();
}

// -----------------------------------------------------------------------------
// Fits [tex] within the rectangle from [x1,y1] to [x2,y2], centered and keeping
// the correct aspect ratio.
// If [upscale] is true the texture will be zoomed to fit the rectangle.
// Returns the resulting texture rectangle coordinates
// -----------------------------------------------------------------------------
Rectd drawing::fitTextureWithin(
	unsigned id,
	double   x1,
	double   y1,
	double   x2,
	double   y2,
	double   padding,
	double   max_scale)
{
	// Ignore empty texture
	if (!gl::Texture::isLoaded(id))
		return {};

	double width  = x2 - x1;
	double height = y2 - y1;

	// Get image dimensions
	auto&  tex_info = gl::Texture::info(id);
	double x_dim    = (double)tex_info.size.x;
	double y_dim    = (double)tex_info.size.y;

	// Get max scale for x and y (including padding)
	double x_scale = ((double)width - padding) / x_dim;
	double y_scale = ((double)width - padding) / y_dim;

	// Set scale to smallest of the 2 (so that none of the texture will be clipped)
	double scale = std::min<double>(x_scale, y_scale);

	// Clamp scale to maximum desired scale
	if (scale > max_scale)
		scale = max_scale;

	// Return the fitted rectangle
	return { x1 + width * 0.5 - (scale * x_dim * 0.5),
			 y1 + height * 0.5 - (scale * y_dim * 0.5),
			 x1 + width * 0.5 + (scale * x_dim * 0.5),
			 y1 + height * 0.5 + (scale * y_dim * 0.5) };
}

// -----------------------------------------------------------------------------
// Draws [tex] within the rectangle from [x1,y1] to [x2,y2], centered and
// keeping the correct aspect ratio.
// If [upscale] is true the texture will be zoomed to fit the rectangle
// -----------------------------------------------------------------------------
void drawing::drawTextureWithin(
	unsigned id,
	double   x1,
	double   y1,
	double   x2,
	double   y2,
	double   padding,
	double   max_scale)
{
	// Ignore empty texture
	if (!gl::Texture::isLoaded(id))
		return;

	double width  = x2 - x1;
	double height = y2 - y1;

	// Get image dimensions
	auto&  tex_info = gl::Texture::info(id);
	double x_dim    = (double)tex_info.size.x;
	double y_dim    = (double)tex_info.size.y;

	// Get max scale for x and y (including padding)
	double x_scale = ((double)width - padding) / x_dim;
	double y_scale = ((double)height - padding) / y_dim;

	// Set scale to smallest of the 2 (so that none of the texture will be clipped)
	double scale = std::min<double>(x_scale, y_scale);

	// Clamp scale to maximum desired scale
	if (scale > max_scale)
		scale = max_scale;

	// Now draw the texture
	gl::Texture::bind(id);
	glPushMatrix();
	glTranslated(x1 + width * 0.5, y1 + height * 0.5, 0); // Translate to middle of area
	glScaled(scale, scale, scale);                        // Scale to fit within area
	glTranslated(x_dim * -0.5, y_dim * -0.5, 0);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 0);
	glVertex2d(0, 0);
	glTexCoord2d(0, 1);
	glVertex2d(0, y_dim);
	glTexCoord2d(1, 1);
	glVertex2d(x_dim, y_dim);
	glTexCoord2d(1, 0);
	glVertex2d(x_dim, 0);
	glEnd();
	glPopMatrix();
}

// -----------------------------------------------------------------------------
// Sets the [thickness] and [colour] of the outline to use when drawing text
// -----------------------------------------------------------------------------
void drawing::setTextOutline(double thickness, const ColRGBA& colour)
{
	text_outline_width = thickness;
	outline_colour     = colour;
}

// -----------------------------------------------------------------------------
// Draws doom hud offset guide lines, from the center
// -----------------------------------------------------------------------------
void drawing::drawHud()
{
	// Determine some variables
	int hw = 160;
	int hh = 100;
	if (hud_wide)
		hw = 177;

	// Draw 320x200 screen outline
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glLineWidth(1.5f);
	drawRect(-hw, -hh, hw, hh);

	// Draw statusbar line if needed
	glLineWidth(1.0f);
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	if (hud_statusbar)
	{
		drawLine(-hw, 68, hw, 68); // Doom's status bar: 32 pixels tall
		drawLine(-hw, 62, hw, 62); // Hexen: 38 pixels
		drawLine(-hw, 58, hw, 58); // Heretic: 42 pixels
	}

	// Draw center lines if needed
	if (hud_center)
	{
		drawLine(-hw, 0, hw, 0);
		drawLine(0, -hh, 0, hh);
	}

	// Draw normal screen edge guides if widescreen
	if (hud_wide)
	{
		drawLine(-160, -100, -160, 100);
		drawLine(160, -100, 160, 100);
	}

	// Draw weapon bobbing guides
	if (hud_bob)
	{
		glLineWidth(0.8f);
		drawRect(-hw - 16, -hh - 16, hw + 16, hh + 16);
	}
}


// The following functions are taken from CodeLite (http://codelite.org)

wxColour drawing::systemPanelBGColour()
{
	if (app::platform() == app::Windows && app::isDarkTheme())
		return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	else
		return wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
}

wxColour drawing::systemMenuTextColour()
{
	return wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT);
}

wxColour drawing::systemMenuBarBGColour()
{
	return wxSystemSettings::GetColour(wxSYS_COLOUR_MENU);
}

wxColour drawing::lightColour(const wxColour& colour, float percent)
{
	if (percent == 0)
	{
		return colour;
	}

	// Convert to HSL
	ColHSL hsl = ColRGBA(colour).asHSL();

	// Increase luminance
	hsl.l += (float)((percent * 5.0) / 100.0);
	if (hsl.l > 1.0)
		hsl.l = 1.0;

	ColRGBA rgb = hsl.asRGB();
	return wxColour(rgb.r, rgb.g, rgb.b);
}

wxColour drawing::darkColour(const wxColour& colour, float percent)
{
	if (percent == 0)
	{
		return colour;
	}

	// Convert to HSL
	ColHSL hsl = ColRGBA(colour).asHSL();

	// Decrease luminance
	hsl.l -= (float)((percent * 5.0) / 100.0);
	if (hsl.l < 0)
		hsl.l = 0;

	ColRGBA rgb = hsl.asRGB();
	return wxColour(rgb.r, rgb.g, rgb.b);
}


// -----------------------------------------------------------------------------
//
// TextBox Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextBox class constructor
// -----------------------------------------------------------------------------
TextBox::TextBox(string_view text, drawing::Font font, int width, int line_height) :
	font_{ font },
	width_{ width },
	line_height_{ line_height }
{
	setText(text);
}

// -----------------------------------------------------------------------------
// Splits [text] into separate lines (split by newlines), also performs further
// splitting to word wrap the text within the box
// -----------------------------------------------------------------------------
void TextBox::split(string_view text)
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
		double width = drawing::textExtents(lines_[line], font_).x;

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
			width = drawing::textExtents(lines_[line].substr(0, c), font_).x;
		}

		// Increment length until it doesn't fit
		while (width < width_)
		{
			c++;
			width = drawing::textExtents(lines_[line].substr(0, c), font_).x;
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
	if (line_height_ < 0)
		height_ = drawing::textExtents(lines_[0], font_).y * lines_.size();
	else
		height_ = line_height_ * lines_.size();
}

// -----------------------------------------------------------------------------
// Sets the text box text
// -----------------------------------------------------------------------------
void TextBox::setText(string_view text)
{
	text_ = text;
	split(text);
}

// -----------------------------------------------------------------------------
// Sets the text box width
// -----------------------------------------------------------------------------
void TextBox::setSize(int width)
{
	width_ = width;
	split(text_);
}

// -----------------------------------------------------------------------------
// Draws the text box
// -----------------------------------------------------------------------------
void TextBox::draw(int x, int y, const ColRGBA& colour, drawing::Align alignment)
{
	Rectd b;
	drawing::enableTextStateReset(false);
	drawing::setTextState(true);
	for (const auto& line : lines_)
	{
		drawText(line, x, y, colour, font_, alignment, &b);

		if (line_height_ < 0)
			y += b.height();
		else
			y += line_height_;
	}
	drawing::enableTextStateReset(true);
	drawing::setTextState(false);
}
