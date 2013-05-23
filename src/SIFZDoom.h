
class SIFImgz : public SIFormat
{
protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		int width, height, offset_x, offset_y;

		// Setup variables
		imgz_header_t* header = (imgz_header_t*)data.getData();
		width = wxINT16_SWAP_ON_BE(header->width);
		height = wxINT16_SWAP_ON_BE(header->height);
		offset_x = wxINT16_SWAP_ON_BE(header->left);
		offset_y = wxINT16_SWAP_ON_BE(header->top);

		// Create image
		image.create(width, height, ALPHAMAP);
		uint8_t* img_data = imageData(image);

		if (!header->compression)
		{
			// No compression
			memcpy(img_data, data.getData() + sizeof(imgz_header_t), data.getSize() - sizeof(imgz_header_t));

			return true;
		}
		else
		{
			// We'll use wandering pointers. The original pointer is kept for cleanup.
			const uint8_t* read = data.getData() + sizeof(imgz_header_t);
			const uint8_t* readend = read + data.getSize() - 1;
			uint8_t* dest  = img_data;
			uint8_t* destend = dest + width*height;

			uint8_t code = 0; size_t length = 0;

			while (read < readend && dest < destend)
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
					length = 0x101 - code;
					code = *read++;
					memset(dest, code, length);
					dest+=length;
				}
			}
		}

		return true;
	}

public:
	SIFImgz() : SIFormat("imgz")
	{
		name = "IMGZ";
		extension = "imgz";
	}
	~SIFImgz() {}

	bool isThisFormat(MemChunk& mc)
	{
		if (EntryDataFormat::getFormat("img_imgz")->isThisFormat(mc))
			return true;
		else
			return false;
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t info;

		// Get width & height
		imgz_header_t* header = (imgz_header_t*)mc.getData();
		info.width = wxINT16_SWAP_ON_BE(header->width);
		info.height = wxINT16_SWAP_ON_BE(header->height);

		// Other image info
		info.colformat = ALPHAMAP;
		info.format = "imgz";

		return info;
	}
};
