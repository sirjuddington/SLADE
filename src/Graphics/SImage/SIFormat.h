#pragma once

#include "SImage.h"

namespace slade
{
class ArchiveEntry;
class SIFormat
{
public:
	// Conversion options stuff
	enum class Mask
	{
		None = 0,
		Colour,
		Alpha,
		Brightness,
	};
	struct ConvertOptions
	{
		Palette*     pal_current = nullptr;
		Palette*     pal_target  = nullptr;
		Mask         mask_source = Mask::Alpha;
		ColRGBA      mask_colour;
		uint8_t      alpha_threshold = 0;
		bool         transparency    = true;
		SImage::Type col_format      = SImage::Type::Unknown;
	};

	SIFormat(string_view id, string_view name = "Unknown", string_view ext = "dat", uint8_t reliability = 255);
	virtual ~SIFormat() = default;

	const string& id() const { return id_; }
	const string& name() const { return name_; }
	const string& extension() const { return extension_; }

	virtual bool isThisFormat(const MemChunk& mc) = 0;

	// Reading
	virtual SImage::Info info(const MemChunk& mc, int index = 0) = 0;

	bool loadImage(SImage& image, const MemChunk& data, int index = 0)
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
		image.signals_.image_changed();

		return ok;
	}

	// Writing
	enum class Writable
	{
		No,      // Format cannot be written
		Yes,     // Format can be written
		Convert, // Format can be written but a conversion is required
	};
	virtual Writable canWrite(SImage& image) { return Writable::No; }
	virtual bool     canWriteType(SImage::Type type) { return false; }
	virtual bool     convertWritable(SImage& image, ConvertOptions opt) { return false; }
	virtual bool     writeOffset(SImage& image, ArchiveEntry* entry, Vec2i offset) { return false; }

	bool saveImage(SImage& image, MemChunk& out, Palette* pal = nullptr, int index = 0)
	{
		// Attempt to write image data
		out.seek(0, SEEK_SET);
		bool ok = writeImage(image, out, pal, index);

		// Set format if successful
		if (ok)
			image.format_ = this;

		return ok;
	}

	static void      initFormats();
	static SIFormat* getFormat(string_view name);
	static SIFormat* determineFormat(const MemChunk& mc);
	static SIFormat* unknownFormat();
	static SIFormat* rawFormat();
	static SIFormat* flatFormat();
	static SIFormat* generalFormat();
	static void      putAllFormats(vector<SIFormat*>& list);

protected:
	string  id_;
	string  name_        = "Unknown";
	string  extension_   = "dat";
	uint8_t reliability_ = 255;

	// Stuff to access protected image data
	uint8_t* imageData(SImage& image) const { return image.data_.data(); }
	uint8_t* imageMask(SImage& image) const { return image.mask_.data(); }
	Palette& imagePalette(SImage& image) const { return image.palette_; }

	virtual bool readImage(SImage& image, const MemChunk& data, int index) = 0;
	virtual bool writeImage(SImage& image, MemChunk& data, Palette* pal, int index) { return false; }
};
} // namespace slade
