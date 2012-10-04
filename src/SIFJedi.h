// So apparently this all got completely broken by the transition to SIFormat.
// I don't have the energy to find out what's wrong exactly, so I'll just remove
// it entirely for the time being.
#if 0
/* JediRLE0
 * Used by several Jedi Engine formats
 *******************************************************************/
bool JediRLE0(const uint8_t* src, int coloffs, int width, int height, uint8_t * data) {
	for (int x = 0; x < width; ++x) {
		int p = READ_L32(src, (coloffs + (x<<2)));
		uint8_t * endcol = data + height;
		while (data < endcol) {
			int val = src[p++];
			if (val < 0x80) {
				memcpy(data, src + p, val);
				data += val;
				p += val;
			} else {
				memset(data, 0, val - 0x80);
				data += (val - 0x80);
			}
		}
	}
	return true;
}

/* JediRLE
 * Used by several Jedi Engine formats
 *******************************************************************/
bool JediRLE(const uint8_t* src, int coloffs, int width, int height, uint8_t * data) {
	for (int x = 0; x < width; ++x) {
		int p = READ_L32(src, (coloffs + (x<<2)));
		uint8_t * endcol = data + height;
		while (data < endcol) {
			int val = src[p++];
			if (val < 0x80) {
				memcpy(data, src + p, val);
				data += val;
				p += val;
			} else {
				memset(data, src[p++], val - 0x80);
				data += (val - 0x80);
			}
		}
	}
	return true;
}


class SIFJediBM : public SIFormat {
private:
	// Jedi bitmap header structs
	struct JediBMHeader {
		char magic[4];		// "BM "0x1E
		uint16_t width;		// If 1 and height not 1, multiple BM
		uint16_t height;	// If height and width both 1, 1x1 bitmap
		uint16_t wid2;		// Same as width except for multiple BM, then it's 65534
		uint16_t hei2;		// Same as height except for multiple BM, then it's number of images
		uint8_t flag;		// Determines transparency among other stuff
		uint8_t log;		// Worth either log2(height) or 0
		uint16_t rle;		// 0: not compressed, 1: compressed with RLE, 2: compressed with RLE0
		uint32_t coloffs;	// Column offsets in compressed bitmaps
		uint32_t pad[3];	// Padding, should be zero
	};
	struct JediBMSubheader {
		uint16_t width;		// If 1 and height not 1, multiple BM
		uint16_t height;	// If height and width both 1, 1x1 bitmap
		uint16_t wid2;		// Unused
		uint16_t hei2;		// Unused
		uint32_t size;		// Unused
		uint8_t log;		// Worth log2(height)
		uint8_t pad1[11];	// No known use
		uint8_t flag;		// Determines transparency among other stuff
		uint8_t pad2[3];	// No known use
	};

	unsigned getImageInfo(SImage::info_t& info, MemChunk& mc, int index, JediBMHeader* header, bool& transparent) {
		mc.read(header, sizeof(JediBMHeader), 0);
		bool multibm = false;
		transparent = !!(header->flag & 8);
		unsigned data_offset = 32;

		// Check for multiple images
		if (wxINT16_SWAP_ON_BE(header->width) == 1 && wxINT16_SWAP_ON_BE(header->height) != 1) {
			if (header->rle == 0 && wxINT16_SWAP_ON_BE(header->wid2) == 65534) {
				info.numimages = wxINT16_SWAP_ON_BE(header->hei2);
			} else {
				Global::error = "Jedi BM file: invalid multi-BM file";
				return 0;
			}
			multibm = true;
		} else info.numimages = 1;

		// Sanitize index if needed
		index %= info.numimages;
		if (index < 0)
			index = info.numimages + index;

		// Set info
		info.colformat = PALMASK;
		info.imgindex = index;

		if (multibm) {
			// 32 for the header, 2 for control data, then four bytes per subimage
			int offset_offset = 34 + (info.imgindex<<2);
			if (offset_offset + 4 > (int)mc.getSize()) {
				Global::error = "Jedi BM file: invalid multi-BM file";
				return 0;
			}
			const uint32_t * header_poffset = (uint32_t *)(mc.getData() + offset_offset);
			uint32_t header_offset = wxINT32_SWAP_ON_BE(*header_poffset)+34;
			const JediBMSubheader * subheader = (JediBMSubheader *) (mc.getData() + header_offset);
			transparent = !!(subheader->flag & 8);
			info.width = wxINT16_SWAP_ON_BE(subheader->width);
			info.height = wxINT16_SWAP_ON_BE(subheader->height);
			data_offset = header_offset + 28;
		} else {
			info.width = wxINT16_SWAP_ON_BE(header->width);
			info.height = wxINT16_SWAP_ON_BE(header->height);
		}

		// Return data offset to image at [index]
		return data_offset;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) {
		// Get image info
		SImage::info_t info;
		JediBMHeader header;
		bool transparent;
		unsigned data_offset = getImageInfo(info, data, index, &header, transparent);

		// Check valid
		if (data_offset == 0)
			return false;

		// Create image (width/height swapped because column-major)
		image.create(info.height, info.width, PALMASK);
		image.fillAlpha(255);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		if (header.rle == 0) {
			memcpy(img_data, data.getData() + data_offset, info.width * info.height);
		} else if (header.rle == 1) {
			JediRLE(data.getData() + data_offset, header.coloffs, info.height, info.width, img_data);
		} else if (header.rle == 2) {
			JediRLE0(data.getData() + data_offset, header.coloffs, info.height, info.width, img_data);
		} else {
			Global::error = "Jedi BM file: Invalid compression scheme";
			return false;
		}

		// Convert from column-major to row-major
		image.rotate(90);

		// Manage transparency
		if (transparent) {
			for (int i = 0; i < info.width*info.height; ++i) {
				if (img_data[i] == 0)
					img_mask[i] = 0;
			}
		}

		return true;
	}
	
public:
	SIFJediBM() : SIFormat("jedi_bm") {
		name = "Jedi BM";
		extension = "dat";
		reliability = 80;
	}
	~SIFJediBM() {}
	
