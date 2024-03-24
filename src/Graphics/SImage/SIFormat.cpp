
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    SIFormat.cpp
// Description: Base class for SImage format system
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
#undef BOOL
#include "Archive/ArchiveEntry.h"
#include "Archive/EntryType/EntryDataFormat.h"
#include "Graphics/Palette/Palette.h"
#include "SIFormat.h"
#include <FreeImage.h>

#ifndef _WIN32
#undef _WINDOWS_ // Undefine _WINDOWS_ that has been defined by FreeImage
#endif

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
vector<SIFormat*> simage_formats;
SIFormat*         sif_raw     = nullptr;
SIFormat*         sif_flat    = nullptr;
SIFormat*         sif_general = nullptr;
SIFormat*         sif_unknown = nullptr;
} // namespace


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, gfx_extraconv)


// -----------------------------------------------------------------------------
//
// SIF* Classes
//
// -----------------------------------------------------------------------------
#include "Formats/SIFDoom.h"
#include "Formats/SIFHexen.h"
#include "Formats/SIFImages.h"
#include "Formats/SIFJedi.h"
#include "Formats/SIFOther.h"
#include "Formats/SIFQuake.h"
#include "Formats/SIFRott.h"
#include "Formats/SIFZDoom.h"


// -----------------------------------------------------------------------------
// SIFUnknown Class
//
// 'Unknown' format
// -----------------------------------------------------------------------------
class SIFUnknown : public SIFormat
{
protected:
	bool readImage(SImage& image, const MemChunk& data, int index) override { return false; }

public:
	SIFUnknown() : SIFormat("unknown") { reliability_ = 0; }
	~SIFUnknown() override = default;

	bool         isThisFormat(const MemChunk& mc) override { return false; }
	SImage::Info info(const MemChunk& mc, int index) override { return {}; }
};


// -----------------------------------------------------------------------------
// SIFGeneralImage Class
//
// General image format is a special case, only try if no other formats are
// detected
// -----------------------------------------------------------------------------
class SIFGeneralImage : public SIFormat
{
public:
	SIFGeneralImage() : SIFormat("image", "Image", "dat") {}
	~SIFGeneralImage() override = default;

	bool isThisFormat(const MemChunk& mc) override
	{
		auto mem = FreeImage_OpenMemory(const_cast<BYTE*>(mc.data()), mc.size());
		auto fif = FreeImage_GetFileTypeFromMemory(mem, 0);
		FreeImage_CloseMemory(mem);
		return fif != FIF_UNKNOWN;
	}

	SImage::Info info(const MemChunk& mc, int index) override
	{
		SImage::Info info;
		getFIInfo(mc, info);
		return info;
	}

protected:
	bool readImage(SImage& image, const MemChunk& data, int index) override
	{
		// Get image info
		SImage::Info info;
		auto         bm = getFIInfo(data, info);

		// Check it created/read ok
		if (!bm)
		{
			global::error = "Unable to read image data (unsupported format?)";
			return false;
		}

		// Get image palette if it exists
		auto    bm_pal = FreeImage_GetPalette(bm);
		Palette palette;
		if (bm_pal)
		{
			int a = 0;
			int b = FreeImage_GetColorsUsed(bm);
			if (b > 256)
				b = 256;
			for (; a < b; a++)
				palette.setColour(a, ColRGBA(bm_pal[a].rgbRed, bm_pal[a].rgbGreen, bm_pal[a].rgbBlue, 255));
		}

		// Create image
		if (info.has_palette)
			image.create(info, &palette);
		else
			image.create(info);
		auto img_data = imageData(image);

		// Convert to 32bpp & flip vertically
		auto rgba = FreeImage_ConvertTo32Bits(bm);
		if (!rgba)
		{
			log::error("FreeImage_ConvertTo32Bits failed for image data");
			global::error = "Error reading PNG data";
			return false;
		}
		FreeImage_FlipVertical(rgba);

		// Load raw RGBA data
		auto bits_rgba = FreeImage_GetBits(rgba);
		int  c         = 0;
		for (int a = 0; a < info.width * info.height; a++)
		{
			img_data[c++] = bits_rgba[a * 4 + 2]; // Red
			img_data[c++] = bits_rgba[a * 4 + 1]; // Green
			img_data[c++] = bits_rgba[a * 4];     // Blue
			img_data[c++] = bits_rgba[a * 4 + 3]; // Alpha
		}

		// Free memory
		FreeImage_Unload(rgba);
		FreeImage_Unload(bm);

		return true;
	}

