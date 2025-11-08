
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    SImageFormats.cpp
// Description: SImage class - Encapsulates a paletted or 32bit image. Handles
//              loading/saving different formats, palette conversions, offsets,
//              and a bunch of other stuff
//
//              This file contains the load/save functions for font formats
//              (see SIF*.h for image formats)
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
#include "SImage.h"
#include "Utility/Memory.h"
#undef BOOL
#include "General/Misc.h"
#include "SIFormat.h"
#include "thirdparty/lunasvg/include/lunasvg.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Font Formats
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Loads a Doom alpha HUFONT lump and displays it as a picture.
// Why "font0" when it has no FON0 header? Because alpha. ;)
// The format used is simple:
// Offset | Length | Type | Name
//  0x000 |      2 | ui16 | image height (one value for all chars)
//  0x002 |  256*1 | ui08 | characterwidth (one value per char)
//  0x102 |  256*2 | ui16 | characteroffset (one value per char)
//  0x302 |    x*1 | ui08 | pixel color index (one value per pixel)
// So, total size - 302 % value @ 0x00 must be null.
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::loadFont0(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data)
		return false;

	if (size <= 0x302)
		return false;

	offset_x_ = offset_y_ = 0;
	height_               = memory::readL16(gfx_data, 0);

	size_t datasize = size - 0x302;
	if (datasize % height_)
		return false;

	width_ = datasize / height_;

	clearData();
	has_palette_ = false;
	type_        = Type::PalMask;
	format_      = nullptr;

	// Technically each character is its own image, though.
	numimages_ = 1;
	imgindex_  = 0;

	// Create new picture and mask
	auto r = gfx_data + 0x302;
	data_.reSize(datasize, false);
	mask_.reSize(datasize, false);
	mask_.fillData(0xFF);

	// Data is in column-major format, convert to row-major
	size_t p = 0;
	for (size_t i = 0; i < datasize; ++i)
	{
		data_[p] = r[i];

		// Index 0 is transparent
		if (data_[p] == 0)
			mask_[p] = 0;

		// Move to next column
		p += width_;

		// Move to next row
		if (p >= datasize)
		{
			p -= datasize;
			++p;
		}
	}
	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a ZDoom FON1 lump and displays it as a picture.
