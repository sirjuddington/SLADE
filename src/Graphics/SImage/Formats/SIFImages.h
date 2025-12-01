
#include "UI/WxUtils.h"
#include <algorithm>
#include <array>
#include <png.h>

class PNGChunk
{
public:
	PNGChunk(string_view name = "----") { memcpy(name_, name.data(), 4); }
	~PNGChunk() = default;

	string    name() const { return string(name_, 4); }
	uint32_t  size() const { return size_; }
	uint32_t  crc() const { return crc_; }
	MemChunk& data() { return data_; }

	void read(MemChunk& mc)
	{
		// Read size and chunk name
		mc.read(&size_, 4);
		mc.read(name_, 4);

		// Endianness correction
		size_ = wxUINT32_SWAP_ON_LE(size_);

		// Read chunk data
		data_.clear();
		mc.readMC(data_, size_);

		// Read crc
		mc.read(&crc_, 4);

		// Endianness correction
		crc_ = wxUINT32_SWAP_ON_LE(crc_);
	}

	void write(MemChunk& mc) const
	{
		// Endianness correction
		uint32_t size_swapped = wxUINT32_SWAP_ON_LE(size_);
		uint32_t crc_swapped  = wxUINT32_SWAP_ON_LE(crc_);

		// Write chunk data
		mc.write(&size_swapped, 4);
		mc.write(name_, 4);
		mc.write(data_.data(), size_);
		mc.write(&crc_swapped, 4);
	}

	void setData(const uint8_t* data, uint32_t size)
	{
		// Read given data
		data_.clear();
		data_.write(data, size);

		// Update variables
		size_ = size;

		// Note that the CRC is not computed just from
		// the chunk data, but also the chunk name! So
		// we need to write all that in a temporary MC
		MemChunk fulldata;
		fulldata.reSize(4 + size);
		fulldata.seek(0, SEEK_SET);
		fulldata.write(name_, 4);
		fulldata.write(data, size);
		crc_ = fulldata.crc();
		fulldata.clear();
	}

	void setData(MemChunk& mc) { setData(mc.data(), mc.size()); }

private:
	uint32_t size_ = 0;
	char     name_[4];
	MemChunk data_;
	uint32_t crc_ = 0;
};

// TODO: Keep PNG chunks in SImage so they are preserved between load/save
class SIFPng : public SIFormat
{
public:
	SIFPng() : SIFormat("png", "PNG", "png") {}

	bool isThisFormat(MemChunk& mc) override
	{
		// Reset MemChunk
		mc.seek(0, SEEK_SET);

		// Check size
		if (mc.size() > 8)
		{
			// Check for PNG header
			if (mc[0] == 137 && mc[1] == 80 && mc[2] == 78 && mc[3] == 71 && mc[4] == 13 && mc[5] == 10 && mc[6] == 26
				&& mc[7] == 10)
				return true;
		}

		return false;
	}

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info inf;
		inf.format = "png";
		inf.width  = 0;
		inf.height = 0;

		// Read first chunk
		mc.seek(8, SEEK_SET);
		PNGChunk chunk;
		chunk.read(mc);
		// Should be IHDR
		int bpp = 32;
		if (chunk.name() == "IHDR")
		{
			// Read IHDR data
			IhdrChunk ihdr;
			chunk.data().read(&ihdr, 13, 0);

			// Set info from IHDR
			inf.width  = ihdr.width;
			inf.height = ihdr.height;
			bpp        = ihdr.bpp;
			if (ihdr.coltype == 3 && ihdr.bpp == 8)
			{
				// Only 8bpp 'indexed' pngs are counted as PalMask for now, all others will be converted to RGBA
				inf.colformat   = SImage::Type::PalMask;
				inf.has_palette = true;
			}
			else
				inf.colformat = SImage::Type::RGBA;
		}

		// Look for other info chunks (grAb or alPh)
		while (true)
		{
			chunk.read(mc);

			// Set format to alpha map if alPh present (and 8bpp)
			if (bpp == 8 && chunk.name() == "alPh")
				inf.colformat = SImage::Type::AlphaMap;

			// Set offsets if grAb present
			else if (chunk.name() == "grAb")
			{
				// Read offsets
				int32_t xoff, yoff;
				chunk.data().read(&xoff, 4, 0);
				chunk.data().read(&yoff, 4);
				inf.offset_x = wxINT32_SWAP_ON_LE(xoff);
				inf.offset_y = wxINT32_SWAP_ON_LE(yoff);
			}

			// Stop on IDAT chunk
			else if (chunk.name() == "IDAT")
				break;
		}

