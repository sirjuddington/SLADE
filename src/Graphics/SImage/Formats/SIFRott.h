
class SIFRottGfx : public SIFormat
{
public:
	SIFRottGfx(string id = "rott") : SIFormat(id)
	{
		name_        = "ROTT Gfx";
		extension_   = "dat";
		reliability_ = 121;
	}
	~SIFRottGfx() {}

	virtual bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::format("img_rott")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info info(MemChunk& mc, int index)
	{
		SImage::Info info;

		// Read header
		const Graphics::ROTTPatchHeader* header = (const Graphics::ROTTPatchHeader*)mc.data();
		info.width                              = wxINT16_SWAP_ON_BE(header->width);
		info.height                             = wxINT16_SWAP_ON_BE(header->height);
		info.offset_x = wxINT16_SWAP_ON_BE(header->left) + (wxINT16_SWAP_ON_BE(header->origsize) / 2);
		info.offset_y = wxINT16_SWAP_ON_BE(header->top) + wxINT16_SWAP_ON_BE(header->origsize);

		// Setup other info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readRottGfx(SImage& image, MemChunk& data, bool mask)
	{
		// Get image info
		SImage::Info info = this->info(data, 0);

		// Setup variables
		size_t hdr_size   = sizeof(Graphics::ROTTPatchHeader);
		short  translevel = 255;
		if (mask)
		{
			translevel = data.readL16(hdr_size);
			hdr_size += 2;
		}

		// Read column offsets
		vector<uint16_t> col_offsets(info.width);
		const uint16_t*  c_ofs = (const uint16_t*)(data.data() + hdr_size);
		for (int a = 0; a < info.width; a++)
			col_offsets[a] = wxUINT16_SWAP_ON_BE(c_ofs[a]);

		// Create image
		image.create(info);

		// Load data
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);
		for (int c = 0; c < info.width; c++)
		{
			// Get current column offset
			uint16_t col_offset = col_offsets[c];

			// Check column offset is valid
			if (col_offset >= (unsigned)data.size())
				return false;

			// Go to start of column
			const uint8_t* bits = data.data() + col_offset;

			// Read posts
			int counter = 0;
			while (1)
			{
				// Get row offset
				uint8_t row = *bits++;

				if (row == 0xFF) // End of column?
					break;

				// Get no. of pixels
				uint8_t n_pix = *bits++;
				for (uint8_t p = 0; p < n_pix; p++)
				{
					// Get pixel position
					int pos = ((row + p) * info.width + c);

					// Stop if we're outside the image
					if (pos > info.width * info.height)
						break;

					// Stop if for some reason we're outside the gfx data
					if (bits > data.data() + data.size())
						break;

					// Fail if bogus data gives a negative pos (this corrupts the heap!)
					if (pos < 0)
						return false;

					// Write pixel data
					if (mask && *bits == 254)
					{
						img_data[pos] = 0;
						img_mask[pos] = translevel;
					}
					else
					{
						img_data[pos] = *bits++;
						img_mask[pos] = 0xFF;
					}
				}
				if (mask && *bits == 254)
					++bits;
			}
		}

		return true;
	}

	virtual bool readImage(SImage& image, MemChunk& data, int index) { return readRottGfx(image, data, false); }
};

class SIFRottGfxMasked : public SIFRottGfx
{
public:
	SIFRottGfxMasked() : SIFRottGfx("rottmask")
	{
		name_        = "ROTT Masked Gfx";
		reliability_ = 120;
	}
	~SIFRottGfxMasked() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::format("img_rottmask")->isThisFormat(mc))
			return true;
		else
			return false;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) { return readRottGfx(image, data, true); }
};

