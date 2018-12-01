
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    SImage.cpp
// Description: SImage class - Encapsulates a paletted or 32bit image. Handles
//              loading/saving different formats, palette conversions, offsets,
//              and a bunch of other stuff
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
#include "Graphics/Translation.h"
#include "SIFormat.h"
#include "Utility/MathStuff.h"

#undef BOOL


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Float, col_greyscale_r)
EXTERN_CVAR(Float, col_greyscale_g)
EXTERN_CVAR(Float, col_greyscale_b)


// -----------------------------------------------------------------------------
//
// SImage Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SImage class constructor
// -----------------------------------------------------------------------------
SImage::SImage(SIType type)
{
	width_       = 0;
	height_      = 0;
	data_        = nullptr;
	mask_        = nullptr;
	offset_x_    = 0;
	offset_y_    = 0;
	format_      = nullptr;
	this->type_  = type;
	numimages_   = 1;
	imgindex_    = 0;
	has_palette_ = false;
}

// -----------------------------------------------------------------------------
// SImage class destructor
// -----------------------------------------------------------------------------
SImage::~SImage()
{
	clearData();
}

// -----------------------------------------------------------------------------
// Loads the image as RGBA data into [mc].
// Returns false if image is invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::putRGBAData(MemChunk& mc, Palette* pal)
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgba data
	mc.reSize(width_ * height_ * 4, false);

	// If data is already in RGBA format just return a copy
	if (type_ == RGBA)
	{
		mc.importMem(data_, width_ * height_ * 4);
		return true;
	}

	// Convert paletted
	else if (type_ == PALMASK)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		uint8_t rgba[4];
		for (int a = 0; a < width_ * height_; a++)
		{
			// Get colour
			rgba_t col = pal->colour(data_[a]);

			// Set alpha
			if (mask_)
				col.a = mask_[a];
			else
				col.a = 255;

			col.write(rgba);   // Write colour to array
			mc.write(rgba, 4); // Write array to MemChunk
		}

		return true;
	}

	// Convert if alpha map
	else if (type_ == ALPHAMAP)
	{
		uint8_t rgba[4];
		rgba_t  col;
		for (int a = 0; a < width_ * height_; a++)
		{
			// Get pixel as colour (greyscale)
			col.set(data_[a], data_[a], data_[a], data_[a]);

			col.write(rgba);   // Write colour to array
			mc.write(rgba, 4); // Write array to MemChunk
		}
	}

	return false; // Invalid image type
}

// -----------------------------------------------------------------------------
// Loads the image as RGB data into [mc].
// Returns false if image is invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::putRGBData(MemChunk& mc, Palette* pal)
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgb data
	mc.reSize(width_ * height_ * 3, false);

	if (type_ == RGBA)
	{
		// RGBA format, remove alpha information
		for (int a = 0; a < width_ * height_ * 4; a += 4)
			mc.write(&data_[a], 3);

		return true;
	}
	else if (type_ == PALMASK)
	{
		// Paletted, convert to RGB

		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Build RGB data
		uint8_t rgba[4];
		for (int a = 0; a < width_ * height_; a++)
		{
			pal->colour(data_[a]).write(rgba);
			mc.write(rgba, 3);
		}

		return true;
	}
	else if (type_ == ALPHAMAP)
	{
		// Alpha map, convert to RGB

		uint8_t rgba[4];
		rgba_t  col;
		for (int a = 0; a < width_ * height_; a++)
		{
			// Get pixel as colour (greyscale)
			col.set(data_[a], data_[a], data_[a], data_[a]);

			col.write(rgba);   // Write colour to array
			mc.write(rgba, 3); // Write array to MemChunk
		}
	}

	return false; // Invalid image type
}

// -----------------------------------------------------------------------------
// Loads the image as index data into [mc].
// Returns false if image is invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::putIndexedData(MemChunk& mc)
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgb data
	mc.reSize(width_ * height_, false);

	// Cannot do this for trucolor graphics.
	if (type_ == RGBA)
		return false;

	else if (type_ == PALMASK)
	{
		mc.write(data_, width_ * height_);
		return true;
	}

	else if (type_ == ALPHAMAP)
	{
		mc.write(data_, width_ * height_);
		return true;
	}

	return false; // Invalid image type
}

// -----------------------------------------------------------------------------
// Returns the number of bytes per image row
// -----------------------------------------------------------------------------
unsigned SImage::stride()
{
	if (type_ == RGBA)
		return width_ * 4;
	else
		return width_;
}

// -----------------------------------------------------------------------------
// Returns the number of bytes per image pixel
// -----------------------------------------------------------------------------
uint8_t SImage::bpp()
{
	if (type_ == RGBA)
		return 4;
	else
		return 1;
}

// -----------------------------------------------------------------------------
// Returns an info struct with image information
// -----------------------------------------------------------------------------
SImage::info_t SImage::info()
{
	info_t inf;

	// Set values
	inf.width     = width_;
	inf.height    = height_;
	inf.colformat = type_;
	if (format_)
		inf.format = format_->id();
	inf.numimages   = numimages_;
	inf.imgindex    = imgindex_;
	inf.offset_x    = offset_x_;
	inf.offset_y    = offset_y_;
	inf.has_palette = has_palette_;

	return inf
	;
}

