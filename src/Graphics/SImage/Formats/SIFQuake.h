
class SIFQuakeGfx : public SIFormat
{
public:
	SIFQuakeGfx() : SIFormat("quake", "Quake Gfx", "dat") {}
	~SIFQuakeGfx() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_quake")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get image properties
		info.width  = wxINT16_SWAP_ON_BE(*(const uint16_t*)(mc.data()));
		info.height = wxINT16_SWAP_ON_BE(*(const uint16_t*)(mc.data() + 4));

		// Determine colour type
		info.colformat = SImage::Type::RGBA;
		uint8_t mode   = mc[3];
		if (mode == Palette || mode == Alpha)
			info.colformat = SImage::Type::PalMask;
		else if (mode == Intensity)
			info.colformat = SImage::Type::AlphaMap;

		info.format = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image properties
		int     width  = wxINT16_SWAP_ON_BE(*(const uint16_t*)(data.data()));
		int     height = wxINT16_SWAP_ON_BE(*(const uint16_t*)(data.data() + 4));
		uint8_t mode   = data[3];

		// Determine image type
		auto type = SImage::Type::RGBA;
		if (mode == Palette || mode == Alpha)
			type = SImage::Type::PalMask;
		else if (mode == Intensity)
			type = SImage::Type::AlphaMap;

		// Create image
		image.create(width, height, type);

		// --- Read image data ---
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);

		// Paletted image
		if (mode == Palette)
		{
			// Read raw pixel data
			memcpy(img_data, data.data() + 8, width * height);

			// Create mask (all opaque, except index 255 is transparent)
			for (int i = 0; i < width * height; ++i)
			{
				if (img_data[i] == 255)
					img_mask[i] = 0;
				else
					img_mask[i] = 255;
			}
		}

		// Intensity map
		else if (mode == Intensity)
		{
			// Read raw pixel data
			memcpy(img_data, data.data() + 8, width * height);
		}

		// Paletted image + alpha channel (i think?)
		else if (mode == Alpha)
		{
			// Read raw pixel data
			for (unsigned i = 0; i < data.size() - 8; i += 2)
			{
				img_data[i / 2] = data[i + 8];
				img_mask[i / 2] = data[i + 9];
			}
		}

		// 24bit (RGB)
		else if (mode == RGB24)
		{
			// Read raw pixel data
			for (unsigned i = 0; i < data.size() - 8; i += 3)
			{
				img_data[(i / 3) * 4]     = data[i + 8];
				img_data[(i / 3) * 4 + 1] = data[i + 9];
				img_data[(i / 3) * 4 + 2] = data[i + 10];
				img_data[(i / 3) * 4 + 3] = 255;
			}
		}

		// 32bit (RGBA)
		else if (mode == RGB32)
		{
			// Read raw pixel data
			memcpy(img_data, data.data() + 8, width * height * 4);
		}

		return true;
	}

private:
	enum QuakePicType
	{
		Palette   = 0,
		Intensity = 1,
		Alpha     = 2,
		RGB24     = 3,
		RGB32     = 4
	};
};

class SIFQuakeSprite : public SIFormat
{
public:
	SIFQuakeSprite() : SIFormat("qspr", "Quake Sprite", "dat") {}
	~SIFQuakeSprite() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_qspr")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		// Get image info
		SImage::Info info;
		sprInfo(mc, index, info);

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		SImage::Info info;
		unsigned     imgofs = sprInfo(data, index, info);

		// Check data is valid
		if (imgofs == 0)
			return false;

		// Create image
		image.create(info.width, info.height, SImage::Type::PalMask, nullptr, index, info.numimages);
		image.setXOffset(info.offset_x);
		image.setYOffset(info.offset_y);

		// Load image data
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);
		memcpy(img_data, data.data() + imgofs + 16, info.width * info.height);
		memset(img_mask, 0xFF, info.width * info.height);
		for (int a = 0; a < info.width * info.height; ++a)
		{
			if (img_data[a] == 0xFF)
				img_mask[a] = 0;
		}

		return true;
	}

