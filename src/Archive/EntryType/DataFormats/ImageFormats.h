#pragma once

#include "Graphics/GameFormats.h"

class PNGDataFormat : public EntryDataFormat
{
public:
	PNGDataFormat() : EntryDataFormat("img_png") {}
	~PNGDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 8)
		{
			// Check for PNG header
			if (mc[0] == 137 && mc[1] == 80 && mc[2] == 78 && mc[3] == 71 && mc[4] == 13 && mc[5] == 10 && mc[6] == 26
				&& mc[7] == 10)
				return MATCH_TRUE;
		}

		return MATCH_FALSE;
	}
};

class BMPDataFormat : public EntryDataFormat
{
public:
	BMPDataFormat() : EntryDataFormat("img_bmp") {}
	~BMPDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 30)
		{
			// Check for BMP header
			if (mc[0] == 'B' && mc[1] == 'M')
			{
				// Check for DIB header, should be one of the following: 12, 40, 52, 56, 64, 108 or 124
				size_t dibhdrsz = mc.readL32(14);
				if (dibhdrsz != 12 && dibhdrsz != 40 && dibhdrsz != 52 && dibhdrsz != 56 && dibhdrsz != 64
					&& dibhdrsz != 108 && dibhdrsz != 124)
					return MATCH_FALSE;
				// Normally, file size is a DWORD at offset 2, and offsets 6 to 9 should be zero.
				if (mc.readL32(2) == mc.size() && mc.readL32(6) == 0)
					return MATCH_TRUE;
				// But I have found exceptions so I must allow some leeway here.
				else if (mc.size() > 12 + dibhdrsz)
					return MATCH_MAYBE;
			}
		}

		return MATCH_FALSE;
	}
};

class GIFDataFormat : public EntryDataFormat
{
public:
	GIFDataFormat() : EntryDataFormat("img_gif") {}
	~GIFDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 6)
		{
			// Check for GIF header
			if (mc[0] == 'G' && mc[1] == 'I' && mc[2] == 'F' && mc[3] == '8' && (mc[4] == '7' || mc[4] == '9')
				&& mc[5] == 'a')
				return MATCH_TRUE;
		}

		return MATCH_FALSE;
	}
};

class PCXDataFormat : public EntryDataFormat
{
public:
	PCXDataFormat() : EntryDataFormat("img_pcx") {}
	~PCXDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() < 129)
			return MATCH_FALSE;
		// Manufacturer and encoding fields: must always be 10 and 1 respectively
		if (mc[0] != 0x0A || mc[2] != 0x01)
			return MATCH_FALSE;
		// Version field: only 0, 2, 3, 4 and 5 exist
		if (mc[1] > 5 || mc[1] == 1)
			return MATCH_FALSE;
		// Bit depth and color plane fields are used in combination
		switch (mc[3])
		{
		case 1: // Monochrome, EGA or VGA
			if (mc[65] != 1 && mc[65] != 3 && mc[65] != 4)
				return MATCH_FALSE;
			break;
		case 2: // CGA
			if (mc[65] != 1)
				return MATCH_FALSE;
			break;
		case 4: // EGA or VGA
			if (mc[65] != 1 && mc[65] != 2)
				return MATCH_FALSE;
			break;
		case 8: // VGA, SVGA or SVGA with alpha
			if (mc[65] != 1 && mc[65] != 3 && mc[65] != 4)
				return MATCH_FALSE;
			break;
		default: // Not a valid bit depth
			return MATCH_FALSE;
		}
		// In 256-color mode, the palette, if any, is contained at the end
		// of the file and preceded by a 0x0C. Only version 5 is concerned.
		if (mc[1] == 5 && ((mc[3] == 8 && mc[65] == 1) || (mc[3] == 4 && mc[65] == 2)))
		{
			size_t filesize = mc.size();
			if (filesize < 900 || mc[filesize - 769] != 12)
				return MATCH_FALSE;
		}
		// Reserved value; theoretically values other than 0 can be valid
		// if the image was created by some old version of Paintbrush, but
		// it's unlikely such pictures would be manipulated by SLADE3, so
		// instead we use it to cull false positives.
		if (mc[64] != 0)
			return MATCH_FALSE;
		// Padding filler bits; theoretically they might be set to garbage
		// values but again it's better to use them to cull false positives.
		for (size_t i = 74; i < 128; ++i)
			if (mc[i] != 0)
				return MATCH_FALSE;
		int16_t offsx = (int16_t)mc.readL16(4);
		int16_t limx  = (int16_t)mc.readL16(8);
		int16_t width = 1 + limx - offsx;
		// Compute number of bytes needed per scanline, and account for possible padding
		int16_t bnpsl = (width * mc[3]) / 8;
		if (bnpsl % 2)
			bnpsl++;
		// Bytes per scanline field is always an even number and should correspond to guessed value
		int16_t bpsl = (int16_t)mc.readL16(66);
		if (bpsl % 2 || bpsl != bnpsl)
			return MATCH_FALSE;
		// Passed all tests, so this seems to be a valid PCX
		return MATCH_TRUE;
	}
};

class TGADataFormat : public EntryDataFormat
{
public:
	TGADataFormat() : EntryDataFormat("img_tga") {}
	~TGADataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Size check for the header
		if (mc.size() < 18)
			return MATCH_FALSE;

		// Check dimensions, both ZDoom and Vavoom refuse to load TGA
		// with image sizes greater than 2048 so let's use that as well
		uint16_t width  = mc.readL16(12);
		uint16_t height = mc.readL16(14);
		if (width > 2048 || height > 2048)
			return MATCH_FALSE;

		// Let's have halfway "reasonable" limits on the compression ratio
		// that can be expected from a TGA picture...
		if ((unsigned)(5000u * mc.size()) < (unsigned)(height * width))
			return MATCH_FALSE;

		// Check image type, must be a value between 1 and 3 or 9 and 11
		if (mc[2] == 0 || mc[2] > 11 || (mc[2] > 3 && mc[2] < 9))
			return MATCH_FALSE;

		// The colormap bool must be 0 or 1
		if (mc[1] != 0 && mc[1] != 1)
			return MATCH_FALSE;

		// If there is no colormap, then colormap info must be null
		if (mc[1] == 0 && (mc.readL32(3) != 0 || mc[7] != 0))
			return MATCH_FALSE;

		// Bits per pixel can be 8, 15, 16, 24 or 32
		if (mc[16] != 8 && mc[16] != 15 && mc[16] != 16 && mc[16] != 24 && mc[16] != 32)
			return MATCH_FALSE;

		// ZDoom and Vavoom both refuse exotic directions in the descriptor, so same
		if ((mc[17] & 16) != 0)
			return MATCH_FALSE;

		return MATCH_TRUE;
	}
};

