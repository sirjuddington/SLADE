
// -----------------------------------------------------------------------------
// RLE decompression algorithm for various Jedi Engine formats
// -----------------------------------------------------------------------------
inline bool JediRLE(const uint8_t* src, uint32_t coloffs, int width, int height, uint8_t* data)
{
	for (int x = 0; x < width; ++x)
	{
		auto     p      = memory::readL32(src, coloffs + (x << 2));
		uint8_t* endcol = data + height;
		while (data < endcol)
		{
			int val = src[p++];
			if (val < 0x80)
			{
				memcpy(data, src + p, val);
				data += val;
				p += val;
			}
			else
			{
				memset(data, src[p++], val - 0x80);
				data += (val - 0x80);
			}
		}
	}
	return true;
}

// -----------------------------------------------------------------------------
// RLE0 decompression algorithm for various Jedi Engine formats
// Same as RLE, but fills with zeroes instead of a specified value
// -----------------------------------------------------------------------------
inline bool JediRLE0(const uint8_t* src, uint32_t coloffs, int width, int height, uint8_t* data)
{
	for (int x = 0; x < width; ++x)
	{
		auto     p      = memory::readL32(src, coloffs + (x << 2));
		uint8_t* endcol = data + height;
		while (data < endcol)
		{
			int val = src[p++];
			if (val < 0x80)
			{
				memcpy(data, src + p, val);
				data += val;
				p += val;
			}
			else
			{
				memset(data, 0, val - 0x80);
				data += (val - 0x80);
			}
		}
	}
	return true;
}


class SIFJediBM : public SIFormat
{
public:
	SIFJediBM() : SIFormat("jedi_bm")
	{
		name_        = "Jedi BM";
		extension_   = "dat";
		reliability_ = 80;
	}
	~SIFJediBM() override = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_jedi_bm")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;
		JediBMHeader header;
		bool         transparent;
		getImageInfo(info, mc, index, &header, transparent);
		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		SImage::Info info;
		JediBMHeader header;
		bool         transparent;
		unsigned     data_offset = getImageInfo(info, data, index, &header, transparent);

		// Check valid
		if (data_offset == 0)
			return false;

		// Create image (width/height swapped because column-major)
		image.create(info.height, info.width, SImage::Type::PalMask);
		image.fillAlpha(255);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		if (header.rle == 0)
		{
			memcpy(img_data, data.data() + data_offset, info.width * info.height);
		}
		else if (header.rle == 1)
		{
			JediRLE(data.data() + data_offset, header.coloffs, info.width, info.height, img_data);
		}
		else if (header.rle == 2)
		{
			JediRLE0(data.data() + data_offset, header.coloffs, info.width, info.height, img_data);
		}
		else
		{
			global::error = "Jedi BM file: Invalid compression scheme";
			return false;
		}

		// Manage transparency
		if (transparent)
		{
			for (int i = 0; i < info.width * info.height; ++i)
				if (img_data[i] == 0)
					img_mask[i] = 0;
		}

		// Convert from column-major to row-major
		image.rotate(270);

		return true;
	}

private:
	// Jedi bitmap header structs
	struct JediBMHeader
	{
		char     magic[4]; // "BM "0x1E
		uint16_t width;    // If 1 and height not 1, multiple BM
		uint16_t height;   // If height and width both 1, 1x1 bitmap
		uint16_t wid2;     // Same as width except for multiple BM, then it's 65534
		uint16_t hei2;     // Same as height except for multiple BM, then it's number of images
		uint8_t  flag;     // Determines transparency among other stuff
		uint8_t  log;      // Worth either log2(height) or 0
		uint16_t rle;      // 0: not compressed, 1: compressed with RLE, 2: compressed with RLE0
		uint32_t coloffs;  // Column offsets in compressed bitmaps
		uint32_t pad[3];   // Padding, should be zero
	};
	struct JediBMSubheader
	{
		uint16_t width;    // If 1 and height not 1, multiple BM
		uint16_t height;   // If height and width both 1, 1x1 bitmap
		uint16_t wid2;     // Unused
		uint16_t hei2;     // Unused
		uint32_t size;     // Unused
		uint8_t  log;      // Worth log2(height)
		uint8_t  pad1[11]; // No known use
		uint8_t  flag;     // Determines transparency among other stuff
		uint8_t  pad2[3];  // No known use
	};

	unsigned getImageInfo(SImage::Info& info, MemChunk& mc, int index, JediBMHeader* header, bool& transparent)
	{
		mc.read(header, sizeof(JediBMHeader), 0);
		bool multibm         = false;
		transparent          = !!(header->flag & 8);
		unsigned data_offset = sizeof(JediBMHeader);

		// Check for multiple images
		if (wxINT16_SWAP_ON_BE(header->width) == 1 && wxINT16_SWAP_ON_BE(header->height) != 1)
		{
			if (header->rle == 0 && wxINT16_SWAP_ON_BE(header->wid2) == 65534)
			{
				info.numimages = wxINT16_SWAP_ON_BE(header->hei2);
			}
			else
			{
				global::error = "Jedi BM file: invalid multi-BM file";
				return 0;
			}
			multibm = true;
		}
		else
			info.numimages = 1;

		// Sanitize index if needed
		index %= info.numimages;
		if (index < 0)
			index = info.numimages + index;

		// Set info
		info.colformat = SImage::Type::PalMask;
		info.imgindex  = index;

		if (multibm)
		{
			// 32 for the header, 2 for control data, then four bytes per subimage
			int offset_offset = 34 + (info.imgindex << 2);
			if (offset_offset + 4 > static_cast<int>(mc.size()))
			{
				global::error = "Jedi BM file: invalid multi-BM file";
				return 0;
			}
			auto header_poffset = reinterpret_cast<uint32_t*>(mc.data() + offset_offset);
			auto header_offset  = wxINT32_SWAP_ON_BE(*header_poffset) + 34;
			auto subheader      = reinterpret_cast<JediBMSubheader*>(mc.data() + header_offset);
			transparent         = !!(subheader->flag & 8);
			info.width          = wxINT16_SWAP_ON_BE(subheader->width);
			info.height         = wxINT16_SWAP_ON_BE(subheader->height);
			data_offset         = header_offset + 28;
		}
		else
		{
			info.width  = wxINT16_SWAP_ON_BE(header->width);
			info.height = wxINT16_SWAP_ON_BE(header->height);
		}

		// Return data offset to image at [index]
		return data_offset;
	}
};

