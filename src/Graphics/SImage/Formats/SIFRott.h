
class SIFRottGfx : public SIFormat
{
protected:
	bool readRottGfx(SImage& image, MemChunk& data, bool mask)
	{
		// Get image info
		SImage::info_t info = getInfo(data, 0);

		// Setup variables
		size_t hdr_size = sizeof(rottpatch_header_t);
		short translevel = 255;
		if (mask) 
		{
			translevel = READ_L16(data, hdr_size);
			hdr_size += 2;
		}

		// Read column offsets
		vector<uint16_t> col_offsets(info.width);
		const uint16_t* c_ofs = (const uint16_t*)(data.getData() + hdr_size);
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
			if (col_offset >= (unsigned)data.getSize())
				return false;

			// Go to start of column
			const uint8_t* bits = data.getData() + col_offset;

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
					int pos = ((row + p)*info.width + c);

					// Stop if we're outside the image
					if (pos > info.width*info.height)
						break;

					// Stop if for some reason we're outside the gfx data
					if (bits > data.getData() + data.getSize())
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

	virtual bool readImage(SImage& image, MemChunk& data, int index)
	{
		return readRottGfx(image, data, false);
	}

public:
	SIFRottGfx(string id = "rott") : SIFormat(id)
	{
		name = "ROTT Gfx";
		extension = "dat";
		reliability = 121;
	}
	~SIFRottGfx() {}

	virtual bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::getFormat("img_rott")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t info;

		// Read header
		const rottpatch_header_t* header = (const rottpatch_header_t*)mc.getData();
		info.width = wxINT16_SWAP_ON_BE(header->width);
		info.height = wxINT16_SWAP_ON_BE(header->height);
		info.offset_x = wxINT16_SWAP_ON_BE(header->left) + (wxINT16_SWAP_ON_BE(header->origsize)/2);
		info.offset_y = wxINT16_SWAP_ON_BE(header->top) + wxINT16_SWAP_ON_BE(header->origsize);

		// Setup other info
		info.colformat = PALMASK;
		info.format = id;

		return info;
	}
};

class SIFRottGfxMasked : public SIFRottGfx
{
protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		return readRottGfx(image, data, true);
	}

public:
	SIFRottGfxMasked() : SIFRottGfx("rottmask")
	{
		name = "ROTT Masked Gfx";
		reliability = 120;
	}
	~SIFRottGfxMasked() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::getFormat("img_rottmask")->isThisFormat(mc))
			return true;
		else
			return false;
	}
};

class SIFRottLbm : public SIFormat
{
protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::info_t info = getInfo(data, index);

		// ROTT source code says: "LIMITATIONS - Only works with 320x200!!!"
		if (info.width != 320 || info.height != 200)
			return false;

		// Build palette
		Palette8bit palette;
		for (size_t c = 0; c < 256; ++c)
		{
			rgba_t color;
			color.r = data[(c*3)+4];
			color.g = data[(c*3)+5];
			color.b = data[(c*3)+6];
			palette.setColour(c, color);
		}

		// Create image
		image.create(info, &palette);
		uint8_t* img_data = imageData(image);
		image.fillAlpha(255);

		// Create some variables needed for LBM decompression
		const uint8_t* read = data.getData() + 768 +4;
		const uint8_t* readend = data.getData() + data.getSize();
		uint8_t* dest = img_data;
		uint8_t* destend = img_data + (info.width * info.height);
		uint8_t code = 0;
		uint8_t length = 0;
		uint8_t count = 0;

		// Read image data
		while (read < readend && dest < destend && count < info.width)
		{
			code = *read++;
			if (code < 0x80)
			{
				length = code + 1;
				memcpy(dest, read, length);
				dest+=length;
				read+=length;
			}
			else if (code > 0x80)
			{
				length = (code^0xFF)+2;;
				code = *read++;
				memset(dest, code, length);
				dest+=length;
			}
			else length = 0;

			count += length;
		}

		return true;
	}

