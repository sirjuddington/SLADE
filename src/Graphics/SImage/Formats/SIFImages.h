
class PNGChunk
{
private:
	uint32_t	size;
	char		name[4];
	MemChunk	data;
	uint32_t	crc;

public:
	PNGChunk(string name = "----")
	{
		// Init variables
		memcpy(this->name, CHR(name), 4);
		this->size = 0;
		this->crc = 0;
	}

	~PNGChunk() {}

	string		getName() { return wxString::FromAscii(name, 4); }
	uint32_t	getSize() { return size; }
	uint32_t	getCRC() { return crc; }
	MemChunk&	getData() { return data; }

	void read(MemChunk& mc)
	{
		// Read size and chunk name
		mc.read(&size, 4);
		mc.read(name, 4);

		// Endianness correction
		size = wxUINT32_SWAP_ON_LE(size);

		// Read chunk data
		data.clear();
		mc.readMC(data, size);

		// Read crc
		mc.read(&crc, 4);

		// Endianness correction
		crc = wxUINT32_SWAP_ON_LE(crc);
	}

	void write(MemChunk& mc)
	{
		// Endianness correction
		uint32_t size_swapped = wxUINT32_SWAP_ON_LE(size);
		uint32_t crc_swapped = wxUINT32_SWAP_ON_LE(crc);

		// Write chunk data
		mc.write(&size_swapped, 4);
		mc.write(name, 4);
		mc.write(data.getData(), size);
		mc.write(&crc_swapped, 4);
	}

	void setData(const uint8_t* data, uint32_t size)
	{
		// Read given data
		this->data.clear();
		this->data.write(data, size);

		// Update variables
		this->size = size;

		// Note that the CRC is not computed just from
		// the chunk data, but also the chunk name! So
		// we need to write all that in a temporary MC
		MemChunk fulldata;
		fulldata.reSize(4 + size);
		fulldata.seek(0, SEEK_SET);
		fulldata.write(name, 4);
		fulldata.write(data, size);
		crc = fulldata.crc();
		fulldata.clear();
	}

	void setData(MemChunk& mc)
	{
		setData(mc.getData(), mc.getSize());
	}
};

// TODO: Keep PNG chunks in SImage so they are preserved between load/save
class SIFPng : public SIFormat
{
private:
	// grAb chunk struct
	struct grab_chunk_t
	{
		int32_t xoff;
		int32_t yoff;
	};

	// IHDR chunk struct
	struct ihdr_chunk_t
	{
		uint32_t	width;
		uint32_t	height;
		uint8_t		bpp;
		uint8_t		coltype;
		uint8_t		compression;
		uint8_t		filter;
		uint8_t		interlace;
	};

protected:
	bool readImage(SImage& image, MemChunk& data, int index)
	{
		// Create FreeImage bitmap from entry data
		FIMEMORY* mem = FreeImage_OpenMemory((BYTE*)data.getData(), data.getSize());
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem, 0);
		FIBITMAP* bm = FreeImage_LoadFromMemory(fif, mem, 0);
		FreeImage_CloseMemory(mem);

		// Check it created/read ok
		if (!bm)
		{
			Global::error = "Error reading PNG data";
			return false;
		}

		// Get image info
		int width = FreeImage_GetWidth(bm);
		int height = FreeImage_GetHeight(bm);
		int bpp = FreeImage_GetBPP(bm);
		SIType type = RGBA;

		// Read extra info from various PNG chunks
		int32_t xoff = 0;
		int32_t yoff = 0;
		bool alPh_chunk = false;
		bool grAb_chunk = false;
		data.seek(8, SEEK_SET);	// Start after PNG header
		PNGChunk chunk;
		while (true)
		{
			// Read next PNG chunk
			chunk.read(data);

			// Check for 'grAb' chunk
			if (!grAb_chunk && chunk.getName() == "grAb")
			{
				// Read offsets
				chunk.getData().read(&xoff, 4, 0);
				chunk.getData().read(&yoff, 4);
				xoff = wxINT32_SWAP_ON_LE(xoff);
				yoff = wxINT32_SWAP_ON_LE(yoff);
				grAb_chunk = true;
			}

			// Check for 'alPh' chunk
			if (!alPh_chunk && chunk.getName() == "alPh")
				alPh_chunk = true;

			// If both chunks are found no need to search further
			if (grAb_chunk && alPh_chunk)
				break;

			// Stop searching when we get to IDAT chunk
			if (chunk.getName() == "IDAT")
				break;
		}

