
#include "Main.h"
#undef BOOL
#include <FreeImage.h>
#include "Misc.h"
#include "EntryType.h"

EXTERN_CVAR(Bool, gfx_extraconv)

#include "SIFormat.h"
#include "SIFImages.h"
#include "SIFDoom.h"
#include "SIFHexen.h"
#include "SIFZDoom.h"
#include "SIFQuake.h"
#include "SIFOther.h"
#include "SIFRott.h"
#include "SIFJedi.h"

vector<SIFormat*>	simage_formats;
SIFormat*			sif_raw = NULL;
SIFormat*			sif_flat = NULL;
SIFormat*			sif_general = NULL;
SIFormat*			sif_unknown = NULL;


// 'Unknown' format
class SIFUnknown : public SIFormat
{
protected:
	bool readImage(SImage& image, MemChunk& data, int index) { return false; }

public:
	SIFUnknown() : SIFormat("unknown") { reliability = 0; }
	~SIFUnknown() {}

	bool			isThisFormat(MemChunk& mc) { return false; }
	SImage::info_t	getInfo(MemChunk& mc, int index) { return SImage::info_t(); }
};

// General image format is a special case, only try if no other formats detected
class SIFGeneralImage : public SIFormat
{
private:
	FIBITMAP* getFIInfo(MemChunk& data, SImage::info_t& info)
	{
		// Get FreeImage bitmap info from entry data
		FIMEMORY* mem = FreeImage_OpenMemory((BYTE*)data.getData(), data.getSize());
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem, 0);
		FIBITMAP* bm = FreeImage_LoadFromMemory(fif, mem, 0);
		FreeImage_CloseMemory(mem);

		// Check it created/read ok
		if (!bm)
			return NULL;

		// Get info from image
		info.width = FreeImage_GetWidth(bm);
		info.height = FreeImage_GetHeight(bm);
		info.colformat = RGBA;	// Generic images always converted to RGBA on loading
		info.format = id;

		// Check if palette supplied
		if (FreeImage_GetColorsUsed(bm) > 0)
			info.has_palette = true;

		return bm;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::info_t info;
		FIBITMAP* bm = getFIInfo(data, info);

		// Check it created/read ok
		if (!bm)
		{
			Global::error = "Unable to read image data (unsupported format?)";
			return false;
		}

		// Get image palette if it exists
		RGBQUAD* bm_pal = FreeImage_GetPalette(bm);
		Palette8bit palette;
		if (bm_pal)
		{
			int a = 0;
			int b = FreeImage_GetColorsUsed(bm);
			if (b > 256)
				b = 256;
			for (; a < b; a++)
				palette.setColour(a, rgba_t(bm_pal[a].rgbRed, bm_pal[a].rgbGreen, bm_pal[a].rgbBlue, 255));
		}

		// Create image
		if (info.has_palette)
			image.create(info, &palette);
		else
			image.create(info);
		uint8_t* img_data = imageData(image);

		// Convert to 32bpp & flip vertically
		FIBITMAP* rgba = FreeImage_ConvertTo32Bits(bm);
		FreeImage_FlipVertical(rgba);

		// Load raw RGBA data
		uint8_t* bits_rgba = FreeImage_GetBits(rgba);
		int c = 0;
		for (int a = 0; a < info.width * info.height; a++)
		{
			img_data[c++] = bits_rgba[a * 4 + 2];	// Red
			img_data[c++] = bits_rgba[a * 4 + 1];	// Green
			img_data[c++] = bits_rgba[a * 4];		// Blue
			img_data[c++] = bits_rgba[a * 4 + 3];	// Alpha
		}

		// Free memory
		FreeImage_Unload(rgba);
		FreeImage_Unload(bm);

		return true;
	}

	bool writeImage(SImage& image, MemChunk& out, Palette8bit* pal, int index)
	{
		return false;
	}

public:
	SIFGeneralImage() : SIFormat("image")
	{
		name = "Image";
		extension = "dat";
	}
	~SIFGeneralImage() {}

	bool isThisFormat(MemChunk& mc)
	{
		FIMEMORY* mem = FreeImage_OpenMemory((BYTE*)mc.getData(), mc.getSize());
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem, 0);
		FreeImage_CloseMemory(mem);
		if (fif == FIF_UNKNOWN)
			return false;
		else
			return true;
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t info;

		getFIInfo(mc, info);

		return info;
	}
};

