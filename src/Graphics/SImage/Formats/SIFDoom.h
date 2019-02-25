
#include "Graphics/GameFormats.h"

class SIFDoomGfx : public SIFormat
{
public:
	SIFDoomGfx(std::string_view id = "doom", std::string_view name = "Doom Gfx", int reliability = 230) :
		SIFormat(id, name, "lmp", reliability)
	{
	}
	~SIFDoomGfx() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::format("img_doom")->isThisFormat(mc))
			return true;
		else
			return false;
	}

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		Graphics::PatchHeader hdr;
		mc.read(&hdr, 8, 0);

		// Setup info
		info.width       = hdr.width;
		info.height      = hdr.height;
		info.offset_x    = hdr.left;
		info.offset_y    = hdr.top;
		info.colformat   = SImage::Type::PalMask;
		info.has_palette = false;
		info.format      = id_;

		return info;
	}

	Writable canWrite(SImage& image) override
	{
		// Must be converted to paletted to be written
		if (image.type() == SImage::Type::PalMask)
			return Writable::Yes;
		else
			return Writable::Convert;
	}

	bool canWriteType(SImage::Type type) override
	{
		// Doom format gfx can only be written as paletted
		if (type == SImage::Type::PalMask)
			return true;
		else
			return false;
	}

	bool convertWritable(SImage& image, ConvertOptions opt) override
	{
		// Do mask conversion
		if (!opt.transparency)
			image.fillAlpha(255);
		else if (opt.mask_source == Mask::Colour)
			image.maskFromColour(opt.mask_colour, opt.pal_target);
		else if (opt.mask_source == Mask::Alpha)
			image.cutoffMask(opt.alpha_threshold);

		// Convert to paletted
		image.convertPaletted(opt.pal_target, opt.pal_current);

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
	bool readDoomFormat(SImage& image, MemChunk& data, int version) const
	{
		// Init variables
		auto gfx_data = data.data();
		int  width    = 0;
		int  height   = 0;
		int  offset_x = 0;
		int  offset_y = 0;

		// Read header
		uint8_t hdr_size = 0;
		if (version > 1)
		{
			width    = gfx_data[0];
			height   = gfx_data[1];
			offset_x = (int8_t)gfx_data[2];
			offset_y = (int8_t)gfx_data[3];
			hdr_size = 4;
		}
		else
		{
			auto header = (Graphics::PatchHeader*)gfx_data;
			width       = wxINT16_SWAP_ON_BE(header->width);
			height      = wxINT16_SWAP_ON_BE(header->height);
			offset_x    = wxINT16_SWAP_ON_BE(header->left);
			offset_y    = wxINT16_SWAP_ON_BE(header->top);
			hdr_size    = 8;
		}

		// Create image
		image.create(width, height, SImage::Type::PalMask);

		// Read column offsets
		vector<uint32_t> col_offsets(width);
		if (version > 0)
		{
			auto c_ofs = (uint16_t*)((uint8_t*)gfx_data + hdr_size);
			for (int a = 0; a < width; a++)
				col_offsets[a] = wxUINT16_SWAP_ON_BE(c_ofs[a]);
		}
		else
		{
			auto c_ofs = (uint32_t*)((uint8_t*)gfx_data + hdr_size);
			for (int a = 0; a < width; a++)
				col_offsets[a] = wxUINT32_SWAP_ON_BE(c_ofs[a]);
		}

		// Load data
		uint8_t* img_data = imageData(image);
		memset(img_data, 0, width * height); // Set colour to palette index 0
		uint8_t* img_mask = imageMask(image);
		memset(img_mask, 0, width * height); // Set mask to fully transparent

		// Check for the Pleiades hack:
		// Roger Ritenour's pleiades.wad for ZDoom uses 256-tall sky textures,
		// and since the patch format uses 8-bit values for the length of a column,
		// the 256 height overflows to 0. To detect this situation, we check if
		// every column represents precisely 261 bytes, in other words just enough
		// for a single post of 256 pixels.
		bool pleiadeshack = false;
		if (height == 256)
		{
			pleiadeshack = true;
			for (int c = 1; c < width; ++c)
			{
				if (col_offsets[c] - col_offsets[c - 1] != 261)
				{
					pleiadeshack = false;
					break;
				}
			}
			if (data.size() - col_offsets[width - 1] != 261)
				pleiadeshack = false;
		}

		// Load data
		for (int c = 0; c < width; c++)
		{
			// Get current column offset (byteswap if needed)
			auto col_offset = col_offsets[c]; // wxUINT32_SWAP_ON_BE(col_offsets[c]);

			// Check column offset is valid
			if (col_offset >= data.size())
				return false;

			// Go to start of column
			const uint8_t* bits = gfx_data;
			bits += col_offset;

			// Read posts
			int top = -1;
			while (true)
			{
				// Get row offset
				auto row = *bits;

				if (row == 0xFF) // End of column?
					break;

				// Tall patches support
				if (row <= top && version == 0)
					top += row;
				else
					top = row;

				// Get no. of pixels
				bits++;
				uint16_t n_pix = *bits;

				// If this is a Pleiades sky, the height is 256.
				if (pleiadeshack)
					n_pix = 256;

				if (version == 0)
					bits++; // Skip buffer
				for (uint16_t p = 0; p < n_pix; p++)
				{
					// Get pixel position
					bits++;
					int pos = ((top + p) * width + c);

					// Stop if we're outside the image
					if (pos >= width * height)
						break;

					// Stop if for some reason we're outside the gfx data
					if (bits >= gfx_data + data.size())
						break;

					// Fail if bogus data gives a negative pos (this corrupts the heap!)
					if (pos < 0)
						return false;

					// Write pixel data
					img_data[pos] = *bits;
					img_mask[pos] = 255;
				}
				if (version == 0)
					bits++; // Skip buffer
				bits++;     // Go to next row offset
			}
		}

		// Setup variables
		image.setXOffset(offset_x);
		image.setYOffset(offset_y);

		return true;
	}

	bool readImage(SImage& image, MemChunk& data, int index) override { return readDoomFormat(image, data, 0); }

	bool writeImage(SImage& image, MemChunk& out, Palette* pal, int index) override
	{
		// Convert image to column/post structure
		vector<Column> columns;
		auto           data = imageData(image);
		auto           mask = imageMask(image);

		// Go through columns
		uint32_t offset = 0;
		for (int c = 0; c < image.width(); c++)
		{
			Column col;
			Post   post;
			post.row_off   = 0;
			bool ispost    = false;
			bool first_254 = true; // First 254 pixels should use absolute offsets

			offset          = c;
			uint8_t row_off = 0;
			for (int r = 0; r < image.height(); r++)
			{
				// For vanilla-compatible dimensions, use a split at 128 to prevent tiling.
				if (image.height() < 256)
				{
					if (row_off == 128)
					{
						// Finish current post if any
						if (ispost)
						{
							col.posts.push_back(post);
							post.pixels.clear();
							ispost = false;
						}
					}
				}

				// Taller images cannot be expressed without tall patch support.
				// If we're at offset 254, create a dummy post for tall doom gfx support
				else if (row_off == 254)
				{
					// Finish current post if any
					if (ispost)
					{
						col.posts.push_back(post);
						post.pixels.clear();
						ispost = false;
					}

					// Begin relative offsets
					first_254 = false;

					// Create dummy post
					post.row_off = 254;
					col.posts.push_back(post);

					// Clear post
					row_off = 0;
					ispost  = false;
				}

				// If the current pixel is not transparent, add it to the current post
				if (!mask || mask[offset] > 0)
				{
					// If we're not currently building a post, begin one and set its offset
					if (!ispost)
					{
						// Set offset
						post.row_off = row_off;

						// Reset offset if we're in relative offsets mode
						if (!first_254)
							row_off = 0;

						// Start post
						ispost = true;
					}

					// Add the pixel to the post
					post.pixels.push_back(data[offset]);
				}
				else if (ispost)
				{
					// If the current pixel is transparent and we are currently building
					// a post, add the current post to the list and clear it
					col.posts.push_back(post);
					post.pixels.clear();
					ispost = false;
				}

				// Go to next row
				offset += image.width();
				row_off++;
			}

			// If the column ended with a post, add it
			if (ispost)
				col.posts.push_back(post);

			// Add the column data
			columns.push_back(col);

			// Go to next column
			offset++;
		}

		// Write doom gfx data to output
		out.clear();
		out.seek(0, SEEK_SET);

		// Setup header
		Graphics::PatchHeader header;
		header.top    = image.offset().y;
		header.left   = image.offset().x;
		header.width  = image.width();
		header.height = image.height();

		// Byteswap header values if needed
		header.top    = wxINT16_SWAP_ON_BE(header.top);
		header.left   = wxINT16_SWAP_ON_BE(header.left);
		header.width  = wxINT16_SWAP_ON_BE(header.width);
		header.height = wxINT16_SWAP_ON_BE(header.height);

		// Write it
		out.write(&header.width, 2);
		out.write(&header.height, 2);
		out.write(&header.left, 2);
		out.write(&header.top, 2);

		// Write dummy column offsets for now
		vector<uint32_t> col_offsets(columns.size());
		out.write(col_offsets.data(), columns.size() * 4);

		// Write columns
		for (size_t c = 0; c < columns.size(); c++)
		{
			// Record column offset
			col_offsets[c] = wxUINT32_SWAP_ON_BE(out.currentPos());

			// Determine column size (in bytes)
			uint32_t col_size = 0;
			for (auto& post : columns[c].posts)
				col_size += post.pixels.size() + 4;

			// Allocate memory to write the column data
			out.reSize(out.size() + col_size, true);

			// Write column posts
			for (auto& post : columns[c].posts)
			{
				// Write row offset
				out.write(&post.row_off, 1);

				// Write no. of pixels
				uint8_t npix = post.pixels.size();
				out.write(&npix, 1);

				// Write unused byte
				uint8_t temp = (npix > 0) ? post.pixels[0] : 0;
				out.write(&temp, 1);

				// Write pixels
				for (auto& pixel : post.pixels)
					out.write(&pixel, 1);

				// Write unused byte
				temp = (npix > 0) ? post.pixels.back() : 0;
				out.write(&temp, 1);
			}

			// Write '255' row to signal end of column
			uint8_t temp = 255;
			out.write(&temp, 1);
		}

		// Now we write column offsets
		out.seek(8, SEEK_SET);
		out.write(col_offsets.data(), columns.size() * 4);

		return true;
	}

private:
	// Doom Gfx format structs
	struct Post
	{
		uint8_t         row_off;
		vector<uint8_t> pixels;
	};

	struct Column
	{
		vector<Post> posts;
	};
};