		// Get image palette if it exists
		RGBQUAD* bm_pal = FreeImage_GetPalette(bm);
		Palette8bit palette;
		if (bpp == 8 && bm_pal)
		{
			type = PALMASK;
			int a = 0;
			int b = FreeImage_GetColorsUsed(bm);
			if (b > 256)
				b = 256;
			for (; a < b; a++)
				palette.setColour(a, rgba_t(bm_pal[a].rgbRed, bm_pal[a].rgbGreen, bm_pal[a].rgbBlue, 255));
		}

		// If it's a ZDoom alpha map
		if (alPh_chunk && bpp == 8)
			type = ALPHAMAP;

		// Create image
		if (bm_pal)
			image.create(width, height, type, &palette);
		else
			image.create(width, height, type, NULL);

		// Load image data
		uint8_t* img_data = imageData(image);
		if (type == PALMASK || type == ALPHAMAP)
		{
			// Flip vertically
			FreeImage_FlipVertical(bm);

			// Load indexed data
			unsigned c = 0;
			for (int row = 0; row < height; row++)
			{
				uint8_t* scanline = FreeImage_GetScanLine(bm, row);
				for (int x = 0; x < width; x++)
					img_data[c++] = scanline[x];
			}

			// Set mask
			if (type == PALMASK)
			{
				uint8_t* mask = imageMask(image);
				uint8_t* alphatable = FreeImage_GetTransparencyTable(bm);
				if (alphatable)
				{
					for (int a = 0; a < width * height; a++)
						mask[a] = alphatable[img_data[a]];
				}
				else
					image.fillAlpha(255);
			}
		}
		else if (type == RGBA)
		{
			// Convert to 32bpp & flip vertically
			FIBITMAP* rgb = FreeImage_ConvertTo32Bits(bm);
			FreeImage_FlipVertical(rgb);

			// Load raw RGBA data
			uint8_t* bits_rgb = FreeImage_GetBits(rgb);
			int c = 0;
			for (int a = 0; a < width * height; a++)
			{
				img_data[c++] = bits_rgb[a * 4 + 2];	// Red
				img_data[c++] = bits_rgb[a * 4 + 1];	// Green
				img_data[c++] = bits_rgb[a * 4];		// Blue
				img_data[c++] = bits_rgb[a * 4 + 3];	// Alpha
			}

			FreeImage_Unload(rgb);
		}

		// Set offsets
		image.setXOffset(xoff);
		image.setYOffset(yoff);

		// Clean up
		FreeImage_Unload(bm);