// -----------------------------------------------------------------------------
// Returns the colour of the pixel at [x,y] in the image, or black+invisible if
// out of range
// -----------------------------------------------------------------------------
rgba_t SImage::pixelAt(unsigned x, unsigned y, Palette* pal)
{
	// Get pixel index
	unsigned index = y * stride() + x * bpp();

	// Check it
	if (index >= unsigned(width_ * height_ * bpp()))
		return rgba_t(0, 0, 0, 0);

	// Get colour at pixel
	rgba_t col;
	if (type_ == RGBA)
	{
		col.r = data_[index];
		col.g = data_[index + 1];
		col.b = data_[index + 2];
		col.a = data_[index + 3];
	}
	else if (type_ == PALMASK)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		col.set(pal->colour(data_[index]));
		if (mask_)
			col.a = mask_[index];
	}
	else if (type_ == ALPHAMAP)
	{
		col.r = data_[index];
		col.g = data_[index];
		col.b = data_[index];
		col.a = data_[index];
	}

	return col;
}

// -----------------------------------------------------------------------------
// Returns the palette index of the pixel at [x,y] in the image, or 0 if the
// position is out of bounds or the image is not paletted
// -----------------------------------------------------------------------------
uint8_t SImage::pixelIndexAt(unsigned x, unsigned y)
{
	// Get pixel index
	unsigned index = y * stride() + x * bpp();

	// Check it
	if (index >= unsigned(width_ * height_ * bpp()) || type_ == RGBA)
		return 0;

	return data_[index];
}

// -----------------------------------------------------------------------------
// Changes the image X offset
// -----------------------------------------------------------------------------
void SImage::setXOffset(int offset)
{
	// Change the X offset
	offset_x_ = offset;

	// Announce change
	announce("offsets_changed");
}

// -----------------------------------------------------------------------------
// Changes the image Y offset
// -----------------------------------------------------------------------------
void SImage::setYOffset(int offset)
{
	// Change the Y offset
	offset_y_ = offset;

	// Announce change
	announce("offsets_changed");
}

// -----------------------------------------------------------------------------
// Deletes/clears any existing image data
// -----------------------------------------------------------------------------
void SImage::clearData(bool clear_mask)
{
	// Delete data if it exists
	if (data_)
	{
		delete[] data_;
		data_ = nullptr;
	}
	// Delete mask if it exists
	if (mask_ && clear_mask)
	{
		delete[] mask_;
		mask_ = nullptr;
	}
}

// -----------------------------------------------------------------------------
// Creates an empty image
// -----------------------------------------------------------------------------
void SImage::create(int width, int height, SIType type, Palette* pal, int index, int numimages)
{
	// Check valid width/height
	if (width < 0 || height < 0)
		return;

	// Clear current image
	clearData();

	// Create blank image
	if (type == PALMASK)
	{
		data_ = new uint8_t[width * height];
		memset(data_, 0, width * height);
		mask_ = new uint8_t[width * height];
		memset(mask_, 0, width * height);
	}
	else if (type == RGBA)
	{
		data_ = new uint8_t[width * height * 4];
		memset(data_, 0, width * height * 4);
	}
	else if (type == ALPHAMAP)
	{
		data_ = new uint8_t[width * height];
		memset(data_, 0, width * height);
	}

	// Set image properties
	this->width_     = width;
	this->height_    = height;
	this->type_      = type;
	this->offset_x_  = 0;
	this->offset_y_  = 0;
	this->numimages_ = numimages;
	this->imgindex_  = index;
	if (pal)
	{
		palette_.copyPalette(pal);
		has_palette_ = true;
	}
	else
		has_palette_ = false;
}

// -----------------------------------------------------------------------------
// Creates an empty image, initialising with properties from [info]
// -----------------------------------------------------------------------------
void SImage::create(SImage::info_t info, Palette* pal)
{
	// Normal creation
	create(info.width, info.height, (SIType)info.colformat, pal, info.imgindex, info.numimages);

	// Set other info
	offset_x_    = info.offset_x;
	offset_y_    = info.offset_y;
	has_palette_ = info.has_palette;
}

// -----------------------------------------------------------------------------
// Deletes/clears any existing image data, and resets the image to zero-sized
// -----------------------------------------------------------------------------
void SImage::clear()
{
	// Clear image data
	clearData(true);

	// Reset variables
	width_    = 0;
	height_   = 0;
	offset_x_ = 0;
	offset_y_ = 0;

	// Announce change
	announce("image_changed");
}

// -----------------------------------------------------------------------------
// 'Fills' the alpha channel or mask with the given <alpha> value
// -----------------------------------------------------------------------------
void SImage::fillAlpha(uint8_t alpha)
{
	// Check image is valid
	if (!isValid())
		return;

	if (type_ == RGBA)
	{
		// RGBA format, set alpha values to given one
		for (int a = 3; a < width_ * height_ * 4; a += 4)
			data_[a] = alpha;
	}
	else if (type_ == PALMASK)
	{
		// Paletted masked format, fill mask with alpha value
		if (!mask_)
			mask_ = new uint8_t[width_ * height_];

		memset(mask_, alpha, width_ * height_);
	}
	else if (type_ == ALPHAMAP)
		memset(data_, alpha, width_ * height_);

	// Announce change
	announce("image_changed");
}

// -----------------------------------------------------------------------------
// Returns the first unused palette index, or -1 if the image is not paletted or
// uses all 256 colours
// -----------------------------------------------------------------------------
short SImage::findUnusedColour()
{
	// Only for paletted images
	if (type_ != PALMASK)
		return -1;

	// Init used colours list
	uint8_t used[256];
	memset(used, 0, 256);

	// Go through image data and mark used colours
	for (int a = 0; a < width_ * height_; a++)
		used[data_[a]] = 1;

	// Find first unused
	for (int a = 0; a < 256; a++)
	{
		if (!used[a])
			return a;
	}

	// No unused colours found
	return -1;
}