class TIFFDataFormat : public EntryDataFormat
{
public:
	TIFFDataFormat() : EntryDataFormat("img_tiff") {}
	~TIFFDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size, minimum size is 26 if I'm not mistaken:
		// 8 for the image header, +2 for at least one image
		// file directory, +12 for at least one directory entry,
		// +4 for a NULL offset for the next IFD
		size_t size = mc.size();
		if (size < 26)
			return MATCH_FALSE;
		// First two bytes must be identical, and either II or MM
		if (mc[0] != mc[1] || (mc[0] != 0x49 && mc[0] != 0x4D))
			return MATCH_FALSE;
		bool littleendian = (mc[0] == 'I');
		// The value of 42 (0x2A) is present in the next two bytes,
		// in the given endianness
		if (42
			!= (littleendian ? wxUINT16_SWAP_ON_BE((const uint16_t)(mc[2])) :
                               wxUINT16_SWAP_ON_LE((const uint16_t)(mc[2]))))
			return MATCH_FALSE;
		// First offset must be on a word boundary (therefore, %2 == 0) and
		// somewhere within the file, but not in the header of course.
		size_t offset =
			(littleendian ? wxUINT32_SWAP_ON_BE((const uint32_t)(mc[4])) :
                            wxUINT32_SWAP_ON_LE((const uint32_t)(mc[4])));
		if (offset < 8 || offset >= size || offset % 2)
			return MATCH_FALSE;
		// Check the first IFD for validity
		uint16_t numentries =
			(littleendian ? wxUINT16_SWAP_ON_BE((const uint16_t)(mc[offset])) :
                            wxUINT16_SWAP_ON_LE((const uint16_t)(mc[offset])));
		if (offset + 6 + (numentries * 12) > size)
			return MATCH_FALSE;
		// Okay, it seems valid so far
		return MATCH_TRUE;
	}
};

class JPEGDataFormat : public EntryDataFormat
{
public:
	JPEGDataFormat() : EntryDataFormat("img_jpeg") {}
	~JPEGDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 128)
		{
			// Check for JPEG header
			if ((mc[6] == 'J' && mc[7] == 'F' && mc[8] == 'I' && mc[9] == 'F')
				|| (mc[6] == 'E' && mc[7] == 'x' && mc[8] == 'i' && mc[9] == 'f'))
			{
				if (mc[0] == 255 && mc[1] == 216 && mc[2] == 255)
				{
					return MATCH_TRUE;
				}
			}
		}

		return MATCH_FALSE;
	}
};

class ILBMDataFormat : public EntryDataFormat
{
public:
	ILBMDataFormat() : EntryDataFormat("img_ilbm") {}
	~ILBMDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 48)
		{
			// Check for ILBM header, we'll also accept ACBM and PBM files, hoping FreeImage handles them all.
			// There's more info and documentation on these by Sander van der Burg at
			// https://github.com/svanderburg/libilbm
			if ((mc[0] == 'F' && mc[1] == 'O' && mc[2] == 'R' && mc[3] == 'M')
				&& ((mc[8] == 'I' && mc[9] == 'L' && mc[10] == 'B' && mc[11] == 'M') || // Interleaved Bitmap
					(mc[8] == 'A' && mc[9] == 'C' && mc[10] == 'B' && mc[11] == 'M') || // Amiga Continuous Bitmap
					(mc[8] == 'P' && mc[9] == 'B' && mc[10] == 'M' && mc[11] == ' ')))  // Deluxe Paint PC Bitmap
			{
				size_t chunksize = 8 + mc.readB32(4);
				if (chunksize != mc.size())
					return MATCH_FALSE;
				return MATCH_TRUE;
			}
		}

		return MATCH_FALSE;
	}
};

class WebPDataFormat : public EntryDataFormat
{
public:
	WebPDataFormat() : EntryDataFormat("img_webp") {}
	~WebPDataFormat() override = default;

	int isThisFormat(MemChunk& mc) override
	{
		if (mc.size() < 12)
			return MATCH_FALSE;

		// Check header
		if ((mc[0] == 'R' && mc[1] == 'I' && mc[2] == 'F' && mc[3] == 'F')
			&& (mc[8] == 'W' && mc[9] == 'E' && mc[10] == 'B' && mc[11] == 'P'))
		{
			// Check size
			auto size = mc.readL32(4) + 8;
			if (size != mc.size())
				return MATCH_FALSE;

			return MATCH_TRUE;
		}

		return MATCH_FALSE;
	}
};

class DoomGfxDataFormat : public EntryDataFormat
{
public:
	DoomGfxDataFormat() : EntryDataFormat("img_doom") {}
	~DoomGfxDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		const uint8_t* data = mc.data();

		// Check size
		if (mc.size() > sizeof(gfx::PatchHeader))
		{
			const gfx::PatchHeader* header = (const gfx::PatchHeader*)data;

			// Check header values are 'sane'
			if (header->height > 0 && header->height < 4096 && header->width > 0 && header->width < 4096
				&& header->top > -2000 && header->top < 2000 && header->left > -2000 && header->left < 2000)
			{
				uint32_t* col_offsets = (uint32_t*)((const uint8_t*)data + sizeof(gfx::PatchHeader));

				// Check there is room for needed column pointers
				if (mc.size() < sizeof(gfx::PatchHeader) + (header->width * sizeof(uint32_t)))
					return MATCH_FALSE;

				// Check column pointers are within range
				for (int a = 0; a < header->width; a++)
				{
					if (col_offsets[a] > mc.size() || col_offsets[a] < sizeof(gfx::PatchHeader))
						return MATCH_FALSE;
				}

				// Check if total size is reasonable; this computation corresponds to the most inefficient
				// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
				int numpixels  = (header->height + 2 + header->height % 2) / 2;
				int maxcolsize = sizeof(uint32_t) + (numpixels * 5) + 1;
				if (mc.size() > (sizeof(gfx::PatchHeader) + (header->width * maxcolsize)))
				{
					return MATCH_UNLIKELY; // This may still be good anyway
				}

				// Passed all checks, so probably is doom gfx
				return MATCH_TRUE;
			}
		}

		return MATCH_FALSE;
	}
};

