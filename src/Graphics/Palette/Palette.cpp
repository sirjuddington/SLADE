
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    Palette.cpp
// Description: Palette class, handles a 256-colour palette and performs
//              various colour transformations etc
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "Palette.h"
#include "General/Misc.h"
#include "Graphics/Translation.h"
#include "Graphics/SImage/SIFormat.h"
#include "Utility/Tokenizer.h"
#include "Utility/CIEDeltaEquations.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
CVAR(Int,	col_match, (int)Palette::ColourMatch::Old, CVAR_SAVE)
CVAR(Float, col_match_r, 1.0, CVAR_SAVE)
CVAR(Float, col_match_g, 1.0, CVAR_SAVE)
CVAR(Float, col_match_b, 1.0, CVAR_SAVE)
CVAR(Float, col_match_h, 1.0, CVAR_SAVE)
CVAR(Float, col_match_s, 1.0, CVAR_SAVE)
CVAR(Float, col_match_l, 1.0, CVAR_SAVE)
EXTERN_CVAR(Float, col_greyscale_r);
EXTERN_CVAR(Float, col_greyscale_g);
EXTERN_CVAR(Float, col_greyscale_b);


// ----------------------------------------------------------------------------
//
// Palette Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Palette::Palette
//
// Palette class constructor
// ----------------------------------------------------------------------------
Palette::Palette(unsigned size) :
	colours_{ size },
	colours_hsl_{ size },
	colours_lab_{ size },
	index_trans_{ -1 }
{
	// Init palette (to greyscale)
	for (unsigned a = 0; a < size; a++)
	{
		double mult = (double)a / (double)size;
		colours_[a].set(mult * 255, mult * 255, mult * 255, 255, -1, a);
		colours_lab_[a].l = mult;
		colours_hsl_[a].l = mult;
	}
}

// ----------------------------------------------------------------------------
// Palette::~Palette
//
// Palette class destructor
// ----------------------------------------------------------------------------
Palette::~Palette()
{
}

// ----------------------------------------------------------------------------
// Palette::loadMem
//
// Reads colour information from raw data (MemChunk)
// ----------------------------------------------------------------------------
bool Palette::loadMem(MemChunk& mc)
{
	// Check that the given data has at least 1 colour (3 bytes)
	if (mc.getSize() < 3)
		return false;

	// Read in colours
	mc.seek(0, SEEK_SET);
	int c = 0;
	for (size_t a = 0; a < mc.getSize(); a += 3)
	{
		uint8_t rgb[3] = {0, 0, 0};

		// Read RGB value
		if (mc.read(rgb, 3))
		{
			// Set colour in palette
			colours_[c].set(rgb[0], rgb[1], rgb[2], 255, -1, c);
			colours_lab_[c] = Misc::rgbToLab((double)rgb[0]/255.0, (double)rgb[1]/255.0, (double)rgb[2]/255.0);
			colours_hsl_[c++] = Misc::rgbToHsl((double)rgb[0]/255.0, (double)rgb[1]/255.0, (double)rgb[2]/255.0);
		}

		// If we have read 256 colours, finish
		if (c == 256)
			break;
	}
	mc.seek(0, SEEK_SET);

	return true;
}

// ----------------------------------------------------------------------------
// Palette::loadMem
//
// Reads colour information from raw data
// ----------------------------------------------------------------------------
bool Palette::loadMem(const uint8_t* data, uint32_t size)
{
	// Check that the given data has at least 1 colour (3 bytes)
	if (size < 3)
		return false;

	// Read in colours
	int c = 0;
	for (size_t a = 0; a < size; a += 3)
	{
		// Set colour in palette
		colours_[c].set(data[a], data[a+1], data[a+2], 255, -1, c);
		colours_lab_[c] = Misc::rgbToLab((double)data[a]/255.0, (double)data[a+1]/255.0, (double)data[a+2]/255.0);
		colours_hsl_[c++] = Misc::rgbToHsl((double)data[a]/255.0, (double)data[a+1]/255.0, (double)data[a+2]/255.0);

		// If we have read 256 colours, finish
		if (c == 256)
			break;
	}

	return true;
}

