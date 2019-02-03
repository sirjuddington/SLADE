
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DrawingSFML.cpp
// Description: SFML implementation of OpenGL text drawing functions
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
#ifdef USE_SFML_RENDERWINDOW
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Drawing.h"
#include "MapEditor/UI/MapCanvas.h"
#include "Utility/MathStuff.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace Drawing
{
typedef std::unique_ptr<sf::Font> FontPtr;

FontPtr font_normal;
FontPtr font_condensed;
FontPtr font_bold;
FontPtr font_boldcondensed;
FontPtr font_mono;

sf::RenderWindow* render_target    = nullptr;
bool              text_state_reset = true;
} // namespace Drawing


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
namespace Drawing
{
extern double  text_outline_width;
extern ColRGBA outline_colour;
} // namespace Drawing
EXTERN_CVAR(Int, gl_font_size)


// -----------------------------------------------------------------------------
//
// Drawing Namespace Functions
//
// -----------------------------------------------------------------------------
namespace Drawing
{
// -----------------------------------------------------------------------------
// Returns an SFML font created from the resource archive entry [res_path]
// -----------------------------------------------------------------------------
FontPtr createFont(const string& res_path, int& counter)
{
	auto entry = App::archiveManager().programResourceArchive()->entryAtPath(res_path);
	if (!entry)
		return nullptr;

	auto font = std::make_unique<sf::Font>();
	font->loadFromMemory(entry->rawData(), entry->size());

	++counter;

	return font;
}

// -----------------------------------------------------------------------------
// Returns the requested [font]
// -----------------------------------------------------------------------------
sf::Font* getFont(Font font)
{
	switch (font)
	{
	case Font::Normal: return font_normal.get();
	case Font::Condensed: return font_condensed.get();
	case Font::Bold: return font_bold.get();
	case Font::BoldCondensed: return font_boldcondensed.get();
	case Font::Monospace: return font_mono.get();
	case Font::Small: return font_normal.get();
	default: return font_normal.get();
	};
}
} // namespace Drawing

// -----------------------------------------------------------------------------
// Loads all needed fonts for rendering. SFML 2.x implementation
// -----------------------------------------------------------------------------
int Drawing::initFonts()
{
	// --- Load general fonts ---
	int ret = 0;

	font_normal        = createFont("fonts/dejavu_sans.ttf", ret);
	font_condensed     = createFont("fonts/dejavu_sans_c.ttf", ret);
	font_bold          = createFont("fonts/dejavu_sans_b.ttf", ret);
	font_boldcondensed = createFont("fonts/dejavu_sans_cb.ttf", ret);
	font_mono          = createFont("fonts/dejavu_mono.ttf", ret);

	return ret;
}

// -----------------------------------------------------------------------------
// Cleans up all created fonts
// -----------------------------------------------------------------------------
void Drawing::cleanupFonts()
{
	font_normal.reset(nullptr);
	font_condensed.reset(nullptr);
	font_bold.reset(nullptr);
	font_boldcondensed.reset(nullptr);
	font_mono.reset(nullptr);
}

// -----------------------------------------------------------------------------
// Draws [text] at [x,y]. If [bounds] is not null, the bounding coordinates of
// the rendered text string are written to it.
// -----------------------------------------------------------------------------
void Drawing::drawText(const string& text, int x, int y, ColRGBA colour, Font font, Align alignment, Rectd* bounds)
{
	// Setup SFML string
	sf::Text sf_str;
	sf_str.setString(UTF8(text));
	sf_str.setFillColor(sf::Color(colour.r, colour.g, colour.b, colour.a));

	// Set font
	auto f = getFont(font);
	sf_str.setFont(*f);
	if (font == Font::Small)
		sf_str.setCharacterSize((UI::scalePx(gl_font_size) * 0.6) + 1);
	else
		sf_str.setCharacterSize(UI::scalePx(gl_font_size));

	// Setup alignment
	if (alignment != Align::Left)
	{
		float width = sf_str.getLocalBounds().width;

		if (alignment == Align::Center)
			x -= MathStuff::round(width * 0.5);
		else
			x -= width;
	}
	sf_str.setPosition(x, y);

	// Set bounds rect
	if (bounds)
	{
		auto rect = sf_str.getGlobalBounds();
		bounds->set(rect.left, rect.top, rect.left + rect.width, rect.top + rect.height);
	}

	// Draw the string
	if (render_target)
	{
		if (text_state_reset)
			setTextState(true);

		if (text_outline_width > 0)
		{
			sf_str.setOutlineThickness(text_outline_width);
			sf_str.setOutlineColor(
				sf::Color(outline_colour.r, outline_colour.g, outline_colour.b, outline_colour.a));
		}

		// Draw
		render_target->draw(sf_str);

		if (text_state_reset)
			setTextState(false);
	}
}

// -----------------------------------------------------------------------------
// Returns the width and height of [text] when drawn with [font]
// -----------------------------------------------------------------------------
Vec2d Drawing::textExtents(const string& text, Font font)
{
	// Setup SFML string
	sf::Text sf_str;
	sf_str.setString(CHR(text));

	// Set font
	auto f = getFont(font);
	sf_str.setFont(*f);
	if (font == Font::Small)
		sf_str.setCharacterSize((UI::scalePx(gl_font_size) * 0.6) + 1);
	else
		sf_str.setCharacterSize(UI::scalePx(gl_font_size));

	// Return width and height of text
	auto rect = sf_str.getGlobalBounds();
	return { rect.width, rect.height };
}

// -----------------------------------------------------------------------------
// Sets or restores (depending on [set]) the OpenGL state for SFML text
// rendering
// -----------------------------------------------------------------------------
void Drawing::setTextState(bool set)
{
	if (set)
	{
		// Push related states
		glPushMatrix();
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glPushAttrib(GL_VIEWPORT_BIT);
		render_target->resetGLStates();
	}
	else
	{
		// Pop related states
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
}

// -----------------------------------------------------------------------------
// When enabled, the OpenGL state is set for text rendering each time drawText
// is called and restored after (SFML only)
// -----------------------------------------------------------------------------
void Drawing::enableTextStateReset(bool enable)
{
	text_state_reset = enable;
}

// -----------------------------------------------------------------------------
// Sets the SFML render target to [target]
// -----------------------------------------------------------------------------
void Drawing::setRenderTarget(sf::RenderWindow* target)
{
	render_target = target;
}

#endif