// Graphically-speaking, a FON1 lump is a column of 256 characters, each
// width*height as indicated by the header.
// Of course, it would be better to convert that into a 16x16 grid, which would
// be a lot more legible...
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::loadFont1(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data)
		return false;

	// Check/setup size
	size_t charwidth  = gfx_data[4] + 256 * gfx_data[5];
	width_            = charwidth;
	size_t charheight = gfx_data[6] + 256 * gfx_data[7];
	height_           = charheight << 8;

	// Setup variables
	offset_x_ = offset_y_ = 0;
	has_palette_          = false;
	type_                 = Type::PalMask;

	// Technically each character is its own image, though.
	numimages_ = 1;
	imgindex_  = 0;

	// Clear current data if it exists
	clearData();
	format_ = nullptr;

	// Read raw pixel data
	data_.reSize(width_ * height_, false);
	mask_.reSize(width_ * height_, false);
	mask_.fillData(0xFF);

	// Since gfx_data is a const pointer, we can't work on it.
	auto tempdata = new uint8_t[size];
	memcpy(tempdata, gfx_data, size);


	// We'll use wandering pointers. The original pointer is kept for cleanup.
	uint8_t* read    = tempdata + 8;
	uint8_t* readend = tempdata + size - 1;
	uint8_t* dest    = data_.data();
	uint8_t* destend = dest + width_ * height_;

	uint8_t code   = 0;
	size_t  length = 0;

	// This uses the same RLE algorithm as compressed IMGZ.
	while (read < readend && dest < destend)
	{
		code = *read++;
		if (code < 0x80)
		{
			length = code + 1;
			memcpy(dest, read, length);
			dest += length;
			read += length;
		}
		else if (code > 0x80)
		{
			length = 0x101 - code;
			code   = *read++;
			memset(dest, code, length);
			dest += length;
		}
	}
	delete[] tempdata;
	// Add transparency to mask
	for (size_t i = 0; i < static_cast<unsigned>(width_ * height_); ++i)
		if (data_[i] == 0)
			mask_[i] = 0x00;

	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a ZDoom FON2 lump and displays it as a picture.
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
struct Font2Char
{
	uint16_t width;
	uint8_t* data;
	Font2Char()
	{
		width = 0;
		data  = nullptr;
	}
};
struct Font2Header
{
	uint8_t  headr[4];
	uint16_t charheight;
	uint8_t  firstc;
	uint8_t  lastc;
	uint8_t  constantw;
	uint8_t  shading;
	uint8_t  palsize;
	uint8_t  kerning; // Theoretically a flag field, but with only one flag...
};
bool SImage::loadFont2(const uint8_t* gfx_data, int size)
{
	// Clear current data if it exists
	clearData();

	// Initializes some stuff
	offset_x_ = offset_y_ = 0;
	has_palette_          = true;
	type_                 = Type::PalMask;
	format_               = nullptr;

	// Technically each character is its own image, though.
	numimages_ = 1;
	imgindex_  = 0;

	if (size < static_cast<int>(sizeof(Font2Header)))
		return false;

	const Font2Header* header = reinterpret_cast<const Font2Header*>(gfx_data);
	width_                    = 0;
	height_                   = wxUINT16_SWAP_ON_BE(header->charheight);

	// We can't deal with a null height, of course
	if (height_ == 0)
		return false;

	const uint8_t* p = gfx_data + sizeof(Font2Header);

	// Skip kerning information which do not concern us.
	if (header->kerning)
		p += 2;

	// Start building the character table.
	size_t numchars = wxUINT16_SWAP_ON_BE(header->lastc) - wxUINT16_SWAP_ON_BE(header->firstc) + 1;
	auto*  chars    = new Font2Char[numchars];
	for (size_t i = 0; i < numchars; ++i)
	{
		chars[i].width = wxUINT16_SWAP_ON_BE(*reinterpret_cast<const uint16_t*>(p));
		// Let's increase the total width
		width_ += chars[i].width;
		if (chars[i].width > 0) // Add spacing between characters
			width_++;
		// The width information is enumerated for each character only if they are
		// not constant width. Regardless, move the read pointer away after the last.
		if (!(header->constantw) || (i == numchars - 1))
			p += 2;
	}

	width_--; // Remove spacing after last character

	// Let's build the palette now.
	for (size_t i = 0; i < header->palsize + 1u; ++i)
	{
		ColRGBA color;
		color.r = *p++;
		color.g = *p++;
		color.b = *p++;
		palette_.setColour(i, color);
	}

	// 0 is transparent, last is border color, the rest of the palette entries should
	// be increasingly bright
	palette_.setTransIndex(0);

	// The picture data follows, using the same RLE as FON1 and IMGZ.
	for (size_t i = 0; i < numchars; ++i)
	{
		// A big font is not necessarily continuous; several characters
		// may be skipped; they are given a width of 0.
		if (chars[i].width)
		{
			size_t numpixels = chars[i].width * height_;
			chars[i].data    = new uint8_t[numpixels];
			uint8_t* d       = chars[i].data;
			uint8_t  code    = 0;
			size_t   length  = 0;

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
					d += length;
					p += length;
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
					d += length;
					numpixels -= length;
				}
			}
		}
	}

	// Now let's assemble all characters together in a single picture.
	if (!width_)
		return false;

	// Clear current data if it exists
	clearData();

	data_.reSize(width_ * height_);
	data_.fillData(0);
	uint8_t* d = data_.data();
	for (size_t i = 0; i < static_cast<unsigned>(height_); ++i)
	{
		d = data_.data() + i * width_;
		for (size_t j = 0; j < numchars; ++j)
		{
			if (chars[j].width)
			{
				memcpy(d, chars[j].data + (i * chars[j].width), chars[j].width);
				d += chars[j].width + 1;
			}
		}
	}

	// Clean up the characters once the big picture is finished
	for (size_t i = 0; i < numchars; ++i)
		delete[] chars[i].data;
	delete[] chars;

	// Now transparency for the mask
	mask_.reSize(width_ * height_);
	mask_.fillData(0xFF);
	for (size_t i = 0; i < static_cast<unsigned>(width_ * height_); ++i)
		if (data_[i] == 0)
			mask_[i] = 0;

	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a byte map font lump and displays it as a picture.