// ----------------------------------------------------------------------------
// Palette::loadMem
//
// Reads colour information from a palette format (MemChunk)
// ----------------------------------------------------------------------------
bool Palette::loadMem(MemChunk& mc, Format format)
{
	// Raw data
	if (format == Format::Raw)
		return loadMem(mc);
	
	// Image
	else if (format == Format::Image)
	{
		SImage image;
		image.open(mc);

		// Verify validity and only accept square images
		if (!image.isValid())
		{
			Global::error = "Palette information cannot be loaded from an invalid image";
			LOG_MESSAGE(0, Global::error);
			return false;
		}
		int side = image.getHeight();
		if (side != image.getWidth() || side%16)
		{
			Global::error = "Palette information cannot be loaded from a non-square image";
			LOG_MESSAGE(0, Global::error);
			return false;
		}

		// Find color cell size
		int cell = side / 16;

		// Add colors to the palette
		for (size_t a = 0; a < 256; ++a)
		{
			// Find position in grid
			int x = (a % 16) * cell;
			int y = (a / 16) * cell;

			// Ignore possible borders
			if (cell > 3)
				++x, ++y;

			// Get color from image
			rgba_t col = image.getPixel(x, y);
			col.index = a;

			// Validate color cell
			for (int b = x; b < (x + (cell > 3 ? cell - 1 : cell)); ++b)
				for (int c = y; c < (y + (cell > 3 ? cell - 1 : cell)); ++c)
					if (!col.equals(image.getPixel(b, c)))
						LOG_MESSAGE(0, "Image does not seem to be a valid palette, color discrepancy in cell %u at [%u, %u]", a, b, c);

			// Color is validated, so add it
			LOG_MESSAGE(3, "Colour index %d / at %d,%d / rgb %d,%d,%d", a, x, y, col.r, col.g, col.b);
			setColour(a, col);
		}

		return true;
	}

	// Text formats
	else if (format == Format::CSV || format == Format::JASC || format == Format::GIMP)
	{

		if (memchr(mc.getData(), 0, mc.getSize() -1))
			return false; // Not text

		Tokenizer tz;
		tz.setSpecialCharacters(",:#");
		tz.openMem(mc, "Palette Import");

		// Parse headers
		if (format == Format::JASC)
		{
			if (!tz.checkToken("JASC-PAL") || !tz.checkToken("0100"))
			{
				Global::error = "Invalid JASC palette (unknown header)";
				LOG_MESSAGE(0, Global::error);
				return false;
			}
			int count = tz.getInteger();
			if (count > 256 || count < 0)
			{
				Global::error = "Invalid JASC palette (wrong count)";
				LOG_MESSAGE(0, Global::error);
				return false;
			}
		}
		else if (format == Format::GIMP)
		{
			if (!tz.checkToken("GIMP") || !tz.checkToken("Palette"))
			{
				Global::error = "Invalid GIMP palette (unknown header)";
				LOG_MESSAGE(0, Global::error);
				return false;
			}
		}
		// Now, parse
		string s1, s2, s3;
		rgba_t col(0, 0, 0, 255, -1);
		int c = 0;
		do
		{
			// Get the first token. If it begins with #, it's a comment in GIMP. Ignore.
			// Since the lexer expects ## for comments, not just #, tell it explicitly to skip.
			s1 = tz.getToken(); if (format == Format::CSV) tz.checkToken(",");
			else if (format == Format::GIMP && !s1.Cmp("#")) { tz.advToEndOfLine(); continue; }

			// Get the second token. If it is :, then that means the first word was a field name.
			// Since we're ignoring them, skip the line.
			s2 = tz.getToken(); if (format == Format::CSV) tz.checkToken(",");
			else if (format == Format::GIMP && !s2.Cmp(":")) { tz.advToEndOfLine(); continue; }

			// Get the third token. In GIMP, the RGB values are followed by the color name, which
			// can include spaces and is unquoted, so just skip the whole rest of the line.
			s3 = tz.getToken(); if (format == Format::CSV) tz.checkToken(",");
			if (format == Format::GIMP) tz.advToEndOfLine();

			// If we haven't skipped this part from a continue, then we have a colour triplet.
			col.r = atoi(CHR(s1)); col.g = atoi(CHR(s2)); col.b = atoi(CHR(s3)); col.index = c;
			setColour(c++, col);
		}
		while (c < 256 && !tz.peekToken().IsEmpty());

		return true;
	}

	else
	{
		Global::error = "Palette could not be imported, this format is not supported yet for import.";
		LOG_MESSAGE(0, Global::error);
	}

	return false;
}