class SIFJediFME : public SIFormat
{
public:
	SIFJediFME(string id = "jedi_fme") : SIFormat(id)
	{
		name_        = "Jedi FME";
		extension_   = "dat";
		reliability_ = 80;
	}
	~SIFJediFME() override = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_jedi_fme")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		// Read headers
		SImage::Info info;
		const auto   header1 = reinterpret_cast<JediFMEHeader1*>(mc.data());
		const auto   header2 = reinterpret_cast<JediFMEHeader2*>(mc.data() + wxINT32_SWAP_ON_BE(header1->head2));

		// Setup variables
		info.offset_x  = 0 - wxINT32_SWAP_ON_BE(header1->offsx);
		info.offset_y  = 0 - wxINT32_SWAP_ON_BE(header1->offsy);
		info.width     = wxINT32_SWAP_ON_BE(header2->width);
		info.height    = wxINT32_SWAP_ON_BE(header2->height);
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	struct JediFMEHeader1
	{
		int32_t  offsx;  // X offset (right is positive)
		int32_t  offsy;  // Y offset (down is positive)
		uint32_t flag;   // Only one flag used: 1 for flipped horizontally
		uint32_t head2;  // Offset to secondary header
		uint32_t width;  // Unused; the data from the secondary header is used instead
		uint32_t height; // Unused; the data from the secondary header is used instead
		uint32_t pad[2]; // Padding, should be zero
	};
	struct JediFMEHeader2
	{
		uint32_t width;   // Used instead of the value in the primary header
		uint32_t height;  // Used instead of the value in the primary header
		uint32_t flag;    // Only one flag used: 1 for RLE0 compression
		uint32_t size;    // Data size for compressed FME, same as file size - 32
		uint32_t coloffs; // Offset to column data, practically always 0
		uint32_t padding; // No known use
	};

	bool readFrame(SImage& image, MemChunk& data, unsigned offset, unsigned numimages = 1) const
	{
		SImage::Info info;
		const auto   header1 = reinterpret_cast<JediFMEHeader1*>(data.data() + offset);
		const auto   header2 = reinterpret_cast<JediFMEHeader2*>(data.data() + wxINT32_SWAP_ON_BE(header1->head2));
		bool         flip    = !!(header1->flag & 1);

		info.offset_x  = 0 - wxINT32_SWAP_ON_BE(header1->offsx);
		info.offset_y  = 0 - wxINT32_SWAP_ON_BE(header1->offsy);
		info.colformat = SImage::Type::PalMask;
		info.height    = wxINT32_SWAP_ON_BE(header2->width);
		info.width     = wxINT32_SWAP_ON_BE(header2->height);
		info.numimages = numimages;

		// Create image
		image.create(info);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		// Fill data with pixel data
		if (header2->flag == 0)
			data.read(img_data, info.width * info.height, wxINT32_SWAP_ON_BE(header1->head2) + 24);
		else
			JediRLE0(
				data.data() + wxINT32_SWAP_ON_BE(header1->head2),
				24,
				wxINT32_SWAP_ON_BE(header2->width),
				wxINT32_SWAP_ON_BE(header2->height),
				img_data);

		// Manage transparency
		for (int i = 0; i < info.width * info.height; ++i)
			img_mask[i] = (img_data[i] == 0) ? 0 : 255;

		// Convert from column-major to row-major
		image.rotate(270);

		// Mirror if needed
		if (flip)
			image.mirror(false);

		return true;
	}