class DoomGfxAlphaDataFormat : public EntryDataFormat
{
public:
	DoomGfxAlphaDataFormat() : EntryDataFormat("img_doom_alpha") {}
	~DoomGfxAlphaDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > sizeof(gfx::OldPatchHeader))
		{
			// Check that it ends on a FF byte
			if (mc[mc.size() - 1] != 0xFF)
				return MATCH_FALSE;

			const gfx::OldPatchHeader* header = (const gfx::OldPatchHeader*)mc.data();

			// Check header values are 'sane'
			if (header->width > 0 && header->height > 0)
			{
				// Check there is room for needed column pointers
				if (mc.size() < sizeof(gfx::OldPatchHeader) + (header->width * sizeof(uint16_t)))
					return MATCH_FALSE;

				uint16_t col_offsets[255]; // Old format headers do not allow dimensions greater than 255.
				for (uint16_t a = 0; a < header->width; a++)
				{
					col_offsets[a] = mc.readL16((sizeof(gfx::OldPatchHeader) + a * sizeof(uint16_t)));
				}


				// Check column pointers are within range
				for (int a = 0; a < header->width; a++)
				{
					if (col_offsets[a] > mc.size() || col_offsets[a] < sizeof(gfx::OldPatchHeader))
						return MATCH_FALSE;
				}

				// Check if total size is reasonable; this computation corresponds to the most inefficient
				// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
				int numpixels  = (header->height + 2 + header->height % 2) / 2;
				int maxcolsize = sizeof(uint16_t) + (numpixels * 3) + 1;
				if (mc.size() > (sizeof(gfx::OldPatchHeader) + (header->width * maxcolsize)))
				{
					return MATCH_FALSE;
				}

				// Passed all checks, so probably is doom gfx
				return MATCH_TRUE;
			}
		}

		return MATCH_FALSE;
	}
};

class DoomGfxBetaDataFormat : public EntryDataFormat
{
public:
	DoomGfxBetaDataFormat() : EntryDataFormat("img_doom_beta") {}
	~DoomGfxBetaDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() <= sizeof(gfx::PatchHeader))
			return MATCH_FALSE;

		const uint8_t* data = mc.data();

		// Check that it ends on a FF byte.
		if (mc[mc.size() - 1] != 0xFF)
		{
			// The lumps in the beta have sometimes up to three garbage 00 bytes; probably a question of byte alignment.
			for (uint8_t i = 1; i < 4; i++)
			{
				if (mc[mc.size() - i] == 0xFF)
					// Cool, we found the ending byte so it's okay.
					break;
				else if (mc[mc.size() - i] != 0x00)
					// It's not 00 and it's not FF, so it's a wrong byte.
					return MATCH_FALSE;
			}
		}

		const gfx::PatchHeader* header = (const gfx::PatchHeader*)data;

		// Check header values are 'sane'
		if (header->height > 0 && header->height < 256 && header->width > 0 && header->width < 384 && header->top > -200
			&& header->top < 200 && header->left > -200 && header->left < 200)
		{
			uint16_t* col_offsets = (uint16_t*)((const uint8_t*)data + sizeof(gfx::PatchHeader));

			// Check there is room for needed column pointers
			if (mc.size() < sizeof(gfx::PatchHeader) + (header->width * sizeof(uint16_t)))
				return MATCH_FALSE;

			// Check column pointers are within range
			for (int a = 0; a < header->width; a++)
			{
				if (col_offsets[a] > mc.size() || col_offsets[a] < sizeof(gfx::PatchHeader))
					return MATCH_FALSE;
			}

			// Check if total size is reasonable; this computation corresponds to the most inefficient
			// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
			int numpixels  = (header->height + 2 + header->height % 2) / 2;
			int maxcolsize = sizeof(uint16_t) + (numpixels * 3) + 1;
			if (mc.size() > (sizeof(gfx::PatchHeader) + (header->width * maxcolsize)))
			{
				return MATCH_FALSE;
			}

			// Passed all checks, so probably is doom gfx
			return MATCH_TRUE;
		}

		return MATCH_FALSE;
	}
};

class DoomSneaDataFormat : public EntryDataFormat
{
public:
	DoomSneaDataFormat() : EntryDataFormat("img_doom_snea") {}
	~DoomSneaDataFormat() override = default;

	/* The following is the documentation about sneas from
	 * the DeuTex source:
	 *	The snea format was used for certain graphics in Doom
	 *	alpha 0.4 and 0.5. It consists in a 2-byte header
	 *	followed by an interleaved bitmap. The first byte, W, is
	 *	the quarter of the width. The second byte, H is the
	 *	height. The bitmap is made of 4xWxH bytes. The first WxH
	 *	bytes contain the bitmap for columns 0, 4, 8, etc. The
	 *	next WxH bytes contain the bitmap for columns 1, 5, 9,
	 *	etc., and so on. No transparency.
	 */
	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() < 6)
			return MATCH_FALSE;

		const uint8_t* data   = mc.data();
		uint8_t        qwidth = data[0]; // quarter of width
		uint8_t        height = data[1];
		if (qwidth == 0 || height == 0
			|| (mc.size() != (2 + (4 * qwidth * height)) &&
				// The TITLEPIC in the Doom Press-Release Beta has
				// two extraneous null bytes at the end, for padding.
				(qwidth != 80 || height != 200 || mc.size() != 64004)))
			return MATCH_FALSE;
		return MATCH_TRUE;
	}
};

class DoomArahDataFormat : public EntryDataFormat
{
public:
	DoomArahDataFormat() : EntryDataFormat("img_doom_arah") {}
	~DoomArahDataFormat() override = default;

	/* This format is used in Doom alpha 0.2. DeuTex doesn't know it,
	 * but it seems a really simple format, basically a eight-byte
	 * header for size and offsets followed by a raw format dump.
	 * Therefore I christened it the ARAH format: Alpha Raw And Header.
	 * The header has the same format as the final patch format.
	 * To be honest, I'm not actually sure there are offset fields
	 * since those values always seem to be set to 0, but hey.
	 */
	int isThisFormat(const MemChunk& mc) override
	{
		if (mc.size() < sizeof(gfx::PatchHeader))
			return MATCH_FALSE;

		const uint8_t*          data   = mc.data();
		const gfx::PatchHeader* header = (const gfx::PatchHeader*)data;

		// Check header values are 'sane'
		if (!(header->height > 0 && header->height < 4096 && header->width > 0 && header->width < 4096
			  && header->top > -2000 && header->top < 2000 && header->left > -2000 && header->left < 2000))
			return MATCH_FALSE;

		// Check the size matches
		if (mc.size() != (sizeof(gfx::PatchHeader) + (header->width * header->height)))
			return MATCH_FALSE;

		return MATCH_TRUE;
	}
};

class DoomJaguarDataFormat : public EntryDataFormat
{
public:
	DoomJaguarDataFormat(int colmajor = 0, string_view id = "img_doom_jaguar") :
		EntryDataFormat(id),
		colmajor(colmajor){};
	~DoomJaguarDataFormat() = default;

