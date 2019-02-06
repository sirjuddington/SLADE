
class SIFPlanar : public SIFormat
{
public:
	SIFPlanar() : SIFormat("planar", "Planar", "lmp", 240) {}
	~SIFPlanar() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		// Can only go by image size
		if (mc.size() == 153648)
			return true;
		else
			return false;
	}

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Set info (always the same)
		info.width       = 640;
		info.height      = 480;
		info.colformat   = SImage::Type::PalMask;
		info.has_palette = true;
		info.format      = id_;

		return info;
	}


	Writable canWrite(SImage& image) override
	{
		if (!gfx_extraconv)
			return Writable::No;
		// Can write paletted images of size 640x480
		if (image.width() == 640 && image.height() == 480 && image.type() == SImage::Type::PalMask)
			return Writable::Yes;
		// Otherwise it's possible to convert the image as long as it's at least 640x480
		else if (image.width() >= 640 && image.height() >= 480)
			return Writable::Convert;
		// If it wouldn't work, it wouldn't work
		return Writable::No;
	}

	bool canWriteType(SImage::Type type) override
	{
		// Only writable as paletted
		if (type == SImage::Type::PalMask)
			return true;
		else
			return false;
	}

	bool convertWritable(SImage& image, ConvertOptions opt) override
	{
		// First convert image to paletted
		image.convertPaletted(opt.pal_target, opt.pal_current);

		// Now crop the image if it's too large
		if (image.width() > 640 || image.height() > 480)
			image.crop(0, 0, 640, 480);

		return true;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Variables
		Palette palette;
		int     width  = 640;
		int     height = 480;

		union {
			RGBQUAD  color;
			uint32_t quad;
		};
		color.rgbReserved = 0;
		ColRGBA colour(0, 0, 0, 0, -1);

		// Initialize the bitmap palette.
		for (int i = 0; i < 16; ++i)
		{
			color.rgbRed   = data[i * 3 + 0];
			color.rgbGreen = data[i * 3 + 1];
			color.rgbBlue  = data[i * 3 + 2];
			// Convert from 6-bit per component to 8-bit per component.
			quad     = (quad << 2) | ((quad >> 4) & 0x03030303);
			colour.r = color.rgbRed;
			colour.g = color.rgbGreen;
			colour.b = color.rgbBlue;
			palette.setColour(i, colour);
		}
		// Fill the rest of the palette with clones of index 0
		for (int i = 16; i < 256; ++i)
		{
			palette.setColour(i, palette.colour(0));
		}

		// Prepare image data and mask (opaque)
		image.create(640, 480, SImage::Type::PalMask, &palette);
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);
		memset(img_mask, 0xFF, width * height);

		auto           dest = img_data;
		int            y, x;
		const uint8_t *src1, *src2, *src3, *src4;
		size_t         plane_size = width / 8 * height;

		src1 = data.data() + 48;  // 80: 10000000	08: 00001000
		src2 = src1 + plane_size; // 40: 01000000 04: 00000100
		src3 = src2 + plane_size; // 20: 00100000 02: 00000010
		src4 = src3 + plane_size; // 10: 00010000 01: 00000001

		for (y = height; y > 0; --y)
		{
			for (x = width; x > 0; x -= 8)
			{
				dest[0] = ((*src4 & 0x80) >> 4) | ((*src3 & 0x80) >> 5) | ((*src2 & 0x80) >> 6) | ((*src1 & 0x80) >> 7);
				dest[1] = ((*src4 & 0x40) >> 3) | ((*src3 & 0x40) >> 4) | ((*src2 & 0x40) >> 5) | ((*src1 & 0x40) >> 6);
				dest[2] = ((*src4 & 0x20) >> 2) | ((*src3 & 0x20) >> 3) | ((*src2 & 0x20) >> 4) | ((*src1 & 0x20) >> 5);
				dest[3] = ((*src4 & 0x10) >> 1) | ((*src3 & 0x10) >> 2) | ((*src2 & 0x10) >> 3) | ((*src1 & 0x10) >> 4);
				dest[4] = ((*src4 & 0x08)) | ((*src3 & 0x08) >> 1) | ((*src2 & 0x08) >> 2) | ((*src1 & 0x08) >> 3);
				dest[5] = ((*src4 & 0x04) << 1) | ((*src3 & 0x04)) | ((*src2 & 0x04) >> 1) | ((*src1 & 0x04) >> 2);
				dest[6] = ((*src4 & 0x02) << 2) | ((*src3 & 0x02) << 1) | ((*src2 & 0x02)) | ((*src1 & 0x02) >> 1);
				dest[7] = ((*src4 & 0x01) << 3) | ((*src3 & 0x01) << 2) | ((*src2 & 0x01) << 1) | ((*src1 & 0x01));
				dest += 8;
				src1 += 1;
				src2 += 1;
				src3 += 1;
				src4 += 1;
			}
		}

		return true;
	}

	bool writeImage(SImage& image, MemChunk& out, Palette* pal, int index) override
	{
		// Is there really any point to being able to write this format?
		// Answer: yeah, no other tool can do it. :p

		if (!gfx_extraconv)
			return false;

		// Check if data is paletted
		if (image.type() != SImage::Type::PalMask)
		{
			Log::error("Cannot convert truecolour image to planar format - convert to 16-colour first.");
			return false;
		}

		if (image.countColours() > 16)
		{
			Log::error(wxString::Format("Cannot convert to planar format, too many colors (%d)", image.countColours()));
			return false;
		}

		// Check image size
		if (!(image.width() == 640 && image.height() == 480))
		{
			Log::error("Cannot convert to planar format, invalid size (must be 640x480)");
			return false;
		}

		// Get palette to use
		Palette usepal;
		if (image.hasPalette())
			usepal.copyPalette(image.palette());
		else if (pal)
			usepal.copyPalette(pal);

		// Backup current image data (since shrinkPalette remaps the image colours)
		MemChunk backup(640 * 480);
		backup.write(imageData(image), 640 * 480);

		// Make sure all used colors are in the first 16 entries of the palette
		image.shrinkPalette(&usepal);
		// Re-read shrunk palette from image
		usepal.copyPalette(image.palette());

		// Create planar palette
		auto mycolors = new uint8_t[3];
		for (size_t i = 0; i < 16; ++i)
		{
			mycolors[0] = usepal.colour(i).r >> 2;
			mycolors[1] = usepal.colour(i).g >> 2;
			mycolors[2] = usepal.colour(i).b >> 2;
			out.write(mycolors, 3);
		}
		delete[] mycolors;

		// Create bitplanes
		auto planes = new uint8_t[153600];

		uint8_t *pln1, *pln2, *pln3, *pln4, *read;
		size_t   plane_size = 153600 / 4;

		read = imageData(image);
		pln1 = planes;            // 80: 10000000	08: 00001000
		pln2 = pln1 + plane_size; // 40: 01000000 04: 00000100
		pln3 = pln2 + plane_size; // 20: 00100000 02: 00000010
		pln4 = pln3 + plane_size; // 10: 00010000 01: 00000001

		for (int y = 480; y > 0; --y)
		{
			for (int x = 640; x > 0; x -= 8)
			{
				*pln1 =
					((read[0] & 0x01) << 7 | (read[1] & 0x01) << 6 | (read[2] & 0x01) << 5 | (read[3] & 0x01) << 4
					 | (read[4] & 0x01) << 3 | (read[5] & 0x01) << 2 | (read[6] & 0x01) << 1 | (read[7] & 0x01));
				*pln2 =
					((read[0] & 0x02) << 6 | (read[1] & 0x02) << 5 | (read[2] & 0x02) << 4 | (read[3] & 0x02) << 3
					 | (read[4] & 0x02) << 2 | (read[5] & 0x02) << 1 | (read[6] & 0x02) | (read[7] & 0x02) >> 1);
				*pln3 =
					((read[0] & 0x04) << 5 | (read[1] & 0x04) << 4 | (read[2] & 0x04) << 3 | (read[3] & 0x04) << 2
					 | (read[4] & 0x04) << 1 | (read[5] & 0x04) | (read[6] & 0x04) >> 1 | (read[7] & 0x04) >> 2);
				*pln4 =
					((read[0] & 0x08) << 4 | (read[1] & 0x08) << 3 | (read[2] & 0x08) << 2 | (read[3] & 0x08) << 1
					 | (read[4] & 0x08) | (read[5] & 0x08) >> 1 | (read[6] & 0x08) >> 2 | (read[7] & 0x08) >> 3);
				read += 8;
				pln1 += 1;
				pln2 += 1;
				pln3 += 1;
				pln4 += 1;
			}
		}

		// Write image and cleanup
		out.write(planes, 153600);
		delete[] planes;
		backup.seek(0, SEEK_SET);
		backup.read(imageData(image), image.width() * image.height());

		return true;
	}
};

