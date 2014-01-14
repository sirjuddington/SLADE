
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SFont.cpp
 * Description: SFont class, encapsulates a bitmap font, with
 *              functions to read various bitmap font formats and
 *              draw characters and strings with the font in opengl
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "SFont.h"
#include "SImage.h"
#include "ArchiveManager.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
SFont SFont::font_vga;
SFont SFont::font_slade;


/*******************************************************************
 * SFONTCHAR CLASS FUNCTIONS
 *******************************************************************/

/* SFontChar::SFontChar
 * SFontChar class constructor
 *******************************************************************/
SFontChar::SFontChar()
{
	// Init variables
	width = 0;
	height = 0;
}

/* SFontChar::~SFontChar
 * SFontChar class destructor
 *******************************************************************/
SFontChar::~SFontChar()
{
}


/*******************************************************************
 * SFONT CLASS FUNCTIONS
 *******************************************************************/

/* SFont::SFont
 * SFont class constructor
 *******************************************************************/
SFont::SFont()
{
	// Init character map
	for (unsigned a = 0; a < 256; a++)
		characters[a] = NULL;
}

/* SFont::~SFont
 * SFont class destructor
 *******************************************************************/
SFont::~SFont()
{
}

/* SFont::loadFont0
 * Loads a Doom alpha HUFONT font. Returns true on success, false
 * otherwise
 *******************************************************************/
bool SFont::loadFont0(MemChunk& mc)
{
	return false;
}

/* SFont::loadFont1
 * Loads a ZDoom FON1 font. Returns true on success, false otherwise
 *******************************************************************/
bool SFont::loadFont1(MemChunk& mc)
{
	return false;
}

/* SFont::loadFont2
 * Loads a ZDoom FON2 font. Returns true on success, false otherwise
 *******************************************************************/
bool SFont::loadFont2(MemChunk& mc)
{
	return false;
}

/* SFont::loadFontM
 * Loads a monochrome vga font. Returns true on success, false
 * otherwise
 *******************************************************************/
bool SFont::loadFontM(MemChunk& mc)
{
	// Check data
	if (mc.getSize() == 0 || mc.getSize() % 256)
		return false;

	// Init variables
	unsigned width = 256;
	unsigned height = 256;
	SImage image;

	// Determine character size
	unsigned charwidth = 8;
	unsigned charheight = mc.getSize() >> 8;

	// Setup font properties
	line_height = charheight + 1;
	spacing = 1;

	// Setup image
	image.resize(width, height);

	// Draw characters to image
	unsigned xoff = 0;
	unsigned yoff = 0;
	unsigned i = 0;
	bool test = true;
	for (unsigned a = 0; a < 256; a++)
	{
		// Setup character info
		SFontChar* c = new SFontChar();
		c->tex_bounds.set(xoff, yoff, xoff+charwidth, yoff+charheight);
		c->width = charwidth;
		c->height = charheight;
		characters[a] = c;

		// Write rows to image
		uint8_t val = 255;
		for (unsigned row = 0; row < charheight; row++)
		{
			for (unsigned p = 0; p < charwidth; ++p)
			{
				val = ((mc[i]>>(7-p)) & 1) * 255;
				image.setPixel(xoff + p, yoff + row, rgba_t(val, val, val, val));
			}
			i++;
		}

		// Go to next character
		xoff += charwidth;
		if (xoff >= 256)
		{
			xoff = 0;
			yoff += charheight;
		}

		test = !test;
	}

	// Load the generated image to the font texture
	texture.loadImage(&image);

	return true;
}

/* SFont::loadBMF
 * Loads a BMF font. Returns true on success, false otherwise
 *******************************************************************/
bool SFont::loadBMF(MemChunk& mc)
{
	return false;
}

/* SFont::drawCharacter
 * Draws the character [c] with the font, in [colour]
 *******************************************************************/
