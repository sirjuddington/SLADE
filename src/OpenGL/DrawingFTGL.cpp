
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DrawingFTGL.cpp
// Description: FTGL implementation of OpenGL text drawing functions
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
#ifndef USE_SFML_RENDERWINDOW
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Drawing.h"
#include "MapEditor/UI/MapCanvas.h"
#include "Utility/MathStuff.h"
#include <FTGL/ftgl.h>

// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace Drawing
{
FTFont* font_normal        = nullptr;
FTFont* font_condensed     = nullptr;
FTFont* font_bold          = nullptr;
FTFont* font_boldcondensed = nullptr;
FTFont* font_mono          = nullptr;
FTFont* font_small         = nullptr;
} // namespace Drawing


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
namespace Drawing
{
extern double  text_outline_width;
extern ColRGBA text_outline_colour;
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
// Loads all needed fonts for rendering. Non-SFML implementation
// -----------------------------------------------------------------------------
int initFonts()
{
	// --- Load general fonts ---
	int ret = 0;

	if (font_normal)
	{
		delete font_normal;
		font_normal = nullptr;
	}
	if (font_condensed)
	{
		delete font_condensed;
		font_condensed = nullptr;
	}
	if (font_bold)
	{
		delete font_bold;
		font_bold = nullptr;
	}
	if (font_boldcondensed)
	{
		delete font_boldcondensed;
		font_boldcondensed = nullptr;
	}
	if (font_mono)
	{
		delete font_mono;
		font_mono = nullptr;
	}
	if (font_small)
	{
		delete font_small;
		font_small = nullptr;
	}

	// Normal
	auto entry = App::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans.ttf");
	if (entry)
	{
		font_normal = new FTTextureFont(entry->rawData(), entry->size());
		font_normal->FaceSize(UI::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_normal->Error())
		{
			delete font_normal;
			font_normal = nullptr;
		}
		else
			++ret;
	}

	// Condensed
	entry = App::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans_c.ttf");
	if (entry)
	{
		font_condensed = new FTTextureFont(entry->rawData(), entry->size());
		font_condensed->FaceSize(UI::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_condensed->Error())
		{
			delete font_condensed;
			font_condensed = nullptr;
		}
		else
			++ret;
	}

	// Bold
	entry = App::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans_b.ttf");
	if (entry)
	{
		font_bold = new FTTextureFont(entry->rawData(), entry->size());
		font_bold->FaceSize(UI::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_bold->Error())
		{
			delete font_bold;
			font_bold = nullptr;
		}
		else
			++ret;
	}

	// Condensed bold
	entry = App::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans_cb.ttf");
	if (entry)
	{
		font_boldcondensed = new FTTextureFont(entry->rawData(), entry->size());
		font_boldcondensed->FaceSize(UI::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_boldcondensed->Error())
		{
			delete font_boldcondensed;
			font_boldcondensed = nullptr;
		}
		else
			++ret;
	}

	// Monospace
	entry = App::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_mono.ttf");
	if (entry)
	{
		font_mono = new FTTextureFont(entry->rawData(), entry->size());
		font_mono->FaceSize(UI::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_mono->Error())
		{
			delete font_mono;
			font_mono = nullptr;
		}
		else
			++ret;
	}

	// Small
	entry = App::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans.ttf");
	if (entry)
	{
		font_small = new FTTextureFont(entry->rawData(), entry->size());
		font_small->FaceSize((UI::scalePx(gl_font_size) * 0.6) + 1);

		// Check it loaded ok
		if (font_small->Error())
		{
			delete font_small;
			font_small = nullptr;
		}
		else
			++ret;
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Cleans up all created fonts
// -----------------------------------------------------------------------------
void cleanupFonts()
{
	delete font_normal;
	delete font_condensed;
	delete font_bold;
	delete font_boldcondensed;
	delete font_mono;
	delete font_small;
}

// -----------------------------------------------------------------------------
// Returns the requested [font]
// -----------------------------------------------------------------------------
FTFont* getFont(Font font)
{
	switch (font)
	{
	case Font::Normal: return font_normal;
	case Font::Condensed: return font_condensed;
	case Font::Bold: return font_bold;
	case Font::BoldCondensed: return font_boldcondensed;
	case Font::Monospace: return font_mono;
	case Font::Small: return font_small;
	default: return font_normal;
	};
}
} // namespace Drawing

// -----------------------------------------------------------------------------
// Draws [text] at [x,y]. If [bounds] is not null, the bounding coordinates of
// the rendered text string are written to it.
// -----------------------------------------------------------------------------
void Drawing::drawText(const string& text, int x, int y, ColRGBA colour, Font font, Align alignment, Rectf* bounds)
{
	// Get desired font
	auto ftgl_font = getFont(font);

	// If FTGL font is invalid, do nothing
	if (!ftgl_font)
		return;

	// Setup alignment
	auto  bbox   = ftgl_font->BBox(CHR(text), -1);
	int   xpos   = x;
	int   ypos   = y;
	float width  = bbox.Upper().X() - bbox.Lower().X();
	float height = ftgl_font->LineHeight();
	if (alignment != Align::Left)
	{
		if (alignment == Align::Center)
			xpos -= MathStuff::round(width * 0.5);
		else
			xpos -= width;
	}

	// Set bounds rect
	if (bounds)
	{
		bbox = ftgl_font->BBox(CHR(text), -1, FTPoint(xpos, ypos));
		bounds->set(bbox.Lower().X(), bbox.Lower().Y(), bbox.Upper().X(), bbox.Lower().Y() + height);
	}

	// Draw the string
	glPushMatrix();
	glTranslatef(xpos, ypos + ftgl_font->FaceSize(), 0.0f);
	glTranslatef(-0.375f, -0.375f, 0);
	glScalef(1.0f, -1.0f, 1.0f);
	if (text_outline_width > 0)
	{
		// Draw outline if set
		OpenGL::setColour(text_outline_colour);
		glTranslatef(-2.0f, -1.0f, 0.0f);
		ftgl_font->Render(CHR(text), -1);
		glTranslatef(0.0f, 2.0f, 0.0f);
		ftgl_font->Render(CHR(text), -1);
		glTranslatef(4.0f, 0.0f, 0.0f);
		ftgl_font->Render(CHR(text), -1);
		glTranslatef(0.0f, -2.0f, 0.0f);
		ftgl_font->Render(CHR(text), -1);
		glTranslatef(-2.0f, 1.0f, 0.0f);
	}
	OpenGL::setColour(colour);
	ftgl_font->Render(CHR(text), -1);
	glPopMatrix();
}

// -----------------------------------------------------------------------------
// Returns the width and height of [text] when drawn with [font]
// -----------------------------------------------------------------------------
Vec2f Drawing::textExtents(const string& text, Font font)
{
	// Get desired font
	auto ftgl_font = getFont(font);

	// If FTGL font is invalid, return empty
	if (!ftgl_font)
		return { 0, 0 };

	// Return width and height of text
	auto bbox = ftgl_font->BBox(CHR(text), -1);
	return Vec2f(bbox.Upper().X() - bbox.Lower().X(), ftgl_font->LineHeight());
}

// -----------------------------------------------------------------------------
// Sets or restores (depending on [set]) the OpenGL state for SFML text
// rendering (does nothing for FTGL)
// -----------------------------------------------------------------------------
void Drawing::setTextState(bool set) {}

// -----------------------------------------------------------------------------
// When enabled, the OpenGL state is set for text rendering each time drawText
// is called and restored after (SFML only, does nothing for FTGL)
// -----------------------------------------------------------------------------
void Drawing::enableTextStateReset(bool enable) {}

#endif