	bool writeImage(SImage& image, MemChunk& out, Palette* pal, int index) override { return false; }

private:
	FIBITMAP* getFIInfo(const MemChunk& data, SImage::Info& info) const
	{
		// Get FreeImage bitmap info from entry data
		auto mem = FreeImage_OpenMemory(const_cast<BYTE*>(data.data()), data.size());
		auto fif = FreeImage_GetFileTypeFromMemory(mem, 0);
		auto bm  = FreeImage_LoadFromMemory(fif, mem, 0);
		FreeImage_CloseMemory(mem);

		// Check it created/read ok
		if (!bm)
			return nullptr;

		// Get info from image
		info.width     = FreeImage_GetWidth(bm);
		info.height    = FreeImage_GetHeight(bm);
		info.colformat = SImage::Type::RGBA; // Generic images always converted to RGBA on loading
		info.format    = id_;

		// Check if palette supplied
		if (FreeImage_GetColorsUsed(bm) > 0)
			info.has_palette = true;

		return bm;
	}
};

// Define valid raw flat sizes
uint32_t valid_flat_size[][3] = {
	{ 2, 2, 0 },       // lol Heretic F_SKY1
	{ 10, 12, 0 },     // gnum format
	{ 16, 16, 1 },     // |
	{ 32, 32, 1 },     // |
	{ 32, 64, 0 },     // Strife startup sprite
	{ 48, 48, 0 },     // |
	{ 64, 64, 1 },     // standard flat size
	{ 64, 65, 0 },     // Heretic flat size variant
	{ 64, 128, 0 },    // Hexen flat size variant
	{ 80, 50, 1 },     // SRB2 fade mask size 1
	{ 128, 128, 1 },   // |
	{ 160, 100, 1 },   // SRB2 fade mask size 2
	{ 256, 34, 1 },    // SRB2 colormap
	{ 256, 66, 0 },    // Blake Stone colormap
	{ 256, 200, 0 },   // Rise of the Triad sky
	{ 256, 256, 1 },   // hires flat size
	{ 320, 200, 0 },   // full screen format
	{ 512, 512, 1 },   // hires flat size
	{ 640, 400, 1 },   // SRB2 fade mask size 4
	{ 1024, 1024, 1 }, // hires flat size
	{ 2048, 2048, 1 }, // super hires flat size (SRB2)
	{ 4096, 4096, 1 }, // |
};
uint32_t n_valid_flat_sizes = 22;


// -----------------------------------------------------------------------------
// SIFRaw Class
//
// Raw format is a special case - not detectable
// -----------------------------------------------------------------------------
class SIFRaw : public SIFormat
{
public:
	SIFRaw(string_view id = "raw") : SIFormat(id, "Raw", "dat") {}
	~SIFRaw() override = default;

	bool isThisFormat(const MemChunk& mc) override
	{
		// Just check the size
		return validSize(mc.size());
	}

	SImage::Info info(const MemChunk& mc, int index) override
	{
		SImage::Info info;
		unsigned     size = mc.size();

		// Determine dimensions
		bool valid_size = false;
		for (uint32_t a = 0; a < n_valid_flat_sizes; a++)
		{
			uint32_t w = valid_flat_size[a][0];
			uint32_t h = valid_flat_size[a][1];

			if (size == w * h || size - 4 == w * h)
			{
				info.width  = w;
				info.height = h;
				valid_size  = true;
				break;
			}
		}
		if (size == 8776) // Inkworks and its signature at the end of COLORMAPS
		{
			size = 8704;
		}
		if (!valid_size)
		{
			if (size % 320 == 0) // This should handle any custom AUTOPAGE
			{
				info.width  = 320;
				info.height = size / 320;
			}
			else if (size % 256 == 0) // This allows display of COLORMAPS
			{
				info.width  = 256;
				info.height = size / 256;
			}
		}

		// Setup other info
		info.colformat = SImage::Type::PalMask;
		info.format    = "raw";

		return info;
	}

	bool canWriteType(SImage::Type type) override
	{
		// Raw format only supports paletted images
		return type == SImage::Type::PalMask;
	}

protected:
	bool validSize(unsigned size) const
	{
		for (unsigned a = 0; a < n_valid_flat_sizes; a++)
		{
			if (valid_flat_size[a][0] * valid_flat_size[a][1] == size)
				return true;
		}

		// COLORMAP size
		if (size == 8776)
			size = 8704; // Ignore inkworks signature
		if (size % 256 == 0)
			return true;

		// AUTOPAGE size
		if (size % 320 == 0)
			return true;

		return false;
	}