void SFont::drawCharacter(char c, rgba_t colour)
{
	// Bind texture
	if (!texture.bind())
		return;

	// Set colour
	colour.set_gl();

	// Get character to draw
	SFontChar* ch = characters[c];
	if (!ch)
		return;

	// Draw it
	frect_t tex_rect;
	tex_rect.tl.set((double)ch->tex_bounds.x1() / (double)texture.getWidth(), (double)ch->tex_bounds.y1() / (double)texture.getHeight());
	tex_rect.br.set((double)ch->tex_bounds.x2() / (double)texture.getWidth(), (double)ch->tex_bounds.y2() / (double)texture.getHeight());
	glBegin(GL_QUADS);
	glTexCoord2d(tex_rect.x1(), tex_rect.y1());		glVertex2d(0, 0);
	glTexCoord2d(tex_rect.x1(), tex_rect.y2());		glVertex2d(0, ch->height);
	glTexCoord2d(tex_rect.x2(), tex_rect.y2());		glVertex2d(ch->width, ch->height);
	glTexCoord2d(tex_rect.x2(), tex_rect.y1());		glVertex2d(ch->width, 0);
	glEnd();
}

/* SFont::drawString
 * Draws the string [srt] with the font, in [colour]. [align] can be
 * one of SF_ALIGN_LEFT, SF_ALIGN_RIGHT or SF_ALIGN_CENTER
 *******************************************************************/
void SFont::drawString(string str, rgba_t colour, uint8_t align)
{
	// Bind texture
	if (!texture.bind())
		return;

	// Set colour
	colour.set_gl();

	// Determine string length (for alignment)
	int total_width = 0;
	for (unsigned a = 0; a < str.size(); a++)
	{
		// Get character
		SFontChar* ch = characters[CHR(str)[a]];

		// Increment total width
		if (ch)
			total_width += ch->width + spacing;
		else
			total_width += spacing;
	}

	// Translate for alignment
	glPushMatrix();
	if (align == SF_ALIGN_RIGHT)
		glTranslated(-total_width, 0, 0);
	else if (align == SF_ALIGN_CENTER)
		glTranslated(-(total_width*0.5), 0, 0);

	// Draw the string
	unsigned xoff = 0;
	for (unsigned a = 0; a < str.size(); a++)
	{
		// Get character
		SFontChar* ch = characters[CHR(str)[a]];
		if (!ch)
		{
			xoff += spacing;
			continue;
		}

		// Draw it
		frect_t tex_rect;
		tex_rect.tl.set((double)ch->tex_bounds.x1() / (double)texture.getWidth(), (double)ch->tex_bounds.y1() / (double)texture.getHeight());
		tex_rect.br.set((double)ch->tex_bounds.x2() / (double)texture.getWidth(), (double)ch->tex_bounds.y2() / (double)texture.getHeight());
		glBegin(GL_QUADS);
		glTexCoord2d(tex_rect.x1(), tex_rect.y1());		glVertex2d(xoff, 0);
		glTexCoord2d(tex_rect.x1(), tex_rect.y2());		glVertex2d(xoff, ch->height);
		glTexCoord2d(tex_rect.x2(), tex_rect.y2());		glVertex2d(xoff+ch->width, ch->height);
		glTexCoord2d(tex_rect.x2(), tex_rect.y1());		glVertex2d(xoff+ch->width, 0);
		glEnd();

		// Increment x to next character (1 pixel spacing)
		xoff += ch->width + spacing;
	}

	glPopMatrix();
}


/*******************************************************************
 * SFONT CLASS STATIC FUNCTIONS
 *******************************************************************/

/* SFont::sladeFont
 * Returns the global SLADE font
 *******************************************************************/
SFont& SFont::sladeFont()
{
	if (!font_slade.texture.isLoaded())
	{
		// Load slade font
	}

	return font_slade;
}

/* SFont::vgaFont
 * Returns the global VGA font
 *******************************************************************/
SFont& SFont::vgaFont()
{
	if (!font_vga.texture.isLoaded())
	{
		// Get vga font entry
		ArchiveEntry* entry_vgafont = theArchiveManager->programResourceArchive()->entryAtPath("vga-rom-font.16");

		// Load font
		if (entry_vgafont)
			font_vga.loadFontM(entry_vgafont->getMCData());
	}

	return font_vga;
}