		return inf;
	}

	Writable canWrite(SImage& image) override
	{
		// PNG format is always writable
		return Writable::Yes;
	}

	bool canWriteType(SImage::Type type) override
	{
		// PNG format is always writable
		return true;
	}

	bool convertWritable(SImage& image, ConvertOptions opt) override
	{
		// Just convert to requested colour type

		// Paletted
		if (opt.col_format == SImage::Type::PalMask)
		{
			// Convert mask
			if (opt.mask_source == Mask::Alpha)
				image.cutoffMask(opt.alpha_threshold);
			else if (opt.mask_source == Mask::Colour)
				image.maskFromColour(opt.mask_colour, opt.pal_current);
			else
				image.fillAlpha(255);

			// Convert colours
			image.convertPaletted(opt.pal_target, opt.pal_current);
		}

		// RGBA
		else if (opt.col_format == SImage::Type::RGBA)
		{
			image.convertRGBA(opt.pal_current);

			// Convert alpha channel
			if (opt.mask_source == Mask::Colour)
				image.maskFromColour(opt.mask_colour, opt.pal_current);
			else if (opt.mask_source == Mask::Brightness)
				image.maskFromBrightness(opt.pal_current);
		}

		// Alpha Map
		else if (opt.col_format == SImage::Type::AlphaMap)
		{
			if (opt.mask_source == Mask::Alpha)
				image.convertAlphaMap(SImage::AlphaSource::Alpha, opt.pal_current);
			else if (opt.mask_source == Mask::Colour)
			{
				image.maskFromColour(opt.mask_colour, opt.pal_current);
				image.convertAlphaMap(SImage::AlphaSource::Alpha, opt.pal_current);
			}
			else
				image.convertAlphaMap(SImage::AlphaSource::Brightness, opt.pal_current);
		}

		// If transparency is disabled
		if (!opt.transparency)
			image.fillAlpha(255);

		return true;
	}

	bool writeOffset(SImage& image, ArchiveEntry* entry, Vec2i offset) override
	{
		MemChunk mc;
		image.setXOffset(offset.x);
		image.setYOffset(offset.y);
		return (writeImage(image, mc, nullptr, 0) && entry->importMemChunk(mc));
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Check for grAb (offsets) and alPh chunks
		int32_t xoff = 0, yoff = 0;
		bool    alPh = false;
		{
			MemChunk scan;
			scan.importMem(data);
			scan.seek(8, SEEK_SET);
			PNGChunk chunk;
			while (true)
			{
				chunk.read(scan);
				if (chunk.name() == "grAb")
				{
					chunk.data().read(&xoff, 4, 0);
					chunk.data().read(&yoff, 4);
					xoff = wxINT32_SWAP_ON_LE(xoff);
					yoff = wxINT32_SWAP_ON_LE(yoff);
				}
				else if (chunk.name() == "alPh")
					alPh = true;
				if (chunk.name() == "IDAT")
					break;
			}
		}

		// Setup libpng for reading from memory
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png_ptr)
		{
			global::error = "Error initialising libpng";
			return false;
		}
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_read_struct(&png_ptr, nullptr, nullptr);
			global::error = "Error initialising libpng info";
			return false;
		}
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
			global::error = "Error reading PNG data";
			return false;
		}

		PngReadContext ctx{ data.data(), data.size(), 0 };
		png_set_read_fn(png_ptr, &ctx, pngReadData);

		// Read header
		png_read_info(png_ptr, info_ptr);
		png_uint_32 width = 0, height = 0;
		int         bit_depth = 0, color_type = 0, interlace = 0, comp = 0, filter = 0;
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace, &comp, &filter);

		// Determine image type
		SImage::Type type = SImage::Type::RGBA;
		if (alPh)
			type = SImage::Type::AlphaMap;
		else if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth == 8)
			type = SImage::Type::PalMask;

		// Setup png options based on type
		if (type == SImage::Type::PalMask)
		{
			if (bit_depth < 8)
				png_set_packing(png_ptr);
		}
		else if (type == SImage::Type::AlphaMap)
		{
			// Convert everything to 8-bit RGB so we can use red channel as alpha map
			if (color_type == PNG_COLOR_TYPE_PALETTE)
				png_set_palette_to_rgb(png_ptr);
			if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
				png_set_expand_gray_1_2_4_to_8(png_ptr);
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
				png_set_tRNS_to_alpha(png_ptr);
		}
		else
		{
			// RGBA
			if (color_type == PNG_COLOR_TYPE_PALETTE)
				png_set_palette_to_rgb(png_ptr);
			if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
				png_set_gray_to_rgb(png_ptr);
			if (bit_depth < 8)
				png_set_expand(png_ptr);
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
				png_set_tRNS_to_alpha(png_ptr);
			if (!(color_type & PNG_COLOR_MASK_ALPHA))
				png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
		}

		png_read_update_info(png_ptr, info_ptr);

		// Create image
		if (type == SImage::Type::PalMask)
		{
			Palette    pal(256);
			png_colorp plte = nullptr;
			int        num  = 0;
			if (png_get_PLTE(png_ptr, info_ptr, &plte, &num) & PNG_INFO_PLTE)
			{
				for (int i = 0; i < num && i < 256; ++i)
				{
					ColRGBA c{ plte[i].red, plte[i].green, plte[i].blue, 255 };
					pal.setColour(i, c);
				}
			}
			image.create(static_cast<int>(width), static_cast<int>(height), SImage::Type::PalMask, &pal);

			// Read indexed data
			vector<png_bytep> rows(height);
			for (png_uint_32 y = 0; y < height; ++y)
				rows[y] = imageData(image) + y * width;
			png_read_image(png_ptr, rows.data());

			// Build mask from tRNS if present
			png_bytep trans_alpha = nullptr;
			int       num_trans   = 0;
			if (png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, nullptr) & PNG_INFO_tRNS)
			{
				auto mask = imageMask(image);
				for (png_uint_32 y = 0; y < height; ++y)
				{
					for (png_uint_32 x = 0; x < width; ++x)
					{
						uint8_t idx         = rows[y][x];
						mask[y * width + x] = (idx < num_trans) ? trans_alpha[idx] : 255;
					}
				}
			}
			else
				image.fillAlpha(255);
		}
		else if (type == SImage::Type::AlphaMap)
		{
			image.create(static_cast<int>(width), static_cast<int>(height), SImage::Type::AlphaMap, nullptr);
			const auto      rowbytes = png_get_rowbytes(png_ptr, info_ptr);
			vector<uint8_t> row(rowbytes);
			for (png_uint_32 y = 0; y < height; ++y)
			{
				png_read_row(png_ptr, row.data(), nullptr);
				const int bpp = static_cast<int>(rowbytes / width);
				for (png_uint_32 x = 0; x < width; ++x)
					imageData(image)[y * width + x] = row[x * bpp];
			}
		}
		else // RGBA
		{
			image.create(static_cast<int>(width), static_cast<int>(height), SImage::Type::RGBA, nullptr);
			vector<png_bytep> rows(height);
			for (png_uint_32 y = 0; y < height; ++y)
				rows[y] = imageData(image) + y * width * 4;
			png_read_image(png_ptr, rows.data());
		}

		png_read_end(png_ptr, nullptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

		// Set offsets
		image.setXOffset(xoff);
		image.setYOffset(yoff);

		return true;
	}

	bool writeImage(SImage& image, MemChunk& out, Palette* pal, int index) override
	{
		// Setup libpng for writing to memory
		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png_ptr)
			return false;
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_write_struct(&png_ptr, nullptr);
			return false;
		}
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			png_destroy_write_struct(&png_ptr, &info_ptr);
			return false;
		}

		out.clear();
		PngWriteContext wctx{ &out };
		png_set_write_fn(png_ptr, &wctx, pngWriteData, pngFlush);

		auto type   = image.type();
		int  width  = image.width();
		int  height = image.height();

		// IHDR
		int color_type = PNG_COLOR_TYPE_RGBA;
		int bit_depth  = 8;
		if (type == SImage::Type::PalMask)
			color_type = PNG_COLOR_TYPE_PALETTE;
		else if (type == SImage::Type::AlphaMap)
			color_type = PNG_COLOR_TYPE_GRAY;
		png_set_IHDR(
			png_ptr,
			info_ptr,
			width,
			height,
			bit_depth,
			color_type,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);

		// Palette
		if (type == SImage::Type::PalMask)
		{
			const auto used_pal = (pal && !image.hasPalette()) ? pal : image.palette();
			png_color  palette_plte[256]{};
			for (int i = 0; i < 256; ++i)
			{
				const auto c    = used_pal->colour(i);
				palette_plte[i] = { c.r, c.g, c.b };
			}
			png_set_PLTE(png_ptr, info_ptr, palette_plte, 256);

			// Build per-index transparency (tRNS) from mask: take minimum alpha across all pixels of an index
			std::array<uint8_t, 256> trans;
			trans.fill(255);
			bool       any_trans   = false;
			int        max_used_ix = -1;
			const auto data        = imageData(image);
			if (const auto mask = imageMask(image))
			{
				for (int i = 0; i < width * height; ++i)
				{
					const uint8_t pi = data[i];
					const uint8_t a  = mask[i];
					trans[pi]        = std::min(a, trans[pi]);
					max_used_ix      = std::max<int>(pi, max_used_ix);
				}
				for (int i = 0; i <= max_used_ix; ++i)
				{
					if (trans[i] != 255)
					{
						any_trans = true;
						break;
					}
				}
			}
			if (any_trans)
			{
				int count = (max_used_ix >= 0) ? (max_used_ix + 1) : 0;
				if (count < 1)
					count = 256; // fallback
				png_set_tRNS(png_ptr, info_ptr, trans.data(), count, nullptr);
			}
		}

		png_write_info(png_ptr, info_ptr);

		// Write grAb/alPh chunks before IDAT
		if (image.offset().x != 0 || image.offset().y != 0)
		{
			uint8_t grab[8];
			int32_t x = wxINT32_SWAP_ON_LE((int32_t)image.offset().x);
			int32_t y = wxINT32_SWAP_ON_LE((int32_t)image.offset().y);
			memcpy(grab + 0, &x, 4);
			memcpy(grab + 4, &y, 4);
			png_write_chunk(png_ptr, reinterpret_cast<png_bytep>("grAb"), grab, 8);
		}
		if (type == SImage::Type::AlphaMap)
		{
			png_write_chunk(png_ptr, reinterpret_cast<png_bytep>("alPh"), nullptr, 0);
		}

		// Write rows
		if (type == SImage::Type::RGBA)
		{
			vector<png_bytep> rows(height);
			for (int y = 0; y < height; ++y)
				rows[y] = imageData(image) + y * width * 4;
			png_write_image(png_ptr, rows.data());
		}
		else // PalMask or AlphaMap (both 1 byte per pixel)
		{
			vector<png_bytep> rows(height);
			for (int y = 0; y < height; ++y)
				rows[y] = imageData(image) + y * width;
			png_write_image(png_ptr, rows.data());
		}

		png_write_end(png_ptr, info_ptr);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return true;
	}