// Specs for the format are here: http://bmf.wz.cz/bmf-format.htm
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
struct BMFChar
{
	uint8_t which;  // 0
	uint8_t width;  // 1
	uint8_t height; // 2
	int8_t  offsx;  // 3
	int8_t  offsy;  // 4
	uint8_t shift;  // 5
	// Rest is not part of the header proper
	const uint8_t* cdata;
	BMFChar() : cdata{ nullptr } { which = width = height = offsx = offsy = shift = 0; }
};
struct BMFFont
{
	uint8_t headr[4]    = { 0, 0, 0, 0 }; //  0
	uint8_t version     = 0;              //  4
	uint8_t lineheight  = 0;              //  5
	int8_t  size_over   = 0;              //  6
	int8_t  size_under  = 0;              //  7
	int8_t  add_space   = 0;              //  8
	int8_t  size_inner  = 0;              //  9
	uint8_t num_colors  = 0;              // 10
	uint8_t top_color   = 0;              // 11
	uint8_t reserved[4] = { 0, 0, 0, 0 }; // 12
	uint8_t pal_size    = 0;              // 16

	// Rest is not part of the header proper
	uint8_t     info_size = 0;
	uint16_t    num_chars = 0;
	const char* info      = nullptr;
	BMFChar*    chars     = nullptr;

	BMFFont(const BMFFont* other) :
		lineheight{ other->lineheight },
		size_over{ other->size_over },
		size_under{ other->size_under },
		add_space{ other->add_space },
		size_inner{ other->size_inner },
		num_colors{ other->num_colors },
		top_color{ other->top_color },
		pal_size{ other->pal_size },
		info_size{ other->info_size },
		num_chars{ other->num_chars }
	{
	}
	~BMFFont() { delete[] chars; }
};
bool SImage::loadBMF(const uint8_t* gfx_data, int size)
{
	if (!gfx_data || size < 24)
		return false;
	const uint8_t* eod = gfx_data + size;

	BMFFont mf(reinterpret_cast<const BMFFont*>(gfx_data));

	// Check for invalid data, we need at least one visible color
	if (mf.pal_size == 0)
		return false;

	// Clean up old data and set up variables
	clearData();
	type_        = Type::PalMask;
	has_palette_ = true;
	format_      = nullptr;

	// Technically each character is its own image, though.
	numimages_ = 1;
	imgindex_  = 0;

	const uint8_t* ofs = gfx_data + 17;

	// Setup palette -- it's a 6-bit palette (63 max) so we have to convert it to 8-bit.
	// Palette index 0 is used as the transparent color and not described at all.
	palette_.setColour(0, ColRGBA(0, 0, 0, 0));
	for (size_t i = 0; i < mf.pal_size; ++i)
	{
		palette_.setColour(
			i + 1,
			ColRGBA(
				(ofs[(i * 3)] << 2) + (ofs[(i * 3)] >> 4),
				(ofs[(i * 3) + 1] << 2) + (ofs[(i * 3)] >> 4),
				(ofs[(i * 3) + 2] << 2) + (ofs[(i * 3) + 2] >> 4),
				255));
	}

	// Move to after the palette and get amount of characters
	ofs += mf.pal_size * 3;
	if (ofs >= eod)
	{
		log::error("BMF aborted: no data after palette");
		return false;
	}
	mf.info_size = ofs[0];
	if (mf.info_size > 0)
	{
		mf.info = reinterpret_cast<const char*>(ofs) + 1;
	}
	ofs += mf.info_size + 1;
	mf.num_chars = memory::readL16(ofs, 0);
	if (mf.num_chars == 0)
		return false;

	// Now we are at the beginning of character data
	ofs += 2;
	if (ofs >= eod)
	{
		log::error("BMF aborted: no data after char size");
		return false;
	}
	// Let's create each character's data and compute the total size
	mf.chars = new BMFChar[mf.num_chars];
	int miny = ofs[4], maxy = ofs[2];
	width_ = ofs[5] + ofs[3];
	for (size_t i = 0; i < mf.num_chars; ++i)
	{
		mf.chars[i].which  = ofs[0];
		mf.chars[i].width  = ofs[1];
		mf.chars[i].height = ofs[2];
		mf.chars[i].offsx  = ofs[3];
		mf.chars[i].offsy  = ofs[4];
		mf.chars[i].shift  = ofs[5];
		mf.chars[i].cdata  = ofs + 6;
		ofs += (6 + (mf.chars[i].width * mf.chars[i].height));
		// Sanity check, some supposedly-valid fonts do not have all the
		// characters they pretend to have (e.g., 274.bmf). Being truncated
		// does not prevent them from being considered valid, so cut them off
		if (ofs >= eod && (i + 1) < mf.num_chars)
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
			width_ += mf.add_space + mf.chars[i].shift;
		}
	}
	height_ = maxy - miny;

	// Create new fully transparent image
	size_t pixela = 0, pixelb = 0, pixels = width_ * height_;
	data_.reSize(pixels);
	mask_.reSize(pixels);
	data_.fillData(0x00);
	mask_.fillData(0x00);

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
					pixela = v * mc->width + u;
					// Destination pixel
					pixelb = (starty + v + mc->offsy) * width_ + startx + u + mc->offsx;
					// Only paint if appropriate
					if ((mc->cdata + pixela < eod) && (mc->cdata + pixela < mf.chars[i + 1].cdata - 6)
						&& mc->cdata[pixela] && pixelb < pixels)
					{
						data_[pixelb] = mc->cdata[pixela];
						mask_[pixelb] = 0xFF;
					}
				}
			}
			startx += (mf.add_space + mc->shift);
		}
	}
	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a monochrome, monospaced font and displays it as a picture.
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::loadFontM(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data || size % 256)
		return false;

	// Setup variables
	offset_x_ = offset_y_ = 0;
	has_palette_          = false;
	type_                 = Type::PalMask;
	format_               = nullptr;

	size_t charwidth  = 8;
	size_t charheight = size >> 8;
	width_            = charwidth;
	height_           = charheight << 8;

	if (width_ * height_ != size * 8)
		return false;

	// reset data
	clearData();
	data_.reSize(width_ * height_);
	data_.fillData(0xFF);
	mask_.reSize(width_ * height_);
	mask_.fillData(0x00);

	// Technically each character is its own image, though.
	numimages_ = 1;
	imgindex_  = 0;

	// Each pixel is described as a single bit, either on or off
	for (size_t i = 0; i < static_cast<unsigned>(size); ++i)
	{
		for (size_t p = 0; p < 8; ++p)
			mask_[(i * 8) + p] = ((gfx_data[i] >> (7 - p)) & 1) * 255;
	}
	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a Wolf3D-format font.