		return true;
	}

	bool writeImage(SImage& image, MemChunk& data, Palette8bit* pal, int index)
	{
		// Variables
		FIBITMAP*	bm = NULL;
		uint8_t*	img_data = imageData(image);
		uint8_t*	img_mask = imageMask(image);
		int			type = image.getType();
		int			width = image.getWidth();
		int			height = image.getHeight();

		if (type == RGBA)
		{
			// Init 32bpp FIBITMAP
			bm = FreeImage_Allocate(width, height, 32, 0x0000FF00, 0x00FF0000, 0x000000FF);

			// Write image data
			uint8_t* bits = FreeImage_GetBits(bm);
			uint32_t c = 0;
			for (int a = 0; a < width * height * 4; a += 4)
			{
				bits[c++] = img_data[a+2];
				bits[c++] = img_data[a+1];
				bits[c++] = img_data[a];
				bits[c++] = img_data[a+3];
			}
		}
		else if (type == PALMASK)
		{
			// Init 8bpp FIBITMAP
			bm = FreeImage_Allocate(width, height, 8);

			// Get palette to use
			Palette8bit usepal;
			if (image.hasPalette())
				usepal.copyPalette(&imagePalette(image));
			else if (pal)
				usepal.copyPalette(pal);

			// Set palette
			RGBQUAD* bm_pal = FreeImage_GetPalette(bm);
			for (int a = 0; a < 256; a++)
			{
				bm_pal[a].rgbRed = usepal.colour(a).r;
				bm_pal[a].rgbGreen = usepal.colour(a).g;
				bm_pal[a].rgbBlue = usepal.colour(a).b;
			}

			// Handle transparency if needed
			if (img_mask)
			{
				if (usepal.transIndex() < 0)
				{
					// Find unused colour (for transparency)
					short unused = image.findUnusedColour();

					// Set any transparent pixels to this colour (if we found an unused colour)
					bool has_trans = false;
					if (unused >= 0)
					{
						for (int a = 0; a < width * height; a++)
						{
							if (img_mask[a] == 0)
							{
								img_data[a] = unused;
								has_trans = true;
							}
						}

						// Set palette transparency
						if (has_trans)
							usepal.setTransIndex(unused);
					}
				}

				// Set freeimage palette transparency if needed
				if (usepal.transIndex() >= 0)
					FreeImage_SetTransparentIndex(bm, usepal.transIndex());
			}

			// Write image data
			for (int row = 0; row < height; row++)
			{
				uint8_t* scanline = FreeImage_GetScanLine(bm, row);
				memcpy(scanline, img_data + (row * width), width);
			}
		}
		else if (type == ALPHAMAP)
		{
			// Init 8bpp FIBITMAP
			bm = FreeImage_Allocate(width, height, 8);

			// Set palette (greyscale)
			RGBQUAD* bm_pal = FreeImage_GetPalette(bm);
			for (int a = 0; a < 256; a++)
			{
				bm_pal[a].rgbRed = a;
				bm_pal[a].rgbGreen = a;
				bm_pal[a].rgbBlue = a;
			}

			// Write image data
			for (int row = 0; row < height; row++)
			{
				uint8_t* scanline = FreeImage_GetScanLine(bm, row);
				memcpy(scanline, img_data + (row * width), width);
			}
		}
		else
			return false;

		// Flip the image
		FreeImage_FlipVertical(bm);

		// Write the image to a temp file
		FreeImage_Save(FIF_PNG, bm, CHR(App::path("temp.png", App::Dir::Temp)));

		// Load it into a memchunk
		MemChunk png;
		png.importFile(App::path("temp.png", App::Dir::Temp));

		// Check it loaded ok
		if (png.getSize() == 0)
		{
			LOG_MESSAGE(1, "Error reading temporary file");
			return false;
		}

		// Write PNG header and IHDR
		const uint8_t* png_data = png.getData();
		data.clear();
		data.write(png_data, 33);

		// Create grAb chunk with offsets (only if offsets exist)
		if (image.offset().x != 0 || image.offset().y != 0)
		{
			PNGChunk grAb("grAb");
			grab_chunk_t gc = { wxINT32_SWAP_ON_LE((int32_t)image.offset().x), wxINT32_SWAP_ON_LE((int32_t)image.offset().y) };
			grAb.setData((const uint8_t*)&gc, 8);
			grAb.write(data);
		}

		// Create alPh chunk if it's an alpha map
		if (type == ALPHAMAP)
		{
			PNGChunk alPh("alPh");
			alPh.write(data);
		}

		// Write remaining PNG data
		data.write(png_data + 33, png.getSize() - 33);

		// Clean up
		wxRemoveFile(App::path("temp.png", App::Dir::Temp));

		// Success
		return true;
	}