	int isThisFormat(const MemChunk& mc) override
	{
		if (mc.size() < sizeof(gfx::JagPicHeader))
			return MATCH_FALSE;

		const uint8_t*           data   = mc.data();
		const gfx::JagPicHeader* header = (const gfx::JagPicHeader*)data;
		int                      width  = wxINT16_SWAP_ON_LE(header->width);
		int                      height = wxINT16_SWAP_ON_LE(header->height);
		int                      depth  = wxINT16_SWAP_ON_LE(header->depth);
		int                      flags  = wxINT16_SWAP_ON_LE(header->flags);

		if ((flags & 1) != colmajor)
			return MATCH_FALSE;

		// Check header values are 'sane'
		if (!(height > 0 && height < 4096 && width > 0 && width < 4096 && (depth == 2 || depth == 3)))
			return MATCH_FALSE;

		// Check the size matches
		int size = width * height;
		if (depth == 2)
			size >>= 1;
		if (mc.size() < (sizeof(gfx::JagPicHeader) + size))
			return MATCH_FALSE;

		return MATCH_TRUE;
	}

private:
	int colmajor;
};

class DoomJaguarColMajorDataFormat : public DoomJaguarDataFormat
{
public:
	DoomJaguarColMajorDataFormat() : DoomJaguarDataFormat(1, "img_doom_jaguar_colmajor"){};
	~DoomJaguarColMajorDataFormat() final = default;
};

class DoomJagTexDataFormat : public EntryDataFormat
{
public:
	DoomJagTexDataFormat() : EntryDataFormat("img_jaguar_texture") {}
	~DoomJagTexDataFormat() override = default;

	/* This format is used in the Jaguar Doom IWAD. It can be recognized by the fact the last 320 bytes are a copy of
	 * the first.
	 */
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		// Smallest pic size 832 (32x16), largest pic size 33088 (256x128)
		if (size < 832 || size % 32 || size > 33088)
			return MATCH_FALSE;

		// Verify duplication of content
		const uint8_t* data = mc.data();
		size_t         dupe = size - 320;
		for (size_t p = 0; p < 320; ++p)
		{
			if (data[p] != data[dupe + p])
				return MATCH_FALSE;
		}
		return MATCH_TRUE;
	}
};

class DoomJagSpriteDataFormat : public EntryDataFormat
{
public:
	DoomJagSpriteDataFormat() : EntryDataFormat("img_jaguar_sprite") {}
	~DoomJagSpriteDataFormat() override = default;

	/* This format is used in the Jaguar Doom IWAD. It is an annoying format.
	 */
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 16)
			return MATCH_FALSE;

		// Validate content
		size_t width    = mc.readB16(0);
		size_t height   = mc.readB16(2);
		int    offset_x = mc.readB16(4);
		int    offset_y = mc.readB16(6);

		// width and height should not be 0
		if ((width == 0) || (height == 0))
			return MATCH_FALSE;

		// Read column offsets
		if (size < (8 + (width * 6)))
			return MATCH_FALSE;
		uint16_t* col_offsets = new uint16_t[width];
		for (size_t w = 0; w < width; ++w)
			col_offsets[w] = mc.readB16(8 + 2 * w);

		const int result = size < static_cast<unsigned>(4 + col_offsets[width - 1]) ?
							   MATCH_FALSE :
                               MATCH_TRUE; // We can't test validity of pixel data here

		delete[] col_offsets;

		return result;
	}
};

class DoomPSXDataFormat : public EntryDataFormat
{
public:
	DoomPSXDataFormat() : EntryDataFormat("img_doom_psx") {}
	~DoomPSXDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		if (mc.size() < sizeof(gfx::PSXPicHeader))
			return MATCH_FALSE;

		const uint8_t*           data   = mc.data();
		const gfx::PSXPicHeader* header = (const gfx::PSXPicHeader*)data;

		// Check header values are 'sane'
		if (!(header->height > 0 && header->height < 4096 && header->width > 0 && header->width < 4096
			  && header->top > -2000 && header->top < 2000 && header->left > -2000 && header->left < 2000))
			return MATCH_FALSE;

		// Check the size matches
		size_t rawsize = (sizeof(gfx::PSXPicHeader) + (header->width * header->height));
		if (mc.size() < rawsize || mc.size() >= rawsize + 4)
			return MATCH_FALSE;

		return MATCH_TRUE;
	}
};

class IMGZDataFormat : public EntryDataFormat
{
public:
	IMGZDataFormat() : EntryDataFormat("img_imgz") {}
	~IMGZDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// A format created by Randy Heit and used by some crosshairs in ZDoom.
		uint32_t size = mc.size();

		if (size < sizeof(gfx::IMGZHeader))
			return MATCH_FALSE;

		const uint8_t*         data   = mc.data();
		const gfx::IMGZHeader* header = (const gfx::IMGZHeader*)data;

		// Check signature
		if (header->magic[0] != 'I' || header->magic[1] != 'M' || header->magic[2] != 'G' || header->magic[3] != 'Z')
			return MATCH_FALSE;

		// Check that values are sane
		if (header->width == 0xFFFF || !header->width || !header->height)
			return MATCH_FALSE;

		// The reserved values should all be null
		for (uint8_t i = 0; i < 11; ++i)
			if (header->reserved[i])
				return MATCH_FALSE;

		// This is probably a genuine IMGZ
		return MATCH_TRUE;
	}
};

class QuakeGfxDataFormat : public EntryDataFormat
{
public:
	QuakeGfxDataFormat() : EntryDataFormat("img_quake") {}
	~QuakeGfxDataFormat() override = default;

	// A data format found while rifling through some Legacy mods,
	// specifically High Tech Hell 2. It seems to be how it works.
	int isThisFormat(const MemChunk& mc) override
	{
		uint32_t size = mc.size();
		if (size < 9)
			return MATCH_FALSE;
		// These three values must all be zeroes
		if (mc[2] | mc[6] | mc[7])
			return MATCH_FALSE;
		if (mc[3] > 4)
			return MATCH_FALSE;
		uint8_t  bpp    = (mc[3] ? mc[3] : 1);
		uint16_t width  = mc.readL16(0);
		uint16_t height = mc.readL16(4);
		if (size != (8 + width * height * bpp))
			return MATCH_FALSE;
		return MATCH_TRUE;
	}
};

class QuakeSpriteDataFormat : public EntryDataFormat
{
public:
	QuakeSpriteDataFormat() : EntryDataFormat("img_qspr") {}
	~QuakeSpriteDataFormat() override = default;