// The format used is simple, basically like the Doom alpha HUFONT, except not
// in the same order:
// Offset | Length | Type | Name
//  0x000 |      2 | ui16 | image height (one value for all chars)
//  0x002 |  256*2 | ui16 | characteroffset (one value per char)
//  0x202 |  256*1 | ui08 | characterwidth (one value per char)
//  0x302 |    x*1 | ui08 | pixel color index (one value per pixel)
// So, total size - 302 % value @ 0x00 must be null.
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::loadWolfFont(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data)
		return false;

	if (size <= 0x302)
		return false;

	offset_x_ = offset_y_ = 0;
	height_               = memory::readL16(gfx_data, 0);

	size_t datasize = size - 0x302;
	if (datasize % height_)
		return false;

	width_ = datasize / height_;

	clearData();
	has_palette_ = false;
	type_        = Type::PalMask;
	format_      = nullptr;

	// Technically each character is its own image, though.
	numimages_ = 1;
	imgindex_  = 0;

	// Create new picture and mask
	const uint8_t* r = gfx_data + 0x302;
	data_.reSize(datasize);
	mask_.reSize(datasize);
	mask_.fillData(0xFF);
	data_.fillData(*r);

	size_t p = 0; // Previous width
	size_t w = 0; // This character's width
	// Run through each character
	for (size_t c = 0; c < 256; ++c)
	{
		p += w;                  // Add previous character width to total
		w = gfx_data[c + 0x202]; // Get this character's width
		if (!w)
			continue;
		size_t o = memory::readL16(gfx_data, ((c << 1) + 2));
		for (size_t i = 0; i < w * height_; ++i)
		{
			// Compute source and destination offsets
			size_t s = o + i;
			size_t d = ((i / w) * width_) + (i % w) + p;
			data_[d] = gfx_data[s];
			// Index 0 is transparent
			if (data_[d] == 0)
				mask_[d] = 0;
		}
	}
	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a Jedi Engine-format bitmap font.