class SIF4BitChunk : public SIFormat
{
public:
	SIF4BitChunk() : SIFormat("4bit", "4-bit", "lmp", 80) {}
	~SIF4BitChunk() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		// Can only detect by size
		return (mc.size() == 32 || mc.size() == 184);
	}

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Check size
		if (mc.size() == 32)
		{
			info.width  = 4;
			info.height = 16;
		}
		else if (mc.size() == 184)
		{
			info.width  = 16;
			info.height = 23;
		}

		// Set other info
		info.colformat = SImage::Type::PalMask;
		info.format    = "4bit";

		return info;
	}

	Writable canWrite(SImage& image) override
	{
		if (!gfx_extraconv)
			return Writable::No;
		// Can write paletted images of size 4x16 or 16x23
		if (image.type() == SImage::Type::PalMask
			&& ((image.width() == 4 && image.height() == 16) || (image.width() == 16 && image.height() == 23)))
			return Writable::Yes;
		// If it wouldn't work, it wouldn't work
		return Writable::No;
	}

	bool canWriteType(SImage::Type type) override
	{
		// Only writable as paletted
		if (type == SImage::Type::PalMask)
			return true;
		else
			return false;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		int width, height;

		// Check size
		if (data.size() == 32)
		{
			width  = 4;
			height = 16;
		}
		else if (data.size() == 184)
		{
			width  = 16;
			height = 23;
		}
		else
			return false;

		image.create(width, height, SImage::Type::PalMask);
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);
		memset(img_mask, 0xFF, width * height);

		for (unsigned i = 0; i < data.size(); ++i)
		{
			img_data[i * 2]     = ((data[i] & 0xF0) >> 4);
			img_data[i * 2 + 1] = (data[i] & 0x0F);
		}

		return true;
	}

	bool writeImage(SImage& image, MemChunk& out, Palette* pal, int index) override
	{
		// Again, don't see much point
		if (!gfx_extraconv)
			return false;

		// Check if data is paletted
		if (image.type() != SImage::Type::PalMask)
		{
			Log::error(
				wxString::Format("Cannot convert truecolour image to 4-bit format - convert to 16-colour first."));
			return false;
		}

		if (image.countColours() > 16)
		{
			Log::error(wxString::Format("Cannot convert to 4-bit format, too many colors (%d)", image.countColours()));
			return false;
		}

		// Check image size
		if (!((image.width() == 4 && image.height() == 16) || (image.width() == 16 && image.height() == 23)))
		{
			Log::error(wxString::Format(
				"No point in converting to 4-bit format, image isn't a valid Hexen size (4x16 or 16x23)"));
			return false;
		}

		// Get palette to use
		Palette usepal;
		if (image.hasPalette())
			usepal.copyPalette(image.palette());
		else if (pal)
			usepal.copyPalette(pal);

		// Backup current image data (since shrinkPalette remaps the image colours)
		MemChunk backup(image.width() * image.height());
		backup.write(imageData(image), image.width() * image.height());

		// Make sure all used colors are in the first 16 entries of the palette
		image.shrinkPalette();

		size_t          filesize = image.width() * image.height() / 2;
		vector<uint8_t> temp(filesize);

		for (int i = 0; i < image.width() * image.height(); i += 2)
			temp[i / 2] = imageData(image)[i] << 4 | imageData(image)[i + 1];

		// Write image and cleanup
		out.write(temp.data(), filesize);
		backup.seek(0, SEEK_SET);
		backup.read(imageData(image), image.width() * image.height());

		return true;
	}
};
