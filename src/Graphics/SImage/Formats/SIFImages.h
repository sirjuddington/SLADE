
#include "UI/WxUtils.h"
#include <wx/mstream.h>

class PNGChunk
{
public:
	PNGChunk(string_view name = "----") { memcpy(name_, name.data(), 4); }
	~PNGChunk() = default;

	string    name() const { return { name_, 4 }; }
	uint32_t  size() const { return size_; }
	uint32_t  crc() const { return crc_; }
	MemChunk& data() { return data_; }

	void read(const MemChunk& mc)
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

	bool isThisFormat(const MemChunk& mc) override
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

	SImage::Info info(const MemChunk& mc, int index) override
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
			inf.width  = wxINT32_SWAP_ON_LE(ihdr.width);
			inf.height = wxINT32_SWAP_ON_LE(ihdr.height);
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
	bool readImage(SImage& image, const MemChunk& data, int index) override
	{
		wxMemoryInputStream memstream(data.data(), data.size());
		const wxImage       wx_image(memstream, wxBITMAP_TYPE_PNG);

		// Check it created/read ok
		if (!wx_image.IsOk())
		{
			global::error = "Error reading PNG data";
			return false;
		}

		// Get image info
		int  width  = wx_image.GetWidth();
		int  height = wx_image.GetHeight();
		auto type   = wx_image.HasPalette() ? SImage::Type::PalMask : SImage::Type::RGBA;

		// Read extra info from various PNG chunks
		int32_t xoff       = 0;
		int32_t yoff       = 0;
		bool    alPh_chunk = false;
		bool    grAb_chunk = false;
		data.seek(8, SEEK_SET); // Start after PNG header
		PNGChunk chunk;
		while (true)
		{
			// Read next PNG chunk
			chunk.read(data);

			// Check for 'grAb' chunk
			if (!grAb_chunk && chunk.name() == "grAb")
			{
				// Read offsets
				chunk.data().read(&xoff, 4, 0);
				chunk.data().read(&yoff, 4);
				xoff       = wxINT32_SWAP_ON_LE(xoff);
				yoff       = wxINT32_SWAP_ON_LE(yoff);
				grAb_chunk = true;
			}

			// Check for 'alPh' chunk
			if (!alPh_chunk && chunk.name() == "alPh")
				alPh_chunk = true;

			// If both chunks are found no need to search further
			if (grAb_chunk && alPh_chunk)
				break;

			// Stop searching when we get to IDAT chunk
			if (chunk.name() == "IDAT")
				break;
		}

		// If it's a ZDoom alpha map
		if (alPh_chunk && type == SImage::Type::PalMask)
			type = SImage::Type::AlphaMap;

		// Create image
		if (type == SImage::Type::PalMask)
		{
			auto palette = wxutil::paletteFromWx(wx_image.GetPalette());
			image.create(width, height, type, &palette);
		}
		else
			image.create(width, height, type, nullptr);

		// Load image data
		auto img_data = imageData(image);
		if (type == SImage::Type::PalMask || type == SImage::Type::AlphaMap)
		{
			// Load indexed data
			auto data_rgb = wx_image.GetData();
			for (unsigned c = 0; c < width * height; c++)
			{
				// wxImage always loads as RGB, so we need to convert to palette index
				img_data[c] = wx_image.GetPalette().GetPixel(data_rgb[c * 3], data_rgb[c * 3 + 1], data_rgb[c * 3 + 2]);
			}

			// Set mask
			if (type == SImage::Type::PalMask)
			{
				auto mask = imageMask(image);
				if (auto alpha = wx_image.GetAlpha())
					memcpy(mask, alpha, width * height);
				else
					image.fillAlpha(255);
			}
		}
		else
		{
			// Load raw RGBA data
			const auto data_rgb = wx_image.GetData();
			int        c        = 0;
			if (wx_image.HasAlpha())
			{
				// Image has alpha channel
				const auto data_alpha = wx_image.GetAlpha();
				for (int a = 0; a < width * height; a++)
				{
					img_data[c++] = data_rgb[a * 3];     // Red
					img_data[c++] = data_rgb[a * 3 + 1]; // Green
					img_data[c++] = data_rgb[a * 3 + 2]; // Blue
					img_data[c++] = data_alpha[a];       // Alpha
				}
			}
			else
			{
				// No alpha channel
				for (int a = 0; a < width * height; a++)
				{
					img_data[c++] = data_rgb[a * 3];     // Red
					img_data[c++] = data_rgb[a * 3 + 1]; // Green
					img_data[c++] = data_rgb[a * 3 + 2]; // Blue
					img_data[c++] = 255;                 // Alpha
				}
			}
		}

		// Set offsets
		image.setXOffset(xoff);
		image.setYOffset(yoff);

		return true;
	}

	bool writeImage(SImage& image, MemChunk& data, const Palette* pal, int index) override
	{
		auto type   = image.type();
		int  width  = image.width();
		int  height = image.height();

		// First up, create the wxImage to be saved as png data
		wxImage wx_image;
		MemChunk data_rgb, data_alpha;
		switch (type)
		{
		case SImage::Type::RGBA:
		{
			image.putRGBData(data_rgb);
			image.putAlphaData(data_alpha);

			wx_image.Create(width, height, data_rgb.data(), data_alpha.data(), true);
			wx_image.SetOption(wxIMAGE_OPTION_PNG_FORMAT, wxPNG_TYPE_COLOUR);

			break;
		}

		case SImage::Type::PalMask:
		{
			image.putRGBData(data_rgb, pal);
			image.putAlphaData(data_alpha);

			wx_image.Create(width, height, data_rgb.data(), data_alpha.data(), true);
			wx_image.SetOption(wxIMAGE_OPTION_PNG_FORMAT, wxPNG_TYPE_PALETTE);
			wx_image.SetPalette(wxutil::paletteToWx(pal && !image.hasPalette() ? *pal : imagePalette(image)));
			wx_image.ConvertAlphaToMask(1);

			break;
		}

		case SImage::Type::AlphaMap:
		{
			image.putRGBData(data_rgb, pal);

			static Palette pal_greyscale;

			wx_image.Create(width, height, data_rgb.data(), true);
			wx_image.SetOption(wxIMAGE_OPTION_PNG_FORMAT, wxPNG_TYPE_PALETTE);
			wx_image.SetPalette(wxutil::paletteToWx(pal_greyscale));

			break;
		}

		default: // Unknown type
			log::error("Unknown image type for PNG write");
			return false;
		}

		// Write png data to memory
		wxMemoryOutputStream stream;
		wx_image.SaveFile(stream, wxBITMAP_TYPE_PNG);

		// Write PNG header and IHDR
		auto png_size = stream.GetSize();
		auto png_data = static_cast<uint8_t*>(stream.GetOutputStreamBuffer()->GetBufferStart());
		data.clear();
		data.write(png_data, 33);

		// Create grAb chunk with offsets (only if offsets exist)
		if (image.offset().x != 0 || image.offset().y != 0)
		{
			PNGChunk  grAb("grAb");
			GrabChunk gc = { wxINT32_SWAP_ON_LE((int32_t)image.offset().x),
							 wxINT32_SWAP_ON_LE((int32_t)image.offset().y) };
			grAb.setData(reinterpret_cast<const uint8_t*>(&gc), 8);
			grAb.write(data);
		}

		// Create alPh chunk if it's an alpha map
		if (type == SImage::Type::AlphaMap)
		{
			PNGChunk alPh("alPh");
			alPh.write(data);
		}

		// Write remaining PNG data
		data.write(png_data + 33, png_size - 33);

		// Success
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
};