// -----------------------------------------------------------------------------
// Returns the number of unique colors in a paletted image
// -----------------------------------------------------------------------------
size_t SImage::countColours()
{
	// If the picture is not paletted, return 0.
	if (type_ != PALMASK)
		return 0;

	bool* usedcolours = new bool[256];
	memset(usedcolours, 0, 256);
	size_t used = 0;

	for (int a = 0; a < width_ * height_; ++a)
	{
		usedcolours[data_[a]] = true;
	}
	for (size_t b = 0; b < 256; ++b)
	{
		if (usedcolours[b])
			++used;
	}

	delete[] usedcolours;
	return used;
}

// -----------------------------------------------------------------------------
// Shifts all the used colours to the beginning of the palette
// -----------------------------------------------------------------------------
void SImage::shrinkPalette(Palette* pal)
{
	// If the picture is not paletted, stop.
	if (type_ != PALMASK)
		return;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Init variables
	Palette newpal;
	bool*   usedcolours = new bool[256];
	int*    remap       = new int[256];
	memset(usedcolours, 0, 256);
	size_t used = 0;

	// Count all color indices actually used on the picture
	for (int a = 0; a < width_ * height_; ++a)
	{
		usedcolours[data_[a]] = true;
	}

	// Create palette remapping information
	for (size_t b = 0; b < 256; ++b)
	{
		if (usedcolours[b])
		{
			newpal.setColour(used, pal->colour(b));
			remap[b] = used;
			++used;
		}
	}

	// Remap image to new palette indices
	for (int c = 0; c < width_ * height_; ++c)
	{
		data_[c] = remap[data_[c]];
	}
	pal->copyPalette(&newpal);

	// Cleanup
	delete[] usedcolours;
	delete[] remap;
}