class SIFDoomBetaGfx : public SIFDoomGfx
{
public:
	SIFDoomBetaGfx() : SIFDoomGfx("doom_beta", "Doom Gfx (Beta)", 160) {}
	~SIFDoomBetaGfx() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_doom_beta")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		auto info   = SIFDoomGfx::info(mc, index);
		info.format = id_;
		return info;
	}

	// Cannot write this format
	Writable canWrite(SImage& image) override { return Writable::No; }
	bool     canWriteType(SImage::Type type) override { return false; }
	bool     convertWritable(SImage& image, ConvertOptions opt) override { return false; }

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override { return readDoomFormat(image, data, 1); }
};

class SIFDoomAlphaGfx : public SIFDoomGfx
{
public:
	SIFDoomAlphaGfx() : SIFDoomGfx("doom_alpha", "Doom Gfx (Alpha)", 100) {}
	~SIFDoomAlphaGfx() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_doom_alpha")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Setup info
		info.width     = mc[0];
		info.height    = mc[1];
		info.offset_x  = mc[2];
		info.offset_y  = mc[3];
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

	// Cannot write this format
	Writable canWrite(SImage& image) override { return Writable::No; }
	bool     canWriteType(SImage::Type type) override { return false; }
	bool     convertWritable(SImage& image, ConvertOptions opt) override { return false; }

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override { return readDoomFormat(image, data, 2); }
};

