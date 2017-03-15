
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SImageFormats.cpp
 * Description: SImage class - Encapsulates a paletted or 32bit image.
 *              Handles loading/saving different formats, palette
 *              conversions, offsets, and a bunch of other stuff
 *
 *				This file contains the load/save functions for font
 *              formats (see SIF*.h for image formats)
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
#include "SImage.h"
#undef BOOL
#include "General/Misc.h"
#include "SIFormat.h"


/*******************************************************************
 * FONT FORMATS
 *******************************************************************/

/* SImage::loadFont0
 * Loads a Doom alpha HUFONT lump and displays it as a picture.
 * Why "font0" when it has no FON0 header? Because alpha. ;)
 * The format used is simple:
 * Offset | Length | Type | Name
 *  0x000 |      2 | ui16 | image height (one value for all chars)
 *  0x002 |  256*1 | ui08 | characterwidth (one value per char)
 *  0x102 |  256*2 | ui16 | characteroffset (one value per char)
 *  0x302 |    x*1 | ui08 | pixel color index (one value per pixel)
 * So, total size - 302 % value @ 0x00 must be null.
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
bool SImage::loadFont0(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data)
		return false;

	if (size <= 0x302)
		return false;

	offset_x = offset_y = 0;
	height = READ_L16(gfx_data, 0);

	size_t datasize = size - 0x302;
	if (datasize % height)
		return false;

	width = datasize / height;

	clearData();
	has_palette = false;
	type = PALMASK;
	format = NULL;

	// Technically each character is its own image, though.
	numimages = 1;
	imgindex = 0;

	// Create new picture and mask
	const uint8_t* r = gfx_data + 0x302;
	data = new uint8_t[datasize];
	mask = new uint8_t[datasize];
	memset(mask, 0xFF, datasize);

	// Data is in column-major format, convert to row-major
	size_t p = 0;
	for (size_t i = 0; i < datasize; ++i)
	{
		data[p] = r[i];

		// Index 0 is transparent
		if (data[p] == 0)
			mask[p] = 0;

		// Move to next column
		p+=width;

		// Move to next row
		if (p >= datasize)
		{
			p -= datasize;
			++p;
		}
	}
	// Announce change and return success
	announce("image_changed");
	return true;
}

/* SImage::loadFont1
 * Loads a ZDoom FON1 lump and displays it as a picture.
 * Graphically-speaking, a FON1 lump is a column of 256 characters,
 * each width*height as indicated by the header. Of course,
 * it would be better to convert that into a 16x16 grid, which
 * would be a lot more legible...
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
bool SImage::loadFont1(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data)
		return false;

	// Check/setup size
	size_t charwidth = gfx_data[4]+256*gfx_data[5];
	width = charwidth;
	size_t charheight = gfx_data[6]+256*gfx_data[7];
	height = charheight<<8;

	// Setup variables
	offset_x = offset_y = 0;
	has_palette = false;
	type = PALMASK;

	// Technically each character is its own image, though.
	numimages = 1;
	imgindex = 0;

	// Clear current data if it exists
	clearData();
	format = NULL;

	// Read raw pixel data
	data = new uint8_t[width*height];
	mask = new uint8_t[width*height];
	memset(mask, 0xFF, width*height);

	// Since gfx_data is a const pointer, we can't work on it.
	uint8_t* tempdata = new uint8_t[size];
	memcpy(tempdata, gfx_data, size);


	// We'll use wandering pointers. The original pointer is kept for cleanup.
	uint8_t* read = tempdata + 8;
	uint8_t* readend = tempdata + size - 1;
	uint8_t* dest  = data;
	uint8_t* destend = dest + width*height;

	uint8_t code = 0; size_t length = 0;

	// This uses the same RLE algorithm as compressed IMGZ.
	while (read < readend && dest < destend)
	{
		code = *read++;
		if (code < 0x80)
		{
			length = code + 1;
			memcpy(dest, read, length);
			dest+=length;
			read+=length;
		}
		else if (code > 0x80)
		{
			length = 0x101 - code;
			code = *read++;
			memset(dest, code, length);
			dest+=length;
		}
	}
	delete[] tempdata;
	// Add transparency to mask
	for (size_t i = 0; i < (unsigned)(width*height); ++i)
		if (data[i] == 0)
			mask[i] = 0x00;

	// Announce change and return success
	announce("image_changed");
	return true;
}

/* SImage::loadFont2
 * Loads a ZDoom FON2 lump and displays it as a picture.
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
struct Font2Char
{
	uint16_t width;
	uint8_t* data;
	Font2Char() { width = 0; data = NULL; }
};
struct Font2Header
{
	uint8_t headr[4];
	uint16_t charheight;
	uint8_t firstc;
	uint8_t lastc;
	uint8_t constantw;
	uint8_t shading;
	uint8_t palsize;
	uint8_t kerning;	// Theoretically a flag field, but with only one flag...
};
bool SImage::loadFont2(const uint8_t* gfx_data, int size)
{
	// Clear current data if it exists
	clearData();

	// Initializes some stuff
	offset_x = offset_y = 0;
	has_palette = true;
	type = PALMASK;
	format = NULL;

	// Technically each character is its own image, though.
	numimages = 1;
	imgindex = 0;

	if (size < sizeof(Font2Header))
		return false;

	const Font2Header* header = (Font2Header*) gfx_data;
	width = 0;
	height = wxUINT16_SWAP_ON_BE(header->charheight);

	// We can't deal with a null height, of course
	if (height == 0)
		return false;

	const uint8_t* p = gfx_data + sizeof(Font2Header);

	// Skip kerning information which do not concern us.
	if (header->kerning)
		p+=2;

	// Start building the character table.
	size_t numchars = wxUINT16_SWAP_ON_BE(header->lastc) - wxUINT16_SWAP_ON_BE(header->firstc) +1;
	Font2Char* chars = new Font2Char[numchars];
	for (size_t i = 0; i < numchars; ++i)
	{
		chars[i].width = wxUINT16_SWAP_ON_BE(*(uint16_t*)p);
		// Let's increase the total width
		width+=chars[i].width+1;
		// The width information is enumerated for each character only if they are
		// not constant width. Regardless, move the read pointer away after the last.
		if (!(header->constantw) || (i == numchars - 1))
			p+=2;
	}

	// Let's build the palette now.
	for (size_t i = 0; i < header->palsize + 1u; ++i)
	{
		rgba_t color;
		color.r = *p++;
		color.g = *p++;
		color.b = *p++;
		palette.setColour(i, color);
	}

	// 0 is transparent, last is border color, the rest of the palette entries should
	// be increasingly bright
	palette.setTransIndex(0);

	// The picture data follows, using the same RLE as FON1 and IMGZ.
	for (size_t i = 0; i < numchars; ++i)
	{
		// A big font is not necessarily continuous; several characters
		// may be skipped; they are given a width of 0.
		if (chars[i].width)
		{
			size_t numpixels = chars[i].width * height;
			chars[i].data = new uint8_t[numpixels];
			uint8_t* d = chars[i].data;
			uint8_t code = 0; size_t length = 0;

			while (numpixels)
			{
				code = *p++;
				if (code < 0x80)
				{
					length = code + 1;
					// Overflows shouldn't happen
					if (length > numpixels)
					{
						delete[] chars;
						return false;
					}
					memcpy(d, p, length);
					d+=length;
					p+=length;
					numpixels -= length;
				}
				else if (code > 0x80)
				{
					length = 0x101 - code;
					// Overflows shouldn't happen
					if (length > numpixels)
					{
						delete[] chars;
						return false;
					}
					code = *p++;
					memset(d, code, length);
					d+=length;
					numpixels -= length;
				}
			}
		}
	}

	// Now let's assemble all characters together in a single picture.
	if (!width)
		return false;

	// Clear current data if it exists
	clearData();

	data = new uint8_t[width*height];
	memset(data, header->palsize, width*height);
	uint8_t* d = data;
	for (size_t i = 0; i < (unsigned)height; ++i)
	{
		for (size_t j = 0; j < numchars; ++j)
		{
			if (chars[j].width)
			{
				memcpy(d, chars[j].data+(i*chars[j].width), chars[j].width);
				d+=chars[j].width+1;
			}
		}
	}

	// Clean up the characters once the big picture is finished
	for (size_t i = 0; i < numchars; ++i)
	{
		if (chars[i].data)
			delete[] chars[i].data;
	}
	delete[] chars;

	// Now transparency for the mask
	mask = new uint8_t[width*height];
	memset(mask, 0xFF, width*height);
	for (size_t i = 0; i < (unsigned)(width*height); ++i)
		if (data[i] == 0)
			mask[i] = 0;

	// Announce change and return success
	announce("image_changed");
	return true;
}

/* SImage::loadBMF
 * Loads a byte map font lump and displays it as a picture.
 * Specs for the format are here: http://bmf.wz.cz/bmf-format.htm
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
struct BMFChar
{
	uint8_t which; // 0
	uint8_t width; // 1
	uint8_t height;// 2
	int8_t offsx; // 3
	int8_t offsy; // 4
	uint8_t shift; // 5
	// Rest is not part of the header proper
	const uint8_t* cdata;
	BMFChar() { which = width = height = offsx = offsy = shift = 0; cdata = NULL; }
};
struct BMFFont
{
	uint8_t headr[4];	//  0
	uint8_t version;	//  4
	uint8_t lineheight;	//  5
	int8_t size_over;	//  6
	int8_t size_under;	//  7
	int8_t add_space;	//  8
	int8_t size_inner;	//  9
	uint8_t num_colors;	// 10
	uint8_t top_color;	// 11
	uint8_t reserved[4];// 12
	uint8_t pal_size;	// 16
	// Rest is not part of the header proper
	uint8_t info_size;
	uint16_t num_chars;
	const char* info;
	BMFChar* chars;
	BMFFont()
	{
		lineheight = size_over = size_under = add_space = size_inner =
		        num_colors = top_color = pal_size = info_size = num_chars = 0;
		info = NULL; chars = NULL;
	}
	BMFFont(BMFFont* other)
	{
		lineheight = other->lineheight;
		size_over = other->size_over;
		size_under = other->size_under;
		add_space = other->add_space;
		size_inner = other->size_inner;
		num_colors = other->num_colors;
		top_color = other->top_color;
		pal_size = other->pal_size;
		info_size = other->info_size;
		num_chars = other->num_chars;
		info = NULL; chars = NULL;
	}
	~BMFFont() { if (chars) delete[] chars; }
};
bool SImage::loadBMF(const uint8_t* gfx_data, int size)
{
	if (!gfx_data || size < 24)
		return false;
	const uint8_t* eod = gfx_data + size;

	BMFFont mf((BMFFont*)gfx_data);

	// Check for invalid data, we need at least one visible color
	if (mf.pal_size == 0)
		return false;

	// Clean up old data and set up variables
	clearData();
	type = PALMASK;
	has_palette = true;
	format = NULL;

	// Technically each character is its own image, though.
	numimages = 1;
	imgindex = 0;

	const uint8_t* ofs = gfx_data+17;

	// Setup palette -- it's a 6-bit palette (63 max) so we have to convert it to 8-bit.
	// Palette index 0 is used as the transparent color and not described at all.
	palette.setColour(0, rgba_t(0, 0, 0, 0));
	for (size_t i = 0; i < mf.pal_size; ++i)
	{
		palette.setColour(i+1, rgba_t((ofs[(i*3)]<<2)+(ofs[(i*3)]>>4),
		                              (ofs[(i*3)+1]<<2)+(ofs[(i*3)]>>4), (ofs[(i*3)+2]<<2)+(ofs[(i*3)+2]>>4), 255));
	}

	// Move to after the palette and get amount of characters
	ofs += mf.pal_size*3;
	if (ofs >= eod)
	{
		wxLogMessage("BMF aborted: no data after palette");
		return false;
	}
	mf.info_size = ofs[0];
	if (mf.info_size > 0)
	{
		mf.info = (char*) ofs+1;
	}
	ofs+=mf.info_size+1;
	mf.num_chars = READ_L16(ofs, 0);
	if (mf.num_chars == 0)
		return false;

	// Now we are at the beginning of character data
	ofs+=2;
	if (ofs >= eod)
	{
		wxLogMessage("BMF aborted: no data after char size");
		return false;
	}
	// Let's create each character's data and compute the total size
	mf.chars = new BMFChar[mf.num_chars];
	int miny = ofs[4], maxy = ofs[2];
	width = ofs[5] + ofs[3];
	for (size_t i = 0; i < mf.num_chars; ++i)
	{
		mf.chars[i].which = ofs[0];
		mf.chars[i].width = ofs[1];
		mf.chars[i].height= ofs[2];
		mf.chars[i].offsx = ofs[3];
		mf.chars[i].offsy = ofs[4];
		mf.chars[i].shift = ofs[5];
		mf.chars[i].cdata = ofs+6;
		ofs+=(6+ (mf.chars[i].width * mf.chars[i].height));
		// Sanity check, some supposedly-valid fonts do not have all the
		// characters they pretend to have (e.g., 274.bmf). Being truncated
		// does not prevent them from being considered valid, so cut them off
		if (ofs >= eod && (i+1) < mf.num_chars)
		{
			mf.num_chars = i;
		}
		// Zap empty characters, no need to waste space displaying their void.
		if (mf.chars[i].width == 0 && mf.chars[i].height == 0)
		{
			--i;
			--mf.num_chars;
		}
		else
		{
			if (miny > mf.chars[i].offsy)
				miny = mf.chars[i].offsy;
			if (maxy < mf.chars[i].height)
				maxy = mf.chars[i].height;
			width += mf.add_space + mf.chars[i].shift;
		}
	}
	height = maxy - miny;

	// Create new fully transparent image
	size_t pixela = 0, pixelb = 0, pixels = width * height;
	data = new uint8_t[pixels];
	mask = new uint8_t[pixels];
	memset(data, 0x00, pixels);
	memset(mask, 0x00, pixels);

	// Start processing each character, painting it on the empty canvas
	int startx = (mf.chars[0].offsy < 0 ? 0 : mf.chars[0].offsy);
	int starty = (miny < 0 ? 0 - miny : 0);
	for (int i = 0; i < mf.num_chars; ++i)
	{
		BMFChar* mc = &mf.chars[i];
		if (mc->width && mc->height)
		{
			for (int v = 0; v < mc->height; ++v)
			{
				for (int u = 0; u < mc->width; ++u)
				{
					// Source pixel
					pixela = v*mc->width + u;
					// Destination pixel
					pixelb = (starty+v+mc->offsy)*width+startx+u+mc->offsx;
					// Only paint if appropriate
					if ((mc->cdata + pixela < eod) && (mc->cdata + pixela < mf.chars[i+1].cdata - 6) &&
					        mc->cdata[pixela] && pixelb < pixels)
					{
						data[pixelb] = mc->cdata[pixela];
						mask[pixelb] = 0xFF;
					}
				}
			}
			startx+=(mf.add_space + mc->shift);
		}
	}
	// Announce change and return success
	announce("image_changed");
	return true;
}

/* SImage::loadFontM
 * Loads a monochrome, monospaced font and displays it as a picture.
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
bool SImage::loadFontM(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data || size % 256)
		return false;

	// Setup variables
	offset_x = offset_y = 0;
	has_palette = false;
	type = PALMASK;
	format = NULL;

	size_t charwidth = 8;
	size_t charheight = size>>8;
	width = charwidth;
	height = charheight << 8;

	if (width * height != size * 8)
		return false;

	// reset data
	clearData();
	data = new uint8_t[width*height];
	memset(data, 0xFF, width*height);
	mask = new uint8_t[width*height];
	memset(mask, 0x00, width*height);

	// Technically each character is its own image, though.
	numimages = 1;
	imgindex = 0;

	//Each pixel is described as a single bit, either on or off
	for (size_t i = 0; i < (unsigned)size; ++i)
	{
		for (size_t p = 0; p < 8; ++p)
			mask[(i*8)+p] = ((gfx_data[i]>>(7-p)) & 1) * 255;
	}
	// Announce change and return success
	announce("image_changed");
	return true;
}

/* SImage::loadWolfFont
 * Loads a Wolf3D-format font.
 * The format used is simple, basically like the Doom alpha HUFONT,
 * except not in the same order:
 * Offset | Length | Type | Name
 *  0x000 |      2 | ui16 | image height (one value for all chars)
 *  0x002 |  256*2 | ui16 | characteroffset (one value per char)
 *  0x202 |  256*1 | ui08 | characterwidth (one value per char)
 *  0x302 |    x*1 | ui08 | pixel color index (one value per pixel)
 * So, total size - 302 % value @ 0x00 must be null.
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
bool SImage::loadWolfFont(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data)
		return false;

	if (size <= 0x302)
		return false;

	offset_x = offset_y = 0;
	height = READ_L16(gfx_data, 0);

	size_t datasize = size - 0x302;
	if (datasize % height)
		return false;

	width = datasize / height;

	clearData();
	has_palette = false;
	type = PALMASK;
	format = NULL;

	// Technically each character is its own image, though.
	numimages = 1;
	imgindex = 0;

	// Create new picture and mask
	const uint8_t* r = gfx_data + 0x302;
	data = new uint8_t[datasize];
	mask = new uint8_t[datasize];
	memset(mask, 0xFF, datasize);
	memcpy(data, r, datasize);

	size_t p = 0; // Previous width
	size_t w = 0; // This character's width
	// Run through each character
	for (size_t c = 0; c < 256; ++c)
	{
		p += w; // Add previous character width to total
		w = gfx_data[c + 0x202]; // Get this character's width
		if (!w) continue;
		size_t o = READ_L16(gfx_data, ((c<<1)+2));
		for (size_t i = 0; i < w * height; ++i)
		{
			// Compute source and destination offsets
			size_t s = o + i;
			size_t d = ((i/w) * width) + (i%w) + p;
			data[d] = gfx_data[s];
			// Index 0 is transparent
			if (data[d] == 0)
				mask[d] = 0;
		}
	}
	// Announce change and return success
	announce("image_changed");
	return true;
}


/* SImage::loadJediFNT
 * Loads a Jedi Engine-format bitmap font.
 * The header tells the height and which are the first and last
 * characters described. Then the character data consists of a
 * single byte of data for the width of that character, followed
 * by a list of columns. The characters are listed in order.
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
bool SImage::loadJediFNT(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data)
		return false;

	if (size <= 35)
		return false;

	// The character data is presented as a list of columns
	// preceded by a byte for
	offset_x = offset_y = 0;

	// Since the format is column-major, we'll use our usual cheat of
	// inverting height and width to build the picture, then rotating it.
	width = gfx_data[4];

	// First and last characters
	uint8_t firstc = gfx_data[8];
	uint8_t lastc  = gfx_data[9];
	uint8_t numchr = 1 + lastc - firstc;

	// Go through each character to compute the total width (pre-rotation height)
	height = 0;
	size_t wo = 32; // Offset to width of next character
	for (uint8_t i = 0; i < numchr; ++i)
	{
		height += gfx_data[wo];
		wo += 1 + (width * gfx_data[wo]);
	}

	clearData();
	has_palette = false;
	type = PALMASK;
	format = NULL;

	// Technically each character is its own image, though.
	numimages = 1;
	imgindex = 0;

	// Create new picture and mask
	data = new uint8_t[width * height];
	mask = new uint8_t[width * height];
	memset(mask, 0xFF, width * height);

	// Run through each character and add the pixel data
	wo = 32;
	size_t col = 0;
	for (uint8_t i = 0; i < numchr; ++i)
	{
		uint8_t numcols = gfx_data[wo++];
		memcpy(data + (col * width), gfx_data + wo, numcols * width);
		col += numcols;
		wo += width * numcols;
	}

	// Make index 0 transparent
	for (int i = 0; i < width * height; ++i)
		if (data[i] == 0)
			mask[i] = 0;

	// Convert from column-major to row-major
	rotate(90);

	// Announce change and return success
	announce("image_changed");
	return true;
}

/* SImage::loadJediFONT
 * Loads a Jedi Engine-format monochrome font.
 * Contrarily to what the DF specs claim, the first two int16
 * values are not the first and last characters as in the FNT
 * format; instead, they are the first character and the number
 * of characters! They're also mistaken about character width.
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
bool SImage::loadJediFONT(const uint8_t* gfx_data, int size)
{
	// Check data
	if (size < 16)
		return false;

	int numchr = READ_L16(gfx_data, 2);

	// Setup variables
	offset_x = offset_y = 0;
	height = READ_L16(gfx_data, 6) * numchr;
	width = READ_L16(gfx_data, 4);
	has_palette = false;
	type = PALMASK;
	format = NULL;

	// reset data
	clearData();
	data = new uint8_t[width*height];
	memset(data, 0xFF, width*height);
	mask = new uint8_t[width*height];
	memset(mask, 0x00, width*height);

	// Technically each character is its own image, though.
	numimages = 1;
	imgindex = 0;

	// We don't care about the character widths since
	// technically it's always eight anyway. The offset
	// to graphic data corresponds to 12 (header size)
	// plus one byte by character for width.
	int o = 12 + numchr;
	int bpc = width / 8;

	//Each pixel is described as a single bit, either on or off
	for (int i = 0; i < height; ++i)
	{
		for (int p = 0; p < width; ++p)
		{
			switch (bpc)
			{
			case 1: mask[(i*width)+p] = ((gfx_data[o+i]>>(7-p)) & 1) * 255; break;
			case 2: mask[(i*width)+p] = ((READ_B16(gfx_data, o+(i*2))>>(15-p)) & 1) * 255; break;
			case 3: mask[(i*width)+p] = ((READ_B24(gfx_data, o+(i*3))>>(23-p)) & 1) * 255; break;
			case 4: mask[(i*width)+p] = ((READ_B32(gfx_data, o+(i*4))>>(31-p)) & 1) * 255; break;
			default: clearData(); Global::error = "Jedi FONT: Weird word width"; return false;
			}
		}
	}
	// Announce change and return success
	announce("image_changed");
	return true;
}

/* SImage::loadJaguarSprite
 * Loads a Jaguar Doom sprite. This needs manual handling because
 * the data is split in two separate lumps, one with the header and
 * the other with raw pixel data. So we need to have access to both.
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
bool SImage::loadJaguarSprite(const uint8_t* header, int hdr_size, const uint8_t* gfx_data, int size)
{
	if (header == NULL || gfx_data == NULL || hdr_size < 16 || size == 0)
	{
		Global::error = "Invalid Jaguar sprite";
		return false;
	}

	// Setup variables
	int16_t ofsx, ofsy;
	width	 = READ_B16(header, 0);
	height	 = READ_B16(header, 2);
	ofsx	 = READ_B16(header, 4);
	ofsy	 = READ_B16(header, 6);
	offset_x = ofsx; offset_y = ofsy;
	has_palette = false;
	type = PALMASK;
	format = NULL;
	numimages = 1;
	imgindex = 0;

	// reset data
	clearData();
	data = new uint8_t[width*height];
	memset(data, 0x00, width*height);
	mask = new uint8_t[width*height];
	memset(mask, 0x00, width*height);

	// Read column offsets
	if (hdr_size < (8 + (width * 6)))
	{
		Global::error = S_FMT("Invalid Jaguar sprite: header too small (%d) for column offsets (%d)", hdr_size, (8 + (width * 6)));
		return false;
	}
	vector<uint16_t> col_offsets(width);
	for (int w = 0; w < width; ++w)
	{
		col_offsets[w] = READ_B16(header, 8+2*w);
	}
	if (hdr_size < (4 + col_offsets[width - 1]))
	{
		Global::error = S_FMT("Invalid Jaguar sprite: header too small (%d) for post offsets (%d)", hdr_size, 4 + col_offsets[width - 1]);
		return false;
	}

	// Okay, so it's finally time to read some pixel data
	for (int w = 0; w < width; ++w)
	{
		int post_p = col_offsets[w];
		// Process all posts in the column
		while (READ_B16(header, post_p) != 0xFFFF)
		{
			int top = header[post_p];
			int len = header[post_p + 1];
			int pixel_p = READ_B16(header, post_p + 2);
			if (pixel_p + len > size)
			{
				Global::error = S_FMT("Invalid Jaguar sprite: body too small (%d) for pixel data (%d)", size, pixel_p + len);
				return false;
			}
			// Copy pixels
			for (int p = 0; p < len; ++p)
			{
				size_t pos = w + width * (top + p);
				data[pos] = gfx_data[pixel_p + p];
				mask[pos] = 0xFF;
			}
			post_p +=4;
		}
	}

	// Announce change and return success
	announce("image_changed");
	return true;
}

/* SImage::loadJaguarTexture
 * Loads a Jaguar Doom texture. This needs manual handling because
 * the dimensions are contained in the TEXTURE1 lump instead.
 * Returns false if the image data was invalid, true otherwise.
 *******************************************************************/
bool SImage::loadJaguarTexture(const uint8_t* gfx_data, int size, int i_width, int i_height)
{
	// Check data
	if (i_width * i_height == 0 || size < i_width*i_height + 320)
	{
		Global::error = S_FMT("Size is %d, expected %d", size, i_width*i_height + 320);
		return false;
	}

	// Setup variables
	offset_x = offset_y = 0;
	width = i_height;	// Format is column-major
	height = i_width;	// We'll rotate them afterwards
	has_palette = false;
	type = PALMASK;
	format = NULL;
	numimages = 1;
	imgindex = 0;

	// reset data
	clearData();
	data = new uint8_t[width*height];
	memcpy(data, gfx_data, width*height);
	mask = new uint8_t[width*height];
	memset(mask, 0xFF, width*height);

	// rotate and mirror image
	rotate(90);
	mirror(false);

	// Announce change and return success
	announce("image_changed");
	return true;
}