public:
	SIFPng() : SIFormat("png")
	{
		name = "PNG";
		extension = "png";
	}

	bool isThisFormat(MemChunk& mc)
	{
		// Reset MemChunk
		mc.seek(0, SEEK_SET);

		// Check size
		if (mc.getSize() > 8)
		{
			// Check for PNG header
			if (mc[0] == 137 && mc[1] == 80 &&
			        mc[2] == 78 && mc[3] == 71 &&
			        mc[4] == 13 && mc[5] == 10 &&
			        mc[6] == 26 && mc[7] == 10)
				return true;
		}

		return false;
	}

	SImage::info_t getInfo(MemChunk& mc, int index)
	{
		SImage::info_t inf;
		inf.format = "png";
		inf.width = 0;
		inf.height = 0;

		// Read first chunk
		mc.seek(8, SEEK_SET);
		PNGChunk chunk;
		chunk.read(mc);
		// Should be IHDR
		int bpp = 32;
		if (chunk.getName() == "IHDR")
		{
			// Read IHDR data
			ihdr_chunk_t ihdr;
			chunk.getData().read(&ihdr, 13, 0);

			// Set info from IHDR
			inf.width = ihdr.width;
			inf.height = ihdr.height;
			bpp = ihdr.bpp;
			if (ihdr.coltype == 3 && ihdr.bpp == 8)
			{
				// Only 8bpp 'indexed' pngs are counted as PALMASK for now, all others will be converted to RGBA
				inf.colformat = PALMASK;
				inf.has_palette = true;
			}
			else
				inf.colformat = RGBA;
		}

		// Look for other info chunks (grAb or alPh)
		while (1)
		{
			chunk.read(mc);

			// Set format to alpha map if alPh present (and 8bpp)
			if (bpp == 8 && chunk.getName() == "alPh")
				inf.colformat = ALPHAMAP;

			// Set offsets if grAb present
			else if (chunk.getName() == "grAb")
			{
				// Read offsets
				int32_t xoff, yoff;
				chunk.getData().read(&xoff, 4, 0);
				chunk.getData().read(&yoff, 4);
				inf.offset_x = wxINT32_SWAP_ON_LE(xoff);
				inf.offset_y = wxINT32_SWAP_ON_LE(yoff);
			}

			// Stop on IDAT chunk
			else if (chunk.getName() == "IDAT")
				break;
		}

		return inf;
	}

	int canWrite(SImage& image)
	{
		// PNG format is always writable
		return WRITABLE;
	}

	bool canWriteType(SIType type)
	{
		// PNG format is always writable
		return true;
	}

	bool convertWritable(SImage& image, convert_options_t opt)
	{
		// Just convert to requested colour type

		// Paletted
		if (opt.col_format == PALMASK)
		{
			// Convert mask
			if (opt.mask_source == MASK_ALPHA)
				image.cutoffMask(opt.alpha_threshold);
			else if (opt.mask_source == MASK_COLOUR)
				image.maskFromColour(opt.mask_colour, opt.pal_current);
			else
				image.fillAlpha(255);

			// Convert colours
			image.convertPaletted(opt.pal_target, opt.pal_current);
		}

		// RGBA
		else if (opt.col_format == RGBA)
		{
			image.convertRGBA(opt.pal_current);

			// Convert alpha channel
			if (opt.mask_source == MASK_COLOUR)
				image.maskFromColour(opt.mask_colour, opt.pal_current);
			else if (opt.mask_source == MASK_BRIGHTNESS)
				image.maskFromBrightness(opt.pal_current);
		}

		// Alpha Map
		else if (opt.col_format == ALPHAMAP)
		{
			if (opt.mask_source == SIFormat::MASK_ALPHA)
				image.convertAlphaMap(SImage::ALPHA, opt.pal_current);
			else if (opt.mask_source == SIFormat::MASK_COLOUR)
			{
				image.maskFromColour(opt.mask_colour, opt.pal_current);
				image.convertAlphaMap(SImage::ALPHA, opt.pal_current);
			}
			else
				image.convertAlphaMap(SImage::BRIGHTNESS, opt.pal_current);
		}

		// If transparency is disabled
		if (!opt.transparency)
			image.fillAlpha(255);

		return true;
	}

	virtual bool writeOffset(SImage& image, ArchiveEntry* entry, point2_t offset)
	{
		MemChunk mc;
		image.setXOffset(offset.x);
		image.setYOffset(offset.y);
		return (writeImage(image, mc, NULL, 0) && entry->importMemChunk(mc));
	}
};