	// A Quake sprite can contain several frames and each frame may contain several pictures.
	int isThisFormat(const MemChunk& mc) override
	{
		uint32_t size = mc.size();
		// Minimum size for a sprite with a single frame containing a single 2x2 picture
		if (size < 64)
			return MATCH_FALSE;
		// Check magic word
		if (mc[0] != 'I' || mc[1] != 'D' || mc[2] != 'S' || mc[3] != 'P')
			return MATCH_FALSE;
		// Check special values, version must be 1 and type must be between 0 and 4
		if (mc.readL32(4) != 1 || mc.readL32(8) > 4)
			return MATCH_FALSE;
		// Check maximum image size
		uint32_t width  = mc.readL32(16);
		uint32_t height = mc.readL32(20);
		if (width == 0 || height == 0)
			return MATCH_FALSE;

		// Check amount of frames
		uint32_t nframes = mc.readL32(24);
		if (nframes == 0)
			return MATCH_FALSE;

		// Validate frames
		uint32_t offset = 36; // Offset to start of first frame
		for (size_t a = 0; a < nframes; ++a)
		{
			if (mc.readL32(offset) != 0)
			{
				// We have a frame with a group of picture
				uint32_t grpsz = mc.readL32(offset + 4);
				// Move to end of group header
				offset += (grpsz + 2) << 2;
				for (size_t b = 0; b < grpsz; ++b)
				{
					uint32_t pw = mc.readL32(offset + 8);
					uint32_t ph = mc.readL32(offset + 12);
					if (pw > width || ph > height)
						return MATCH_FALSE;
					// Move to end of picture data
					offset += 16 + pw * ph;
					if (offset > (unsigned)size)
					{
						return MATCH_FALSE;
					}
				}
			}
			else
			{
				// We have a frame with a single picture
				offset += 4;
				uint32_t pw = mc.readL32(offset + 8);
				uint32_t ph = mc.readL32(offset + 12);
				if (pw > width || ph > height)
					return MATCH_FALSE;
				// Move to end of picture data
				offset += 16 + pw * ph;
			}
			if (offset > (unsigned)size)
			{
				return MATCH_FALSE;
			}
		}

		return MATCH_TRUE;
	}
};

class QuakeTexDataFormat : public EntryDataFormat
{
public:
	QuakeTexDataFormat() : EntryDataFormat("img_quaketex") {}
	~QuakeTexDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 125)
			return MATCH_FALSE;

		size_t width  = mc.readL32(16);
		size_t height = mc.readL32(20);
		if (!width || !height || width % 8 || height % 8)
			return MATCH_FALSE;
		for (int m = 0; m < 4; ++m)
		{
			size_t offset = mc.readL32((24 + (m << 2)));
			if (!offset || size < offset + ((width >> m) * (height >> m)))
				return MATCH_FALSE;
		}
		return MATCH_TRUE;
	}
};

class QuakeIIWalDataFormat : public EntryDataFormat
{
public:
	QuakeIIWalDataFormat() : EntryDataFormat("img_quake2wal") {}
	~QuakeIIWalDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 101)
			return MATCH_FALSE;
		// Avoid some false positives by looking for "garbage" characters
		// after the end of the "name"
		bool nameend = false;
		for (int i = 0; i < 32; ++i)
		{
			if (mc[i] == 0)
			{
				if (i == 0)
					return false;
				nameend = true;
			}
			else if (nameend)
			{
				return false;
			}
		}
		size_t width  = mc.readL32(32);
		size_t height = mc.readL32(36);
		if (!width || !height || width % 8 || height % 8)
			return MATCH_FALSE;
		for (int m = 0; m < 4; ++m)
		{
			size_t offset = mc.readL32((40 + (m << 2)));
			if (width >> m == 0 && height >> m == 0 && offset == 0)
				break;
			else if (!offset || size < offset + ((width >> m) * (height >> m)))
				return MATCH_FALSE;
		}
		return MATCH_TRUE;
	}
};


class ShadowCasterGfxFormat : public EntryDataFormat
{
public:
	ShadowCasterGfxFormat() : EntryDataFormat("img_scgfx") {}
	~ShadowCasterGfxFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// If those were static functions, then I could
		// just do this instead of such copypasta:
		//	return DoomArahDataFormat::isThisFormat(mc);
		if (mc.size() < sizeof(gfx::PatchHeader))
			return MATCH_FALSE;

		const uint8_t*          data   = mc.data();
		const gfx::PatchHeader* header = (const gfx::PatchHeader*)data;

		// Check header values are 'sane'
		if (!(header->height > 0 && header->height < 4096 && header->width > 0 && header->width < 4096
			  && header->top > -2000 && header->top < 2000 && header->left > -2000 && header->left < 2000))
			return MATCH_FALSE;

		// Check the size matches
		if (mc.size() != (sizeof(gfx::PatchHeader) + (header->width * header->height)))
			return MATCH_FALSE;

		return MATCH_TRUE;
	}
};

class ShadowCasterSpriteFormat : public EntryDataFormat
{
public:
	ShadowCasterSpriteFormat() : EntryDataFormat("img_scsprite") {}
	~ShadowCasterSpriteFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		int size = mc.size();
		if (size < 4)
			return MATCH_FALSE;
		int width = mc.readL16(2);
		if (width <= 0 || width > (signed)(size / 4))
			return MATCH_FALSE;
		int height = 0;
		// Error checking with average column height and proportion of empty columns
		int avgcolheight = 0, pnumemptycol = 0;
		for (int j = 0; j < width; ++j)
		{
			int pos = (j << 1) + 4;
			if (pos + 2 >= size)
				return MATCH_FALSE;
			int offstart = mc.readL16(pos);
			if (offstart == 0)
				continue;
			if (offstart < 0 || size < offstart + 2 || offstart < (width * 2 + 4))
				return MATCH_FALSE;
			int start     = mc[offstart];
			int stop      = mc[offstart + 1];
			int colheight = start - stop;
			if (colheight < 0 || size < offstart + colheight + 1)
				return MATCH_FALSE;
			if (start > height)
				height = start;
			avgcolheight += colheight;
			if (colheight == 0)
				pnumemptycol++;
		}
		if (height == 0)
			return MATCH_FALSE;

		avgcolheight *= 16;
		avgcolheight /= width;
		pnumemptycol *= 16;
		pnumemptycol /= width;

		// Arbitrary value: sprite must be at least about 12% filled
		if ((avgcolheight < height / 2) || (pnumemptycol > 14))
			return MATCH_UNLIKELY;

		// Least efficient sprites: single rows (amounts to 6 bytes per pixel + 4 header bytes)
		return (size < (5 + ((5 + height) * width))) ? MATCH_TRUE : MATCH_UNLIKELY;
	}
};

class ShadowCasterWallFormat : public EntryDataFormat
{
public:
	ShadowCasterWallFormat() : EntryDataFormat("img_scwall") {}
	~ShadowCasterWallFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		int size = mc.size();
		// Minimum valid size for such a picture to be
		// successfully loaded: 130 header, +1 line of 64.
		if (size < 194)
			return MATCH_FALSE;
		return (size == (mc[0] * 256) + 130) ? MATCH_TRUE : MATCH_FALSE;
	}
};

