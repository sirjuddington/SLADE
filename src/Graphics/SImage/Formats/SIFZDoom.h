
class SIFImgz : public SIFormat
{
public:
	SIFImgz() : SIFormat("imgz", "IMGZ", "imgz") {}
	~SIFImgz() override = default;

	bool isThisFormat(const MemChunk& mc) override { return EntryDataFormat::format("img_imgz")->isThisFormat(mc); }

	SImage::Info info(const MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get width & height
		auto header = reinterpret_cast<const gfx::IMGZHeader*>(mc.data());
		info.width  = wxINT16_SWAP_ON_BE(header->width);
		info.height = wxINT16_SWAP_ON_BE(header->height);

		// Other image info
		info.colformat = SImage::Type::AlphaMap;
		info.format    = "imgz";

		return info;
	}

protected:
	bool readImage(SImage& image, const MemChunk& data, int index) override
	{
		// Setup variables
		auto header   = reinterpret_cast<const gfx::IMGZHeader*>(data.data());
		int  width    = wxINT16_SWAP_ON_BE(header->width);
		int  height   = wxINT16_SWAP_ON_BE(header->height);
		int  offset_x = wxINT16_SWAP_ON_BE(header->left);
		int  offset_y = wxINT16_SWAP_ON_BE(header->top);

		// Create image
		image.create(width, height, SImage::Type::AlphaMap);
		auto img_data = imageData(image);

		if (!header->compression)
		{
			// No compression
			memcpy(img_data, data.data() + sizeof(gfx::IMGZHeader), data.size() - sizeof(gfx::IMGZHeader));

			return true;
		}
		else
		{
			// We'll use wandering pointers. The original pointer is kept for cleanup.
			auto read    = data.data() + sizeof(gfx::IMGZHeader);
			auto readend = read + data.size() - 1;
			auto dest    = img_data;
			auto destend = dest + width * height;

			uint8_t code   = 0;
			size_t  length = 0;

			while (read < readend && dest < destend)
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
					length = 0x101 - code;
					code   = *read++;
					memset(dest, code, length);
					dest += length;
				}
			}
		}

		return true;
	}
};