private:
	// grAb chunk struct
	struct GrabChunk
	{
		int32_t xoff;
		int32_t yoff;
	};

	// IHDR chunk struct
	struct IhdrChunk
	{
		uint32_t width;
		uint32_t height;
		uint8_t  bpp;
		uint8_t  coltype;
		uint8_t  compression;
		uint8_t  filter;
		uint8_t  interlace;
	};

	// libpng IO helpers
	struct PngReadContext
	{
		const uint8_t* data;
		size_t         size;
		size_t         offset;
	};

	struct PngWriteContext
	{
		MemChunk* out;
	};

	static void pngReadData(png_structp png_ptr, png_bytep outBytes, png_size_t byteCount)
	{
		auto* ctx = static_cast<PngReadContext*>(png_get_io_ptr(png_ptr));
		if (!ctx || ctx->offset + byteCount > ctx->size)
			png_error(png_ptr, "png read beyond end of buffer");
		memcpy(outBytes, ctx->data + ctx->offset, byteCount);
		ctx->offset += byteCount;
	}

	static void pngWriteData(png_structp png_ptr, png_bytep inBytes, png_size_t byteCount)
	{
		auto* ctx = static_cast<PngWriteContext*>(png_get_io_ptr(png_ptr));
		if (!ctx || !ctx->out)
			png_error(png_ptr, "png write invalid context");
		ctx->out->write(inBytes, static_cast<uint32_t>(byteCount));
	}

	static void pngFlush(png_structp)
	{
		// Not needed for MemChunk
	}
};