class SIFRottLbm : public SIFormat
{
public:
	SIFRottLbm() : SIFormat("rottlbm")
	{
		name_        = "ROTT Lbm";
		extension_   = "dat";
		reliability_ = 80;
	}
	~SIFRottLbm() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::format("img_rottlbm")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info info(MemChunk& mc, int index)
	{
		SImage::Info info;

		// Setup info
		info.width       = mc.readL16(0);
		info.height      = mc.readL16(2);
		info.colformat   = SImage::Type::PalMask;
		info.has_palette = true;
		info.format      = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::Info info = this->info(data, index);

		// ROTT source code says: "LIMITATIONS - Only works with 320x200!!!"
		if (info.width != 320 || info.height != 200)
			return false;

		// Build palette
		Palette palette;
		for (size_t c = 0; c < 256; ++c)
		{
			ColRGBA color;
			color.r = data[(c * 3) + 4];
			color.g = data[(c * 3) + 5];
			color.b = data[(c * 3) + 6];
			palette.setColour(c, color);
		}

		// Create image
		image.create(info, &palette);
		uint8_t* img_data = imageData(image);
		image.fillAlpha(255);

		// Create some variables needed for LBM decompression
		const uint8_t* read    = data.data() + 768 + 4;
		const uint8_t* readend = data.data() + data.size();
		uint8_t*       dest    = img_data;
		uint8_t*       destend = img_data + (info.width * info.height);
		uint8_t        code    = 0;
		uint8_t        length  = 0;
		uint8_t        count   = 0;

		// Read image data
		while (read < readend && dest < destend && count < info.width)
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
				length = (code ^ 0xFF) + 2;
				;
				code = *read++;
				memset(dest, code, length);
				dest += length;
			}
			else
				length = 0;

			count += length;
		}

		return true;
	}
};

class SIFRottRaw : public SIFormat
{
public:
	SIFRottRaw() : SIFormat("rottraw")
	{
		name_        = "ROTT Raw";
		extension_   = "dat";
		reliability_ = 101;
	}
	~SIFRottRaw() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::format("img_rottraw")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info info(MemChunk& mc, int index)
	{
		SImage::Info info;

		// Read header
		auto header   = (const Graphics::PatchHeader*)mc.data();
		info.width    = wxINT16_SWAP_ON_BE(header->width);
		info.height   = wxINT16_SWAP_ON_BE(header->height);
		info.offset_x = wxINT16_SWAP_ON_BE(header->left);
		info.offset_y = wxINT16_SWAP_ON_BE(header->top);

		// Set other info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::Info info = this->info(data, index);

		// Create image (swapped width/height because column-major)
		image.create(info.height, info.width, SImage::Type::PalMask);
		image.fillAlpha(255);

		// Read raw pixel data
		data.read(imageData(image), info.width * info.height, 8);

		// Convert from column-major to row-major
		image.rotate(90);
		image.mirror(true);

		return true;
	}
};

class SIFRottPic : public SIFormat
{
public:
	SIFRottPic() : SIFormat("rottpic")
	{
		name_        = "ROTT Picture";
		extension_   = "dat";
		reliability_ = 60;
	}
	~SIFRottPic() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::format("img_rottpic")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info info(MemChunk& mc, int index)
	{
		SImage::Info info;

		// Read dimensions
		info.width  = mc[0] * 4;
		info.height = mc[1];

		// Setup other info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::Info info = this->info(data, index);

		// Check data
		if (data.size() != 4 + info.width * info.height)
			return false;

		// Create image
		image.create(info);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		// Read data
		const uint8_t* entryend = data.data() + data.size() - 2;
		uint8_t*       dataend  = img_data + data.size() - 4;
		const uint8_t* pixel    = data.data() + 2;
		uint8_t*       brush    = img_data;
		while (pixel < entryend)
		{
			*brush = *pixel++;
			brush += 4;
			if (brush >= dataend)
				brush -= data.size() - 5;
		}

		// Create mask (index 255 is transparent)
		for (int a = 0; a < info.width * info.height; a++)
		{
			if (img_data[a] == 255)
				img_mask[a] = 0;
			else
				img_mask[a] = 255;
		}

		return true;
	}
};

class SIFRottWall : public SIFormat
{
public:
	SIFRottWall() : SIFormat("rottwall")
	{
		name_        = "ROTT Flat";
		extension_   = "dat";
		reliability_ = 10;
	}
	~SIFRottWall() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (mc.size() == 4096 || mc.size() == 51200)
			return true;
		else
			return false;
	}

	SImage::Info info(MemChunk& mc, int index)
	{
		SImage::Info info;

		// Always the same thing
		info.width     = mc.size() == 4096 ? 64 : 256;
		info.height    = mc.size() == 4096 ? 64 : 200;
		info.offset_x  = 0;
		info.offset_y  = 0;
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::Info info = this->info(data, index);

		// Create image (swapped width/height because column-major)
		image.create(info.height, info.width, SImage::Type::PalMask);
		image.fillAlpha(255);

		// Read raw pixel data
		data.seek(0, SEEK_SET);
		data.read(imageData(image), info.height * info.width);

		// Convert from column-major to row-major
		image.rotate(90);
		image.mirror(false);

		return true;
	}
};