// Define valid raw flat sizes
uint32_t valid_flat_size[][3] =
{
	{	2,	 2,	0 },	// lol Heretic F_SKY1
	{  10,  12,	0 },	// gnum format
	{  16,  16,	0 },	// |
	{  32,  64,	0 },	// Strife startup sprite
	{  48,  48,	0 },	// |
	{  64,  64,	1 },	// standard flat size
	{  64,	65,	0 },	// Heretic flat size variant
	{  64, 128,	0 },	// Hexen flat size variant
	{ 128, 128,	1 },	// |
	{ 256,  66,	0 },	// Blake Stone colormap
	{ 256, 200,	0 },	// Rise of the Triad sky
	{ 256, 256,	1 },	// hires flat size
	{ 320, 200,	0 },	// full screen format
	{ 512, 512,	1 },	// hires flat size
	{1024,1024,	1 },	// |
	{2048,2048,	1 },	// super hires flat size (SRB2)
	{4096,4096,	1 },	// |
};
uint32_t	n_valid_flat_sizes = 17;

// Raw format is a special case - not detectable
class SIFRaw : public SIFormat
{
protected:
	bool validSize(unsigned size)
	{
		for (unsigned a = 0; a < n_valid_flat_sizes; a++)
		{
			if (valid_flat_size[a][0] * valid_flat_size[a][1] == size)
				return true;
		}

		// COLORMAP size
		if (size == 8776) size = 8704;	// Ignore inkworks signature
		if (size % 256 == 0)
			return true;

		// AUTOPAGE size
		if (size % 320 == 0)
			return true;

		return false;
	}