// ----------------------------------------------------------------------------
// Palette::saveMem
//
// Writes colour information to a MemChunk
// ----------------------------------------------------------------------------
bool Palette::saveMem(MemChunk& mc, Format format, const string& name)
{
	// Clear memchunk
	mc.clear();

	// Write to requested format

	// Raw data
	if (format == Format::Raw)
	{
		mc.reSize(768);
		for (unsigned a = 0; a < 256; a++)
			mc.write(&colours_[a], 3);
	}

	// CSV
	else if (format == Format::CSV)
	{
		string csv;
		for (unsigned a = 0; a < 256; a++)
			csv += S_FMT("%d, %d, %d\n", colours_[a].r, colours_[a].g, colours_[a].b);
		mc.importMem((const uint8_t*)((const char*)csv.ToAscii()), csv.Length());
	}

	// JASC palette
	else if (format == Format::JASC)
	{
		string jasc = "JASC-PAL\n0100\n256\n";
		for (unsigned a = 0; a < 256; a++)
			jasc += S_FMT("%d %d %d\n", colours_[a].r, colours_[a].g, colours_[a].b);
		mc.importMem((const uint8_t*)((const char*)jasc.ToAscii()), jasc.Length());
	}

	// GIMP palette
	else if (format == Format::GIMP)
	{
		string gimp = S_FMT("GIMP Palette\nName: %s\n#\n", name);
		for (unsigned a = 0; a < 256; a++)
			gimp += S_FMT("%d\t%d\t%d\tIndex %u\n", colours_[a].r, colours_[a].g, colours_[a].b, a);
		mc.importMem((const uint8_t*)((const char*)gimp.ToAscii()), gimp.Length());
	}

	// Image
	else if (format == Format::Image)
	{
		SImage image;

		// Generate palette image
		image.create(128, 128, PALMASK, this);
		unsigned xoff = 0;
		unsigned yoff = 0;
		for (unsigned a = 0; a < 256; a++)
		{
			// Draw colour square
			for (unsigned y = 0; y < 8; y++)
			{
				for (unsigned x = 0; x < 8; x++)
					image.setPixel(xoff+x, yoff+y, a, 255);
			}

			// Go to next square
			xoff += 8;
			if (xoff >= 128)
			{
				xoff = 0;
				yoff += 8;
			}
		}

		// Write PNG format
		SIFormat::getFormat("png")->saveImage(image, mc, this);
	}

	// Invalid format
	else
		return false;

	return true;
}

// ----------------------------------------------------------------------------
// Palette::saveFile
//
// Writes colour information to a file at [filename].
// Returns false if the file could not be opened/created, true otherwise
// ----------------------------------------------------------------------------
bool Palette::saveFile(const string& filename, Format format)
{
	// Get palette name
	wxFileName fn(filename);
	string name = fn.GetName();

	// Write data to MemChunk
	MemChunk mc;
	if (!saveMem(mc, format, name))
		return false;

	// Write MemChunk to file
	return mc.exportFile(filename);
}