class AnaMipImageFormat : public EntryDataFormat
{
public:
	AnaMipImageFormat() : EntryDataFormat("img_mipimage") {}
	~AnaMipImageFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 4)
			return MATCH_FALSE;
		size_t width  = mc.readL16(0);
		size_t height = mc.readL16(2);
		if ((width | height) == 0)
			return MATCH_FALSE;
		size_t pixels = width * height;
		return (size >= (pixels + 4) && (size < (2 * pixels + 4))) ? MATCH_TRUE : MATCH_FALSE;
	}
};

class BuildTileFormat : public EntryDataFormat
{
public:
	BuildTileFormat() : EntryDataFormat("img_arttile") {}
	~BuildTileFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 16)
			return MATCH_FALSE;
		// Check for "BUILDART" magic string (for Ion Fury)
		size_t headeroffset = 0;
		if (mc[0] == 'B' && mc[1] == 'U' && mc[2] == 'I' && mc[3] == 'L' && mc[4] == 'D' && mc[5] == 'A' && mc[6] == 'R'
			&& mc[7] == 'T')
		{
			headeroffset = 8;
		}
		uint32_t version = mc.readL32(0 + headeroffset);
		if (version != 1)
			return MATCH_FALSE;
		uint32_t firsttile = mc.readL32(8 + headeroffset);
		uint32_t lasttile  = mc.readL32(12 + headeroffset);
		uint32_t tilecount = 1 + lasttile - firsttile;
		size_t   datastart = (16 + headeroffset + (tilecount * 8));
		if (size < datastart)
			return MATCH_FALSE;
		size_t gfxdatasize = 0;
		size_t xofs        = 16 + headeroffset;
		size_t yofs        = xofs + (tilecount << 1);
		for (size_t a = 0; a < tilecount; ++a)
		{
			if (size <= xofs + (a << 1) || size <= yofs + (a << 1))
				return MATCH_FALSE;
			gfxdatasize += (mc[xofs + (a << 1)] * mc[yofs + (a << 1)]);
		}
		if (size < (datastart + gfxdatasize))
			return MATCH_FALSE;
		return MATCH_TRUE;
	}
};

class Heretic2M8Format : public EntryDataFormat
{
public:
	Heretic2M8Format() : EntryDataFormat("img_m8") {}
	~Heretic2M8Format() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 1040)
			return MATCH_FALSE;
		uint32_t version = mc.readL32(0);
		if (version != 2)
			return MATCH_FALSE;
		for (int m = 0; m < 16; ++m)
		{
			size_t width  = mc.readL32((36 + (m << 2)));
			size_t height = mc.readL32((100 + (m << 2)));
			size_t offset = mc.readL32((164 + (m << 2)));
			if (width == 0 && height == 0 && offset == 0)
				break;
			else if (
				(width == 0 && (height | offset) != 0) || (height == 0 && (width | offset) != 0)
				|| (offset == 0 && (width | height) != 0))
				return MATCH_FALSE;
			else if (offset + (width * height) > size)
				return MATCH_FALSE;
		}
		return MATCH_TRUE;
	}
};

class Heretic2M32Format : public EntryDataFormat
{
public:
	Heretic2M32Format() : EntryDataFormat("img_m32") {}
	~Heretic2M32Format() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 1040)
			return MATCH_FALSE;
		uint32_t version = mc.readL32(0);
		if (version != 4)
			return MATCH_FALSE;
		for (int m = 0; m < 16; ++m)
		{
			size_t width  = mc.readL32((516 + (m << 2)));
			size_t height = mc.readL32((580 + (m << 2)));
			size_t offset = mc.readL32((644 + (m << 2)));
			if (width == 0 && height == 0 && offset == 0)
				break;
			else if (
				(width == 0 && (height | offset) != 0) || (height == 0 && (width | offset) != 0)
				|| (offset == 0 && (width | height) != 0))
				return MATCH_FALSE;
			else if (offset + (4 * width * height) > size)
				return MATCH_FALSE;
		}
		return MATCH_TRUE;
	}
};

class HalfLifeTextureFormat : public EntryDataFormat
{
public:
	HalfLifeTextureFormat() : EntryDataFormat("img_hlt") {}
	~HalfLifeTextureFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 812)
			return MATCH_FALSE;
		size_t width  = mc.readL32(16);
		size_t height = mc.readL32(20);
		if (!width || !height || width % 8 || height % 8)
			return MATCH_FALSE;
		for (int m = 0; m < 4; ++m)
		{
			size_t offset = mc.readL32((24 + (m << 2)));
			if (width >> m == 0 && height >> m == 0 && offset == 0)
				break;
			else if (!offset || size < offset + ((width >> m) * (height >> m)))
				return MATCH_FALSE;
		}
		width >>= 3;
		height >>= 3;
		size_t offset = mc.readL32(36) + (width * height);
		if (size < offset + 5)
			return MATCH_FALSE;
		size_t palsize = mc.readL16(offset);
		if (size < offset + 2 + (3 * palsize))
			return MATCH_FALSE;
		return MATCH_TRUE;
	}
};

class RottGfxDataFormat : public EntryDataFormat
{
public:
	RottGfxDataFormat() : EntryDataFormat("img_rott") {}
	~RottGfxDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		const uint8_t* data = mc.data();

		// Check size
		if (mc.size() > sizeof(gfx::ROTTPatchHeader))
		{
			const gfx::ROTTPatchHeader* header = (const gfx::ROTTPatchHeader*)data;

			// Check header values are 'sane'
			if (header->height > 0 && header->height < 4096 && header->width > 0 && header->width < 4096
				&& header->top > -2000 && header->top < 2000 && header->left > -2000 && header->left < 2000)
			{
				uint16_t* col_offsets = (uint16_t*)((const uint8_t*)data + sizeof(gfx::ROTTPatchHeader));

				// Check there is room for needed column pointers
				if (mc.size() < sizeof(gfx::ROTTPatchHeader) + (header->width * sizeof(uint16_t)))
					return MATCH_FALSE;

				// Check column pointers are within range
				for (int a = 0; a < header->width; a++)
				{
					if (col_offsets[a] > mc.size()
						|| col_offsets[a] < (header->width << 1) + sizeof(gfx::ROTTPatchHeader))
						return MATCH_FALSE;
				}

				// Check if total size is reasonable; this computation corresponds to the most inefficient
				// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
				int numpixels  = (header->height + 2 + header->height % 2) / 2;
				int maxcolsize = sizeof(uint32_t) + (numpixels * 3) + 1;
				if (mc.size() > (2 + sizeof(gfx::ROTTPatchHeader) + (header->width * maxcolsize)))
				{
					return MATCH_UNLIKELY; // This may still be good anyway
				}

				// Passed all checks, so probably is ROTT gfx
				return MATCH_TRUE;
			}
		}
		return MATCH_FALSE;
	}
};