	bool validSize(int width, int height) const
	{
		for (unsigned a = 0; a < n_valid_flat_sizes; a++)
		{
			if (static_cast<int>(valid_flat_size[a][0]) == width && static_cast<int>(valid_flat_size[a][1]) == height
				&& (valid_flat_size[a][2] == 1 || gfx_extraconv))
				return true;
		}

		// COLORMAP size special case
		if (width == 256 && height >= 32 && height <= 34)
			return true;

		// Fullscreen gfx special case
		if (width == 320) // autopage, too && height == 200)
			return true;

		return false;
	}

	bool readImage(SImage& image, const MemChunk& data, int index) override
	{
		// Get info
		auto inf = info(data, index);

		// Create image from data
		image.create(inf.width, inf.height, SImage::Type::PalMask);
		data.read(imageData(image), inf.width * inf.height, 0);
		image.fillAlpha(255);

		return true;
	}
};


// -----------------------------------------------------------------------------
//
// SIFRawFlat Class
//
// -----------------------------------------------------------------------------
class SIFRawFlat : public SIFRaw
{
public:
	SIFRawFlat() : SIFRaw("raw_flat")
	{
		name_      = "Doom Flat";
		extension_ = "lmp";
	}
	~SIFRawFlat() override = default;

	Writable canWrite(SImage& image) override
	{
		// If it's the correct size and colour format, it's writable
		int width  = image.width();
		int height = image.height();

		// Shouldn't happen but...
		if (width < 0 || height < 0)
			return Writable::No;

		if (image.type() == SImage::Type::PalMask && validSize(image.width(), image.height()))
			return Writable::Yes;

		// Otherwise, check if it can be cropped to a valid size
		for (unsigned a = 0; a < n_valid_flat_sizes; a++)
			if ((static_cast<unsigned>(width) >= valid_flat_size[a][0]
				 && static_cast<unsigned>(height) >= valid_flat_size[a][1] && valid_flat_size[a][2] == 1)
				|| gfx_extraconv)
				return Writable::Convert;

		return Writable::No;
	}

	bool convertWritable(SImage& image, ConvertOptions opt) override
	{
		// Firstly, make image paletted
		image.convertPaletted(opt.pal_target, opt.pal_current);

		// Secondly, remove any alpha information
		image.fillAlpha(255);

		// Quick hack for COLORMAP size
		// TODO: Remove me when a proper COLORMAP editor is implemented
		if (image.width() == 256 && image.height() >= 32 && image.height() <= 34)
			return true;

		// Check for fullscreen/autopage size
		if (image.width() == 320)
			return true;

		// And finally, find a suitable flat size and crop to that size
		int width  = 0;
		int height = 0;

		for (unsigned a = 1; a < n_valid_flat_sizes; a++)
		{
			bool writable = (valid_flat_size[a][2] == 1 || gfx_extraconv);

			// Check for exact match (no need to crop)
			if (image.width() == static_cast<int>(valid_flat_size[a][0])
				&& image.height() == static_cast<int>(valid_flat_size[a][1]) && writable)
				return true;

			// If the flat will fit within this size, crop to the previous size
			// (this works because flat sizes list is in size-order)
			if (image.width() <= static_cast<int>(valid_flat_size[a][0])
				&& image.height() <= static_cast<int>(valid_flat_size[a][1]) && width > 0 && height > 0)
			{
				image.crop(0, 0, width, height);
				return true;
			}

			// Save 'previous' valid size
			if (writable)
			{
				width  = valid_flat_size[a][0];
				height = valid_flat_size[a][1];
			}
		}

		return false;
	}

protected:
	bool writeImage(SImage& image, MemChunk& data, Palette* pal, int index) override
	{
		// Can't write if RGBA
		if (image.type() == SImage::Type::RGBA)
			return false;

		// Check size
		if (!validSize(image.width(), image.height()))
			return false;

		// Just dump image data to memchunk
		data.clear();
		data.write(imageData(image), image.width() * image.height());

		return true;
	}
};