class SIFDoomArah : public SIFormat
{
public:
	SIFDoomArah() : SIFormat("doom_arah", "Doom Arah", "lmp", 100) {}
	~SIFDoomArah() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_doom_arah")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		Graphics::PatchHeader header;
		mc.read(&header, 8, 0);

		// Set info
		info.width     = wxINT16_SWAP_ON_BE(header.width);
		info.height    = wxINT16_SWAP_ON_BE(header.height);
		info.offset_x  = wxINT16_SWAP_ON_BE(header.left);
		info.offset_y  = wxINT16_SWAP_ON_BE(header.top);
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Setup variables
		Graphics::PatchHeader header;
		data.read(&header, 8, 0);
		int width    = wxINT16_SWAP_ON_BE(header.width);
		int height   = wxINT16_SWAP_ON_BE(header.height);
		int offset_x = wxINT16_SWAP_ON_BE(header.left);
		int offset_y = wxINT16_SWAP_ON_BE(header.top);

		// Create image
		image.create(width, height, SImage::Type::PalMask);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		// Read raw pixel data
		data.read(img_data, width * height, 8);

		// Create mask (all opaque)
		memset(img_mask, 255, width * height);

		// Mark as transparent all pixels that are index 255
		for (size_t i = 0; i < (unsigned)(width * height); ++i)
			if (img_data[i] == 255)
				img_mask[i] = 0;

		// Setup other image properties
		image.setXOffset(offset_x);
		image.setYOffset(offset_y);

		return true;
	}
};