public:
	SIFRottLbm() : SIFormat("rottlbm")
	{
		name = "ROTT Lbm";
		extension = "dat";
		reliability = 80;
	}
	~SIFRottLbm() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::getFormat("img_rottlbm")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t info;

		// Setup info
		info.width = READ_L16(mc.getData(), 0);
		info.height = READ_L16(mc.getData(), 2);
		info.colformat = PALMASK;
		info.has_palette = true;
		info.format = id;

		return info;
	}
};

class SIFRottRaw : public SIFormat
{
protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::info_t info = getInfo(data, index);

		// Create image (swapped width/height because column-major)
		image.create(info.height, info.width, PALMASK);
		image.fillAlpha(255);

		// Read raw pixel data
		data.read(imageData(image), info.width*info.height, 8);

		// Convert from column-major to row-major
		image.rotate(90);
		image.mirror(true);

		return true;
	}

public:
	SIFRottRaw() : SIFormat("rottraw")
	{
		name = "ROTT Raw";
		extension = "dat";
		reliability = 101;
	}
	~SIFRottRaw() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::getFormat("img_rottraw")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t info;

		// Read header
		const patch_header_t* header = (const patch_header_t*)mc.getData();
		info.width = wxINT16_SWAP_ON_BE(header->width);
		info.height = wxINT16_SWAP_ON_BE(header->height);
		info.offset_x = wxINT16_SWAP_ON_BE(header->left);
		info.offset_y = wxINT16_SWAP_ON_BE(header->top);

		// Set other info
		info.colformat = PALMASK;
		info.format = id;

		return info;
	}
};

class SIFRottPic : public SIFormat
{
protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::info_t info = getInfo(data, index);

		// Check data
		if (data.getSize() != 4 + info.width*info.height)
			return false;

		// Create image
		image.create(info);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		// Read data
		const uint8_t* entryend = data.getData() + data.getSize() - 2;
		uint8_t* dataend = img_data + data.getSize() - 4;
		const uint8_t* pixel = data.getData() + 2;
		uint8_t* brush = img_data;
		while (pixel < entryend)
		{
			*brush = *pixel++;
			brush += 4;
			if (brush >= dataend)
				brush -= data.getSize() - 5;
		}

		// Create mask (index 255 is transparent)
		for (int a = 0; a < info.width*info.height; a++)
		{
			if (img_data[a] == 255)
				img_mask[a] = 0;
			else
				img_mask[a] = 255;
		}

		return true;
	}

public:
	SIFRottPic() : SIFormat("rottpic")
	{
		name = "ROTT Picture";
		extension = "dat";
		reliability = 60;
	}
	~SIFRottPic() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::getFormat("img_rottpic")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t info;

		// Read dimensions
		info.width = mc[0] * 4;
		info.height = mc[1];

		// Setup other info
		info.colformat = PALMASK;
		info.format = id;

		return info;
	}
};

class SIFRottWall : public SIFormat
{
protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Get image info
		SImage::info_t info = getInfo(data, index);

		// Create image (swapped width/height because column-major)
		image.create(info.height, info.width, PALMASK);
		image.fillAlpha(255);

		// Read raw pixel data
		data.seek(0, SEEK_SET);
		data.read(imageData(image), info.height * info.width);

		// Convert from column-major to row-major
		image.rotate(90);
		image.mirror(false);

		return true;
	}

public:
	SIFRottWall() : SIFormat("rottwall")
	{
		name = "ROTT Flat";
		extension = "dat";
		reliability = 10;
	}
	~SIFRottWall() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() == 4096 || mc.getSize() == 51200)
			return true;
		else
			return false;
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t info;

		// Always the same thing
		info.width = mc.getSize() == 4096 ? 64 : 256;
		info.height = mc.getSize() == 4096 ? 64 : 200;
		info.offset_x = 0;
		info.offset_y = 0;
		info.colformat = PALMASK;
		info.format = id;

		return info;
	}
};