// -----------------------------------------------------------------------------
// Copies all data and properties from [image]
// -----------------------------------------------------------------------------
bool SImage::copyImage(SImage* image)
{
	// Check image was given
	if (!image)
		return false;

	// Clear current data
	clearData();

	// Copy image properties
	width_  = image->width_;
	height_ = image->height_;
	type_   = image->type_;
	palette_.copyPalette(&image->palette_);
	has_palette_ = image->has_palette_;
	offset_x_    = image->offset_x_;
	offset_y_    = image->offset_y_;
	imgindex_    = image->imgindex_;
	numimages_   = image->numimages_;

	// Copy image data
	if (image->data_)
	{
		data_ = new uint8_t[width_ * height_ * bpp()];
		memcpy(data_, image->data_, width_ * height_ * bpp());
	}
	if (image->mask_)
	{
		mask_ = new uint8_t[width_ * height_];
		memcpy(mask_, image->mask_, width_ * height_);
	}

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Detects the format of [data] and, if it's a valid image format, loads it into
// this image
// -----------------------------------------------------------------------------
bool SImage::open(MemChunk& data, int index, string type_hint)
{
	// Check with type hint format first
	if (!type_hint.IsEmpty())
	{
		SIFormat* format = SIFormat::getFormat(type_hint);
		if (format != SIFormat::unknownFormat() && format->isThisFormat(data))
			return format->loadImage(*this, data, index);
	}

	// No type hint given or didn't match, autodetect format with SIFormat system instead
	return SIFormat::determineFormat(data)->loadImage(*this, data, index);
}

// -----------------------------------------------------------------------------
// Converts the image to 32bpp (RGBA).
// Returns false if the image was already 32bpp, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::convertRGBA(Palette* pal)
{
	// If it's already 32bpp do nothing
	if (type_ == RGBA)
		return false;

	// Get 32bit data
	MemChunk rgba_data;
	putRGBAData(rgba_data, pal);

	// Clear current data
	clearData(true);

	// Copy it
	data_ = new uint8_t[width_ * height_ * 4];
	memcpy(data_, rgba_data.data(), width_ * height_ * 4);

	// Set new type & update variables
	type_        = RGBA;
	has_palette_ = false;

	// Announce change
	announce("image_changed");

	// Done
	return true;
}

// -----------------------------------------------------------------------------
// Converts the image to paletted + mask.
// [pal_target] is the new palette to convert to (the image's palette will also
// be set to this).
// [pal_current] will be used as the image's current palette if it doesn't
// already have one
// -----------------------------------------------------------------------------
bool SImage::convertPaletted(Palette* pal_target, Palette* pal_current)
{
	// Check image/parameters are valid
	if (!isValid() || !pal_target)
		return false;

	// Get image data as RGBA
	MemChunk rgba_data;
	putRGBAData(rgba_data, pal_current);

	// Create mask from alpha info (if converting from RGBA)
	if (type_ == RGBA || type_ == ALPHAMAP)
	{
		// Clear current mask
		if (mask_)
			delete[] mask_;

		// Init mask
		mask_ = new uint8_t[width_ * height_];

		// Get values from alpha channel
		int c = 0;
		for (int a = 3; a < width_ * height_ * 4; a += 4)
			mask_[c++] = rgba_data[a];
	}

	// Load given palette
	palette_.copyPalette(pal_target);

	// Clear current image data (but not mask)
	clearData(false);

	// Do conversion
	data_      = new uint8_t[width_ * height_];
	unsigned i = 0;
	rgba_t   col;
	for (int a = 0; a < width_ * height_; a++)
	{
		col.r    = rgba_data[i++];
		col.g    = rgba_data[i++];
		col.b    = rgba_data[i++];
		data_[a] = palette_.nearestColour(col);
		i++; // Skip alpha
	}

	// Update variables
	type_        = PALMASK;
	has_palette_ = true;

	// Announce change
	announce("image_changed");

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Converts the image to an alpha map, generating alpha values from either pixel
// brightness or existing alpha, depending on the value of [alpha_source]
// -----------------------------------------------------------------------------
bool SImage::convertAlphaMap(int alpha_source, Palette* pal)
{
	// Get RGBA data
	MemChunk rgba;
	putRGBAData(rgba, pal);

	// Recreate image
	create(width_, height_, ALPHAMAP);

	// Generate alpha mask
	unsigned c = 0;
	for (int a = 0; a < width_ * height_; a++)
	{
		// Determine alpha for this pixel
		uint8_t alpha = 0;
		if (alpha_source == BRIGHTNESS) // Pixel brightness
			alpha = double(rgba[c]) * 0.3 + double(rgba[c + 1]) * 0.59 + double(rgba[c + 2]) * 0.11;
		else // Existing alpha
			alpha = rgba[c + 3];

		// Set pixel
		data_[a] = alpha;

		// Next RGBA pixel
		c += 4;
	}

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that pixels that match [colour] are fully
// transparent, and all other pixels fully opaque
// -----------------------------------------------------------------------------
bool SImage::maskFromColour(rgba_t colour, Palette* pal)
{
	if (type_ == PALMASK)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Palette+Mask type, go through the mask
		for (int a = 0; a < width_ * height_; a++)
		{
			if (pal->colour(data_[a]).equals(colour))
				mask_[a] = 0;
			else
				mask_[a] = 255;
		}
	}
	else if (type_ == RGBA)
	{
		// RGBA type, go through alpha channel
		uint32_t c = 0;
		for (int a = 0; a < width_ * height_; a++)
		{
			rgba_t pix_col(data_[c], data_[c + 1], data_[c + 2], 255);

			if (pix_col.equals(colour))
				data_[c + 3] = 0;
			else
				data_[c + 3] = 255;

			// Skip to next pixel
			c += 4;
		}
	}
	else
		return false;

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that each pixel's transparency matches its
// brigntness level (where black is fully transparent)
// -----------------------------------------------------------------------------
bool SImage::maskFromBrightness(Palette* pal)
{
	if (type_ == PALMASK)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Go through pixel data
		for (int a = 0; a < width_ * height_; a++)
		{
			// Set mask from pixel colour brightness value
			rgba_t col = pal->colour(data_[a]);
			mask_[a]   = ((double)col.r * 0.3) + ((double)col.g * 0.59) + ((double)col.b * 0.11);
		}
	}
	else if (type_ == RGBA)
	{
		// Go through pixel data
		unsigned c = 0;
		for (int a = 0; a < width_ * height_; a++)
		{
			// Set alpha from pixel colour brightness value
			data_[c + 3] = (double)data_[c] * 0.3 + (double)data_[c + 1] * 0.59 + (double)data_[c + 2] * 0.11;
			// Skip alpha
			c += 4;
		}
	}
	// ALPHAMASK type is already a brightness mask

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that any pixel alpha level currently
// greater than [threshold] is fully opaque, and all other pixels fully
// transparent
// -----------------------------------------------------------------------------
bool SImage::cutoffMask(uint8_t threshold)
{
	if (type_ == PALMASK)
	{
		// Paletted, go through mask
		for (int a = 0; a < width_ * height_; a++)
		{
			if (mask_[a] > threshold)
				mask_[a] = 255;
			else
				mask_[a] = 0;
		}
	}
	else if (type_ == RGBA)
	{
		// RGBA format, go through alpha channel
		uint32_t c = 0;
		for (int a = 3; a < width_ * height_ * 4; a += 4)
		{
			if (data_[a] > threshold)
				data_[a] = 255;
			else
				data_[a] = 0;
		}
	}
	else if (type_ == ALPHAMAP)
	{
		// Alpha map, go through pixels
		for (int a = 0; a < width_ * height_; a++)
		{
			if (data_[a] > threshold)
				data_[a] = 255;
			else
				data_[a] = 0;
		}
	}
	else
		return false;

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Sets the pixel at [x],[y] to [colour].
// Returns false if the position is out of range, true otherwise
// -----------------------------------------------------------------------------
bool SImage::setPixel(int x, int y, rgba_t colour, Palette* pal)
{
	// Check position
	if (x < 0 || x >= width_ || y < 0 || y >= height_)
		return false;

	// Set the pixel
	if (type_ == RGBA)
		colour.write(data_ + (y * (width_ * 4) + (x * 4)));
	else if (type_ == PALMASK)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Get color index to use (the rgba_t's index if defined, nearest colour otherwise)
		uint8_t index = (colour.index == -1) ? pal->nearestColour(colour) : colour.index;

		data_[y * width_ + x] = index;
		if (mask_)
			mask_[y * width_ + x] = colour.a;
	}
	else if (type_ == ALPHAMAP)
	{
		// Just use colour alpha
		data_[y * width_ + x] = colour.a;
	}

	// Announce
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Sets the pixel at [x],[y] to the palette colour at [pal_index], and the
// transparency of the pixel to [alpha] (if possible).
// Returns false if the position is out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool SImage::setPixel(int x, int y, uint8_t pal_index, uint8_t alpha)
{
	// Check position
	if (x < 0 || x >= width_ || y < 0 || y >= height_)
		return false;

	// RGBA (use palette colour, probably don't want this, but it's here anyway :P)
	if (type_ == RGBA)
	{
		// Set the pixel
		rgba_t col = palette_.colour(pal_index);
		col.a      = alpha;
		col.write(data_ + (y * (width_ * 4) + (x * 4)));
	}

	// Paletted
	else if (type_ == PALMASK)
	{
		// Set the pixel
		data_[y * width_ + x] = pal_index;
		if (mask_)
			mask_[y * width_ + x] = alpha;
	}

	// Alpha map
	else if (type_ == ALPHAMAP)
	{
		// Set the pixel
		data_[y * width_ + x] = alpha;
	}

	// Invalid type
	else
		return false;

	// Announce
	announce("image_changed");

	// Invalid type
	return true;
}

// -----------------------------------------------------------------------------
// Change the width of the image to the given value, adjusting the height
// automatically
// -----------------------------------------------------------------------------
void SImage::setWidth(int w)
{
	int numpixels = width_ * height_;
	if ((numpixels > w) && ((numpixels % w) == 0))
	{
		width_  = w;
		height_ = numpixels / w;
	}
}

// -----------------------------------------------------------------------------
// Change the height of the image to the given value, adjusting the height
// automatically
// -----------------------------------------------------------------------------
void SImage::setHeight(int h)
{
	int numpixels = width_ * height_;
	if ((numpixels > h) && ((numpixels % h) == 0))
	{
		height_ = h;
		width_  = numpixels / h;
	}
}

// -----------------------------------------------------------------------------
// Rotates the image with an angle of 90°, 180° or 270°.
// Why not use FreeImage_Rotate instead? So as not to bother converting to and
// fro a FIBITMAP...
// -----------------------------------------------------------------------------
bool SImage::rotate(int angle)
{
	if (!data_)
		return false;

	if (angle == 0)
		return true; // Nothing to do
	if (angle % 90)
		return false; // Unsupported angle
	while (angle < 0)
		angle += 360;
	angle %= 360;
	angle = 360 - angle;

	uint8_t *nd, *nm;
	int      nw, nh;

	// Compute new dimensions and numbers of pixels and bytes
	if (angle % 180)
	{
		nw = height_;
		nh = width_;
	}
	else
	{
		nw = width_;
		nh = height_;
	}
	int numpixels = width_ * height_;
	int numbpp    = 0;
	if (type_ == PALMASK)
		numbpp = 1;
	else if (type_ == RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	nd = new uint8_t[numpixels * numbpp];
	if (mask_)
		nm = new uint8_t[numpixels * numbpp];
	else
		nm = nullptr;

	// Remapping loop
	int i, j, k;
	for (i = 0; i < numpixels; ++i)
	{
		switch (angle)
		{
			// Urgh maths...
		case 90: j = (((nh - 1) - (i % width_)) * nw) + (i / width_); break;
		case 180: j = (numpixels - 1) - i; break;
		case 270: j = ((i % width_) * nw) + ((nw - 1) - (i / width_)); break;
		default:
			delete[] nd;
			if (mask_)
				delete[] nm;
			return false;
		}
		if (j >= numpixels)
		{
			LOG_MESSAGE(1, "Pixel %i remapped to %i, how did this even happen?", i, j);
			delete[] nd;
			if (mask_)
				delete[] nm;
			return false;
		}
		for (k = 0; k < numbpp; ++k)
		{
			nd[(j * numbpp) + k] = data_[(i * numbpp) + k];
			if (mask_)
				nm[(j * numbpp) + k] = mask_[(i * numbpp) + k];
		}
	}

	// It worked, yay
	clearData();
	data_   = nd;
	mask_   = nm;
	width_  = nw;
	height_ = nh;

	// Announce change
	announce("image_changed");
	return true;
}

// -----------------------------------------------------------------------------
// Mirrors the image horizontally or vertically.
// -----------------------------------------------------------------------------
bool SImage::mirror(bool vertical)
{
	uint8_t *nd, *nm;

	// Compute numbers of pixels and bytes
	int numpixels = width_ * height_;
	int numbpp    = 0;
	if (type_ == PALMASK)
		numbpp = 1;
	else if (type_ == RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	nd = new uint8_t[numpixels * numbpp];
	if (mask_)
		nm = new uint8_t[numpixels * numbpp];
	else
		nm = nullptr;

	// Remapping loop
	int i, j, k;
	for (i = 0; i < numpixels; ++i)
	{
		if (vertical)
			j = (((height_ - 1) - (i / width_)) * width_) + (i % width_);
		else // horizontal
			j = ((i / width_) * width_) + ((width_ - 1) - (i % width_));
		if (j >= numpixels)
		{
			LOG_MESSAGE(1, "Pixel %i remapped to %i, how did this even happen?", i, j);
			delete[] nd;
			if (mask_)
				delete[] nm;
			return false;
		}
		for (k = 0; k < numbpp; ++k)
		{
			nd[(j * numbpp) + k] = data_[(i * numbpp) + k];
			if (mask_)
				nm[(j * numbpp) + k] = mask_[(i * numbpp) + k];
		}
	}

	// It worked, yay
	clearData();
	data_ = nd;
	mask_ = nm;

	// Announce change
	announce("image_changed");
	return true;
}

// -----------------------------------------------------------------------------
// Converts from column-major to row-major
// -----------------------------------------------------------------------------
bool SImage::imgconv()
{
	int oldwidth = width_;
	width_       = height_;
	height_      = oldwidth;
	rotate(90);
	mirror(true);
	return true;
}

// -----------------------------------------------------------------------------
// Crops a section of the image.
// -----------------------------------------------------------------------------
bool SImage::crop(long x1, long y1, long x2, long y2)
{
	if (x2 == 0 || x2 > width_)
		x2 = width_;
	if (y2 == 0 || y2 > height_)
		y2 = height_;

	// No need to bother with incorrect values
	if (x2 <= x1 || y2 <= y1 || x1 > width_ || y1 > height_)
		return false;

	uint8_t *nd, *nm;
	size_t   nw, nh;
	nw = x2 - x1;
	nh = y2 - y1;

	// Compute numbers of pixels and bytes
	int numpixels = nw * nh;
	int numbpp    = 0;
	if (type_ == PALMASK || type_ == ALPHAMAP)
		numbpp = 1;
	else if (type_ == RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	nd = new uint8_t[numpixels * numbpp];
	if (mask_)
		nm = new uint8_t[numpixels * numbpp];
	else
		nm = nullptr;

	// Remapping loop
	size_t i, a, b;
	for (i = 0; i < nh; ++i)
	{
		a = i * nw * numbpp;
		b = (((i + y1) * width_) + x1) * numbpp;
		memcpy(nd + a, data_ + b, nw * numbpp);
		if (mask_)
			memcpy(nm + a, mask_ + b, nw * numbpp);
	}

	// It worked, yay
	clearData();
	data_   = nd;
	mask_   = nm;
	width_  = nw;
	height_ = nh;

	// Announce change
	announce("image_changed");
	return true;
}

// -----------------------------------------------------------------------------
// Resizes the image, conserving current data
// (will be cropped if new size is smaller)
// -----------------------------------------------------------------------------
bool SImage::resize(int nwidth, int nheight)
{
	// Check values
	if (nwidth < 0 || nheight < 0)
		return false;

	// If either dimension is zero, just clear the image
	if (nwidth == 0 || nheight == 0)
	{
		clear();
		return true;
	}

	// Init new image data
	uint8_t *newdata, *newmask;
	uint8_t  bpp = 1;
	if (type_ == RGBA)
		bpp = 4;
	// Create new image data
	newdata = new uint8_t[nwidth * nheight * bpp];
	memset(newdata, 0, nwidth * nheight * bpp);
	// Create new mask if needed
	if (type_ == PALMASK)
	{
		newmask = new uint8_t[nwidth * nheight];
		memset(newmask, 0, nwidth * nheight);
	}
	else
		newmask = nullptr;

	// Write new image data
	unsigned offset = 0;
	unsigned rowlen = MIN(width_, nwidth) * bpp;
	unsigned nrows  = MIN(height_, nheight);
	for (unsigned y = 0; y < nrows; y++)
	{
		// Copy data row
		memcpy(newdata + offset, data_ + (y * width_ * bpp), rowlen);

		// Copy mask row
		if (newmask)
			memcpy(newmask + offset, mask_ + (y * width_), rowlen);

		// Go to next row
		offset += nwidth * bpp;
	}

	// Update variables
	width_  = nwidth;
	height_ = nheight;
	clearData();
	data_ = newdata;
	mask_ = newmask;

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Sets the image data, size, and type from raw data
// -----------------------------------------------------------------------------
bool SImage::setImageData(uint8_t* ndata, int nwidth, int nheight, SIType ntype)
{
	if (ndata)
	{
		clearData();
		type_   = ntype;
		width_  = nwidth;
		height_ = nheight;
		data_   = ndata;

		// Announce change
		announce("image_changed");

		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
// Applies a palette translation to the image
// -----------------------------------------------------------------------------
bool SImage::applyTranslation(Translation* tr, Palette* pal, bool truecolor)
{
	// Check image is ok
	if (!data_)
		return false;

	// Can't apply a translation to a non-coloured image
	if (type_ == ALPHAMAP)
		return false;

	// Handle truecolor images
	if (type_ == RGBA)
		truecolor = true;
	size_t bpp = this->bpp();

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	uint8_t* newdata = nullptr;
	if (truecolor && type_ == PALMASK)
	{
		newdata = new uint8_t[width_ * height_ * 4];
		memset(newdata, 0, width_ * height_ * 4);
	}
	else
		newdata = data_;

	// Go through pixels
	for (int p = 0; p < width_ * height_; p++)
	{
		// No need to process transparent pixels
		if (mask_ && mask_[p] == 0)
			continue;

		rgba_t col;
		int    q = p * bpp;
		if (type_ == PALMASK)
			col.set(pal->colour(data_[p]));
		else if (type_ == RGBA)
		{
			col.set(data_[q], data_[q + 1], data_[q + 2], data_[q + 3]);

			// skip colours that don't match exactly to the palette
			col.index = pal->nearestColour(col);
			if (!col.equals(pal->colour(col.index)))
				continue;
		}

		col = tr->translate(col, pal);

		if (truecolor)
		{
			q              = p * 4;
			newdata[q + 0] = col.r;
			newdata[q + 1] = col.g;
			newdata[q + 2] = col.b;
			newdata[q + 3] = mask_ ? mask_[p] : col.a;
		}
		else
			data_[p] = col.index;
	}

	if (truecolor && type_ == PALMASK)
	{
		clearData(true);
		data_ = newdata;
		type_ = RGBA;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Applies a palette translation to the image
// -----------------------------------------------------------------------------
bool SImage::applyTranslation(string tr, Palette* pal, bool truecolor)
{
	Translation trans;
	trans.clear();
	trans.parse(tr);
	return applyTranslation(&trans, pal, truecolor);
}

// -----------------------------------------------------------------------------
// Draws a pixel of [colour] at [x],[y], blending it according to the options
// set in [properties].
// If the image is paletted, the resulting pixel colour is converted to its
// nearest match in [pal]
// -----------------------------------------------------------------------------
bool SImage::drawPixel(int x, int y, rgba_t colour, si_drawprops_t& properties, Palette* pal)
{
	// Check valid coords
	if (x < 0 || y < 0 || x >= width_ || y >= height_)
		return false;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Setup alpha
	if (properties.src_alpha)
		colour.a *= properties.alpha;
	else
		colour.a = 255 * properties.alpha;

	// Do nothing if completely transparent
	if (colour.a == 0)
		return true;

	// Get pixel index
	unsigned p = y * stride() + x * bpp();

	// Check for simple case (normal blending, no transparency involved)
	if (colour.a == 255 && properties.blend == NORMAL)
	{
		if (type_ == RGBA)
			colour.write(data_ + p);
		else
		{
			data_[p] = pal->nearestColour(colour);
			mask_[p] = colour.a;
		}

		return true;
	}

	// Not-so-simple case, do full processing
	rgba_t d_colour;
	if (type_ == PALMASK)
		d_colour = pal->colour(data_[p]);
	else
		d_colour.set(data_[p], data_[p + 1], data_[p + 2], data_[p + 3]);
	float alpha = (float)colour.a / 255.0f;

	// Additive blending
	if (properties.blend == ADD)
	{
		d_colour.set(
			MathStuff::clamp(d_colour.r + colour.r * alpha, 0, 255),
			MathStuff::clamp(d_colour.g + colour.g * alpha, 0, 255),
			MathStuff::clamp(d_colour.b + colour.b * alpha, 0, 255),
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Subtractive blending
	else if (properties.blend == SUBTRACT)
	{
		d_colour.set(
			MathStuff::clamp(d_colour.r - colour.r * alpha, 0, 255),
			MathStuff::clamp(d_colour.g - colour.g * alpha, 0, 255),
			MathStuff::clamp(d_colour.b - colour.b * alpha, 0, 255),
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Reverse-Subtractive blending
	else if (properties.blend == REVERSE_SUBTRACT)
	{
		d_colour.set(
			MathStuff::clamp((-d_colour.r) + colour.r * alpha, 0, 255),
			MathStuff::clamp((-d_colour.g) + colour.g * alpha, 0, 255),
			MathStuff::clamp((-d_colour.b) + colour.b * alpha, 0, 255),
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// 'Modulate' blending
	else if (properties.blend == MODULATE)
	{
		d_colour.set(
			MathStuff::clamp(colour.r * d_colour.r / 255, 0, 255),
			MathStuff::clamp(colour.g * d_colour.g / 255, 0, 255),
			MathStuff::clamp(colour.b * d_colour.b / 255, 0, 255),
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Normal blending (or unknown blend type)
	else
	{
		float inv_alpha = 1.0f - alpha;
		d_colour.set(
			d_colour.r * inv_alpha + colour.r * alpha,
			d_colour.g * inv_alpha + colour.g * alpha,
			d_colour.b * inv_alpha + colour.b * alpha,
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Apply new colour
	if (type_ == PALMASK)
	{
		data_[p] = pal->nearestColour(d_colour);
		mask_[p] = d_colour.a;
	}
	else if (type_ == RGBA)
		d_colour.write(data_ + p);
	else if (type_ == ALPHAMAP)
		data_[p] = d_colour.a;

	return true;
}

// -----------------------------------------------------------------------------
// Draws an image on to this image at [x],[y], with blending options set in
// [properties]. [pal_src] is used for the source image, and [pal_dest] is used
// for the destination image, if either is paletted
// -----------------------------------------------------------------------------
bool SImage::drawImage(
	SImage&         img,
	int             x_pos,
	int             y_pos,
	si_drawprops_t& properties,
	Palette*        pal_src,
	Palette*        pal_dest)
{
	// Check images
	if (!data_ || !img.data_)
		return false;

	// Setup palettes
	if (img.has_palette_ || !pal_src)
		pal_src = &(img.palette_);
	if (has_palette_ || !pal_dest)
		pal_dest = &palette_;

	// Go through pixels
	unsigned s_stride = img.stride();
	uint8_t  s_bpp    = img.bpp();
	unsigned sp       = 0;
	for (int y = y_pos; y < y_pos + img.height_; y++) // Rows
	{
		// Skip out-of-bounds rows
		if (y < 0 || y >= height_)
		{
			sp += s_stride;
			continue;
		}

		for (int x = x_pos; x < x_pos + img.width_; x++) // Columns
		{
			// Skip out-of-bounds columns
			if (x < 0 || x >= width_)
			{
				sp += s_bpp;
				continue;
			}

			// Skip if source pixel is fully transparent
			if ((img.type_ == PALMASK && img.mask_[sp] == 0) || (img.type_ == ALPHAMAP && img.data_[sp] == 0)
				|| (img.type_ == RGBA && img.data_[sp + 3] == 0))
			{
				sp += s_bpp;
				continue;
			}

			// Draw pixel
			if (img.type_ == PALMASK)
			{
				rgba_t col = pal_src->colour(img.data_[sp]);
				col.a      = img.mask_[sp];
				drawPixel(x, y, col, properties, pal_dest);
			}
			else if (img.type_ == RGBA)
				drawPixel(
					x,
					y,
					rgba_t(img.data_[sp], img.data_[sp + 1], img.data_[sp + 2], img.data_[sp + 3]),
					properties,
					pal_dest);
			else if (img.type_ == ALPHAMAP)
				drawPixel(
					x, y, rgba_t(img.data_[sp], img.data_[sp], img.data_[sp], img.data_[sp]), properties, pal_dest);

			// Go to next source pixel
			sp += s_bpp;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Colourises the image to [colour].
// If the image is paletted, each pixel will be set to its nearest matching
// colour in [pal]
// -----------------------------------------------------------------------------
bool SImage::colourise(rgba_t colour, Palette* pal, int start, int stop)
{
	// Can't do this with alpha maps
	if (type_ == ALPHAMAP)
		return false;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Go through all pixels
	uint8_t bpp = this->bpp();
	rgba_t  col;
	for (int a = 0; a < width_ * height_ * bpp; a += bpp)
	{
		// Skip colors out of range if desired
		if (type_ == PALMASK && start >= 0 && stop >= start && stop < 256)
		{
			if (data_[a] < start || data_[a] > stop)
				continue;
		}

		// Get current pixel colour
		if (type_ == RGBA)
			col.set(data_[a], data_[a + 1], data_[a + 2], data_[a + 3]);
		else
			col.set(pal->colour(data_[a]));

		// Colourise it
		float grey = (col.r * col_greyscale_r + col.g * col_greyscale_g + col.b * col_greyscale_b) / 255.0f;
		if (grey > 1.0)
			grey = 1.0;
		col.r = colour.r * grey;
		col.g = colour.g * grey;
		col.b = colour.b * grey;

		// Set pixel colour
		if (type_ == RGBA)
			col.write(data_ + a);
		else
			data_[a] = pal->nearestColour(col);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Tints the image to [colour] by [amount].
// If the image is paletted, each pixel will be set to its nearest matching
// colour in [pal]
// -----------------------------------------------------------------------------
bool SImage::tint(rgba_t colour, float amount, Palette* pal, int start, int stop)
{
	// Can't do this with alpha maps
	if (type_ == ALPHAMAP)
		return false;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Go through all pixels
	uint8_t bpp = this->bpp();
	rgba_t  col;
	for (int a = 0; a < width_ * height_ * bpp; a += bpp)
	{
		// Skip colors out of range if desired
		if (type_ == PALMASK && start >= 0 && stop >= start && stop < 256)
		{
			if (data_[a] < start || data_[a] > stop)
				continue;
		}

		// Get current pixel colour
		if (type_ == RGBA)
			col.set(data_[a], data_[a + 1], data_[a + 2], data_[a + 3]);
		else
			col.set(pal->colour(data_[a]));

		// Tint it
		float inv_amt = 1.0f - amount;
		col.set(
			col.r * inv_amt + colour.r * amount,
			col.g * inv_amt + colour.g * amount,
			col.b * inv_amt + colour.b * amount,
			col.a);

		// Set pixel colour
		if (type_ == RGBA)
			col.write(data_ + a);
		else
			data_[a] = pal->nearestColour(col);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Automatically crop the image to remove fully transparent rows and columns
// from the sides.
// Returns true if successfully cropped.
// -----------------------------------------------------------------------------
bool SImage::adjust()
{
	int x1 = 0, x2 = width_, y1 = 0, y2 = height_;

	// Loop for empty columns on the left
	bool opaquefound = false;
	while (x1 < x2)
	{
		for (int i = 0; i < y2; ++i)
		{
			size_t p = i * width_ + x1; // Pixel position
			switch (type_)
			{
			case PALMASK: // Transparency is mask[p] == 0
				if (mask_[p])
					opaquefound = true;
				break;
			case RGBA: // Transparency is data[p*4 + 3] == 0
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case ALPHAMAP: // Transparency is data[p] == 0
				if (data_[p])
					opaquefound = true;
				break;
			}
			if (opaquefound)
				break;
		}
		if (opaquefound)
			break;
		++x1;
	}

	if (x1 == x2) // Empty image, all columns are empty, crop it to a single pixel
		return crop(0, 0, 1, 1);

	// Now loop for empty columns on the right
	opaquefound = false;
	while (x2 > x1)
	{
		for (int i = 0; i < y2; ++i)
		{
			size_t p = i * width_ + x2 - 1;
			switch (type_)
			{
			case PALMASK:
				if (mask_[p])
					opaquefound = true;
				break;
			case RGBA:
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case ALPHAMAP:
				if (data_[p])
					opaquefound = true;
				break;
			}
			if (opaquefound)
				break;
		}
		if (opaquefound)
			break;
		--x2;
	}

	// Now loop for empty rows from the top
	opaquefound = false;
	while (y1 < y2)
	{
		for (int i = x1; i < x2; ++i)
		{
			size_t p = y1 * width_ + i;
			switch (type_)
			{
			case PALMASK:
				if (mask_[p])
					opaquefound = true;
				break;
			case RGBA:
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case ALPHAMAP:
				if (data_[p])
					opaquefound = true;
				break;
			}
			if (opaquefound)
				break;
		}
		if (opaquefound)
			break;
		++y1;
	}

	// Finally loop for empty rows from the bottom

	opaquefound = false;
	while (y2 > y1)
	{
		for (int i = x1; i < x2; ++i)
		{
			size_t p = (y2 - 1) * width_ + i;
			switch (type_)
			{
			case PALMASK:
				if (mask_[p])
					opaquefound = true;
				break;
			case RGBA:
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case ALPHAMAP:
				if (data_[p])
					opaquefound = true;
				break;
			}
			if (opaquefound)
				break;
		}
		if (opaquefound)
			break;
		--y2;
	}

	// Now we've found the coordinates, so we can crop
	if (x1 == 0 && y1 == 0 && x2 == width_ && y2 == height_)
		return false; // No adjustment needed
	return crop(x1, y1, x2, y2);
}

bool SImage::mirrorpad()
{
	// Only pad images that actually have offsets
	if (offset_x_ == 0 && offset_y_ == 0)
		return false;

	// Only pad images that need it, so for instance if width is 10, and ofsx is 5,
	// then the image is already mirrored. If width is 11, accept ofsx 5 or 6 as good.
	if (offset_x_ == width_ / 2 || (width_ % 2 == 1 && offset_x_ == width_ / 2 + 1))
		return false;

	// Now we need to pad. Padding to the right can be done just by resizing the image,
	// padding to the left requires flipping it, resizing it, and flipping it back.
	bool needflip = offset_x_ < width_ / 2;
	int  extra    = abs((offset_x_ * 2) - width_);

	bool success = true;
	if (needflip)
		success = mirror(false);
	if (success)
		success = resize(width_ + extra, height_);
	else
		return false;
	if (needflip && success)
	{
		success = mirror(false);
		offset_x_ += extra;
	}
	return success;
}