// -----------------------------------------------------------------------------
//
// SIFormat Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SIFormat class constructor
// ----------------------------------------------------------------------------
SIFormat::SIFormat(string_view id, string_view name, string_view ext, uint8_t reliability) :
	id_{ id },
	name_{ name },
	extension_{ ext },
	reliability_{ reliability }
{
	// Add to list of formats
	simage_formats.push_back(this);
}


// -----------------------------------------------------------------------------
//
// SIFormat Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initialises all SIFormats
// -----------------------------------------------------------------------------
void SIFormat::initFormats()
{
	// Non-detectable formats
	sif_unknown = new SIFUnknown();
	sif_raw     = new SIFRaw();
	sif_flat    = new SIFRawFlat();
	sif_general = new SIFGeneralImage();
	simage_formats.clear(); // Remove previously created formats from the list

	// Image formats
	new SIFPng();

	// Doom formats
	new SIFDoomGfx();
	new SIFDoomBetaGfx();
	new SIFDoomAlphaGfx();
	new SIFDoomArah();
	new SIFDoomSnea();
	new SIFDoomJaguar();
	new SIFDoomJaguarColMajor();
	new SIFDoomPSX();

	// Hexen formats
	new SIFPlanar();
	new SIF4BitChunk();

	// ZDoom formats
	new SIFImgz();

	// Quake series formats
	new SIFQuakeGfx();
	new SIFQuakeSprite();
	new SIFQuakeTex();
	new SIFQuake2Wal();

	// ROTT formats
	new SIFRottGfx();
	new SIFRottGfxMasked();
	new SIFRottLbm();
	new SIFRottRaw();
	new SIFRottPic();
	new SIFRottWall();

	// Jedi Engine (Dark Forces) formats
#if 0
	new SIFJediBM();
	new SIFJediFME();
	new SIFJediWAX();
#endif

	// Other game formats
	new SIFHalfLifeTex();
	new SIFSCSprite();
	new SIFSCWall();
	new SIFSCGfx();
	new SIFAnaMip();
	new SIFBuildTile();
	new SIFHeretic2M8();
	new SIFHeretic2M32();
	new SIFWolfPic();
	new SIFWolfSprite();
}

// -----------------------------------------------------------------------------
// Returns the format [id]
// -----------------------------------------------------------------------------
SIFormat* SIFormat::getFormat(string_view id)
{
	// Check for special types
	if (id == "raw")
		return sif_raw;
	else if (id == "raw_flat")
		return sif_flat;
	else if (id == "image")
		return sif_general;

	// Search for format matching id
	for (auto& simage_format : simage_formats)
	{
		if (simage_format->id_ == id)
			return simage_format;
	}

	// Not found, return unknown format
	return sif_unknown;
}

// -----------------------------------------------------------------------------
// Determines the format of the image data in [mc]
// -----------------------------------------------------------------------------
SIFormat* SIFormat::determineFormat(const MemChunk& mc)
{
	// Go through all registered formats
	SIFormat* format = sif_unknown;
	for (auto& simage_format : simage_formats)
	{
		// Don't bother checking if the format is less reliable
		if (simage_format->reliability_ < format->reliability_)
			continue;

		// Check if data matches format
		if (simage_format->isThisFormat(mc))
			format = simage_format;

		// Stop if format detected is 100% reliable
		if (format->reliability_ == 255)
			break;
	}

	// Not found, return unknown format
	return format;
}

// -----------------------------------------------------------------------------
// Returns the 'unknown' image format
// -----------------------------------------------------------------------------
SIFormat* SIFormat::unknownFormat()
{
	return sif_unknown;
}

// -----------------------------------------------------------------------------
// Returns the raw image format
// -----------------------------------------------------------------------------
SIFormat* SIFormat::rawFormat()
{
	return sif_raw;
}

// -----------------------------------------------------------------------------
// Returns the raw/flat image format
// -----------------------------------------------------------------------------
SIFormat* SIFormat::flatFormat()
{
	return sif_flat;
}

// -----------------------------------------------------------------------------
// Returns the 'general' image format
// -----------------------------------------------------------------------------
SIFormat* SIFormat::generalFormat()
{
	return sif_general;
}

// -----------------------------------------------------------------------------
// Adds all image formats to [list]
// -----------------------------------------------------------------------------
void SIFormat::putAllFormats(vector<SIFormat*>& list)
{
	// Clear list
	list.clear();

	// Add formats
	for (auto simage_format : simage_formats)
		list.push_back(simage_format);

	// Add special formats
	list.push_back(sif_general);
	list.push_back(sif_raw);
	list.push_back(sif_flat);
}