	bool isThisFormat(MemChunk& mc) {
		if (EntryDataFormat::getFormat("img_jedi_bm")->isThisFormat(mc))
			return true;
		else
			return false;
	}
	
	SImage::info_t getInfo(MemChunk& mc, int index) {
		SImage::info_t info;
		
		JediBMHeader header;
		bool transparent;
		getImageInfo(info, mc, index, &header, transparent);
		
		return info;
	}
};


class SIFJediFME : public SIFormat {
protected:
	// Jedi engine frame header structs
	struct JediFMEHeader1 {
		int32_t offsx;		// X offset (right is positive)
		int32_t offsy;		// Y offset (down is positive)
		uint32_t flag;		// Only one flag used: 1 for flipped horizontally
		uint32_t head2;		// Offset to secondary header
		uint32_t width;		// Unused; the data from the secondary header is used instead
		uint32_t height;	// Unused; the data from the secondary header is used instead
		uint32_t pad[2];	// Padding, should be zero
	};
	struct JediFMEHeader2 {
		uint32_t width;		// Used instead of the value in the primary header
		uint32_t height;	// Used instead of the value in the primary header
		uint32_t flag;		// Only one flag used: 1 for RLE0 compression
		uint32_t size;		// Data size for compressed FME, same as file size - 32
		uint32_t coloffs;	// Offset to column data, practically always 0
		uint32_t padding;	// No known use
	};

	bool readFrame(SImage& image, MemChunk& data, unsigned offset) {
		SImage::info_t info;

		const JediFMEHeader1 * header1 = (JediFMEHeader1 *) (data.getData() + offset);
		const JediFMEHeader2 * header2 = (JediFMEHeader2 *) (data.getData() + wxINT32_SWAP_ON_BE(header1->head2));
		bool flip = !!(header1->flag & 1);
		size_t data_offset = header1->head2;

		// Setup variables
		info.offset_x = 0 - wxINT32_SWAP_ON_BE(header1->offsx);
		info.offset_y = 0 - wxINT32_SWAP_ON_BE(header1->offsy);
		info.colformat= PALMASK;

		// Little cheat here as they are defined in column-major format,
		// not row-major. So we'll just call the rotate function.
		info.height = wxINT32_SWAP_ON_BE(header2->width);
		info.width = wxINT32_SWAP_ON_BE(header2->height);

		// Create image
		image.create(info);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		// Fill data with pixel data
		if (header2->flag == 0)
			data.read(img_data + header1->head2 + 24, info.width * info.height);
		else JediRLE0(data.getData() + wxINT32_SWAP_ON_BE(header1->head2), 24,
			wxINT32_SWAP_ON_BE(header2->width), wxINT32_SWAP_ON_BE(header2->height), img_data);

		// Convert from column-major to row-major
		image.rotate(90);

		// Mirror if needed
		if (flip) image.mirror(false);

		// Manage transparency
		for (int i = 0; i < info.width*info.height; ++i) {
			if (img_data[i] == 0)
				img_mask[i] = 0;
			else
				img_mask[i] = 255;
		}

		return true;
	}

	virtual bool readImage(SImage& image, MemChunk& data, int index) {
		return readFrame(image, data, 0);
	}
	
public:
	SIFJediFME(string id = "jedi_fme") : SIFormat(id) {
		name = "Jedi FME";
		extension = "dat";
		reliability = 80;
	}
	~SIFJediFME() {}
	
	bool isThisFormat(MemChunk& mc) {
		if (EntryDataFormat::getFormat("img_jedi_fme")->isThisFormat(mc))
			return true;
		else
			return false;
	}
	
	virtual SImage::info_t getInfo(MemChunk& mc, int index) {
		SImage::info_t info;
		
		// Read headers
		const JediFMEHeader1 * header1 = (JediFMEHeader1 *) (mc.getData());
		const JediFMEHeader2 * header2 = (JediFMEHeader2 *) (mc.getData() + wxINT32_SWAP_ON_BE(header1->head2));

		// Setup variables
		info.offset_x = 0 - wxINT32_SWAP_ON_BE(header1->offsx);
		info.offset_y = 0 - wxINT32_SWAP_ON_BE(header1->offsy);
		info.width = wxINT32_SWAP_ON_BE(header2->width);
		info.height = wxINT32_SWAP_ON_BE(header2->height);
		info.colformat = PALMASK;
		info.format = id;
		
		return info;
	}
};