class SIFDoomSnea : public SIFormat
{
public:
	SIFDoomSnea() : SIFormat("doom_snea", "Doom Snea", "lmp") {}
	~SIFDoomSnea() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_doom_snea")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get image info
		uint8_t qwidth = mc[0];
		info.width     = qwidth * 4;
		info.height    = mc[1];
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Check/setup size
		uint8_t qwidth = data[0];
		int     width  = qwidth * 4;
		int     height = data[1];

		// The TITLEPIC in the Doom Press-Release Beta has
		// two extraneous null bytes at the end for padding.
		int size = data.size();
		if (size == width * height + 4)
			size -= 2;

		if (size != 2 + width * height)
			return false;

		// Create image
		image.create(width, height, SImage::Type::PalMask);

		// Read raw pixel data
		auto img_data = imageData(image);

		auto entryend = data.data() + size;
		auto pixel    = data.data() + 2;
		auto dataend  = img_data + size - 2;
		auto brush    = img_data;

		// Algorithm taken from DeuTex.
		// I do not pretend to understand it,
		// but my own attempt didn't work.
		while (pixel < entryend)
		{
			*brush = *pixel++;
			brush += 4;
			if (brush >= dataend)
				brush -= size - 3;
		}

		// Create mask (all opaque)
		image.fillAlpha(255);

		return true;
	}
};


class SIFDoomPSX : public SIFormat
{
public:
	SIFDoomPSX() : SIFormat("doom_psx", "Doom PSX", "lmp", 100) {}
	~SIFDoomPSX() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_doom_psx")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		Graphics::PatchHeader header;
		mc.read(&header, 8, 0);

		// Set info
		info.width     = wxINT16_SWAP_ON_BE(header.width);
		info.height    = wxINT16_SWAP_ON_BE(header.height);
		info.offset_x  = wxINT16_SWAP_ON_BE(header.left);
		info.offset_y  = wxINT16_SWAP_ON_BE(header.top);
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Setup variables
		Graphics::PSXPicHeader header;
		data.read(&header, 8, 0);
		int width    = wxINT16_SWAP_ON_BE(header.width);
		int height   = wxINT16_SWAP_ON_BE(header.height);
		int offset_x = wxINT16_SWAP_ON_BE(header.left);
		int offset_y = wxINT16_SWAP_ON_BE(header.top);

		// Create image
		image.create(width, height, SImage::Type::PalMask);
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);

		// Read raw pixel data
		data.read(img_data, width * height, 8);

		// Create mask (all opaque)
		memset(img_mask, 255, width * height);

		// Mark as transparent all pixels that are index 0
		for (size_t i = 0; i < (unsigned)(width * height); ++i)
			if (img_data[i] == 0)
				img_mask[i] = 0;

		// Setup other image properties
		image.setXOffset(offset_x);
		image.setYOffset(offset_y);

		return true;
	}
};

class SIFDoomJaguar : public SIFormat
{
public:
	SIFDoomJaguar() : SIFormat("doom_jaguar", "Doom Jaguar", "lmp", 85) {}
	~SIFDoomJaguar() = default;

	bool isThisFormat(MemChunk& mc) override { return EntryDataFormat::format("img_doom_jaguar")->isThisFormat(mc); }

	SImage::Info info(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		Graphics::JagPicHeader header;
		mc.read(&header, 16, 0);

		// Set info
		info.width     = wxINT16_SWAP_ON_LE(header.width);
		info.height    = wxINT16_SWAP_ON_LE(header.height);
		info.offset_x  = 0;
		info.offset_y  = 0;
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Setup variables
		Graphics::JagPicHeader header;
		data.read(&header, 16, 0);
		int width  = wxINT16_SWAP_ON_LE(header.width);
		int height = wxINT16_SWAP_ON_LE(header.height);
		int depth  = wxINT16_SWAP_ON_LE(header.depth);
		int shift  = wxINT16_SWAP_ON_LE(header.palshift);

		// Create image
		image.create(width, height, SImage::Type::PalMask);
		auto img_data = imageData(image);
		auto img_mask = imageMask(image);

		// Create mask (all opaque)
		image.fillAlpha(255);

		// Read raw pixel data
		if (depth == 3)
		{
			data.read(img_data, width * height);
		}
		else if (depth == 2)
		{
			if (shift == 0)
				shift = 40;
			for (int p = 0; p < width * height / 2; ++p)
			{
				img_data[p * 2]     = ((data[16 + p] & 0xF0) >> 4) + (shift << 1);
				img_data[p * 2 + 1] = (data[16 + p] & 0x0F) + (shift << 1);
			}
		}
		else
			return false;

		// Mark palette index 0 as transparent
		for (int p = 0; p < width * height; ++p)
		{
			if (img_data[p] == 0)
				img_mask[p] = 0;
		}

		return true;
	}
};