// ----------------------------------------------------------------------------
// Palette::loadFile
//
// Reads colour information from a file at [filename].
// Returns false if the file could not be opened/parsed, true otherwise
// ----------------------------------------------------------------------------
bool Palette::loadFile(const string& filename, Format format)
{
	// Get palette name
	wxFileName fn(filename);
	string name = fn.GetName();

	// Open the file
	wxFile file(filename);

	// Check that it opened ok
	if (!file.IsOpened())
		return false;

	// The file should contain some actual data
	if (file.Length() == 0)
		return false;

	// Write data to MemChunk
	MemChunk mc;
	mc.importFile(filename, 0, file.Length());

	// Now load it
	return loadMem(mc, format);
}

// ----------------------------------------------------------------------------
// Palette::setColour
//
// Sets the colour at [index]
// ----------------------------------------------------------------------------
void Palette::setColour(uint8_t index, rgba_t col)
{
	colours_[index].set(col);
	colours_[index].index = index;
	colours_lab_[index] = Misc::rgbToLab(col.dr(), col.dg(), col.db());
	colours_hsl_[index] = Misc::rgbToHsl(col.dr(), col.dg(), col.db());
}

// ----------------------------------------------------------------------------
// Palette::setColour
//
// Sets the colour at [index]'s red component
// ----------------------------------------------------------------------------
void Palette::setColourR(uint8_t index, uint8_t val)
{
	colours_[index].r = val;
	colours_lab_[index] = Misc::rgbToLab(colours_[index].dr(), colours_[index].dg(), colours_[index].db());
	colours_hsl_[index] = Misc::rgbToHsl(colours_[index].dr(), colours_[index].dg(), colours_[index].db());
}

// ----------------------------------------------------------------------------
// Palette::setColour
//
// Sets the colour at [index]'s green component
// ----------------------------------------------------------------------------
void Palette::setColourG(uint8_t index, uint8_t val)
{
	colours_[index].g = val;
	colours_lab_[index] = Misc::rgbToLab(colours_[index].dr(), colours_[index].dg(), colours_[index].db());
	colours_hsl_[index] = Misc::rgbToHsl(colours_[index].dr(), colours_[index].dg(), colours_[index].db());
}

// ----------------------------------------------------------------------------
// Palette::setColour
//
// Sets the colour at [index]'s blue component
// ----------------------------------------------------------------------------
void Palette::setColourB(uint8_t index, uint8_t val)
{
	colours_[index].b = val;
	colours_lab_[index] = Misc::rgbToLab(colours_[index].dr(), colours_[index].dg(), colours_[index].db());
	colours_hsl_[index] = Misc::rgbToHsl(colours_[index].dr(), colours_[index].dg(), colours_[index].db());
}

// ----------------------------------------------------------------------------
// Palette::setGradient
//
// Creates a gradient between two colous along a specified index range
// ----------------------------------------------------------------------------
void Palette::setGradient(uint8_t startIndex, uint8_t endIndex, rgba_t startCol, rgba_t endCol)
{
	rgba_t gradCol = rgba_t();
	int range = endIndex - startIndex;
	
	float r_range = endCol.fr() - startCol.fr();
	float g_range = endCol.fg() - startCol.fg();
	float b_range = endCol.fb() - startCol.fb();
	
	for (int a = 0; a <= range; a++)
	{
		float perc;
		if (range <= 0)
		{
			perc = 0.0f;
		}
		else
		{
			perc = (float)a / (float)range;
		}
		
		gradCol.set((int) (((r_range * perc) + startCol.fr()) * 255.0f),
					(int) (((g_range * perc) + startCol.fg()) * 255.0f),
					(int) (((b_range * perc) + startCol.fb()) * 255.0f),
					255, -1, a + startIndex);
		colours_[a + startIndex].set(gradCol);
	}
}

// ----------------------------------------------------------------------------
// Palette::copyPalette
//
// Copies the given palette into this one
// ----------------------------------------------------------------------------
void Palette::copyPalette(Palette* copy)
{
	if (!copy)
		return;

	unsigned n_copy = std::min(colours_.size(), copy->colours_.size());
	for (unsigned a = 0; a < n_copy; a++)
		setColour(a, copy->colour(a));

	index_trans_ = copy->transIndex();
}