class SIFJediWAX : public SIFJediFME {
private:
	// Jedi engine wax header structs
	struct JediWAXHeader {
		uint32_t version;	// Constant worth 0x00100100
		uint32_t numseqs;	// Number of sequences
		uint32_t numframes;	// Number of frames
		uint32_t numcells;	// Number of cells
		uint32_t pad[4];	// Unused values
		uint32_t waxes[32];	// Offsets to the WAX subheaders
	};
	struct JediWAXSubheader {
		uint32_t width;		// Scaled width, in world units
		uint32_t height;	// Scaled height, in world units
		uint32_t fps;		// Frames per second of animation
		uint32_t pad[4];	// Unused values, must be zero
		uint32_t seqs[32];	// Offsets to sequences
	};
	struct JediWAXSequence {
		uint32_t pad[4];	// Unused values, must be zero
		uint32_t frames[32];// Offsets to frames
	};

	void readFrameOffsets(MemChunk& data, vector<unsigned>& offsets) {
		const JediWAXHeader * header = (JediWAXHeader *) data.getData();
		vector<size_t> frameoffs;
		// This is a recursive nightmare. What were the LucasArts devs smoking when they specced this format?
		for (int i = 0; i < 32; ++i) {
			uint32_t waxi = wxINT32_SWAP_ON_BE(header->waxes[i]);
			if (waxi > sizeof(header) && waxi < data.getSize() - sizeof(JediWAXSubheader)) {
				const JediWAXSubheader * subhdr = (JediWAXSubheader *) (data.getData() + waxi);
				for (int j = 0; j < 32; ++j) {
					uint32_t seqj = wxINT32_SWAP_ON_BE(subhdr->seqs[j]);
					if (seqj > sizeof(header) && seqj < data.getSize() - sizeof(JediWAXSequence)) {
						const JediWAXSequence * seq = (JediWAXSequence *) (data.getData() + seqj);
						for (int k = 0; k < 32; ++k) {
							uint32_t framk = wxINT32_SWAP_ON_BE(seq->frames[k]);
							if (framk > sizeof(header) && framk < data.getSize() - sizeof(JediWAXSequence)) {
								const JediFMEHeader1 * frame = (JediFMEHeader1 *) (data.getData() + framk);
								uint32_t cell = wxINT32_SWAP_ON_BE(frame->head2);
								if (cell > sizeof(header) && cell < data.getSize() - sizeof(JediFMEHeader2)) {
									bool notfound = true;
									for (size_t l = 0; l < frameoffs.size(); ++l) {
										if (frameoffs[l] == framk) {
											notfound = false;
											break;
										}
									}
									if (notfound) frameoffs.push_back(framk);
								}
							}
						}
					}
				}
			}
		}
		// Urgh. At least it's over now.
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) {
		// Determine all frame offsets
		vector<unsigned> offsets;
		readFrameOffsets(data, offsets);

		// Sanitize index if needed
		int numimages = offsets.size();
		if (offsets.empty()) {
			Global::error = "Jedi WAX error: No cell found in wax!"; // What a surreal error message ;)
			return false;
		}
		index %= numimages;
		if (index < 0)
			index = numimages + index;

		// Load image
		return readFrame(image, data, offsets[index]);
	}

public:
	SIFJediWAX() : SIFJediFME("jedi_wax") {
		name = "Jedi WAX";
		extension = "dat";
		reliability = 60;
	}
	~SIFJediWAX() {}
	
	bool isThisFormat(MemChunk& mc) {
		if (EntryDataFormat::getFormat("img_jedi_wax")->isThisFormat(mc))
			return true;
		else
			return false;
	}
	
	SImage::info_t getInfo(MemChunk& mc, int index) {
		SImage::info_t info;

		// Determine all frame offsets
		vector<unsigned> offsets;
		readFrameOffsets(mc, offsets);

		// Sanitize index if needed
		info.imgindex = index;
		info.numimages = offsets.size();
		if (offsets.empty())
			return info;
		info.imgindex %= info.numimages;
		if (info.imgindex < 0)
			info.imgindex = info.numimages + info.imgindex;
		
		// Read headers
		const JediFMEHeader1 * header1 = (JediFMEHeader1 *) (mc.getData() + offsets[info.imgindex]);
		const JediFMEHeader2 * header2 = (JediFMEHeader2 *) (mc.getData() + wxINT32_SWAP_ON_BE(header1->head2));

		// Setup variables
		info.offset_x = 0 - wxINT32_SWAP_ON_BE(header1->offsx);
		info.offset_y = 0 - wxINT32_SWAP_ON_BE(header1->offsy);
		info.width = wxINT32_SWAP_ON_BE(header2->width);
		info.height = wxINT32_SWAP_ON_BE(header2->height);
		info.colformat = PALMASK;
		info.format = id;
		
		return info;
	}
};
#endif