// The header tells the height and which are the first and last characters
// described. Then the character data consists of a single byte of data for the
// width of that character, followed by a list of columns. The characters are
// listed in order.
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::loadJediFNT(const uint8_t* gfx_data, int size)
{
	// Check data
	if (!gfx_data)
		return false;

	if (size <= 35)
		return false;

	// The character data is presented as a list of columns
	// preceded by a byte for
	offset_x_ = offset_y_ = 0;

	// Since the format is column-major, we'll use our usual cheat of
	// inverting height and width to build the picture, then rotating it.
	width_ = gfx_data[4];

	// First and last characters
	uint8_t firstc = gfx_data[8];
	uint8_t lastc  = gfx_data[9];
	uint8_t numchr = 1 + lastc - firstc;

	// Go through each character to compute the total width (pre-rotation height)
	height_   = 0;
	size_t wo = 32; // Offset to width of next character
	for (uint8_t i = 0; i < numchr; ++i)
	{
		height_ += gfx_data[wo];
		wo += 1 + (width_ * gfx_data[wo]);
	}

	clearData();
	has_palette_ = false;
	type_        = Type::PalMask;
	format_      = nullptr;

	// Technically each character is its own image, though.
	numimages_ = 1;
	imgindex_  = 0;

	// Create new picture and mask
	data_.reSize(width_ * height_);
	mask_.reSize(width_ * height_);
	mask_.fillData(0xFF);

	// Run through each character and add the pixel data
	wo         = 32;
	size_t col = 0;
	for (uint8_t i = 0; i < numchr; ++i)
	{
		uint8_t numcols = gfx_data[wo++];
		memcpy(data_.data() + (col * width_), gfx_data + wo, numcols * width_);
		col += numcols;
		wo += width_ * numcols;
	}

	// Make index 0 transparent
	for (int i = 0; i < width_ * height_; ++i)
		if (data_[i] == 0)
			mask_[i] = 0;

	// Convert from column-major to row-major
	rotate(270);

	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a Jedi Engine-format monochrome font.
// Contrarily to what the DF specs claim, the first two int16 values are not the
// first and last characters as in the FNT format; instead, they are the first
// character and the number of characters! They're also mistaken about character
// width.
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::loadJediFONT(const uint8_t* gfx_data, int size)
{
	// Check data
	if (size < 16)
		return false;

	int numchr = memory::readL16(gfx_data, 2);

	// Setup variables
	offset_x_ = offset_y_ = 0;
	height_               = memory::readL16(gfx_data, 6) * numchr;
	width_                = memory::readL16(gfx_data, 4);
	has_palette_          = false;
	type_                 = Type::PalMask;
	format_               = nullptr;

	// reset data
	clearData();
	data_.reSize(width_ * height_);
	data_.fillData(0xFF);
	mask_.reSize(width_ * height_);
	mask_.fillData(0x00);

	// Technically each character is its own image, though.
	numimages_ = 1;
	imgindex_  = 0;

	// We don't care about the character widths since
	// technically it's always eight anyway. The offset
	// to graphic data corresponds to 12 (header size)
	// plus one byte by character for width.
	int o   = 12 + numchr;
	int bpc = width_ / 8;

	// Each pixel is described as a single bit, either on or off
	for (int i = 0; i < height_; ++i)
	{
		for (int p = 0; p < width_; ++p)
		{
			switch (bpc)
			{
			case 1: mask_[(i * width_) + p] = ((gfx_data[o + i] >> (7 - p)) & 1) * 255; break;
			case 2: mask_[(i * width_) + p] = ((memory::readB16(gfx_data, o + (i * 2)) >> (15 - p)) & 1) * 255; break;
			case 3: mask_[(i * width_) + p] = ((memory::readB24(gfx_data, o + (i * 3)) >> (23 - p)) & 1) * 255; break;
			case 4: mask_[(i * width_) + p] = ((memory::readB32(gfx_data, o + (i * 4)) >> (31 - p)) & 1) * 255; break;
			default:
				clearData();
				global::error = "Jedi FONT: Weird word width";
				return false;
			}
		}
	}
	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a Jaguar Doom sprite. This needs manual handling because the data is
// split in two separate lumps, one with the header and the other with raw pixel
// data. So we need to have access to both.
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::loadJaguarSprite(const uint8_t* header, int hdr_size, const uint8_t* gfx_data, int size)
{
	if (header == nullptr || gfx_data == nullptr || hdr_size < 16 || size == 0)
	{
		global::error = "Invalid Jaguar sprite";
		return false;
	}

	// Setup variables
	int16_t ofsx, ofsy;
	width_       = memory::readB16(header, 0);
	height_      = memory::readB16(header, 2);
	ofsx         = memory::readB16(header, 4);
	ofsy         = memory::readB16(header, 6);
	offset_x_    = ofsx;
	offset_y_    = ofsy;
	has_palette_ = false;
	type_        = Type::PalMask;
	format_      = nullptr;
	numimages_   = 1;
	imgindex_    = 0;

	// reset data
	clearData();
	data_.reSize(width_ * height_);
	data_.fillData(0x00);
	mask_.reSize(width_ * height_);
	mask_.fillData(0x00);

	// Read column offsets
	if (hdr_size < (8 + (width_ * 6)))
	{
		global::error = fmt::format(
			"Invalid Jaguar sprite: header too small ({}) for column offsets ({})", hdr_size, (8 + (width_ * 6)));
		return false;
	}
	vector<uint16_t> col_offsets(width_);
	for (int w = 0; w < width_; ++w)
	{
		col_offsets[w] = memory::readB16(header, 8 + 2 * w);
	}
	if (hdr_size < (4 + col_offsets[width_ - 1]))
	{
		global::error = fmt::format(
			"Invalid Jaguar sprite: header too small ({}) for post offsets ({})",
			hdr_size,
			4 + col_offsets[width_ - 1]);
		return false;
	}

	// Okay, so it's finally time to read some pixel data
	for (int w = 0; w < width_; ++w)
	{
		int top;
		int post_p = col_offsets[w];
		// Process all posts in the column
		while ((top = header[post_p]) != 0xFF)
		{
			int len     = header[post_p + 1];
			int pixel_p = memory::readB16(header, post_p + 2);
			if (pixel_p + len > size)
			{
				global::error = fmt::format(
					"Invalid Jaguar sprite: body too small ({}) for pixel data ({})", size, pixel_p + len);
				return false;
			}
			// Copy pixels
			for (int p = 0; p < len; ++p)
			{
				size_t pos = w + width_ * (top + p);
				data_[pos] = gfx_data[pixel_p + p];
				mask_[pos] = 0xFF;
			}
			post_p += 4;
		}
	}

	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads a Jaguar Doom texture. This needs manual handling because the
// dimensions are contained in the TEXTURE1 lump instead.
// Returns false if the image data was invalid, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::loadJaguarTexture(const uint8_t* gfx_data, int size, int i_width, int i_height)
{
	// Check data
	if (i_width * i_height == 0 || size < i_width * i_height + 320)
	{
		global::error = fmt::format("Size is {}, expected {}", size, i_width * i_height + 320);
		return false;
	}

	// Setup variables
	offset_x_ = offset_y_ = 0;
	width_                = i_height; // Format is column-major
	height_               = i_width;  // We'll rotate them afterwards
	has_palette_          = false;
	type_                 = Type::PalMask;
	format_               = nullptr;
	numimages_            = 1;
	imgindex_             = 0;

	// reset data
	clearData();
	data_.reSize(width_ * height_);
	memcpy(data_.data(), gfx_data, width_ * height_);
	mask_.reSize(width_ * height_);
	mask_.fillData(0xFF);

	// rotate and mirror image
	rotate(90);
	mirror(false);

	// Announce change and return success
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Loads an SVG image, sized to [width x height].
// Returns false if the SVG data was invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::loadSVG(const string& svg_text, int width, int height)
{
	// Load SVG
	const auto svg = lunasvg::Document::loadFromData(svg_text);
	if (!svg)
		return false;

	// Render SVG
	const auto bmp = svg->renderToBitmap(width, height);

	return setImageData(bmp.data(), bmp.width() * bmp.height() * 4, bmp.width(), bmp.height(), Type::RGBA);
}
