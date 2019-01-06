
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
#include "Archive/ArchiveManager.h"
#include "GLTexture.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "OpenGL.h"
#include "Utility/MathStuff.h"

#ifdef __WXGTK3__
#include <gtk-3.0/gtk/gtk.h>
#elif __WXGTK20__
#define GSocket GlibGSocket
#include <gtk-2.0/gtk/gtk.h>
#undef GSocket
#endif


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

namespace Drawing
{
double  text_outline_width  = 0;
ColRGBA text_outline_colour = COL_BLACK;
}; // namespace Drawing


// -----------------------------------------------------------------------------
//
// Drawing Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the configured font size (scaled for DPI etc)
// -----------------------------------------------------------------------------
int Drawing::fontSize()
{
	return UI::scalePx(gl_font_size);
}

// -----------------------------------------------------------------------------
// Draws a line from [start] to [end]
// -----------------------------------------------------------------------------
void Drawing::drawLine(Vec2f start, Vec2f end)
{
	glBegin(GL_LINES);
	glVertex2d(start.x, start.y);
	glVertex2d(end.x, end.y);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a line from [x1,y1] to [x2,y2]
// -----------------------------------------------------------------------------
void Drawing::drawLine(double x1, double y1, double x2, double y2)
{
	glBegin(GL_LINES);
	glVertex2d(x1, y1);
	glVertex2d(x2, y2);
	glEnd();
}

// -----------------------------------------------------------------------------
// Draws a line from [start] to [end]
// -----------------------------------------------------------------------------
void Drawing::drawLineTabbed(Vec2f start, Vec2f end, double tab, double tab_max)
{
	// Draw line
	glBegin(GL_LINES);
	glVertex2d(start.x, start.y);
	glVertex2d(end.x, end.y);
	glEnd();

	// Calculate midpoint
	Vec2f mid;
	mid.x = start.x + ((end.x - start.x) * 0.5);
	mid.y = start.y + ((end.y - start.y) * 0.5);

	// Calculate tab length
	double tablen = MathStuff::distance(start, end) * tab;
	if (tablen > tab_max)
		tablen = tab_max;
	if (tablen < 2)
		tablen = 2;

	// Calculate tab endpoint
	Vec2f invdir(-(end.y - start.y), end.x - start.x);
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
void Drawing::drawArrow(Vec2f p1, Vec2f p2, ColRGBA color, bool twoway, double arrowhead_angle, double arrowhead_length)
{
	Vec2f  a1l, a1r, a2l, a2r;
	Vec2f  vector = p1 - p2;
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
	OpenGL::setColour(color);
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
void Drawing::drawRect(Vec2f tl, Vec2f br)
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
void Drawing::drawRect(double x1, double y1, double x2, double y2)
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
void Drawing::drawFilledRect(Vec2f tl, Vec2f br)
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
void Drawing::drawFilledRect(double x1, double y1, double x2, double y2)
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
void Drawing::drawBorderedRect(Vec2f tl, Vec2f br, ColRGBA colour, ColRGBA border_colour)
{
	drawBorderedRect(tl.x, tl.y, br.x, br.y, colour, border_colour);
}

// -----------------------------------------------------------------------------
// Draws a filled rectangle with a border from [x1,y1] to [x2,y2]
// -----------------------------------------------------------------------------
void Drawing::drawBorderedRect(double x1, double y1, double x2, double y2, ColRGBA colour, ColRGBA border_colour)
{
	// Rect
	OpenGL::setColour(colour, false);
	glBegin(GL_QUADS);
	glVertex2d(x1, y1);
	glVertex2d(x1, y2);
	glVertex2d(x2, y2);
	glVertex2d(x2, y1);
	glEnd();

	// Border
	OpenGL::setColour(border_colour, false);
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
void Drawing::drawEllipse(Vec2f mid, double radius_x, double radius_y, int sides, ColRGBA colour)
{
	// Set colour
	OpenGL::setColour(colour, false);

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
void Drawing::drawFilledEllipse(Vec2f mid, double radius_x, double radius_y, int sides, ColRGBA colour)
{
	// Set colour
	OpenGL::setColour(colour, false);

	// Draw circle as line loop
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

void Drawing::drawTexture(unsigned id, double x, double y, bool flipx, bool flipy)
{
	// Ignore empty texture
	if (!OpenGL::Texture::isLoaded(id))
		return;

	// Flipping?
	auto& tex_info = OpenGL::Texture::info(id);
	if (flipx)
		x += tex_info.size.x;
	if (flipy)
		y += tex_info.size.y;

	// Bind the texture
	OpenGL::Texture::bind(id);

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

void Drawing::drawTextureTiled(unsigned id, uint32_t width, uint32_t height)
{
	// Ignore empty texture
	if (!OpenGL::Texture::isLoaded(id))
		return;

	// Bind the texture
	OpenGL::Texture::bind(id);

	// Calculate texture coordinates
	auto&  tex_info = OpenGL::Texture::info(id);
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
Rectf Drawing::fitTextureWithin(
	unsigned id,
	double   x1,
	double   y1,
	double   x2,
	double   y2,
	double   padding,
	double   max_scale)
{
	// Ignore empty texture
	if (!OpenGL::Texture::isLoaded(id))
		return {};

	double width  = x2 - x1;
	double height = y2 - y1;

	// Get image dimensions
	auto&  tex_info = OpenGL::Texture::info(id);
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
void Drawing::drawTextureWithin(
	unsigned id,
	double   x1,
	double   y1,
	double   x2,
	double   y2,
	double   padding,
	double   max_scale)
{
	// Ignore empty texture
	if (!OpenGL::Texture::isLoaded(id))
		return;

	double width  = x2 - x1;
	double height = y2 - y1;

	// Get image dimensions
	auto&  tex_info = OpenGL::Texture::info(id);
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
	OpenGL::Texture::bind(id);
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
void Drawing::setTextOutline(double thickness, ColRGBA colour)
{
	text_outline_width  = thickness;
	text_outline_colour = colour;
}

// -----------------------------------------------------------------------------
// Draws doom hud offset guide lines, from the center
// -----------------------------------------------------------------------------
void Drawing::drawHud()
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

wxColour Drawing::systemPanelBGColour()
{
#ifdef __WXGTK__
	static bool     intitialized(false);
	static wxColour bgColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));

	if (!intitialized)
	{
		// try to get the background colour from a menu
		GtkWidget* menu = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		GtkStyle*  def  = gtk_rc_get_style(menu);
		if (!def)
			def = gtk_widget_get_default_style();

		if (def)
		{
			GdkColor col = def->bg[GTK_STATE_NORMAL];
			bgColour     = wxColour(col);
		}
		gtk_widget_destroy(menu);
		intitialized = true;
	}
	return bgColour;
#else
	return wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
#endif
}

wxColour Drawing::systemMenuTextColour()
{
	return wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT);
}

wxColour Drawing::systemMenuBarBGColour()
{
	return wxSystemSettings::GetColour(wxSYS_COLOUR_MENU);
}

wxColour Drawing::lightColour(const wxColour& colour, float percent)
{
	if (percent == 0)
	{
		return colour;
	}

	// Convert to HSL
	ColHSL hsl = Misc::rgbToHsl(ColRGBA(COLWX(colour)));

	// Increase luminance
	hsl.l += (float)((percent * 5.0) / 100.0);
	if (hsl.l > 1.0)
		hsl.l = 1.0;

	ColRGBA rgb = Misc::hslToRgb(hsl);
	return wxColour(rgb.r, rgb.g, rgb.b);
}

wxColour Drawing::darkColour(const wxColour& colour, float percent)
{
	if (percent == 0)
	{
		return colour;
	}

	// Convert to HSL
	ColHSL hsl = Misc::rgbToHsl(ColRGBA(COLWX(colour)));

	// Decrease luminance
	hsl.l -= (float)((percent * 5.0) / 100.0);
	if (hsl.l < 0)
		hsl.l = 0;

	ColRGBA rgb = Misc::hslToRgb(hsl);
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
TextBox::TextBox(const string& text, Drawing::Font font, int width, int line_height) :
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
void TextBox::split(const string& text)
{
	// Clear current text lines
	lines_.clear();

	// Do nothing for empty string
	if (text.IsEmpty())
		return;

	// Split at newlines
	auto split = wxSplit(text, '\n');
	for (unsigned a = 0; a < split.Count(); a++)
		lines_.push_back(split[a]);

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
		double width = Drawing::textExtents(lines_[line], font_).x;

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
			width = Drawing::textExtents(lines_[line].Mid(0, c), font_).x;
		}

		// Increment length until it doesn't fit
		while (width < width_)
		{
			c++;
			width = Drawing::textExtents(lines_[line].Mid(0, c), font_).x;
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
		string nl    = lines_[line].Mid(sc);
		lines_[line] = lines_[line].Mid(0, sc);
		lines_.insert(lines_.begin() + line + 1, nl);
		line++;
	}

	// Update height
	if (line_height_ < 0)
		height_ = Drawing::textExtents(lines_[0], font_).y * lines_.size();
	else
		height_ = line_height_ * lines_.size();
}

// -----------------------------------------------------------------------------
// Sets the text box text
// -----------------------------------------------------------------------------
void TextBox::setText(const string& text)
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
void TextBox::draw(int x, int y, ColRGBA colour, Drawing::Align alignment)
{
	Rectf b;
	Drawing::enableTextStateReset(false);
	Drawing::setTextState(true);
	for (const auto& line : lines_)
	{
		drawText(line, x, y, colour, font_, alignment, &b);

		if (line_height_ < 0)
			y += b.height();
		else
			y += line_height_;
	}
	Drawing::enableTextStateReset(true);
	Drawing::setTextState(false);
}
