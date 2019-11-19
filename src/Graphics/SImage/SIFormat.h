
#ifndef __SIFORMAT_H__
#define __SIFORMAT_H__

#include "SImage.h"

class ArchiveEntry;
class SIFormat
{
protected:
	string	id;
	string	name;
	string	extension;
	uint8_t	reliability;

	// Stuff to access protected image data
	uint8_t*		imageData(SImage& image) { return image.data; }
	uint8_t*		imageMask(SImage& image) { return image.mask; }
	Palette&	imagePalette(SImage& image) { return image.palette; }

	virtual bool	readImage(SImage& image, MemChunk& data, int index) = 0;
	virtual bool	writeImage(SImage& image, MemChunk& data, Palette* pal, int index) { return false; }

public:
	// Conversion options stuff
	enum
	{
	    MASK_NONE = 0,
	    MASK_COLOUR,
	    MASK_ALPHA,
	    MASK_BRIGHTNESS,
	};
	struct convert_options_t
	{
		Palette*	pal_current;
		Palette*	pal_target;
		int				mask_source;
		rgba_t			mask_colour;
		uint8_t			alpha_threshold;
		bool			transparency;
		int				col_format;

		convert_options_t()
		{
			pal_current = pal_target = nullptr;
			mask_source = MASK_ALPHA;
			transparency = true;
			col_format = -1;
			alpha_threshold = 0;
		}
	};

	SIFormat(string id);
	~SIFormat();

	string	getId() { return id; }
	string	getName() { return name; }
	string	getExtension() { return extension; }

	virtual bool	isThisFormat(MemChunk& mc) = 0;

	// Reading
	virtual SImage::info_t	getInfo(MemChunk& mc, int index = 0) = 0;

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
			image.format = this;
			image.imgindex = index;
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
	    NOTWRITABLE,	// Format cannot be written
	    WRITABLE,		// Format can be written
	    CONVERTIBLE,	// Format can be written but a conversion is required
	};
	virtual int		canWrite(SImage& image) { return NOTWRITABLE; }
	virtual bool	canWriteType(SIType type) { return false; }
	virtual bool	convertWritable(SImage& image, convert_options_t opt) { return false; }
	virtual bool	writeOffset(SImage& image, ArchiveEntry* entry, point2_t offset) { return false; }

	bool saveImage(SImage& image, MemChunk& out, Palette* pal = nullptr, int index = 0)
	{
		// Attempt to write image data
		out.seek(0, SEEK_SET);
		bool ok = writeImage(image, out, pal, index);

		// Set format if successful
		if (ok)
			image.format = this;

		return ok;
	}

	static void			initFormats();
	static SIFormat*	getFormat(string name);
	static SIFormat*	determineFormat(MemChunk& mc);
	static SIFormat*	unknownFormat();
	static SIFormat*	rawFormat();
	static SIFormat*	flatFormat();
	static SIFormat*	generalFormat();
	static void			getAllFormats(vector<SIFormat*>& list);
};

#endif//__SIFORMAT_H__