class RottTransGfxDataFormat : public EntryDataFormat
{
public:
	RottTransGfxDataFormat() : EntryDataFormat("img_rottmask") {}
	~RottTransGfxDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		const uint8_t* data = mc.data();

		// Check size
		if (mc.size() > sizeof(gfx::ROTTPatchHeader))
		{
			const gfx::ROTTPatchHeader* header = (const gfx::ROTTPatchHeader*)data;

			// Check header values are 'sane'
			if (header->height > 0 && header->height < 4096 && header->width > 0 && header->width < 4096
				&& header->top > -2000 && header->top < 2000 && header->left > -2000 && header->left < 2000)
			{
				uint16_t* col_offsets = (uint16_t*)(2 + (const uint8_t*)data + sizeof(gfx::ROTTPatchHeader));

				// Check there is room for needed column pointers
				if (mc.size() < 2 + sizeof(gfx::ROTTPatchHeader) + (header->width * sizeof(uint16_t)))
					return MATCH_FALSE;

				// Check column pointers are within range
				for (int a = 0; a < header->width; a++)
				{
					if (col_offsets[a] > mc.size()
						|| col_offsets[a] < (header->width << 1) + sizeof(gfx::ROTTPatchHeader))
						return MATCH_FALSE;
				}

				// Check if total size is reasonable; this computation corresponds to the most inefficient
				// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
				int numpixels  = (header->height + 2 + header->height % 2) / 2;
				int maxcolsize = sizeof(uint32_t) + (numpixels * 3) + 1;
				if (mc.size() > (2 + sizeof(gfx::ROTTPatchHeader) + (header->width * maxcolsize)))
				{
					return MATCH_UNLIKELY; // This may still be good anyway
				}

				// Passed all checks, so probably is ROTT masked gfx
				return MATCH_TRUE;
			}
		}

		return MATCH_FALSE;
	}
};

class RottLBMDataFormat : public EntryDataFormat
{
public:
	RottLBMDataFormat() : EntryDataFormat("img_rottlbm") {}
	~RottLBMDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		const uint8_t* data = mc.data();

		// Check size
		if (mc.size() > 800)
		{
			if (data[0] == 0x40 && data[1] == 0x01 && data[2] == 0xC8 && data[3] == 0x00)
			{
				// May be ROTT LBM
				return MATCH_TRUE;
			}
		}
		return MATCH_FALSE;
	}
};

class RottRawDataFormat : public EntryDataFormat
{
public:
	RottRawDataFormat() : EntryDataFormat("img_rottraw") {}
	~RottRawDataFormat() override = default;

	/* How many format does ROTT need? This is just like the raw data plus header
	 * format from the Doom alpha, except that it's column-major instead of row-major.
	 */
	int isThisFormat(const MemChunk& mc) override
	{
		if (mc.size() < sizeof(gfx::PatchHeader))
			return MATCH_FALSE;

		const uint8_t*          data   = mc.data();
		const gfx::PatchHeader* header = (const gfx::PatchHeader*)data;

		// Check header values are 'sane'
		if (!(header->height > 0 && header->height < 4096 && header->width > 0 && header->width < 4096
			  && header->top > -2000 && header->top < 2000 && header->left > -2000 && header->left < 2000))
			return MATCH_FALSE;

		// Check the size matches
		if (mc.size() != (sizeof(gfx::PatchHeader) + (header->width * header->height)))
			return MATCH_FALSE;

		return MATCH_TRUE;
	}
};

class RottPicDataFormat : public EntryDataFormat
{
public:
	RottPicDataFormat() : EntryDataFormat("img_rottpic") {}
	~RottPicDataFormat() override = default;

	// Yet another ROTT image format. Cheesus.
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 8)
			return MATCH_FALSE;

		const uint8_t* data = mc.data();
		if (data[0] && data[1] && (size - 4 == (data[0] * data[1] * 4)) && data[size - 2] == 0 && data[size - 1] == 0)
			return MATCH_TRUE;
		return MATCH_FALSE;
	}
};

class WolfPicDataFormat : public EntryDataFormat
{
public:
	WolfPicDataFormat() : EntryDataFormat("img_wolfpic") {}
	~WolfPicDataFormat() override = default;

	// Wolf picture format
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 4)
			return MATCH_FALSE;
		if ((4 + (mc.readL16(0) * mc.readL16(2))) != mc.size())
			return MATCH_FALSE;

		return MATCH_TRUE;
	}
};

class WolfSpriteDataFormat : public EntryDataFormat
{
public:
	WolfSpriteDataFormat() : EntryDataFormat("img_wolfsprite") {}
	~WolfSpriteDataFormat() override = default;

	// Wolf picture format
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size < 8 || size > 4228)
			return MATCH_FALSE;
		if (mc[0] > 63 || mc[1] || mc[2] > 64 || mc[1] >= mc[2] || mc[3])
			return MATCH_FALSE;

		return MATCH_TRUE;
	}
};

// Dark Forces uses the "Jedi engine" so its data formats are named jedi too.
class JediBMFormat : public EntryDataFormat
{
public:
	JediBMFormat() : EntryDataFormat("img_jedi_bm") {}
	~JediBMFormat() override = default;

	// Jedi engine bitmap format
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size > 32)
		{
			if (mc[0] == 'B' && mc[1] == 'M' && mc[2] == ' ' && mc[3] == 0x1E && mc.readL16(4) != 0
				&& mc.readL16(6) != 0 && mc[14] < 3 && mc[15] == 0)
			{
				// Check that padding is left alone
				for (int i = 20; i < 32; ++i)
					if (mc[i])
						return MATCH_FALSE;
				// Probably okay
				return MATCH_TRUE;
			}
		}
		return MATCH_FALSE;
	}
};

class JediFMEFormat : public EntryDataFormat
{
public:
	JediFMEFormat() : EntryDataFormat("img_jedi_fme") {}
	~JediFMEFormat() override = default;