// ----------------------------------------------------------------------------
// Palette::findColour
//
// Returns the index of the colour in the palette matching [colour], or -1 if
// no match is found
// ----------------------------------------------------------------------------
short Palette::findColour(rgba_t colour)
{
	for (int a = 0; a < 256; a++)
	{
		if (colours_[a].equals(colour))
			return a;
	}

	return -1;
}

// ----------------------------------------------------------------------------
// Palette::colourDiff
//
// Returns the difference between the given colour [rgb]/[hsl]/[lab] and the
// palette colour at [index], using the colour matching method specified in
// [match]
// ----------------------------------------------------------------------------
double Palette::colourDiff(rgba_t& rgb, hsl_t& hsl, lab_t& lab, int index, ColourMatch match)
{
	double d1, d2, d3;
	switch(match)
	{
	default:
	case ColourMatch::Old:		// Directly with integer values
		d1 = rgb.r - colours_[index].r;
		d2 = rgb.g - colours_[index].g;
		d3 = rgb.b - colours_[index].b;
		break;
	case ColourMatch::RGB:		// With doubles, more precise
		d1 = rgb.dr() - colours_[index].dr();
		d2 = rgb.dg() - colours_[index].dg();
		d3 = rgb.db() - colours_[index].db();
		d1*=col_match_r;
		d2*=col_match_g;
		d3*=col_match_b;
		break;
	case ColourMatch::HSL:
		d1 = hsl.h - colours_hsl_[index].h;
		// Hue wraps around!
		if (d1 >  0.5) d1-= 1.0;
		if (d1 < -0.5) d1+= 1.0;
		d2 = hsl.s - colours_hsl_[index].s;
		d3 = hsl.l - colours_hsl_[index].l;
		d1*=col_match_h;
		d2*=col_match_s;
		d3*=col_match_l;
		break;
	case ColourMatch::C76:
		return CIE::CIE76(lab, colours_lab_[index]);
	case ColourMatch::C94:
		return CIE::CIE94(lab, colours_lab_[index]);
	case ColourMatch::C2K:
		return CIE::CIEDE2000(lab, colours_lab_[index]);
	}
	return (d1*d1)+(d2*d2)+(d3*d3);
}

// ----------------------------------------------------------------------------
// Palette::nearestColour
//
// Returns the index of the closest colour in the palette to [colour]
// ----------------------------------------------------------------------------
short Palette::nearestColour(rgba_t colour, ColourMatch match)
{
	double min_d = 999999;
	short index = 0;
	hsl_t chsl = Misc::rgbToHsl(colour);
	lab_t clab = Misc::rgbToLab(colour);

	// Be nice if there was an easier way to convert from int -> enum class,
	// but then that's kind of the point of them I guess
	static vector<ColourMatch> cm_convert =
	{
		ColourMatch::Default,
		ColourMatch::Old,
		ColourMatch::RGB,
		ColourMatch::HSL,
		ColourMatch::C76,
		ColourMatch::C94,
		ColourMatch::C2K,
		ColourMatch::Stop,
	};
	if (match == ColourMatch::Default)
		match = cm_convert[col_match];

	double delta;
	for (short a = 0; a < 256; a++)
	{
		delta = colourDiff(colour, chsl, clab, a, match);

		// Exact match?
		if (delta == 0.0)
			return a;
		else if (delta < min_d)
		{
			min_d = delta;
			index = a;
		}
	}

	return index;
}

// ----------------------------------------------------------------------------
// Palette::countColours
//
// Returns the number of unique colors in a palette
// ----------------------------------------------------------------------------
size_t Palette::countColours()
{
	rgba_t* usedcolours = new rgba_t[256];
	memset(usedcolours, 0, 256*sizeof(rgba_t));
	size_t used = 0;

	for (int a = 0; a < 256; a++)
	{
		bool found = false;
		for (size_t b = 0; b < used; b++)
		{
			if (colours_[a].equals(usedcolours[b]))
			{
				found = true;
				break;
			}
		}
		if (!found)
			usedcolours[used++].set(colours_[a]);
	}
	delete[] usedcolours;
	return used;
}

