
class SIFRottGfx : public SIFormat
{
public:
	SIFRottGfx(string_view id = "rott", string_view name = "ROTT Gfx", int reliability = 121) :
		SIFormat(id, name, "dat", reliability)
	{
	}
	~SIFRottGfx() override = default;

	bool isThisFormat(const MemChunk& mc) override
	{
		return EntryDataFormat::format("img_rott")->isThisFormat(mc) >= EntryDataFormat::MATCH_PROBABLY;
	}

	SImage::Info info(const MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		auto header   = reinterpret_cast<const gfx::ROTTPatchHeader*>(mc.data());
		info.width    = wxINT16_SWAP_ON_BE(header->width);
		info.height   = wxINT16_SWAP_ON_BE(header->height);
		info.offset_x = wxINT16_SWAP_ON_BE(header->left) + (wxINT16_SWAP_ON_BE(header->origsize) / 2);
		info.offset_y = wxINT16_SWAP_ON_BE(header->top) + wxINT16_SWAP_ON_BE(header->origsize);

		// Setup other info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readRottGfx(SImage& image, const MemChunk& data, bool mask)
	{
		// Get image info
		auto info = this->info(data, 0);

		// Setup variables
		size_t hdr_size   = sizeof(gfx::ROTTPatchHeader);
		short  translevel = 255;
		if (mask)
		{
			translevel = data.readL16(hdr_size);
			hdr_size += 2;
		}

		// Read column offsets
		vector<uint16_t> col_offsets(info.width);
		auto             c_ofs = reinterpret_cast<const uint16_t*>(data.data() + hdr_size);
		for (int a = 0; a < info.width; a++)
			col_offsets[a] = wxUINT16_SWAP_ON_BE(c_ofs[a]);

		// Create image
		image.create(info);

		// Load data
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);
		for (int c = 0; c < info.width; c++)
		{
			// Get current column offset
			uint16_t col_offset = col_offsets[c];

			// Check column offset is valid
			if (col_offset >= data.size())
				return false;

			// Go to start of column
			auto bits = data.data() + col_offset;

			// Read posts
			int counter = 0;
			while (true)
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

	bool readImage(SImage& image, const MemChunk& data, int index) override { return readRottGfx(image, data, false); }
};

class SIFRottGfxMasked : public SIFRottGfx
{
public:
	SIFRottGfxMasked() : SIFRottGfx("rottmask", "ROTT Masked Gfx", 120) {}
	~SIFRottGfxMasked() override = default;

	bool isThisFormat(const MemChunk& mc) override { return EntryDataFormat::format("img_rottmask")->isThisFormat(mc); }

protected:
	bool readImage(SImage& image, const MemChunk& data, int index) override { return readRottGfx(image, data, true); }
};

class SIFRottLbm : public SIFormat
{
public:
	SIFRottLbm() : SIFormat("rottlbm", "ROTT Lbm", "dat", 80) {}
	~SIFRottLbm() override = default;

	bool isThisFormat(const MemChunk& mc) override
	{
		return EntryDataFormat::format("img_rottlbm")->isThisFormat(mc) >= EntryDataFormat::MATCH_PROBABLY;
	}

	SImage::Info info(const MemChunk& mc, int index) override
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
	bool readImage(SImage& image, const MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

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
		auto img_data = imageData(image);
		image.fillAlpha(255);

		// Create some variables needed for LBM decompression
		auto    read    = data.data() + 768 + 4;
		auto    readend = data.data() + data.size();
		auto    dest    = img_data;
		auto    destend = img_data + (info.width * info.height);
		uint8_t code    = 0;
		uint8_t length  = 0;
		uint8_t count   = 0;

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
				code   = *read++;
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
	SIFRottRaw() : SIFormat("rottraw", "ROTT Raw", "dat", 101) {}
	~SIFRottRaw() override = default;

	bool isThisFormat(const MemChunk& mc) override
	{
		return EntryDataFormat::format("img_rottraw")->isThisFormat(mc) >= EntryDataFormat::MATCH_PROBABLY;
	}

	SImage::Info info(const MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		auto header   = reinterpret_cast<const gfx::PatchHeader*>(mc.data());
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
	bool readImage(SImage& image, const MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

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
	SIFRottPic() : SIFormat("rottpic", "ROTT Picture", "dat", 60) {}
	~SIFRottPic() override = default;

	bool isThisFormat(const MemChunk& mc) override
	{
		return EntryDataFormat::format("img_rottpic")->isThisFormat(mc) >= EntryDataFormat::MATCH_PROBABLY;
	}

	SImage::Info info(const MemChunk& mc, int index) override
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
	bool readImage(SImage& image, const MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

		// Check data
		if (static_cast<int>(data.size()) != 4 + info.width * info.height)
			return false;

		// Create image
		image.create(info);
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);

		// Read data
		auto entryend = data.data() + data.size() - 2;
		auto dataend  = img_data + data.size() - 4;
		auto pixel    = data.data() + 2;
		auto brush    = img_data;
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
	SIFRottWall() : SIFormat("rottwall", "ROTT Flat", "dat", 10) {}
	~SIFRottWall() override = default;

	bool isThisFormat(const MemChunk& mc) override { return (mc.size() == 4096 || mc.size() == 51200); }

	SImage::Info info(const MemChunk& mc, int index) override
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
	bool readImage(SImage& image, const MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

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