	// Jedi engine frame format
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size > 64)
		{
			// The only constants we have is that byte 8 is either 1 or 0 (h-flip)
			// and byte 12 is always 32 (secondary header after the first header)
			// Then in the secondary header, byte 40 is 1 or 0 (compression) and
			// bytes 44-47 correspond to the little-endian size of the file - 32.
			if (mc[8] < 2 && mc[12] == 32 && (mc[9] | mc[10] | mc[11] | mc[13] | mc[14] | mc[15]) == 0 && mc[40] < 2)
			{
				// Check that padding is left alone
				for (int i = 16; i < 32; ++i)
					if (mc[i])
						return MATCH_FALSE;
				for (int i = 16; i < 32; ++i)
					if (mc[i])
						return MATCH_FALSE;
				// Check size if compressed:
				if (mc[40] == 1)
				{
					if (mc.readL32(44) != size - 32)
						return MATCH_FALSE;
					// Check some padding
					for (int i = 41; i < 44; ++i)
						if (mc[i])
							return MATCH_FALSE;
					for (int i = 48; i < 56; ++i)
						if (mc[i])
							return MATCH_FALSE;
				}
				else
				{
					// All that should be zero if uncompressed
					for (int i = 41; i < 56; ++i)
						if (mc[i])
							return MATCH_FALSE;
				}
				// Probably okay
				return MATCH_TRUE;
			}
		}
		return MATCH_FALSE;
	}
};

class JediWAXFormat : public EntryDataFormat
{
public:
	JediWAXFormat() : EntryDataFormat("img_jedi_wax") {}
	~JediWAXFormat() override = default;

	// Jedi engine wax format
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size > 460)
		{
			// Constant identifier 00 10 01 00 *or* 00 00 01 00
			if (mc[0] == 0 && (mc[1] == 16 || mc[1] == 0) && mc[2] == 1 && mc[3] == 0)
			{
				// The numbers of sequences, frames and cells should not exceed 255.
				// Also check that padding is left alone.
				for (int i = 5; i < 8; ++i)
					if (mc[i])
						return MATCH_FALSE;
				for (int i = 9; i < 12; ++i)
					if (mc[i])
						return MATCH_FALSE;
				for (int i = 13; i < 32; ++i)
					if (mc[i])
						return MATCH_FALSE;
				// Probably okay
				return MATCH_TRUE;
			}
		}
		return MATCH_FALSE;
	}
};


class Font0DataFormat : public EntryDataFormat
{
public:
	Font0DataFormat() : EntryDataFormat("font_doom_alpha") {}
	~Font0DataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		if (mc.size() <= 0x302)
			return MATCH_FALSE;

		const uint16_t* gfx_data = (const uint16_t*)mc.data();

		size_t height = wxINT16_SWAP_ON_BE(gfx_data[0]);

		size_t datasize = mc.size() - 0x302;
		if (height == 0 || datasize % height)
			return MATCH_FALSE;

		// It seems okay so far. Check that one
		// character does start at offset 0x302.
		// The offsets are themselves between
		// offsets 0x102 and 0x302. Halved for int16_t.
		for (size_t i = 0x81; i < 0x181; ++i)
			if (gfx_data[i] == wxINT16_SWAP_ON_BE(0x302))
				return MATCH_TRUE;

		// Doesn't seem to be such a file after all.
		return MATCH_FALSE;
	}
};

class Font1DataFormat : public EntryDataFormat
{
public:
	Font1DataFormat() : EntryDataFormat("font_zd_console") {}
	~Font1DataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for FON1 header
			if (mc[0] == 'F' && mc[1] == 'O' && mc[2] == 'N' && mc[3] == '1')
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class Font2DataFormat : public EntryDataFormat
{
public:
	Font2DataFormat() : EntryDataFormat("font_zd_big") {}
	~Font2DataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for FON2 header
			if (mc[0] == 'F' && mc[1] == 'O' && mc[2] == 'N' && mc[3] == '2')
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class BMFontDataFormat : public EntryDataFormat
{
public:
	BMFontDataFormat() : EntryDataFormat("font_bmf") {}
	~BMFontDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		// Check size
		if (mc.size() > 4)
		{
			// Check for BMF header
			if (mc[0] == 0xE1 && mc[1] == 0xE6 && mc[2] == 0xD5 && mc[3] == 0x1A)
				return MATCH_TRUE;
		}
		return MATCH_FALSE;
	}
};

class FontWolfDataFormat : public EntryDataFormat
{
public:
	FontWolfDataFormat() : EntryDataFormat("font_wolf") {}
	~FontWolfDataFormat() override = default;

	int isThisFormat(const MemChunk& mc) override
	{
		if (mc.size() <= 0x302)
			return MATCH_FALSE;

		const uint16_t* gfx_data = (const uint16_t*)mc.data();

		size_t height = wxINT16_SWAP_ON_BE(gfx_data[0]);

		size_t datasize = mc.size() - 0x302;
		if (height == 0 || datasize % height)
			return MATCH_FALSE;

		// It seems okay so far. Check that one
		// character does start at offset 0x302.
		// The offsets are themselves between
		// offsets 0x002 and 0x202. Halved for int16_t.
		for (size_t i = 0x01; i < 0x101; ++i)
			if (gfx_data[i] == wxINT16_SWAP_ON_BE(0x302))
				return MATCH_TRUE;

		// Doesn't seem to be such a file after all.
		return MATCH_FALSE;
	}
};

class JediFNTFormat : public EntryDataFormat
{
public:
	JediFNTFormat() : EntryDataFormat("font_jedi_fnt") {}
	~JediFNTFormat() override = default;

	// Jedi engine fnt format
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size > 35)
		{
			// Constant identifier FNT\15, height should be greater than 0,
			// last should be greater than first.
			if (mc[0] == 'F' && mc[1] == 'N' && mc[2] == 'T' && mc[3] == 0x15 && mc[4] > 0 && mc[8] <= mc[9])
			{
				// Check that padding is left alone.
				for (int i = 10; i < 32; ++i)
					if (mc[i])
						return MATCH_FALSE;
				// Probably okay
				return MATCH_TRUE;
			}
		}
		return MATCH_FALSE;
	}
};

class JediFONTFormat : public EntryDataFormat
{
public:
	JediFONTFormat() : EntryDataFormat("font_jedi_font") {}
	~JediFONTFormat() override = default;

	// Jedi engine font format
	int isThisFormat(const MemChunk& mc) override
	{
		size_t size = mc.size();
		if (size > 16)
		{
			// Numchar should be greater than 0, width should be multiple of 8,
			// height should be greater than 0, padding should be null.
			if (mc.readL16(2) >= 0 && (mc.readL16(4) % 8) == 0 && mc.readL16(6) >= 0 && mc.readL16(10) == 0)
			{
				size_t numchr = mc.readL16(2);
				if (size < 16 + numchr)
					return MATCH_FALSE;
				// Also check that character width never exceeds max width.
				for (size_t i = 12; i < 12 + numchr; ++i)
					if (mc[i] > mc.readL16(4))
						return MATCH_FALSE;
				// Check that there are enough data to cover all characters and the header
				size_t neededbytes = 12 + numchr + ((numchr * mc.readL16(6)) * (mc.readL16(4) >> 3));
				if (size != neededbytes)
					return MATCH_FALSE;
				// Probably okay
				return MATCH_TRUE;
			}
		}
		return MATCH_FALSE;
	}
};