// ----------------------------------------------------------------------------
// Palette::applyTranslation
//
// Applies the translation [trans] to this palette
// ----------------------------------------------------------------------------
void Palette::applyTranslation(Translation* trans)
{
	// Check translation was given
	if (!trans)
		return;

	// Duplicate palette (so translation ranges don't interfere with eachother)
	Palette temp;
	temp.copyPalette(this);

	// Translate colors
	for (size_t i = 0; i < 256; ++i)
		temp.setColour(i, trans->translate(colours_[i], this));

	// Load translated palette
	copyPalette(&temp);
}

// ----------------------------------------------------------------------------
// Palette::colourise
//
// Colourises the palette to [colour]
// ----------------------------------------------------------------------------
void Palette::colourise(rgba_t colour, int start, int end)
{
	// Handle default values: a range of (-1, -1) means the entire palette
	if (start < 0 || start > 255)
		start = 0;
	if (end < 0 || end > 255)
		end = 255;

	// Colourise all colours in the range
	for (int i = start; i <= end; ++i)
	{
		rgba_t ncol(colours_[i].r, colours_[i].g, colours_[i].b, colours_[i].a, colours_[i].blend);
		double grey = (ncol.r*col_greyscale_r + ncol.g*col_greyscale_g + ncol.b*col_greyscale_b) / 255.0f;
		if (grey > 1.0) grey = 1.0;
		ncol.r = (uint8_t)(colour.r * grey);
		ncol.g = (uint8_t)(colour.g * grey);
		ncol.b = (uint8_t)(colour.b * grey);
		setColour(i, ncol);
	}
}

// ----------------------------------------------------------------------------
// Palette::tint
//
// Tints the palette to [colour] by [amount]
// ----------------------------------------------------------------------------
void Palette::tint(rgba_t colour, float amount, int start, int end)
{
	// Handle default values: a range of (-1, -1) means the entire palette
	if (start < 0 || start > 255)
		start = 0;
	if (end < 0 || end > 255)
		end = 255;

	// Sanitize values just in case
	if (amount < 0.)
		amount = 0.;
	if (amount > 1.)
		amount = 1.;

	// Tint all colours in the range
	for (int i = start; i <= end; ++i)
	{
		float inv_amt = 1.0f - amount;
		// Might want to do something about the precision loss here and elsewhere:
		// it's possible for 0xFFFFFF shifting to 0xFF0000 to become 0xFExxxx...
		// I'll leave this working exactly the same as the SImage function for now.
		float round_delta = /*roundup ? 0.4999999 :*/ 0.0;
		rgba_t ncol(colours_[i].r*inv_amt + colour.r*amount + round_delta,
		            colours_[i].g*inv_amt + colour.g*amount + round_delta,
		            colours_[i].b*inv_amt + colour.b*amount + round_delta,
		            colours_[i].a, colours_[i].blend);
		setColour(i, ncol);
	}
}

// ----------------------------------------------------------------------------
// Palette::idtint
//
// Tints the palette to [colour] by [amount]
// This one uses a different method to tint the colours, which is taken from
// Carmack's own dcolors.c.
// ----------------------------------------------------------------------------
void Palette::idtint(int r, int g, int b, int shift, int steps)
{
	// Tint all colours in the range
	for (int i = 0; i <= 255; ++i)
	{
		// Compute the colour differences
		int dr = r - colours_[i].r;
		int dg = g - colours_[i].g;
		int db = b - colours_[i].b;
		// Then adjust and clamp for safety
		int fr = colours_[i].r + dr*shift/steps; if (fr < 0) fr = 0; else if (fr > 255) fr = 255;
		int fg = colours_[i].g + dg*shift/steps; if (fg < 0) fg = 0; else if (fg > 255) fg = 255;
		int fb = colours_[i].b + db*shift/steps; if (fb < 0) fb = 0; else if (fb > 255) fb = 255;
		// Set the result in the palette
		rgba_t col(fr, fg, fb, colours_[i].a, colours_[i].blend, i);
		setColour(i, col);
	}
}