private:
	unsigned sprInfo(MemChunk& mc, int index, SImage::Info& info) const
	{
		// Setup variables
		uint32_t maxheight = mc.readL32(16);
		uint32_t maxwidth  = mc.readL32(20);
		uint32_t nframes   = mc.readL32(24);
		int      numimages = nframes;

		// Makes sum of frame pictures, not just frames
		// Also gather offsets in a data structure
		vector<size_t> pics;
		uint32_t       imgofs = 36;
		for (size_t a = 0; a < nframes; ++a)
		{
			if (mc.readL32(imgofs) != 0)
			{
				// We have a frame with a group of picture
				uint32_t grpsz = mc.readL32(imgofs + 4);
				if (grpsz == 0)
				{
					global::error = "Quake sprite data contains empty group";
					return 0;
				}
				// Move to end of group header
				imgofs += (grpsz + 2) << 2;
				for (size_t b = 0; b < grpsz; ++b)
				{
					uint32_t pw = mc.readL32(imgofs + 8);
					uint32_t ph = mc.readL32(imgofs + 12);
					// Store image offset
					pics.push_back(imgofs);
					// Move to end of picture data
					imgofs += 16 + pw * ph;
					if (imgofs > (unsigned)mc.size())
					{
						global::error = "Quake sprite data too short";
						return 0;
					}
				}
				// We must have made it through all the group's picture,
				// so replace the count for the group (1) by the group's size.
				numimages += (grpsz - 1);
			}
			else
			{
				// We have a frame with a single picture
				imgofs += 4;
				uint32_t pw = mc.readL32(imgofs + 8);
				uint32_t ph = mc.readL32(imgofs + 12);
				// Store image offset
				pics.push_back(imgofs);
				// Move to end of picture data
				imgofs += 16 + pw * ph;
			}
			if (imgofs > mc.size())
			{
				global::error = "Quake sprite data too short";
				return 0;
			}
		}

		// Sanitize index if needed
		index %= numimages;
		if (index < 0)
			index = numimages + index;

		// Setup variables using appropriate image data
		imgofs        = pics[index];
		info.offset_x = mc.readL32(imgofs + 0);
		info.offset_y = mc.readL32(imgofs + 4);
		info.width    = mc.readL32(imgofs + 8);
		info.height   = mc.readL32(imgofs + 12);
		// Horizontal offsets seem computed differently from Doom, so translate them
		info.offset_x += info.width;

		// Setup other info
		info.colformat = SImage::Type::PalMask;
		info.numimages = numimages;
		info.format    = id_;

		return imgofs;
	}
};

class SIFQuakeTex : public SIFormat
{
public:
	SIFQuakeTex() : SIFormat("quaketex", "Quake Texture", "dat", 11) {}
	~SIFQuakeTex() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_quaketex")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Setup variables
		info.numimages = 4;
		info.colformat = SImage::Type::PalMask;
		info.width     = mc.readL32(16);
		info.height    = mc.readL32(20);
		info.format    = id_;

		// Sanitize index if needed
		index %= info.numimages;
		if (index < 0)
			index = info.numimages + index;

		// Adjust variables according to mip level
		if (index)
		{
			info.width >>= index;
			info.height >>= index;
		}

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

		// Find offset
		uint32_t imgofs = data.readL32(24 + (index << 2));

		// Create image
		image.create(info.width, info.height, (SImage::Type)info.colformat, nullptr, index, info.numimages);
		image.fillAlpha(255);

		// Load image data
		memcpy(imageData(image), data.data() + imgofs, info.width * info.height);

		return true;
	}

	bool writeImage(SImage& image, MemChunk& out, Palette* pal, int index) override { return false; }
};

class SIFQuake2Wal : public SIFormat
{
public:
	SIFQuake2Wal() : SIFormat("quake2wal", "Quake II Wall", "dat", 21) {}
	~SIFQuake2Wal() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_quake2wal")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get image info
		info.colformat = SImage::Type::PalMask;
		info.numimages = 4;
		info.width     = mc.readL32(32) >> index;
		info.height    = mc.readL32(36) >> index;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = this->info(data, index);

		// Sanitize index if needed
		index %= info.numimages;
		if (index < 0)
			index = info.numimages + index;

		// Get data offset for image
		size_t data_offset = data.readL32(40 + (index << 2));
		if (!info.width || !info.height || !data_offset || data.size() < data_offset + (info.width * info.height))
		{
			global::error = "WAL file: invalid data for mip level";
			return false;
		}

		// Create image
		image.create(info.width, info.height, SImage::Type::PalMask, nullptr, index, info.numimages);
		image.fillAlpha(255);

		// Fill data with pixel data
		memcpy(imageData(image), data.data() + data_offset, info.width * info.height);

		return true;
	}
};
