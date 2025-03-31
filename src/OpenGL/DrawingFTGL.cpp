
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "App.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Drawing.h"
#include "MapEditor/UI/MapCanvas.h"
#include "Utility/MathStuff.h"
#include <FTGL/ftgl.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::drawing
{
unique_ptr<FTFont> font_normal;
unique_ptr<FTFont> font_condensed;
unique_ptr<FTFont> font_bold;
unique_ptr<FTFont> font_boldcondensed;
unique_ptr<FTFont> font_mono;
unique_ptr<FTFont> font_small;
} // namespace slade::drawing


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
namespace slade::drawing
{
extern double  text_outline_width;
extern ColRGBA outline_colour;
} // namespace slade::drawing
EXTERN_CVAR(Int, gl_font_size)


// -----------------------------------------------------------------------------
//
// drawing Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::drawing
{
// -----------------------------------------------------------------------------
// Loads all needed fonts for rendering. Non-SFML implementation
// -----------------------------------------------------------------------------
int initFonts()
{
	// --- Load general fonts ---
	int ret = 0;

	font_normal.reset();
	font_condensed.reset();
	font_bold.reset();
	font_boldcondensed.reset();
	font_mono.reset();
	font_small.reset();

	// Normal
	auto entry = app::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans.ttf");
	if (entry)
	{
		font_normal = std::make_unique<FTTextureFont>(entry->rawData(), entry->size());
		font_normal->FaceSize(ui::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_normal->Error())
			font_normal.reset();
		else
			++ret;
	}

	// Condensed
	entry = app::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans_c.ttf");
	if (entry)
	{
		font_condensed = std::make_unique<FTTextureFont>(entry->rawData(), entry->size());
		font_condensed->FaceSize(ui::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_condensed->Error())
			font_condensed.reset();
		else
			++ret;
	}

	// Bold
	entry = app::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans_b.ttf");
	if (entry)
	{
		font_bold = std::make_unique<FTTextureFont>(entry->rawData(), entry->size());
		font_bold->FaceSize(ui::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_bold->Error())
			font_bold.reset();
		else
			++ret;
	}

	// Condensed bold
	entry = app::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans_cb.ttf");
	if (entry)
	{
		font_boldcondensed = std::make_unique<FTTextureFont>(entry->rawData(), entry->size());
		font_boldcondensed->FaceSize(ui::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_boldcondensed->Error())
			font_boldcondensed.reset();
		else
			++ret;
	}

	// Monospace
	entry = app::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_mono.ttf");
	if (entry)
	{
		font_mono = std::make_unique<FTTextureFont>(entry->rawData(), entry->size());
		font_mono->FaceSize(ui::scalePx(gl_font_size));

		// Check it loaded ok
		if (font_mono->Error())
			font_mono.reset();
		else
			++ret;
	}

	// Small
	entry = app::archiveManager().programResourceArchive()->entryAtPath("fonts/dejavu_sans.ttf");
	if (entry)
	{
		font_small = std::make_unique<FTTextureFont>(entry->rawData(), entry->size());
		font_small->FaceSize((ui::scalePx(gl_font_size) * 0.6) + 1);

		// Check it loaded ok
		if (font_small->Error())
			font_small.reset();
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
	font_normal.reset();
	font_condensed.reset();
	font_bold.reset();
	font_boldcondensed.reset();
	font_mono.reset();
	font_small.reset();
}

// -----------------------------------------------------------------------------
// Returns the requested [font]
// -----------------------------------------------------------------------------
FTFont* getFont(Font font)
{
	switch (font)
	{
	case Font::Normal: return font_normal.get();
	case Font::Condensed: return font_condensed.get();
	case Font::Bold: return font_bold.get();
	case Font::BoldCondensed: return font_boldcondensed.get();
	case Font::Monospace: return font_mono.get();
	case Font::Small: return font_small.get();
	default: return font_normal.get();
	}
}
} // namespace slade::drawing

// -----------------------------------------------------------------------------
// Draws [text] at [x,y]. If [bounds] is not null, the bounding coordinates of
// the rendered text string are written to it.
// -----------------------------------------------------------------------------
void drawing::drawText(const string& text, int x, int y, ColRGBA colour, Font font, Align alignment, Rectd* bounds)
{
	// Get desired font
	auto ftgl_font = getFont(font);

	// If FTGL font is invalid, do nothing
	if (!ftgl_font)
		return;

	// Setup alignment
	auto  bbox   = ftgl_font->BBox(text.c_str(), -1);
	int   xpos   = x;
	int   ypos   = y;
	float width  = bbox.Upper().X() - bbox.Lower().X();
	float height = ftgl_font->LineHeight();
	if (alignment != Align::Left)
	{
		if (alignment == Align::Center)
			xpos -= math::round(width * 0.5);
		else
			xpos -= width;
	}

	// Set bounds rect
	if (bounds)
	{
		bbox = ftgl_font->BBox(text.c_str(), -1, FTPoint(xpos, ypos));
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
		gl::setColour(outline_colour);
		glTranslatef(-2.0f, -1.0f, 0.0f);
		ftgl_font->Render(text.c_str(), -1);
		glTranslatef(0.0f, 2.0f, 0.0f);
		ftgl_font->Render(text.c_str(), -1);
		glTranslatef(4.0f, 0.0f, 0.0f);
		ftgl_font->Render(text.c_str(), -1);
		glTranslatef(0.0f, -2.0f, 0.0f);
		ftgl_font->Render(text.c_str(), -1);
		glTranslatef(-2.0f, 1.0f, 0.0f);
	}
	gl::setColour(colour);
	ftgl_font->Render(text.c_str(), -1);
	glPopMatrix();
}

// -----------------------------------------------------------------------------
// Returns the width and height of [text] when drawn with [font]
// -----------------------------------------------------------------------------
Vec2d drawing::textExtents(const string& text, Font font)
{
	// Get desired font
	auto ftgl_font = getFont(font);

	// If FTGL font is invalid, return empty
	if (!ftgl_font)
		return { 0, 0 };

	// Return width and height of text
	auto bbox = ftgl_font->BBox(text.c_str(), -1);
	return Vec2d(bbox.Upper().X() - bbox.Lower().X(), ftgl_font->LineHeight());
}

// -----------------------------------------------------------------------------
// Sets or restores (depending on [set]) the OpenGL state for SFML text
// rendering (does nothing for FTGL)
// -----------------------------------------------------------------------------
void drawing::setTextState(bool set) {}

// -----------------------------------------------------------------------------
// When enabled, the OpenGL state is set for text rendering each time drawText
// is called and restored after (SFML only, does nothing for FTGL)
// -----------------------------------------------------------------------------
void drawing::enableTextStateReset(bool enable) {}