// ----------------------------------------------------------------------------
// Palette::saturate
//
// Saturate the palette by [amount] (in range 0--2)
// ----------------------------------------------------------------------------
void Palette::saturate(float amount, int start, int end)
{
	// Handle default values: a range of (-1, -1) means the entire palette
	if (start < 0 || start > 255)
		start = 0;
	if (end < 0 || end > 255)
		end = 255;

	// Sanitize values just in case
	if (amount < 0.)
		amount = 0.;
	if (amount > 2.)
		amount = 2.;

	// Saturate all colours in the range
	for (int i = start; i <= end; ++i)
	{
		colours_hsl_[i].s *= amount;
		if (colours_hsl_[i].s > 1.)
			colours_hsl_[i].s = 1.;
		setColour(i, Misc::hslToRgb(colours_hsl_[i]));
		colours_lab_[i] = Misc::rgbToLab(colours_[i].dr(), colours_[i].dg(), colours_[i].db());
	}
}

// ----------------------------------------------------------------------------
// Palette::illuminate
//
// Darken or brighten the palette by [amount] (in range 0--2)
// ----------------------------------------------------------------------------
void Palette::illuminate(float amount, int start, int end)
{
	// Handle default values: a range of (-1, -1) means the entire palette
	if (start < 0 || start > 255)
		start = 0;
	if (end < 0 || end > 255)
		end = 255;

	// Sanitize values just in case
	if (amount < 0.)
		amount = 0.;
	if (amount > 2.)
		amount = 2.;

	// Illuminate all colours in the range
	for (int i = start; i <= end; ++i)
	{
		colours_hsl_[i].l *= amount;
		if (colours_hsl_[i].l > 1.)
			colours_hsl_[i].l = 1.;
		setColour(i, Misc::hslToRgb(colours_hsl_[i]));
		colours_lab_[i] = Misc::rgbToLab(colours_[i].dr(), colours_[i].dg(), colours_[i].db());
	}
}

// ----------------------------------------------------------------------------
// Palette::shift
//
// Shift the hue of the palette by [amount] (in range 0--1)
// ----------------------------------------------------------------------------
void Palette::shift(float amount, int start, int end)
{
	// Handle default values: a range of (-1, -1) means the entire palette
	if (start < 0 || start > 255)
		start = 0;
	if (end < 0 || end > 255)
		end = 255;

	// Sanitize values just in case
	if (amount < 0.)
		amount = 0.;
	if (amount > 1.)
		amount = 1.;

	// Shift all colours in the range
	for (int i = start; i <= end; ++i)
	{
		colours_hsl_[i].h += amount;
		if (colours_hsl_[i].h >= 1.)
			colours_hsl_[i].h -= 1.;
		setColour(i, Misc::hslToRgb(colours_hsl_[i]));
		colours_lab_[i] = Misc::rgbToLab(colours_[i].dr(), colours_[i].dg(), colours_[i].db());
	}
}

// ----------------------------------------------------------------------------
// Palette::invert
//
// Inverts the colours of the palette
// ----------------------------------------------------------------------------
void Palette::invert(int start, int end)
{
	// Handle default values: a range of (-1, -1) means the entire palette
	if (start < 0 || start > 255)
		start = 0;
	if (end < 0 || end > 255)
		end = 255;

	// Inverts all colours in the range
	for (int i = start; i <= end; ++i)
	{
		colours_[i].r = 255 - colours_[i].r;
		colours_[i].g = 255 - colours_[i].g;
		colours_[i].b = 255 - colours_[i].b;
		setColour(i, colours_[i]);	// Just to update the HSL values
	}
}
