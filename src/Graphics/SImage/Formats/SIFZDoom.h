
class SIFImgz : public SIFormat
{
public:
	SIFImgz() : SIFormat("imgz")
	{
		name_      = "IMGZ";
		extension_ = "imgz";
	}
	~SIFImgz() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::format("img_imgz")->isThisFormat(mc))
			return true;
		else
			return false;
	}

	SImage::Info info(MemChunk& mc, int index)
	{
		SImage::Info info;

		// Get width & height
		Graphics::IMGZHeader* header = (Graphics::IMGZHeader*)mc.data();
		info.width                   = wxINT16_SWAP_ON_BE(header->width);
		info.height                  = wxINT16_SWAP_ON_BE(header->height);

		// Other image info
		info.colformat = SImage::Type::AlphaMap;
		info.format    = "imgz";

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		int width, height, offset_x, offset_y;

		// Setup variables
		auto header = (Graphics::IMGZHeader*)data.data();
		width       = wxINT16_SWAP_ON_BE(header->width);
		height      = wxINT16_SWAP_ON_BE(header->height);
		offset_x    = wxINT16_SWAP_ON_BE(header->left);
		offset_y    = wxINT16_SWAP_ON_BE(header->top);

		// Create image
		image.create(width, height, SImage::Type::AlphaMap);
		uint8_t* img_data = imageData(image);

		if (!header->compression)
		{
			// No compression
			memcpy(img_data, data.data() + sizeof(Graphics::IMGZHeader), data.size() - sizeof(Graphics::IMGZHeader));

			return true;
		}
		else
		{
			// We'll use wandering pointers. The original pointer is kept for cleanup.
			const uint8_t* read    = data.data() + sizeof(Graphics::IMGZHeader);
			const uint8_t* readend = read + data.size() - 1;
			uint8_t*       dest    = img_data;
			uint8_t*       destend = dest + width * height;

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