	bool readImage(SImage& image, MemChunk& data, int index) override { return readFrame(image, data, 0); }
};

class SIFJediWAX : public SIFJediFME
{
public:
	SIFJediWAX() : SIFJediFME("jedi_wax")
	{
		name_        = "Jedi WAX";
		extension_   = "dat";
		reliability_ = 60;
	}
	~SIFJediWAX() override = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_jedi_wax")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Determine all frame offsets
		vector<unsigned> offsets;
		readFrameOffsets(mc, offsets);

		// Sanitize index if needed
		info.imgindex  = index;
		info.numimages = offsets.size();
		if (offsets.empty())
			return info;
		info.imgindex %= info.numimages;
		if (info.imgindex < 0)
			info.imgindex = info.numimages + info.imgindex;

		// Read headers
		const auto header1 = reinterpret_cast<JediFMEHeader1*>(mc.data() + offsets[info.imgindex]);
		const auto header2 = reinterpret_cast<JediFMEHeader2*>(mc.data() + wxINT32_SWAP_ON_BE(header1->head2));

		// Setup variables
		info.offset_x  = 0 - wxINT32_SWAP_ON_BE(header1->offsx);
		info.offset_y  = 0 - wxINT32_SWAP_ON_BE(header1->offsy);
		info.width     = wxINT32_SWAP_ON_BE(header2->width);
		info.height    = wxINT32_SWAP_ON_BE(header2->height);
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Determine all frame offsets
		vector<unsigned> offsets;
		readFrameOffsets(data, offsets);

		// Sanitize index if needed
		int numimages = offsets.size();
		if (offsets.empty())
		{
			global::error = "Jedi WAX error: No cell found in wax!"; // What a surreal error message ;)
			return false;
		}
		index %= numimages;
		if (index < 0)
			index = numimages + index;

		// Load image
		return readFrame(image, data, offsets[index], numimages);
	}

private:
	// Jedi engine wax header structs
	struct JediWAXHeader
	{
		uint32_t version;   // Constant worth 0x00100100
		uint32_t numseqs;   // Number of sequences
		uint32_t numframes; // Number of frames
		uint32_t numcells;  // Number of cells
		uint32_t pad[4];    // Unused values
		uint32_t waxes[32]; // Offsets to the WAX subheaders
	};
	struct JediWAXSubheader
	{
		uint32_t width;    // Scaled width, in world units
		uint32_t height;   // Scaled height, in world units
		uint32_t fps;      // Frames per second of animation
		uint32_t pad[4];   // Unused values, must be zero
		uint32_t seqs[32]; // Offsets to sequences
	};
	struct JediWAXSequence
	{
		uint32_t pad[4];     // Unused values, must be zero
		uint32_t frames[32]; // Offsets to frames
	};

	void readFrameOffsets(MemChunk& data, vector<unsigned>& offsets)
	{
		const auto     header = reinterpret_cast<JediWAXHeader*>(data.data());
		vector<size_t> frameoffs;

		// This is a recursive nightmare. What were the LucasArts devs smoking when they specced this format?
		for (auto wax_off : header->waxes)
		{
			if (wax_off > sizeof(JediWAXHeader) && wax_off < data.size() - sizeof(JediWAXSubheader))
			{
				const auto subhdr = reinterpret_cast<JediWAXSubheader*>(data.data() + wax_off);
				for (auto seq_off : subhdr->seqs)
				{
					if (seq_off > sizeof(JediWAXHeader) && seq_off < data.size() - sizeof(JediWAXSequence))
					{
						const auto seq = reinterpret_cast<JediWAXSequence*>(data.data() + seq_off);
						for (auto frame_off : seq->frames)
						{
							if (frame_off > sizeof(JediWAXHeader) && frame_off < data.size() - sizeof(JediWAXSequence))
							{
								const auto frame = reinterpret_cast<JediFMEHeader1*>(data.data() + frame_off);
								uint32_t   cell  = wxINT32_SWAP_ON_BE(frame->head2);
								if (cell > sizeof(JediWAXHeader) && cell < data.size() - sizeof(JediFMEHeader2))
								{
									bool notfound = true;
									for (auto frameoff : frameoffs)
									{
										if (frameoff == frame_off)
										{
											notfound = false;
											break;
										}
									}
									if (notfound)
										frameoffs.push_back(frame_off);
								}
							}
						}
					}
				}
			}
		}
		// Urgh. At least it's over now.

		offsets.reserve(frameoffs.size());
		for (auto off : frameoffs)
			offsets.push_back(static_cast<unsigned>(off));
	}
};
