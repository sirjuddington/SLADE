
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Graphics/Palette/Palette.h"
#include "Graphics/Translation.h"
#include "SIFormat.h"
#include "Utility/MathStuff.h"
#undef BOOL

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
Palette pal_greyscale;
}


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
// SImage class constructors
// -----------------------------------------------------------------------------
SImage::SImage() = default;
SImage::SImage(Type type) : type_{ type } {}

// -----------------------------------------------------------------------------
// SImage class copy constructor
// -----------------------------------------------------------------------------
SImage::SImage(const SImage& img) :
	width_{ img.width_ },
	height_{ img.height_ },
	type_{ img.type_ },
	offset_x_{ img.offset_x_ },
	offset_y_{ img.offset_y_ },
	format_{ img.format_ },
	imgindex_{ img.imgindex_ },
	numimages_{ img.numimages_ }
{
	if (img.palette_)
		palette_ = std::make_unique<Palette>(*img.palette_);

	data_.importMem(img.data_);
	mask_.importMem(img.mask_);
}

// -----------------------------------------------------------------------------
// SImage class destructor
// -----------------------------------------------------------------------------
SImage::~SImage() = default;

// -----------------------------------------------------------------------------
// Loads the image as RGBA data into [mc].
// Returns false if image is invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::putRGBAData(MemChunk& mc, const Palette* pal) const
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgba data
	mc.reSize(width_ * height_ * 4, false);

	// If data is already in RGBA format just return a copy
	if (type_ == Type::RGBA)
	{
		mc.importMem(data_);
		return true;
	}

	// Convert paletted
	else if (type_ == Type::PalMask)
	{
		// Get palette to use
		auto palette = paletteToUse(pal);

		uint8_t rgba[4];
		for (int a = 0; a < width_ * height_; a++)
		{
			// Get colour
			ColRGBA col = palette->colour(data_[a]);

			// Set alpha
			if (mask_.data())
				col.a = mask_[a];
			else
				col.a = 255;

			col.write(rgba);   // Write colour to array
			mc.write(rgba, 4); // Write array to MemChunk
		}

		return true;
	}

	// Convert if alpha map
	else if (type_ == Type::AlphaMap)
	{
		uint8_t rgba[4];
		ColRGBA col;
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
bool SImage::putRGBData(MemChunk& mc, const Palette* pal) const
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgb data
	mc.reSize(width_ * height_ * 3, false);

	if (type_ == Type::RGBA)
	{
		// RGBA format, remove alpha information
		for (int a = 0; a < width_ * height_ * 4; a += 4)
			mc.write(&data_[a], 3);

		return true;
	}
	else if (type_ == Type::PalMask)
	{
		// Paletted, convert to RGB

		// Get palette to use
		auto palette = paletteToUse(pal);

		// Build RGB data
		uint8_t rgba[4];
		for (int a = 0; a < width_ * height_; a++)
		{
			palette->colour(data_[a]).write(rgba);
			mc.write(rgba, 3);
		}

		return true;
	}
	else if (type_ == Type::AlphaMap)
	{
		// Alpha map, convert to RGB

		uint8_t rgba[4];
		ColRGBA col;
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
bool SImage::putIndexedData(MemChunk& mc) const
{
	// Check the image is valid
	if (!isValid())
		return false;

	switch (type_)
	{
	case Type::RGBA:     return false; // Cannot do this for trucolor graphics.
	case Type::PalMask:
	case Type::AlphaMap: return mc.write(data_.data(), data_.size());
	default:             return false;
	}
}

// -----------------------------------------------------------------------------
// Loads the image alpha/mask data into [mc].
// Returns false if image is invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::putAlphaData(MemChunk& mc) const
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgba data
	mc.reSize(width_ * height_ * 4, false);

	// If data is already in alpha map format just return a copy
	if (type_ == Type::AlphaMap)
	{
		mc.importMem(data_);
		return true;
	}

	// If paletted, return the mask or full opaque
	else if (type_ == Type::PalMask)
	{
		if (mask_.data())
			mc.importMem(mask_);
		else
		{
			mc.reSize(width_ * height_, false);
			mc.fillData(255);
		}

		return true;
	}

	// If RGBA, return the alpha channel
	else if (type_ == Type::RGBA)
	{
		vector<uint8_t> alpha_map(width_ * height_);
		for (int a = 0; a < width_ * height_; a++)
			alpha_map[a] = data_[a * 4 + 3];
		mc.importMem(alpha_map.data(), width_ * height_);
	}

	return false; // Invalid image type
}

// -----------------------------------------------------------------------------
// Returns the number of bytes per image row
// -----------------------------------------------------------------------------
unsigned SImage::stride() const
{
	if (type_ == Type::RGBA)
		return width_ * 4;
	else
		return width_;
}

// -----------------------------------------------------------------------------
// Returns the number of bytes per image pixel
// -----------------------------------------------------------------------------
uint8_t SImage::bpp() const
{
	if (type_ == Type::RGBA)
		return 4;
	else
		return 1;
}

// -----------------------------------------------------------------------------
// Returns an info struct with image information
// -----------------------------------------------------------------------------
SImage::Info SImage::info() const
{
	Info inf;

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
	inf.has_palette = hasPalette();

	return inf;
}

// -----------------------------------------------------------------------------
// Returns the colour of the pixel at [x,y] in the image, or black+invisible if
// out of range
// -----------------------------------------------------------------------------
ColRGBA SImage::pixelAt(unsigned x, unsigned y, const Palette* pal) const
{
	// Get pixel index
	const unsigned index = y * stride() + x * bpp();

	// Check it
	if (index >= static_cast<unsigned>(width_ * height_ * bpp()))
		return { 0, 0, 0, 0 };

	// Get colour at pixel
	ColRGBA col;
	if (type_ == Type::RGBA)
	{
		col.r = data_[index];
		col.g = data_[index + 1];
		col.b = data_[index + 2];
		col.a = data_[index + 3];
	}
	else if (type_ == Type::PalMask)
	{
		// Get palette to use
		auto palette = paletteToUse(pal);

		col.set(palette->colour(data_[index]));
		if (mask_.hasData())
			col.a = mask_[index];
	}
	else if (type_ == Type::AlphaMap)
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
uint8_t SImage::pixelIndexAt(unsigned x, unsigned y) const
{
	// Get pixel index
	const unsigned index = y * stride() + x * bpp();

	// Check it
	if (index >= static_cast<unsigned>(width_ * height_ * bpp()) || type_ == Type::RGBA)
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
	signals_.offsets_changed(offset_x_, offset_y_);
}

// -----------------------------------------------------------------------------
// Changes the image Y offset
// -----------------------------------------------------------------------------
void SImage::setYOffset(int offset)
{
	// Change the Y offset
	offset_y_ = offset;

	// Announce change
	signals_.offsets_changed(offset_x_, offset_y_);
}

// -----------------------------------------------------------------------------
// Changes the image offsets
// -----------------------------------------------------------------------------
void SImage::setOffsets(const Vec2i& offsets)
{
	// Change the offsets
	offset_x_ = offsets.x;
	offset_y_ = offsets.y;

	// Announce change
	signals_.offsets_changed(offset_x_, offset_y_);
}

// -----------------------------------------------------------------------------
// Sets the image palette to [pal]
// -----------------------------------------------------------------------------
void SImage::setPalette(const Palette* pal)
{
	if (!pal)
	{
		palette_.reset();
		return;
	}

	if (!palette_)
		palette_ = std::make_unique<Palette>(*pal);
	else
		palette_->copyPalette(pal);
}

// -----------------------------------------------------------------------------
// Deletes/clears any existing image data
// -----------------------------------------------------------------------------
void SImage::clearData(bool clear_mask)
{
	// Delete data if it exists
	data_.clear();

	// Delete mask if it exists
	if (clear_mask)
		mask_.clear();
}

// -----------------------------------------------------------------------------
// Creates an empty image
// -----------------------------------------------------------------------------
void SImage::create(int width, int height, Type type, const Palette* pal, int index, int numimages)
{
	// Check valid width/height
	if (width < 0 || height < 0)
		return;

	// Clear current image
	clearData();

	// Set image properties
	width_     = width;
	height_    = height;
	type_      = type;
	offset_x_  = 0;
	offset_y_  = 0;
	numimages_ = numimages;
	imgindex_  = index;
	if (pal)
		palette_ = std::make_unique<Palette>(*pal);
	else
		palette_.reset();

	// Create blank image
	if (width > 0 && height > 0)
	{
		data_.reSize(width * height * bpp(), false);
		data_.fillData(0);
		if (type == Type::PalMask)
		{
			mask_.reSize(width * height, false);
			mask_.fillData(0);
		}
	}
	else
	{
		data_.clear();
		if (type == Type::PalMask)
			mask_.clear();
	}
}

// -----------------------------------------------------------------------------
// Creates an empty image, initialising with properties from [info]
// -----------------------------------------------------------------------------
void SImage::create(const Info& info, const Palette* pal)
{
	// Normal creation
	create(info.width, info.height, info.colformat, pal, info.imgindex, info.numimages);

	// Set other info
	offset_x_ = info.offset_x;
	offset_y_ = info.offset_y;
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
	signals_.image_changed();
}
void SImage::clear(Type new_type)
{
	clear();
	type_ = new_type;
}

// -----------------------------------------------------------------------------
// 'Fills' the alpha channel or mask with the given <alpha> value
// -----------------------------------------------------------------------------
void SImage::fillAlpha(uint8_t alpha)
{
	// Check image is valid
	if (!isValid())
		return;

	if (type_ == Type::RGBA)
	{
		// RGBA format, set alpha values to given one
		for (int a = 3; a < width_ * height_ * 4; a += 4)
			data_[a] = alpha;
	}
	else if (type_ == Type::PalMask)
	{
		// Paletted masked format, fill mask with alpha value
		if (!mask_.hasData())
			mask_.reSize(width_ * height_);

		mask_.fillData(alpha);
	}
	else if (type_ == Type::AlphaMap)
		data_.fillData(alpha);

	// Announce change
	signals_.image_changed();
}

// -----------------------------------------------------------------------------
// Returns the first unused palette index, or -1 if the image is not paletted or
// uses all 256 colours
// If [preferred] is > 0, it will be returned if it's not already in use,
// instead of the first unused colour
// -----------------------------------------------------------------------------
short SImage::findUnusedColour(short preferred) const
{
	// Only for paletted images
	if (type_ != Type::PalMask)
		return -1;

	// Init used colours list
	uint8_t used[256] = {};

	// Go through image data and mark used colours
	for (int a = 0; a < width_ * height_; a++)
		used[data_[a]] = 1;

	// Check if preferred colour is unused
	if (preferred >= 0 && !used[preferred])
		return preferred;

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
size_t SImage::countColours() const
{
	// If the picture is not paletted, return 0.
	if (type_ != Type::PalMask)
		return 0;

	const auto usedcolours = new bool[256];
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
	if (type_ != Type::PalMask)
		return;

	// Get palette to use
	auto palette = paletteToUse(pal);

	// Init variables
	Palette         newpal;
	vector<uint8_t> usedcolours(256);
	vector<int>     remap(256);
	memset(usedcolours.data(), 0, 256);
	size_t used = 0;

	// Count all color indices actually used on the picture
	for (int a = 0; a < width_ * height_; ++a)
		usedcolours[data_[a]] = true;

	// Create palette remapping information
	for (size_t b = 0; b < 256; ++b)
	{
		if (usedcolours[b])
		{
			newpal.setColour(used, palette->colour(b));
			remap[b] = used;
			++used;
		}
	}

	// Remap image to new palette indices
	for (int c = 0; c < width_ * height_; ++c)
		data_[c] = remap[data_[c]];

	palette->copyPalette(&newpal);
}

// -----------------------------------------------------------------------------
// Copies all data and properties from [image]
// -----------------------------------------------------------------------------
bool SImage::copyImage(const SImage* image)
{
	// Check image was given
	if (!image)
		return false;

	// Clear current data
	clearData();

	// Copy image properties
	width_     = image->width_;
	height_    = image->height_;
	type_      = image->type_;
	offset_x_  = image->offset_x_;
	offset_y_  = image->offset_y_;
	imgindex_  = image->imgindex_;
	numimages_ = image->numimages_;

	// Copy palette
	if (image->palette_)
		setPalette(image->palette_.get());
	else
		palette_.reset();

	// Copy image data
	if (image->data_.hasData())
		data_.importMem(image->data_);
	if (image->mask_.hasData())
		mask_.importMem(image->mask_);

	// Announce change
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Detects the format of [data] and, if it's a valid image format, loads it into
// this image
// -----------------------------------------------------------------------------
bool SImage::open(const MemChunk& data, int index, string_view type_hint)
{
	// Check with type hint format first
	if (!type_hint.empty())
	{
		auto format = SIFormat::getFormat(type_hint);
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
bool SImage::convertRGBA(const Palette* pal)
{
	// If it's already 32bpp do nothing
	if (type_ == Type::RGBA)
		return false;

	// Get 32bit data
	MemChunk rgba_data;
	putRGBAData(rgba_data, pal);

	// Clear current data
	clearData(true);

	// Copy it
	data_.importMem(rgba_data);

	// Set new type & update variables
	type_ = Type::RGBA;
	palette_.reset();

	// Announce change
	signals_.image_changed();

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
bool SImage::convertPaletted(const Palette* pal_target, const Palette* pal_current)
{
	// Check image/parameters are valid
	if (!isValid() || !pal_target)
		return false;

	// Get image data as RGBA
	MemChunk rgba_data;
	putRGBAData(rgba_data, pal_current);

	// Create mask from alpha info (if converting from RGBA)
	if (type_ == Type::RGBA || type_ == Type::AlphaMap)
	{
		// Clear current mask
		mask_.clear();

		// Init mask
		mask_.reSize(width_ * height_);

		// Get values from alpha channel
		int c = 0;
		for (int a = 3; a < width_ * height_ * 4; a += 4)
			mask_[c++] = rgba_data[a];
	}

	// Load given palette
	setPalette(pal_target);

	// Clear current image data (but not mask)
	clearData(false);

	// Do conversion
	data_.reSize(width_ * height_);
	unsigned i = 0;
	ColRGBA  col;
	for (int a = 0; a < width_ * height_; a++)
	{
		col.r    = rgba_data[i++];
		col.g    = rgba_data[i++];
		col.b    = rgba_data[i++];
		data_[a] = palette_->nearestColour(col);
		i++; // Skip alpha
	}

	// Update variables
	type_ = Type::PalMask;

	// Announce change
	signals_.image_changed();

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Converts the image to an alpha map, generating alpha values from either pixel
// brightness or existing alpha, depending on the value of [alpha_source]
// -----------------------------------------------------------------------------
bool SImage::convertAlphaMap(AlphaSource alpha_source, const Palette* pal)
{
	// Get RGBA data
	MemChunk rgba;
	putRGBAData(rgba, pal);

	// Recreate image
	create(width_, height_, Type::AlphaMap);

	// Generate alpha mask
	unsigned c = 0;
	for (int a = 0; a < width_ * height_; a++)
	{
		// Determine alpha for this pixel
		uint8_t alpha;
		if (alpha_source == AlphaSource::Brightness) // Pixel brightness
			alpha = static_cast<double>(rgba[c]) * 0.3 + static_cast<double>(rgba[c + 1]) * 0.59
					+ static_cast<double>(rgba[c + 2]) * 0.11;
		else // Existing alpha
			alpha = rgba[c + 3];

		// Set pixel
		data_[a] = alpha;

		// Next RGBA pixel
		c += 4;
	}

	// Announce change
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that pixels that match [colour] are fully
// transparent, and all other pixels fully opaque
// -----------------------------------------------------------------------------
bool SImage::maskFromColour(ColRGBA colour, const Palette* pal)
{
	if (type_ == Type::PalMask)
	{
		// Get palette to use
		auto palette = paletteToUse(pal);

		// Palette+Mask type, go through the mask
		for (int a = 0; a < width_ * height_; a++)
		{
			if (palette->colour(data_[a]).equals(colour))
				mask_[a] = 0;
			else
				mask_[a] = 255;
		}
	}
	else if (type_ == Type::RGBA)
	{
		// RGBA type, go through alpha channel
		uint32_t c = 0;
		for (int a = 0; a < width_ * height_; a++)
		{
			ColRGBA pix_col(data_[c], data_[c + 1], data_[c + 2], 255);

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
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that each pixel's transparency matches its
// brigntness level (where black is fully transparent)
// -----------------------------------------------------------------------------
bool SImage::maskFromBrightness(const Palette* pal)
{
	if (type_ == Type::PalMask)
	{
		// Get palette to use
		auto palette = paletteToUse(pal);

		// Go through pixel data
		for (int a = 0; a < width_ * height_; a++)
		{
			// Set mask from pixel colour brightness value
			const ColRGBA col = palette->colour(data_[a]);
			mask_[a]          = (static_cast<double>(col.r) * 0.3) + (static_cast<double>(col.g) * 0.59)
					   + (static_cast<double>(col.b) * 0.11);
		}
	}
	else if (type_ == Type::RGBA)
	{
		// Go through pixel data
		unsigned c = 0;
		for (int a = 0; a < width_ * height_; a++)
		{
			// Set alpha from pixel colour brightness value
			data_[c + 3] = static_cast<double>(data_[c]) * 0.3 + static_cast<double>(data_[c + 1]) * 0.59
						   + static_cast<double>(data_[c + 2]) * 0.11;
			// Skip alpha
			c += 4;
		}
	}
	// ALPHAMASK type is already a brightness mask

	// Announce change
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that any pixel alpha level currently
// greater than [threshold] is fully opaque, and all other pixels fully
// transparent
// -----------------------------------------------------------------------------
bool SImage::cutoffMask(uint8_t threshold)
{
	if (type_ == Type::PalMask)
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
	else if (type_ == Type::RGBA)
	{
		// RGBA format, go through alpha channel
		for (int a = 3; a < width_ * height_ * 4; a += 4)
		{
			if (data_[a] > threshold)
				data_[a] = 255;
			else
				data_[a] = 0;
		}
	}
	else if (type_ == Type::AlphaMap)
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
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Sets the pixel at [x],[y] to [colour].
// Returns false if the position is out of range, true otherwise
// -----------------------------------------------------------------------------
bool SImage::setPixel(int x, int y, ColRGBA colour, const Palette* pal)
{
	// Check position
	if (x < 0 || x >= width_ || y < 0 || y >= height_)
		return false;

	// Set the pixel
	if (type_ == Type::RGBA)
		colour.write(data_.data() + (y * (width_ * 4) + (x * 4)));
	else if (type_ == Type::PalMask)
	{
		// Get palette to use
		auto palette = paletteToUse(pal);

		// Get color index to use (the ColRGBA's index if defined, nearest colour otherwise)
		const uint8_t index = (colour.index == -1) ? palette->nearestColour(colour) : colour.index;

		data_[y * width_ + x] = index;
		if (mask_.hasData())
			mask_[y * width_ + x] = colour.a;
	}
	else if (type_ == Type::AlphaMap)
	{
		// Just use colour alpha
		data_[y * width_ + x] = colour.a;
	}

	// Announce
	signals_.image_changed();

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
	if (type_ == Type::RGBA)
	{
		// Set the pixel
		auto col = palette_ ? palette_->colour(pal_index) : pal_greyscale.colour(pal_index);
		col.a    = alpha;
		col.write(data_.data() + (y * (width_ * 4) + (x * 4)));
	}

	// Paletted
	else if (type_ == Type::PalMask)
	{
		// Set the pixel
		data_[y * width_ + x] = pal_index;
		if (mask_.hasData())
			mask_[y * width_ + x] = alpha;
	}

	// Alpha map
	else if (type_ == Type::AlphaMap)
	{
		// Set the pixel
		data_[y * width_ + x] = alpha;
	}

	// Invalid type
	else
		return false;

	// Announce
	signals_.image_changed();

	// Invalid type
	return true;
}

// -----------------------------------------------------------------------------
// Change the width of the image to the given value, adjusting the height
// automatically
// -----------------------------------------------------------------------------
void SImage::setWidth(int w)
{
	const int numpixels = width_ * height_;
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
	const int numpixels = width_ * height_;
	if ((numpixels > h) && ((numpixels % h) == 0))
	{
		height_ = h;
		width_  = numpixels / h;
	}
}

// -----------------------------------------------------------------------------
// Rotates the image with an angle of 90°, 180° or 270°
// -----------------------------------------------------------------------------
bool SImage::rotate(int angle)
{
	if (!data_.hasData())
		return false;

	if (angle == 0)
		return true; // Nothing to do
	if (angle % 90)
		return false; // Unsupported angle
	while (angle < 0)
		angle += 360;
	angle %= 360;
	angle = 360 - angle;

	// Compute new dimensions and numbers of pixels and bytes
	int new_width, new_height;
	if (angle % 180)
	{
		new_width  = height_;
		new_height = width_;
	}
	else
	{
		new_width  = width_;
		new_height = height_;
	}
	const int numpixels = width_ * height_;
	int       numbpp;
	if (type_ == Type::PalMask)
		numbpp = 1;
	else if (type_ == Type::RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	vector<uint8_t> new_data(numpixels * numbpp);
	vector<uint8_t> new_mask;
	if (mask_.hasData())
		new_mask.resize(numpixels * numbpp, 0);

	// Remapping loop
	int i, j, k;
	for (i = 0; i < numpixels; ++i)
	{
		switch (angle)
		{
			// Urgh maths...
		case 90:  j = (((new_height - 1) - (i % width_)) * new_width) + (i / width_); break;
		case 180: j = (numpixels - 1) - i; break;
		case 270: j = ((i % width_) * new_width) + ((new_width - 1) - (i / width_)); break;
		default:  return false;
		}
		if (j >= numpixels)
		{
			log::error("Pixel {} remapped to {}, how did this even happen?", i, j);
			return false;
		}
		for (k = 0; k < numbpp; ++k)
		{
			new_data[(j * numbpp) + k] = data_[(i * numbpp) + k];
			if (mask_.hasData())
				new_mask[(j * numbpp) + k] = mask_[(i * numbpp) + k];
		}
	}

	// It worked, yay
	clearData();
	data_.importMem(new_data.data(), new_data.size());
	if (!new_mask.empty())
		mask_.importMem(new_mask.data(), new_mask.size());
	width_  = new_width;
	height_ = new_height;

	// Announce change
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Mirrors the image horizontally or vertically.
// -----------------------------------------------------------------------------
bool SImage::mirror(bool vertical)
{
	// Compute numbers of pixels and bytes
	const int numpixels = width_ * height_;
	int       numbpp;
	if (type_ == Type::PalMask)
		numbpp = 1;
	else if (type_ == Type::RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	vector<uint8_t> new_data(numpixels * numbpp);
	vector<uint8_t> new_mask;
	if (mask_.hasData())
		new_mask.resize(numpixels * numbpp);

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
			log::error("Pixel {} remapped to {}, how did this even happen?", i, j);
			return false;
		}
		for (k = 0; k < numbpp; ++k)
		{
			new_data[(j * numbpp) + k] = data_[(i * numbpp) + k];
			if (mask_.hasData())
				new_mask[(j * numbpp) + k] = mask_[(i * numbpp) + k];
		}
	}

	// It worked, yay
	clearData();
	data_.importMem(new_data.data(), new_data.size());
	if (!new_mask.empty())
		mask_.importMem(new_mask.data(), new_mask.size());

	// Announce change
	signals_.image_changed();
	return true;
}

// -----------------------------------------------------------------------------
// Converts from column-major to row-major
// -----------------------------------------------------------------------------
bool SImage::imgconv()
{
	const int oldwidth = width_;
	width_             = height_;
	height_            = oldwidth;
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

	const size_t new_width  = x2 - x1;
	const size_t new_height = y2 - y1;

	// Compute numbers of pixels and bytes
	const int numpixels = new_width * new_height;
	int       numbpp;
	if (type_ == Type::PalMask || type_ == Type::AlphaMap)
		numbpp = 1;
	else if (type_ == Type::RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	vector<uint8_t> new_data(numpixels * numbpp);
	vector<uint8_t> new_mask;
	if (mask_.hasData())
		new_mask.resize(numpixels * numbpp);

	// Remapping loop
	size_t i, a, b;
	for (i = 0; i < new_height; ++i)
	{
		a = i * new_width * numbpp;
		b = (((i + y1) * width_) + x1) * numbpp;
		memcpy(new_data.data() + a, data_.data() + b, new_width * numbpp);
		if (mask_.hasData())
			memcpy(new_mask.data() + a, mask_.data() + b, new_width * numbpp);
	}

	// It worked, yay
	clearData();
	data_.importMem(new_data.data(), new_data.size());
	if (!new_mask.empty())
		mask_.importMem(new_mask.data(), new_mask.size());
	width_  = new_width;
	height_ = new_height;

	// Announce change
	signals_.image_changed();
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
	uint8_t bpp = 1;
	if (type_ == Type::RGBA)
		bpp = 4;

	// Create new image data
	vector<uint8_t> new_data(nwidth * nheight * bpp, 0);

	// Create new mask if needed
	vector<uint8_t> new_mask;
	if (type_ == Type::PalMask)
		new_mask.resize(nwidth * nheight, 0);

	// Write new image data
	unsigned       offset = 0;
	const unsigned rowlen = std::min<unsigned>(width_, nwidth) * bpp;
	const unsigned nrows  = std::min<unsigned>(height_, nheight);
	for (unsigned y = 0; y < nrows; y++)
	{
		// Copy data row
		memcpy(new_data.data() + offset, data_.data() + (y * width_ * bpp), rowlen);

		// Copy mask row
		if (!new_mask.empty())
			memcpy(new_mask.data() + offset, mask_.data() + (y * width_), rowlen);

		// Go to next row
		offset += nwidth * bpp;
	}

	// Update variables
	width_  = nwidth;
	height_ = nheight;
	clearData();
	data_.importMem(new_data.data(), new_data.size());
	if (!new_mask.empty())
		mask_.importMem(new_mask.data(), new_mask.size());

	// Announce change
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Sets the image data, size, and type from raw data
// -----------------------------------------------------------------------------
bool SImage::setImageData(const vector<uint8_t>& ndata, int nwidth, int nheight, Type ntype)
{
	if (ndata.empty())
		return false;

	clearData();
	type_   = ntype;
	width_  = nwidth;
	height_ = nheight;
	data_.importMem(ndata.data(), ndata.size());

	// Announce change
	signals_.image_changed();

	return true;
}
bool SImage::setImageData(const uint8_t* ndata, unsigned ndata_size, int nwidth, int nheight, Type ntype)
{
	if (!ndata)
		return false;

	clearData();
	type_   = ntype;
	width_  = nwidth;
	height_ = nheight;
	data_.importMem(ndata, ndata_size);

	// Announce change
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Applies a palette translation to the image
// -----------------------------------------------------------------------------
bool SImage::applyTranslation(const Translation* tr, const Palette* pal, bool truecolor)
{
	// Check image is ok
	if (!data_.hasData())
		return false;

	// Can't apply a translation to a non-coloured image
	if (type_ == Type::AlphaMap)
		return false;

	// Handle truecolor images
	if (type_ == Type::RGBA)
		truecolor = true;
	const size_t bpp = this->bpp();

	// Get palette to use
	auto palette = paletteToUse(pal);

	uint8_t* newdata;
	if (truecolor && type_ == Type::PalMask)
	{
		newdata = new uint8_t[width_ * height_ * 4];
		memset(newdata, 0, width_ * height_ * 4);
	}
	else
		newdata = data_.data();

	// Go through pixels
	for (int p = 0; p < width_ * height_; p++)
	{
		// No need to process transparent pixels
		if (mask_.hasData() && mask_[p] == 0)
			continue;

		ColRGBA col;
		int     q = p * bpp;
		if (type_ == Type::PalMask)
			col.set(palette->colour(data_[p]));
		else if (type_ == Type::RGBA)
		{
			col.set(data_[q], data_[q + 1], data_[q + 2], data_[q + 3]);

			// skip colours that don't match exactly to the palette
			col.index = palette->nearestColour(col);
			if (!col.equals(palette->colour(col.index)))
				continue;
		}

		col = tr->translate(col, palette);

		if (truecolor)
		{
			q              = p * 4;
			newdata[q + 0] = col.r;
			newdata[q + 1] = col.g;
			newdata[q + 2] = col.b;
			newdata[q + 3] = mask_.hasData() ? mask_[p] : col.a;
		}
		else
			data_[p] = col.index;
	}

	if (truecolor && type_ == Type::PalMask)
	{
		clearData(true);
		data_.importMem(newdata, width_ * height_ * 4);
		type_ = Type::RGBA;
	}

	// Announce change
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Applies a palette translation to the image
// -----------------------------------------------------------------------------
bool SImage::applyTranslation(string_view tr, const Palette* pal, bool truecolor)
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
bool SImage::drawPixel(int x, int y, ColRGBA colour, const DrawProps& properties, const Palette* pal)
{
	// Check valid coords
	if (x < 0 || y < 0 || x >= width_ || y >= height_)
		return false;

	// Get palette to use
	auto palette = paletteToUse(pal);

	// Setup alpha
	if (properties.src_alpha)
		colour.a *= properties.alpha;
	else
		colour.a = 255 * properties.alpha;

	// Do nothing if completely transparent
	if (colour.a == 0)
		return true;

	// Get pixel index
	const unsigned p = y * stride() + x * bpp();

	// Check for simple case (normal blending, no transparency involved)
	if (colour.a == 255 && properties.blend == BlendType::Normal)
	{
		if (type_ == Type::RGBA)
			colour.write(data_.data() + p);
		else
		{
			data_[p] = palette->nearestColour(colour);
			mask_[p] = colour.a;
		}

		return true;
	}

	// Not-so-simple case, do full processing
	ColRGBA d_colour;
	if (type_ == Type::PalMask)
		d_colour = palette->colour(data_[p]);
	else
		d_colour.set(data_[p], data_[p + 1], data_[p + 2], data_[p + 3]);
	const float alpha = static_cast<float>(colour.a) / 255.0f;

	// Additive blending
	if (properties.blend == BlendType::Add)
	{
		d_colour.set(
			math::clamp(d_colour.r + colour.r * alpha, 0, 255),
			math::clamp(d_colour.g + colour.g * alpha, 0, 255),
			math::clamp(d_colour.b + colour.b * alpha, 0, 255),
			math::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Subtractive blending
	else if (properties.blend == BlendType::Subtract)
	{
		d_colour.set(
			math::clamp(d_colour.r - colour.r * alpha, 0, 255),
			math::clamp(d_colour.g - colour.g * alpha, 0, 255),
			math::clamp(d_colour.b - colour.b * alpha, 0, 255),
			math::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Reverse-Subtractive blending
	else if (properties.blend == BlendType::ReverseSubtract)
	{
		d_colour.set(
			math::clamp((-d_colour.r) + colour.r * alpha, 0, 255),
			math::clamp((-d_colour.g) + colour.g * alpha, 0, 255),
			math::clamp((-d_colour.b) + colour.b * alpha, 0, 255),
			math::clamp(d_colour.a + colour.a, 0, 255));
	}

	// 'Modulate' blending
	else if (properties.blend == BlendType::Modulate)
	{
		d_colour.set(
			math::clamp(colour.r * static_cast<double>(d_colour.r) / 255., 0, 255),
			math::clamp(colour.g * static_cast<double>(d_colour.g) / 255., 0, 255),
			math::clamp(colour.b * static_cast<double>(d_colour.b) / 255., 0, 255),
			math::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Normal blending (or unknown blend type)
	else
	{
		const float inv_alpha = 1.0f - alpha;
		d_colour.set(
			d_colour.r * inv_alpha + colour.r * alpha,
			d_colour.g * inv_alpha + colour.g * alpha,
			d_colour.b * inv_alpha + colour.b * alpha,
			math::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Apply new colour
	if (type_ == Type::PalMask)
	{
		data_[p] = palette->nearestColour(d_colour);
		mask_[p] = d_colour.a;
	}
	else if (type_ == Type::RGBA)
		d_colour.write(data_.data() + p);
	else if (type_ == Type::AlphaMap)
		data_[p] = d_colour.a;

	return true;
}

// -----------------------------------------------------------------------------
// Draws an image on to this image at [x],[y], with blending options set in
// [properties]. [pal_src] is used for the source image, and [pal_dest] is used
// for the destination image, if either is paletted
// -----------------------------------------------------------------------------
bool SImage::drawImage(
	const SImage&    img,
	int              x_pos,
	int              y_pos,
	const DrawProps& properties,
	const Palette*   pal_src,
	const Palette*   pal_dest)
{
	// Check images
	if (!data_.hasData() || !img.data_.hasData())
		return false;

	// Setup palettes
	pal_src  = img.paletteToUse(pal_src);
	pal_dest = paletteToUse(pal_dest);

	// Go through pixels
	const unsigned s_stride = img.stride();
	const uint8_t  s_bpp    = img.bpp();
	unsigned       sp       = 0;
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
			if ((img.type_ == Type::PalMask && img.mask_[sp] == 0)
				|| (img.type_ == Type::AlphaMap && img.data_[sp] == 0)
				|| (img.type_ == Type::RGBA && img.data_[sp + 3] == 0))
			{
				sp += s_bpp;
				continue;
			}

			// Draw pixel
			if (img.type_ == Type::PalMask)
			{
				ColRGBA col = pal_src->colour(img.data_[sp]);
				col.a       = img.mask_[sp];
				drawPixel(x, y, col, properties, pal_dest);
			}
			else if (img.type_ == Type::RGBA)
				drawPixel(
					x,
					y,
					ColRGBA(img.data_[sp], img.data_[sp + 1], img.data_[sp + 2], img.data_[sp + 3]),
					properties,
					pal_dest);
			else if (img.type_ == Type::AlphaMap)
				drawPixel(
					x, y, ColRGBA(img.data_[sp], img.data_[sp], img.data_[sp], img.data_[sp]), properties, pal_dest);

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
bool SImage::colourise(ColRGBA colour, const Palette* pal, int start, int stop)
{
	// Can't do this with alpha maps
	if (type_ == Type::AlphaMap)
		return false;

	// Get palette to use
	auto palette = paletteToUse(pal);

	// Go through all pixels
	const uint8_t bpp = this->bpp();
	ColRGBA       col;
	for (int a = 0; a < width_ * height_ * bpp; a += bpp)
	{
		// Skip colors out of range if desired
		if (type_ == Type::PalMask && start >= 0 && stop >= start && stop < 256)
		{
			if (data_[a] < start || data_[a] > stop)
				continue;
		}

		// Get current pixel colour
		if (type_ == Type::RGBA)
			col.set(data_[a], data_[a + 1], data_[a + 2], data_[a + 3]);
		else
			col.set(palette->colour(data_[a]));

		// Colourise it
		float grey = (col.r * col_greyscale_r + col.g * col_greyscale_g + col.b * col_greyscale_b) / 255.0f;
		if (grey > 1.0)
			grey = 1.0;
		col.r = colour.r * grey;
		col.g = colour.g * grey;
		col.b = colour.b * grey;

		// Set pixel colour
		if (type_ == Type::RGBA)
			col.write(data_.data() + a);
		else
			data_[a] = palette->nearestColour(col);
	}

	// Announce change
	signals_.image_changed();

	return true;
}

// -----------------------------------------------------------------------------
// Tints the image to [colour] by [amount].
// If the image is paletted, each pixel will be set to its nearest matching
// colour in [pal]
// -----------------------------------------------------------------------------
bool SImage::tint(ColRGBA colour, float amount, const Palette* pal, int start, int stop)
{
	// Can't do this with alpha maps
	if (type_ == Type::AlphaMap)
		return false;

	// Get palette to use
	auto palette = paletteToUse(pal);

	// Go through all pixels
	const uint8_t bpp = this->bpp();
	ColRGBA       col;
	for (int a = 0; a < width_ * height_ * bpp; a += bpp)
	{
		// Skip colors out of range if desired
		if (type_ == Type::PalMask && start >= 0 && stop >= start && stop < 256)
		{
			if (data_[a] < start || data_[a] > stop)
				continue;
		}

		// Get current pixel colour
		if (type_ == Type::RGBA)
			col.set(data_[a], data_[a + 1], data_[a + 2], data_[a + 3]);
		else
			col.set(palette->colour(data_[a]));

		// Tint it
		const float inv_amt = 1.0f - amount;
		col.set(
			col.r * inv_amt + colour.r * amount,
			col.g * inv_amt + colour.g * amount,
			col.b * inv_amt + colour.b * amount,
			col.a);

		// Set pixel colour
		if (type_ == Type::RGBA)
			col.write(data_.data() + a);
		else
			data_[a] = palette->nearestColour(col);
	}

	// Announce change
	signals_.image_changed();

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
			const size_t p = i * width_ + x1; // Pixel position
			switch (type_)
			{
			case Type::PalMask: // Transparency is mask[p] == 0
				if (mask_[p])
					opaquefound = true;
				break;
			case Type::RGBA: // Transparency is data[p*4 + 3] == 0
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case Type::AlphaMap: // Transparency is data[p] == 0
				if (data_[p])
					opaquefound = true;
				break;
			default: break;
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
			const size_t p = i * width_ + x2 - 1;
			switch (type_)
			{
			case Type::PalMask:
				if (mask_[p])
					opaquefound = true;
				break;
			case Type::RGBA:
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case Type::AlphaMap:
				if (data_[p])
					opaquefound = true;
				break;
			default: break;
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
			const size_t p = y1 * width_ + i;
			switch (type_)
			{
			case Type::PalMask:
				if (mask_[p])
					opaquefound = true;
				break;
			case Type::RGBA:
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case Type::AlphaMap:
				if (data_[p])
					opaquefound = true;
				break;
			default: break;
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
			const size_t p = (y2 - 1) * width_ + i;
			switch (type_)
			{
			case Type::PalMask:
				if (mask_[p])
					opaquefound = true;
				break;
			case Type::RGBA:
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case Type::AlphaMap:
				if (data_[p])
					opaquefound = true;
				break;
			default: break;
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
	const bool needflip = offset_x_ < width_ / 2;
	const int  extra    = abs((offset_x_ * 2) - width_);

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

// -----------------------------------------------------------------------------
// Generates a checkered pattern of [square_size] squares, alternating between
// [col1] and [col2] (or the nearest colours in the palette)
// -----------------------------------------------------------------------------
void SImage::generateCheckeredPattern(int square_size, const ColRGBA& col1, const ColRGBA& col2, const Palette* pal)
{
	if (type_ == Type::AlphaMap || type_ == Type::Unknown)
		return;

	// Get nearest palette colours for non-RGBA image
	int  pal_c1  = 0;
	int  pal_c2  = 0;
	auto palette = paletteToUse(pal);
	if (type_ == Type::PalMask)
	{
		pal_c1 = palette->nearestColour(col1);
		pal_c2 = palette->nearestColour(col2);
	}

	// Quick and dirty lambda to 'draw' a square
	auto drawSquare = [this, square_size](int x, int y, const ColRGBA& col, int p_index)
	{
		for (auto yp = y; yp < y + square_size; ++yp)
		{
			for (auto xp = x; xp < x + square_size; ++xp)
			{
				if (type_ == Type::PalMask)
					setPixel(xp, yp, p_index);
				else
					setPixel(xp, yp, col);
			}
		}
	};

	// First colour
	int  x       = 0;
	int  y       = 0;
	bool odd_row = false;
	while (y < height_)
	{
		x = odd_row ? square_size : 0;

		while (x < width_)
		{
			drawSquare(x, y, col1, pal_c1);
			x += square_size * 2;
		}

		// Next row
		y += square_size;
		odd_row = !odd_row;
	}

	// Second colour
	y       = 0;
	odd_row = false;
	while (y < height_)
	{
		x = odd_row ? 0 : square_size;

		while (x < width_)
		{
			drawSquare(x, y, col2, pal_c2);
			x += square_size * 2;
		}

		// Next row
		y += square_size;
		odd_row = !odd_row;
	}
}

const Palette* SImage::paletteToUse(const Palette* pal) const
{
	auto palette = palette_ ? palette_.get() : pal;

	if (!palette)
		return &pal_greyscale;

	return palette;
}

Palette* SImage::paletteToUse(Palette* pal) const
{
	auto palette = palette_ ? palette_.get() : pal;

	if (!palette)
		return &pal_greyscale;

	return palette;
}
