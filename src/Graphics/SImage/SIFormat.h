#pragma once

#include "SImage.h"

class ArchiveEntry;
class SIFormat
{
public:
	// Conversion options stuff
	enum
	{
		MASK_NONE = 0,
		MASK_COLOUR,
		MASK_ALPHA,
		MASK_BRIGHTNESS,
	};
	struct ConvertOptions
	{
		Palette* pal_current;
		Palette* pal_target;
		int      mask_source;
		rgba_t   mask_colour;
		uint8_t  alpha_threshold;
		bool     transparency;
		int      col_format;

		ConvertOptions()
		{
			pal_current = pal_target = nullptr;
			mask_source              = MASK_ALPHA;
			transparency             = true;
			col_format               = -1;
			alpha_threshold          = 0;
		}
	};

	SIFormat(string id);
	~SIFormat();

	string id() { return id_; }
	string name() { return name_; }
	string extension() { return extension_; }

	virtual bool isThisFormat(MemChunk& mc) = 0;

	// Reading
	virtual SImage::info_t info(MemChunk& mc, int index = 0) = 0;

	bool loadImage(SImage& image, MemChunk& data, int index = 0)
	{
		// Check format
		if (!isThisFormat(data))
			return false;

		// Attempt to read image data
		bool ok = readImage(image, data, index);

		// Set image properties if successful
		if (ok)
		{
			image.format_   = this;
			image.imgindex_ = index;
		}
		else
			image.clear();

		// Announce
		image.announce("image_changed");

		return ok;
	}

	// Writing
	enum
	{
		NOTWRITABLE, // Format cannot be written
		WRITABLE,    // Format can be written
		CONVERTIBLE, // Format can be written but a conversion is required
	};
	virtual int  canWrite(SImage& image) { return NOTWRITABLE; }
	virtual bool canWriteType(SIType type) { return false; }
	virtual bool convertWritable(SImage& image, ConvertOptions opt) { return false; }
	virtual bool writeOffset(SImage& image, ArchiveEntry* entry, point2_t offset) { return false; }

	bool saveImage(SImage& image, MemChunk& out, Palette* pal = nullptr, int index = 0)
	{
		// Attempt to write image data
		bool ok = writeImage(image, out, pal, index);

		// Set format if successful
		if (ok)
			image.format_ = this;

		return ok;
	}

	static void      initFormats();
	static SIFormat* getFormat(string name);
	static SIFormat* determineFormat(MemChunk& mc);
	static SIFormat* unknownFormat();
	static SIFormat* rawFormat();
	static SIFormat* flatFormat();
	static SIFormat* generalFormat();
	static void      putAllFormats(vector<SIFormat*>& list);

protected:
	string  id_;
	string  name_;
	string  extension_;
	uint8_t reliability_;

	// Stuff to access protected image data
	uint8_t* imageData(SImage& image) { return image.data_; }
	uint8_t* imageMask(SImage& image) { return image.mask_; }
	Palette& imagePalette(SImage& image) { return image.palette_; }

	virtual bool readImage(SImage& image, MemChunk& data, int index) = 0;
	virtual bool writeImage(SImage& image, MemChunk& data, Palette* pal, int index) { return false; }
};