	bool validSize(int width, int height)
	{
		for (unsigned a = 0; a < n_valid_flat_sizes; a++)
		{
			if (valid_flat_size[a][0] == width && valid_flat_size[a][1] == height
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

	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get info
		SImage::info_t info = getInfo(data, index);

		// Create image from data
		image.create(info.width, info.height, PALMASK);
		data.read(imageData(image), info.width*info.height, 0);
		image.fillAlpha(255);

		return true;
	}

public:
	SIFRaw(string id = "raw") : SIFormat(id)
	{
		this->name = "Raw";
		this->extension = "dat";
	}
	~SIFRaw() {}

	bool isThisFormat(MemChunk& mc)
	{
		// Just check the size
		return validSize(mc.getSize());
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t info;
		unsigned size = mc.getSize();

		// Determine dimensions
		bool valid_size = false;
		for (uint32_t a = 0; a < n_valid_flat_sizes; a++)
		{
			uint32_t w = valid_flat_size[a][0];
			uint32_t h = valid_flat_size[a][1];

			if (size == w * h || size - 4 == w * h)
			{
				info.width = w;
				info.height = h;
				valid_size = true;
				break;
			}
		}
		if (size == 8776)   // Inkworks and its signature at the end of COLORMAPS
		{
			size = 8704;
		}
		if (!valid_size)
		{
			if (size % 320 == 0)	 	// This should handle any custom AUTOPAGE
			{
				info.width = 320;
				info.height = size/320;
			}
			else if (size % 256 == 0)   // This allows display of COLORMAPS
			{
				info.width = 256;
				info.height = size/256;
			}
		}

		// Setup other info
		info.colformat = PALMASK;
		info.format = "raw";

		return info;
	}

	bool canWriteType(SIType type)
	{
		// Raw format only supports paletted images
		if (type == PALMASK)
			return true;
		else
			return false;
	}
};

class SIFRawFlat : public SIFRaw
{
protected:
	bool writeImage(SImage& image, MemChunk& data, Palette8bit* pal, int index)
	{
		// Can't write if RGBA
		if (image.getType() == RGBA)
			return false;

		// Check size
		if (!validSize(image.getWidth(), image.getHeight()))
			return false;

		// Just dump image data to memchunk
		data.clear();
		data.write(imageData(image), image.getWidth()*image.getHeight());

		return true;
	}

public:
	SIFRawFlat() : SIFRaw("raw_flat")
	{
		this->name = "Doom Flat";
		this->extension = "lmp";
	}
	~SIFRawFlat() {}

	int canWrite(SImage& image)
	{
		// If it's the correct size and colour format, it's writable
		if (image.getType() == PALMASK &&
		        validSize(image.getWidth(), image.getHeight()))
			return WRITABLE;

		// Otherwise, it can be converted via palettising and cropping
		return CONVERTIBLE;
	}

	bool convertWritable(SImage& image, convert_options_t opt)
	{
		// Firstly, make image paletted
		image.convertPaletted(opt.pal_target, opt.pal_current);

		// Secondly, remove any alpha information
		image.fillAlpha(255);

		// Quick hack for COLORMAP size
		// TODO: Remove me when a proper COLORMAP editor is implemented
		if (image.getWidth() == 256 && image.getHeight() >= 32 && image.getHeight() <= 34)
			return true;

		// Check for fullscreen/autopage size
		if (image.getWidth() == 320)
			return true;

		// And finally, find a suitable flat size and crop to that size
		int width = 0;
		int height = 0;

		for (unsigned a = 1; a < n_valid_flat_sizes; a++)
		{
			// Ignore non-writable flat sizes
			if (valid_flat_size[a][2] == 0)
				continue;

			// Check for exact match (no need to crop)
			if (image.getWidth() == valid_flat_size[a][0] &&
			        image.getHeight() == valid_flat_size[a][1])
				return true;

			// If the flat will fit within this size, crop to the previous size
			// (this works because flat sizes list is in size-order)
			if (image.getWidth() <= (int)valid_flat_size[a][0] &&
			        image.getHeight() <= (int)valid_flat_size[a][1] &&
			        width > 0 && height > 0)
			{
				image.crop(0, 0, width, height);
				return true;
			}

			// Save 'previous' valid size
			width = valid_flat_size[a][0];
			height = valid_flat_size[a][1];
		}

		return false;
	}
};

SIFormat::SIFormat(string id)
{
	// Init variables
	this->id = id;
	this->name = "Unknown";
	this->extension = "dat";
	this->reliability = 255;

	// Add to list of formats
	simage_formats.push_back(this);
}

SIFormat::~SIFormat()
{
}



void SIFormat::initFormats()
{
	// Non-detectable formats
	sif_unknown = new SIFUnknown();
	sif_raw = new SIFRaw();
	sif_flat = new SIFRawFlat();
	sif_general = new SIFGeneralImage();
	simage_formats.clear();	// Remove previously created formats from the list

	// Image formats
	new SIFPng();

	// Doom formats
	new SIFDoomGfx();
	new SIFDoomBetaGfx();
	new SIFDoomAlphaGfx();
	new SIFDoomArah();
	new SIFDoomSnea();
	new SIFDoomJaguar();
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

SIFormat* SIFormat::getFormat(string id)
{
	// Check for special types
	if (id == "raw")
		return sif_raw;
	else if (id == "raw_flat")
		return sif_flat;
	else if (id == "image")
		return sif_general;

	// Search for format matching id
	for (unsigned a = 0; a < simage_formats.size(); a++)
	{
		if (simage_formats[a]->id == id)
			return simage_formats[a];
	}

	// Not found, return unknown format
	return sif_unknown;
}

SIFormat* SIFormat::determineFormat(MemChunk& mc)
{
	// Go through all registered formats
	SIFormat* format = sif_unknown;
	for (unsigned a = 0; a < simage_formats.size(); a++)
	{
		// Don't bother checking if the format is less reliable
		if (simage_formats[a]->reliability < format->reliability)
			continue;

		// Check if data matches format
		if (simage_formats[a]->isThisFormat(mc))
			format = simage_formats[a];

		// Stop if format detected is 100% reliable
		if (format->reliability == 255)
			break;
	}

	// Not found, return unknown format
	return format;
}

SIFormat* SIFormat::unknownFormat()
{
	return sif_unknown;
}

SIFormat* SIFormat::rawFormat()
{
	return sif_raw;
}

SIFormat* SIFormat::flatFormat()
{
	return sif_flat;
}

SIFormat* SIFormat::generalFormat()
{
	return sif_general;
}

void SIFormat::getAllFormats(vector<SIFormat*>& list)
{
	// Clear list
	list.clear();

	// Add formats
	for (unsigned a = 0; a < simage_formats.size(); a++)
		list.push_back(simage_formats[a]);

	// Add special formats
	list.push_back(sif_general);
	list.push_back(sif_raw);
	list.push_back(sif_flat);
}
