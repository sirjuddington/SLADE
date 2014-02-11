#ifndef IMAGEFORMATS_H
#define IMAGEFORMATS_H

class PNGDataFormat : public EntryDataFormat
{
public:
	PNGDataFormat() : EntryDataFormat("img_png") {}
	~PNGDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 8)
		{
			// Check for PNG header
			if (mc[0] == 137 && mc[1] == 80 &&
			        mc[2] == 78 && mc[3] == 71 &&
			        mc[4] == 13 && mc[5] == 10 &&
			        mc[6] == 26 && mc[7] == 10)
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class BMPDataFormat : public EntryDataFormat
{
public:
	BMPDataFormat() : EntryDataFormat("img_bmp") {};
	~BMPDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 30)
		{
			// Check for BMP header
			if (mc[0] == 'B' && mc[1] == 'M')
			{
				// Check for DIB header, should be one of the following: 12, 40, 52, 56, 64, 108 or 124
				size_t dibhdrsz = READ_L32(mc, 14);
				if (dibhdrsz != 12 && dibhdrsz != 40 && dibhdrsz != 52 && dibhdrsz != 56 && 
					dibhdrsz != 64 && dibhdrsz != 108 && dibhdrsz != 124)
					return EDF_FALSE;
				// Normally, file size is a DWORD at offset 2, and offsets 6 to 9 should be zero.
				if (READ_L32(mc, 2) == mc.getSize() && READ_L32(mc, 6) == 0)
					return EDF_TRUE;
				// But I have found exceptions so I must allow some leeway here.
				else if (mc.getSize() > 12 + dibhdrsz)
					return EDF_MAYBE;
			}
		}

		return EDF_FALSE;
	}
};

class GIFDataFormat : public EntryDataFormat
{
public:
	GIFDataFormat() : EntryDataFormat("img_gif") {};
	~GIFDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 6)
		{
			// Check for GIF header
			if (mc[0] == 'G' && mc[1] == 'I' && mc[2] == 'F' && mc[3] == '8' &&
			        (mc[4] == '7' || mc[4] =='9') && mc[5] == 'a')
				return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class PCXDataFormat : public EntryDataFormat
{
public:
	PCXDataFormat() : EntryDataFormat("img_pcx") {};
	~PCXDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() < 129)
			return EDF_FALSE;
		// Manufacturer and encoding fields: must always be 10 and 1 respectively
		if (mc[0] != 0x0A || mc[2] != 0x01)
			return EDF_FALSE;
		// Version field: only 0, 2, 3, 4 and 5 exist
		if (mc[1] > 5 || mc[1] == 1)
			return EDF_FALSE;
		// Bit depth and color plane fields are used in combination
		switch (mc[3])
		{
		case 1: // Monochrome, EGA or VGA
			if (mc[65] != 1 && mc[65] != 3 && mc[65] != 4)	return EDF_FALSE;	break;
		case 2: // CGA
			if (mc[65] != 1)								return EDF_FALSE;	break;
		case 4: // EGA or VGA
			if (mc[65] != 1 && mc[65] != 2)					return EDF_FALSE;	break;
		case 8: // VGA, SVGA or SVGA with alpha
			if (mc[65] != 1 && mc[65] != 3 && mc[65] != 4)	return EDF_FALSE;	break;
		default: // Not a valid bit depth
			return EDF_FALSE;
		}
		// In 256-color mode, the palette, if any, is contained at the end
		// of the file and preceded by a 0x0C. Only version 5 is concerned.
		if (mc[1] == 5 && ((mc[3] == 8 && mc[65] == 1) || (mc[3] == 4 && mc[65] == 2)))
		{
			size_t filesize = mc.getSize();
			if (filesize < 900 || mc[filesize-769] != 12)
				return EDF_FALSE;
		}
		// Reserved value; theoretically values other than 0 can be valid
		// if the image was created by some old version of Paintbrush, but
		// it's unlikely such pictures would be manipulated by SLADE3, so
		// instead we use it to cull false positives.
		if (mc[64] != 0)
			return EDF_FALSE;
		// Padding filler bits; theoretically they might be set to garbage
		// values but again it's better to use them to cull false positives.
		for (size_t i = 74; i < 128; ++i)
			if (mc[i] != 0)
				return EDF_FALSE;
		// Min/Max fields
		int16_t offsx, offsy, limx, limy, width, height;
		offsx = (int16_t) READ_L16(mc, 4);
		offsy = (int16_t) READ_L16(mc, 6);
		limx  = (int16_t) READ_L16(mc, 8);
		limy  = (int16_t) READ_L16(mc, 10);
		width = 1 + limx - offsx; height = 1 + limy - offsy;
		// Compute number of bytes needed per scanline, and account for possible padding
		int16_t bnpsl = (width * mc[3]) / 8; if (bnpsl % 2) bnpsl++;
		// Bytes per scanline field is always an even number and should correspond to guessed value
		int16_t bpsl = (int16_t) READ_L16(mc, 66);
		if (bpsl%2 || bpsl != bnpsl)
			return EDF_FALSE;
		// Passed all tests, so this seems to be a valid PCX
		return EDF_TRUE;
	}
};

class TGADataFormat : public EntryDataFormat
{
public:
	TGADataFormat() : EntryDataFormat("img_tga") {};
	~TGADataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Size check for the header
		if (mc.getSize() < 18)
			return EDF_FALSE;

		// Check dimensions, both ZDoom and Vavoom refuse to load TGA
		// with image sizes greater than 2048 so let's use that as well
		uint16_t width  = READ_L16(mc, 12);
		uint16_t height = READ_L16(mc, 14);
		if (width > 2048 || height > 2048)
			return EDF_FALSE;

		// Let's have halfway "reasonable" limits on the compression ratio
		// that can be expected from a TGA picture...
		if ((unsigned)(5000u * mc.getSize()) < (unsigned)(height * width))
			return EDF_FALSE;

		// Check image type, must be a value between 1 and 3 or 9 and 11
		if (mc[2] == 0 || mc[2] > 11 || (mc[2] > 3 && mc[2] < 9))
			return EDF_FALSE;

		// The colormap bool must be 0 or 1
		if (mc[1] != 0 && mc[1] != 1)
			return EDF_FALSE;

		// Bits per pixel can be 8, 15, 16, 24 or 32
		if (mc[16] != 8 && mc[16] != 15 && mc[16] != 16 && mc[16] !=24 && mc[16] !=32)
			return EDF_FALSE;

		// ZDoom and Vavoom both refuse exotic directions in the descriptor, so same
		if ((mc[17] & 16) != 0)
			return EDF_FALSE;

		return EDF_TRUE;
	}
};

class TIFFDataFormat : public EntryDataFormat
{
public:
	TIFFDataFormat() : EntryDataFormat("img_tiff") {};
	~TIFFDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size, minimum size is 26 if I'm not mistaken:
		// 8 for the image header, +2 for at least one image
		// file directory, +12 for at least one directory entry,
		// +4 for a NULL offset for the next IFD
		size_t size = mc.getSize();
		if (size < 26)
			return EDF_FALSE;
		// First two bytes must be identical, and either II or MM
		if (mc[0] != mc[1] || (mc[0] != 0x49 && mc[0] != 0x4D))
			return EDF_FALSE;
		bool littleendian = (mc[0] == 'I');
		// The value of 42 (0x2A) is present in the next two bytes,
		// in the given endianness
		if (42 != (littleendian ?
		           wxUINT16_SWAP_ON_BE((const uint16_t)(mc[2])) :
		           wxUINT16_SWAP_ON_LE((const uint16_t)(mc[2]))))
			return EDF_FALSE;
		// First offset must be on a word boundary (therefore, %2 == 0) and
		// somewhere within the file, but not in the header of course.
		size_t offset = (littleendian ?
		                 wxUINT32_SWAP_ON_BE((const uint32_t)(mc[4])) :
		                 wxUINT32_SWAP_ON_LE((const uint32_t)(mc[4])));
		if (offset < 8 || offset >= size || offset %2)
			return EDF_FALSE;
		// Check the first IFD for validity
		uint16_t numentries = (littleendian ?
		                       wxUINT16_SWAP_ON_BE((const uint16_t)(mc[offset])) :
		                       wxUINT16_SWAP_ON_LE((const uint16_t)(mc[offset])));
		if (offset + 6 + (numentries * 12) > size)
			return EDF_FALSE;
		// Okay, it seems valid so far
		return EDF_TRUE;
	}
};

class JPEGDataFormat : public EntryDataFormat
{
public:
	JPEGDataFormat() : EntryDataFormat("img_jpeg") {};
	~JPEGDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 128)
		{
			// Check for JPEG header
			if ((mc[6] == 'J' && mc[7] == 'F' && mc[8] == 'I' && mc[9] == 'F') ||
			        (mc[6] == 'E' && mc[7] == 'x' && mc[8] == 'i' && mc[9] == 'f'))
			{
				if (mc[0] == 255 && mc[1] == 216 && mc[2] == 255)
				{
					return EDF_TRUE;
				}
			}
		}

		return EDF_FALSE;
	}
};

class DoomGfxDataFormat : public EntryDataFormat
{
public:
	DoomGfxDataFormat() : EntryDataFormat("img_doom") {};
	~DoomGfxDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		const uint8_t* data = mc.getData();

		// Check size
		if (mc.getSize() > sizeof(patch_header_t))
		{
			const patch_header_t* header = (const patch_header_t*)data;

			// Check header values are 'sane'
			if (header->height > 0 && header->height < 4096 &&
			        header->width > 0 && header->width < 4096 &&
			        header->top > -2000 && header->top < 2000 &&
			        header->left > -2000 && header->left < 2000)
			{
				uint32_t* col_offsets = (uint32_t*)((const uint8_t*)data + sizeof(patch_header_t));

				// Check there is room for needed column pointers
				if (mc.getSize() < sizeof(patch_header_t) + (header->width * sizeof(uint32_t)))
					return EDF_FALSE;

				// Check column pointers are within range
				for (int a = 0; a < header->width; a++)
				{
					if (col_offsets[a] > mc.getSize() || col_offsets[a] < sizeof(patch_header_t))
						return EDF_FALSE;
				}

				// Check if total size is reasonable; this computation corresponds to the most inefficient
				// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
				int numpixels = (header->height + 2 + header->height%2)/2;
				int maxcolsize = sizeof(uint32_t) + (numpixels*5) + 1;
				if (mc.getSize() > (sizeof(patch_header_t) + (header->width * maxcolsize)))
				{
					return EDF_UNLIKELY;	// This may still be good anyway
				}

				// Passed all checks, so probably is doom gfx
				return EDF_TRUE;
			}
		}

		return EDF_FALSE;
	}
};

class DoomGfxAlphaDataFormat : public EntryDataFormat
{
public:
	DoomGfxAlphaDataFormat() : EntryDataFormat("img_doom_alpha") {};
	~DoomGfxAlphaDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Get entry data
		const uint8_t* data = mc.getData();

		// Check size
		if (mc.getSize() > sizeof(oldpatch_header_t))
		{
			// Check that it ends on a FF byte
			if (mc[mc.getSize() -1] != 0xFF)
				return EDF_FALSE;

			const oldpatch_header_t* header = (const oldpatch_header_t*)data;

			// Check header values are 'sane'
			if (header->width > 0 && header->height > 0)
			{
				// Check there is room for needed column pointers
				if (mc.getSize() < sizeof(oldpatch_header_t) + (header->width * sizeof(uint16_t)))
					return EDF_FALSE;

				uint16_t col_offsets[255]; // Old format headers do not allow dimensions greater than 255.
				for (uint16_t a = 0; a < header->width; a++)
				{
					col_offsets[a] = READ_L16(data, (sizeof(oldpatch_header_t) + a * sizeof(uint16_t)));
				}


				// Check column pointers are within range
				for (int a = 0; a < header->width; a++)
				{
					if (col_offsets[a] > mc.getSize() || col_offsets[a] < sizeof(oldpatch_header_t))
						return EDF_FALSE;
				}

				// Check if total size is reasonable; this computation corresponds to the most inefficient
				// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
				int numpixels = (header->height + 2 + header->height%2)/2;
				int maxcolsize = sizeof(uint16_t) + (numpixels*3) + 1;
				if (mc.getSize() > (sizeof(oldpatch_header_t) + (header->width * maxcolsize)))
				{
					return EDF_FALSE;
				}

				// Passed all checks, so probably is doom gfx
				return EDF_TRUE;
			}
		}

		return EDF_FALSE;
	}
};

class DoomGfxBetaDataFormat : public EntryDataFormat
{
public:
	DoomGfxBetaDataFormat() : EntryDataFormat("img_doom_beta") {};
	~DoomGfxBetaDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() <= sizeof(patch_header_t))
			return EDF_FALSE;

		const uint8_t* data = mc.getData();

		// Check that it ends on a FF byte.
		if (mc[mc.getSize() -1] != 0xFF)
		{
			// The lumps in the beta have sometimes up to three garbage 00 bytes; probably a question of byte alignment.
			for (uint8_t i = 1; i < 4; i++)
			{
				if (mc[mc.getSize() - i] == 0xFF)
					// Cool, we found the ending byte so it's okay.
					break;
				else if (mc[mc.getSize() - i] != 0x00)
					// It's not 00 and it's not FF, so it's a wrong byte.
					return EDF_FALSE;
			}
		}

		const patch_header_t* header = (const patch_header_t*)data;

		// Check header values are 'sane'
		if (header->height > 0 && header->height < 4096 &&
		        header->width > 0 && header->width < 4096 &&
		        header->top > -2000 && header->top < 2000 &&
		        header->left > -2000 && header->left < 2000)
		{
			uint16_t* col_offsets = (uint16_t*)((const uint8_t*)data + sizeof(patch_header_t));

			// Check there is room for needed column pointers
			if (mc.getSize() < sizeof(patch_header_t) + (header->width * sizeof(uint16_t)))
				return EDF_FALSE;

			// Check column pointers are within range
			for (int a = 0; a < header->width; a++)
			{
				if (col_offsets[a] > mc.getSize() || col_offsets[a] < sizeof(patch_header_t))
					return EDF_FALSE;
			}

			// Check if total size is reasonable; this computation corresponds to the most inefficient
			// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
			int numpixels = (header->height + 2 + header->height%2)/2;
			int maxcolsize = sizeof(uint16_t) + (numpixels*3) + 1;
			if (mc.getSize() > (sizeof(patch_header_t) + (header->width * maxcolsize)))
			{
				return EDF_FALSE;
			}

			// Passed all checks, so probably is doom gfx
			return EDF_TRUE;
		}

		return EDF_FALSE;
	}
};

class DoomSneaDataFormat : public EntryDataFormat
{
public:
	DoomSneaDataFormat() : EntryDataFormat("img_doom_snea") {};
	~DoomSneaDataFormat() {}

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
	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() < 6)
			return EDF_FALSE;

		const uint8_t* data = mc.getData();
		uint8_t qwidth = data[0]; // quarter of width
		uint8_t height = data[1];
		if (qwidth == 0 || height == 0 ||
			(mc.getSize() != (2 + (4 * qwidth * height)) &&
		        // The TITLEPIC in the Doom Press-Release Beta has
		        // two extraneous null bytes at the end, for padding.
		        (qwidth != 80 || height != 200 || mc.getSize() != 64004)))
			return EDF_FALSE;
		return EDF_TRUE;
	}
};

class DoomArahDataFormat : public EntryDataFormat
{
public:
	DoomArahDataFormat() : EntryDataFormat("img_doom_arah") {};
	~DoomArahDataFormat() {}

	/* This format is used in Doom alpha 0.2. DeuTex doesn't know it,
	 * but it seems a really simple format, basically a eight-byte
	 * header for size and offsets followed by a raw format dump.
	 * Therefore I christened it the ARAH format: Alpha Raw And Header.
	 * The header has the same format as the final patch format.
	 * To be honest, I'm not actually sure there are offset fields
	 * since those values always seem to be set to 0, but hey.
	 */
	int isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() < sizeof(patch_header_t))
			return EDF_FALSE;

		const uint8_t* data = mc.getData();
		const patch_header_t* header = (const patch_header_t*)data;

		// Check header values are 'sane'
		if (!(header->height > 0 && header->height < 4096 &&
		        header->width > 0 && header->width < 4096 &&
		        header->top > -2000 && header->top < 2000 &&
		        header->left > -2000 && header->left < 2000))
			return EDF_FALSE;

		// Check the size matches
		if (mc.getSize() != (sizeof(patch_header_t) + (header->width * header->height)))
			return EDF_FALSE;

		return EDF_TRUE;
	}
};

class DoomJaguarDataFormat : public EntryDataFormat
{
public:
	DoomJaguarDataFormat() : EntryDataFormat("img_doom_jaguar") {};
	~DoomJaguarDataFormat() {}

	/* This format is used in the Jaguar Doom IWAD.
	 */
	int isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() < sizeof(jagpic_header_t))
			return EDF_FALSE;

		const uint8_t* data = mc.getData();
		const jagpic_header_t* header = (const jagpic_header_t*)data;
		int width, height, depth, size;
		width = wxINT16_SWAP_ON_LE(header->width);
		height = wxINT16_SWAP_ON_LE(header->height);
		depth = wxINT16_SWAP_ON_LE(header->depth);

		// Check header values are 'sane'
		if (!(height > 0 && height < 4096 && width > 0 && width < 4096 && (depth == 2 || depth == 3)))
			return EDF_FALSE;

		// Check the size matches
		size = width * height;
		if (depth == 2) size >>= 1;
		if (mc.getSize() < (sizeof(jagpic_header_t) + size))
			return EDF_FALSE;

		return EDF_TRUE;
	}
};

class DoomJagTexDataFormat : public EntryDataFormat
{
public:
	DoomJagTexDataFormat() : EntryDataFormat("img_jaguar_texture") {};
	~DoomJagTexDataFormat() {}

	/* This format is used in the Jaguar Doom IWAD. It can be recognized by the fact the last 320 bytes are a copy of the first.
	 */
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 640 || size % 32)
			return EDF_FALSE;

		// Verify duplication of content
		const uint8_t* data = mc.getData();
		size_t dupe = size - 320;
		for (size_t p = 0; p < 320; ++p)
		{
			if (data[p] != data[dupe + p])
				return EDF_FALSE;
		}
		return EDF_TRUE;
	}
};

class DoomJagSpriteDataFormat : public EntryDataFormat
{
public:
	DoomJagSpriteDataFormat() : EntryDataFormat("img_jaguar_sprite") {};
	~DoomJagSpriteDataFormat() {}

	/* This format is used in the Jaguar Doom IWAD. It is an annoying format.
	 */
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 16)
			return EDF_FALSE;

		// Validate content
		size_t width = READ_B16(mc, 0);
		size_t height= READ_B16(mc, 2);
		int offset_x = READ_B16(mc, 4);
		int offset_y = READ_B16(mc, 6);

		//width and height should not be 0
		if ((width == 0) || (height == 0))
			return EDF_FALSE;

		// Read column offsets
		if (size < (8 + (width * 6)))
			return EDF_FALSE;
		uint16_t* col_offsets = new uint16_t[width];
		for (size_t w = 0; w < width; ++w)
			col_offsets[w] = READ_B16(mc, 8+2*w);
		if (size < unsigned(4 + col_offsets[width - 1]))
			return EDF_FALSE;

		// We can't test validity of pixel data here
		return EDF_TRUE;
	}
};

class DoomPSXDataFormat : public EntryDataFormat
{
public:
	DoomPSXDataFormat() : EntryDataFormat("img_doom_psx") {};
	~DoomPSXDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() < sizeof(psxpic_header_t))
			return EDF_FALSE;

		const uint8_t* data = mc.getData();
		const psxpic_header_t* header = (const psxpic_header_t*)data;

		// Check header values are 'sane'
		if (!(header->height > 0 && header->height < 4096 &&
		        header->width > 0 && header->width < 4096 &&
		        header->top > -2000 && header->top < 2000 &&
		        header->left > -2000 && header->left < 2000))
			return EDF_FALSE;

		// Check the size matches
		size_t rawsize = (sizeof(psxpic_header_t) + (header->width * header->height));
		if (mc.getSize() < rawsize || mc.getSize() >= rawsize + 4)
			return EDF_FALSE;

		return EDF_TRUE;
	}
};

class IMGZDataFormat : public EntryDataFormat
{
public:
	IMGZDataFormat() : EntryDataFormat("img_imgz") {};
	~IMGZDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// A format created by Randy Heit and used by some crosshairs in ZDoom.
		uint32_t size = mc.getSize();

		if (size < sizeof(imgz_header_t))
			return EDF_FALSE;

		const uint8_t* data = mc.getData();
		const imgz_header_t* header = (const imgz_header_t*)data;

		// Check signature
		if (header->magic[0] != 'I' || header->magic[1] != 'M' ||
		        header->magic[2] != 'G' || header->magic[3] != 'Z')
			return EDF_FALSE;

		// Check that values are sane
		if (header->width == 0xFFFF || !header->width || !header->height)
			return EDF_FALSE;

		// The reserved values should all be null
		for (uint8_t i = 0; i < 11 ; ++i)
			if (header->reserved[i]) return EDF_FALSE;

		// This is probably a genuine IMGZ
		return EDF_TRUE;
	}
};

class QuakeGfxDataFormat : public EntryDataFormat
{
public:
	QuakeGfxDataFormat() : EntryDataFormat("img_quake") {};
	~QuakeGfxDataFormat() {}

	// A data format found while rifling through some Legacy mods,
	// specifically High Tech Hell 2. It seems to be how it works.
	int isThisFormat(MemChunk& mc)
	{
		uint32_t size = mc.getSize();
		if (size < 9)
			return EDF_FALSE;
		// These three values must all be zeroes
		if (mc[2] | mc[6] | mc[7])
			return EDF_FALSE;
		if (mc[3] > 4)
			return EDF_FALSE;
		uint8_t bpp = (mc[3]?mc[3]:1);
		uint16_t width  = READ_L16(mc, 0);
		uint16_t height = READ_L16(mc, 4);
		if (size != (8 + width * height * bpp))
			return EDF_FALSE;
		return EDF_TRUE;
	}
};

class QuakeSpriteDataFormat : public EntryDataFormat
{
public:
	QuakeSpriteDataFormat() : EntryDataFormat("img_qspr") {};
	~QuakeSpriteDataFormat() {}

	// A Quake sprite can contain several frames and each frame may contain several pictures.
	int isThisFormat(MemChunk& mc)
	{
		uint32_t size = mc.getSize();
		// Minimum size for a sprite with a single frame containing a single 2x2 picture
		if (size < 64)
			return EDF_FALSE;
		// Check magic word
		if (mc[0] != 'I' || mc[1] != 'D' || mc[2] != 'S' || mc[3] != 'P')
			return EDF_FALSE;
		// Check special values, version must be 1 and type must be between 0 and 4
		if (READ_L32(mc, 4) != 1 || READ_L32(mc, 8) > 4)
			return EDF_FALSE;
		// Check maximum image size
		uint32_t width  = READ_L32(mc, 16);
		uint32_t height = READ_L32(mc, 20);
		if (width == 0 || height == 0)
			return EDF_FALSE;

		// Check amount of frames
		uint32_t nframes = READ_L32(mc, 24);
		if (nframes == 0)
			return EDF_FALSE;

		// Validate frames
		uint32_t offset = 36; // Offset to start of first frame
		for (size_t a = 0; a < nframes; ++a)
		{
			if (READ_L32(mc, offset) != 0)
			{
				// We have a frame with a group of picture
				uint32_t grpsz = READ_L32(mc, offset + 4);
				// Move to end of group header
				offset += (grpsz+2)<<2;
				for (size_t b = 0; b < grpsz; ++b)
				{
					uint32_t pw = READ_L32(mc, offset + 8);
					uint32_t ph = READ_L32(mc, offset + 12);
					if (pw > width || ph > height)
						return EDF_FALSE;
					// Move to end of picture data
					offset += 16 + pw * ph;
					if (offset > (unsigned) size)
					{
						return EDF_FALSE;
					}
				}
			}
			else
			{
				// We have a frame with a single picture
				offset += 4;
				uint32_t pw = READ_L32(mc, offset + 8);
				uint32_t ph = READ_L32(mc, offset + 12);
				if (pw > width || ph > height)
					return EDF_FALSE;
				// Move to end of picture data
				offset += 16 + pw * ph;
			}
			if (offset > (unsigned) size)
			{
				return EDF_FALSE;
			}
		}

		return EDF_TRUE;
	}
};

class QuakeTexDataFormat: public EntryDataFormat
{
public:
	QuakeTexDataFormat() : EntryDataFormat("img_quaketex") {};
	~QuakeTexDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 125)
			return EDF_FALSE;

		size_t width = READ_L32(mc, 16);
		size_t height = READ_L32(mc, 20);
		if (!width || !height || width % 8 || height % 8)
			return EDF_FALSE;
		for (int m = 0; m < 4; ++m)
		{
			size_t offset = READ_L32(mc, (24+(m<<2)));
			if (!offset || size < offset + ((width>>m) * (height>>m)))
				return EDF_FALSE;
		}
		return EDF_TRUE;
	}
};

class QuakeIIWalDataFormat: public EntryDataFormat
{
public:
	QuakeIIWalDataFormat() : EntryDataFormat("img_quake2wal") {};
	~QuakeIIWalDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 101)
			return EDF_FALSE;
		// Avoid some false positives by looking for "garbage" characters
		// after the end of the "name"
		bool nameend = false;
		for (int i = 0; i < 32; ++i)
		{
			if (mc[i] == 0)
			{
				if (i == 0) return false;
				nameend = true;
			}
			else if (nameend)
			{
				return false;
			}
		}
		size_t width = READ_L32(mc, 32);
		size_t height = READ_L32(mc, 36);
		if (!width || !height || width % 8 || height % 8)
			return EDF_FALSE;
		for (int m = 0; m < 4; ++m)
		{
			size_t offset = READ_L32(mc, (40+(m<<2)));
			if (width>>m == 0 && height>>m == 0 && offset == 0)
				break;
			else if (!offset || size < offset + ((width>>m) * (height>>m)))
				return EDF_FALSE;
		}
		return EDF_TRUE;
	}
};


class ShadowCasterGfxFormat: public EntryDataFormat
{
public:
	ShadowCasterGfxFormat() : EntryDataFormat("img_scgfx") {};
	~ShadowCasterGfxFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// If those were static functions, then I could
		// just do this instead of such copypasta:
		//	return DoomArahDataFormat::isThisFormat(mc);
		if (mc.getSize() < sizeof(patch_header_t))
			return EDF_FALSE;

		const uint8_t* data = mc.getData();
		const patch_header_t* header = (const patch_header_t*)data;

		// Check header values are 'sane'
		if (!(header->height > 0 && header->height < 4096 &&
		        header->width > 0 && header->width < 4096 &&
		        header->top > -2000 && header->top < 2000 &&
		        header->left > -2000 && header->left < 2000))
			return EDF_FALSE;

		// Check the size matches
		if (mc.getSize() != (sizeof(patch_header_t) + (header->width * header->height)))
			return EDF_FALSE;

		return EDF_TRUE;
	}
};

class ShadowCasterSpriteFormat: public EntryDataFormat
{
public:
	ShadowCasterSpriteFormat() : EntryDataFormat("img_scsprite") {};
	~ShadowCasterSpriteFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		int size = mc.getSize();
		if (size < 4)
			return EDF_FALSE;
		int width = READ_L16(mc, 2);
		if (width <= 0 || width > (signed)(size/4))
			return EDF_FALSE;
		int height = 0;
		// Error checking with average column height and proportion of empty columns
		int avgcolheight = 0, pnumemptycol = 0;
		for (int j = 0; j < width; ++j)
		{
			int pos = (j<<1)+4;
			if (pos+2 >= size)
				return EDF_FALSE;
			int offstart = READ_L16(mc, pos);
			if (offstart == 0) continue;
			if (offstart < 0 || size < offstart+2 || offstart < (width*2+4))
				return EDF_FALSE;
			int start		= mc[offstart];
			int stop		= mc[offstart+1];
			int colheight = start - stop;
			if (colheight < 0 || size < offstart+colheight+1)
				return EDF_FALSE;
			if (start > height)
				height = start;
			avgcolheight += colheight;
			if (colheight == 0)
				pnumemptycol++;
		}
		if (height == 0)
			return EDF_FALSE;

		avgcolheight *= 16;	avgcolheight /= width;
		pnumemptycol *= 16;	pnumemptycol /= width;

		// Arbitrary value: sprite must be at least about 12% filled
		if ((avgcolheight < height / 2) || (pnumemptycol > 14))
			return EDF_UNLIKELY;

		// Least efficient sprites: single rows (amounts to 6 bytes per pixel + 4 header bytes)
		return (size < (5 + ((5 + height) * width))) ? EDF_TRUE : EDF_UNLIKELY;
	}
};

class ShadowCasterWallFormat: public EntryDataFormat
{
public:
	ShadowCasterWallFormat() : EntryDataFormat("img_scwall") {};
	~ShadowCasterWallFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		int size = mc.getSize();
		// Minimum valid size for such a picture to be
		// successfully loaded: 130 header, +1 line of 64.
		if (size < 194)
			return EDF_FALSE;
		return (size == (mc[0]*256) + 130) ? EDF_TRUE : EDF_FALSE;
	}
};

class AnaMipImageFormat: public EntryDataFormat
{
public:
	AnaMipImageFormat() : EntryDataFormat("img_mipimage") {};
	~AnaMipImageFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 4)
			return EDF_FALSE;
		size_t width = READ_L16(mc, 0);
		size_t height = READ_L16(mc, 2);
		if ((width|height) == 0)
			return EDF_FALSE;
		size_t pixels = width * height;
		return (size >= (pixels + 4) && (size < (2 * pixels + 4))) ? EDF_TRUE : EDF_FALSE;
	}
};

class BuildTileFormat: public EntryDataFormat
{
public:
	BuildTileFormat() : EntryDataFormat("img_arttile") {};
	~BuildTileFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 16)
			return EDF_FALSE;
		uint32_t version = READ_L32(mc, 0);
		if (version != 1)
			return EDF_FALSE;
		uint32_t firsttile = READ_L32(mc, 8);
		uint32_t lasttile  = READ_L16(mc, 12);
		uint32_t tilecount = 1 + lasttile - firsttile;
		size_t datastart = (16 + (tilecount * 8));
		if (size < datastart)
			return EDF_FALSE;
		size_t gfxdatasize = 0;
		size_t xofs = 16;
		size_t yofs = xofs + (tilecount<<1);
		for (size_t a = 0; a < tilecount; ++a)
		{
			if (size <= xofs+(a<<1) || size <= yofs+(a<<1))
				return EDF_FALSE;
			gfxdatasize += (mc[xofs+(a<<1)] * mc[yofs+(a<<1)]);
		}
		if (size < (datastart + gfxdatasize))
			return EDF_FALSE;
		return EDF_TRUE;
	}
};

class Heretic2M8Format: public EntryDataFormat
{
public:
	Heretic2M8Format() : EntryDataFormat("img_m8") {};
	~Heretic2M8Format() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 1040)
			return EDF_FALSE;
		uint32_t version = READ_L32(mc, 0);
		if (version != 2)
			return EDF_FALSE;
		for (int m = 0; m < 16; ++m)
		{
			size_t width = READ_L32(mc, (36+(m<<2)));
			size_t height = READ_L32(mc, (100+(m<<2)));
			size_t offset = READ_L32(mc, (164+(m<<2)));
			if (width == 0 && height == 0 && offset == 0)
				break;
			else if ((width == 0 && (height|offset) != 0) ||
			         (height == 0 && (width|offset) != 0) ||
			         (offset == 0 && (width|height) != 0))
				return EDF_FALSE;
			else if (offset + (width * height) > size)
				return EDF_FALSE;
		}
		return EDF_TRUE;
	}
};

class Heretic2M32Format: public EntryDataFormat
{
public:
	Heretic2M32Format() : EntryDataFormat("img_m32") {};
	~Heretic2M32Format() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 1040)
			return EDF_FALSE;
		uint32_t version = READ_L32(mc, 0);
		if (version != 4)
			return EDF_FALSE;
		for (int m = 0; m < 16; ++m)
		{
			size_t width = READ_L32(mc, (516+(m<<2)));
			size_t height = READ_L32(mc, (580+(m<<2)));
			size_t offset = READ_L32(mc, (644+(m<<2)));
			if (width == 0 && height == 0 && offset == 0)
				break;
			else if ((width == 0 && (height|offset) != 0) ||
			         (height == 0 && (width|offset) != 0) ||
			         (offset == 0 && (width|height) != 0))
				return EDF_FALSE;
			else if (offset + (4 * width * height) > size)
				return EDF_FALSE;
		}
		return EDF_TRUE;
	}
};

class HalfLifeTextureFormat: public EntryDataFormat
{
public:
	HalfLifeTextureFormat() : EntryDataFormat("img_hlt") {};
	~HalfLifeTextureFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 812)
			return EDF_FALSE;
		size_t width = READ_L32(mc, 16);
		size_t height = READ_L32(mc, 20);
		if (!width || !height || width % 8 || height % 8)
			return EDF_FALSE;
		for (int m = 0; m < 4; ++m)
		{
			size_t offset = READ_L32(mc, (24+(m<<2)));
			if (width>>m == 0 && height>>m == 0 && offset == 0)
				break;
			else if (!offset || size < offset + ((width>>m) * (height>>m)))
				return EDF_FALSE;
		}
		width>>=3;
		height>>=3;
		size_t offset = READ_L32(mc, 36) + (width*height);
		if (size < offset + 5)
			return EDF_FALSE;
		size_t palsize = READ_L16(mc, offset);
		if (size < offset + 2 + (3*palsize))
			return EDF_FALSE;
		return EDF_TRUE;
	}
};

class RottGfxDataFormat : public EntryDataFormat
{
public:
	RottGfxDataFormat() : EntryDataFormat("img_rott") {};
	~RottGfxDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		const uint8_t* data = mc.getData();

		// Check size
		if (mc.getSize() > sizeof(rottpatch_header_t))
		{
			const rottpatch_header_t* header = (const rottpatch_header_t*)data;

			// Check header values are 'sane'
			if (header->height > 0 && header->height < 4096 &&
			        header->width > 0 && header->width < 4096 &&
			        header->top > -2000 && header->top < 2000 &&
			        header->left > -2000 && header->left < 2000)
			{
				uint16_t* col_offsets = (uint16_t*)((const uint8_t*)data + sizeof(rottpatch_header_t));

				// Check there is room for needed column pointers
				if (mc.getSize() < sizeof(rottpatch_header_t) + (header->width * sizeof(uint16_t)))
					return EDF_FALSE;

				// Check column pointers are within range
				for (int a = 0; a < header->width; a++)
				{
					if (col_offsets[a] > mc.getSize() ||
					        col_offsets[a] < (header->width<<1) + sizeof(rottpatch_header_t))
						return EDF_FALSE;
				}

				// Check if total size is reasonable; this computation corresponds to the most inefficient
				// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
				int numpixels = (header->height + 2 + header->height%2)/2;
				int maxcolsize = sizeof(uint32_t) + (numpixels*3) + 1;
				if (mc.getSize() > (2 + sizeof(rottpatch_header_t) + (header->width * maxcolsize)))
				{
					return EDF_UNLIKELY;	// This may still be good anyway
				}

				// Passed all checks, so probably is ROTT gfx
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class RottTransGfxDataFormat : public EntryDataFormat
{
public:
	RottTransGfxDataFormat() : EntryDataFormat("img_rottmask") {};
	~RottTransGfxDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		const uint8_t* data = mc.getData();

		// Check size
		if (mc.getSize() > sizeof(rottpatch_header_t))
		{
			const rottpatch_header_t* header = (const rottpatch_header_t*)data;

			// Check header values are 'sane'
			if (header->height > 0 && header->height < 4096 &&
			        header->width > 0 && header->width < 4096 &&
			        header->top > -2000 && header->top < 2000 &&
			        header->left > -2000 && header->left < 2000)
			{
				uint16_t* col_offsets = (uint16_t*)(2 + (const uint8_t*)data + sizeof(rottpatch_header_t));

				// Check there is room for needed column pointers
				if (mc.getSize() < 2 + sizeof(rottpatch_header_t) + (header->width * sizeof(uint16_t)))
					return EDF_FALSE;

				// Check column pointers are within range
				for (int a = 0; a < header->width; a++)
				{
					if (col_offsets[a] > mc.getSize() ||
					        col_offsets[a] < (header->width<<1) + sizeof(rottpatch_header_t))
						return EDF_FALSE;
				}

				// Check if total size is reasonable; this computation corresponds to the most inefficient
				// possible use of space by the format (horizontal stripes of 1 pixel, 1 pixel apart).
				int numpixels = (header->height + 2 + header->height%2)/2;
				int maxcolsize = sizeof(uint32_t) + (numpixels*3) + 1;
				if (mc.getSize() > (2 + sizeof(rottpatch_header_t) + (header->width * maxcolsize)))
				{
					return EDF_UNLIKELY;	// This may still be good anyway
				}

				// Passed all checks, so probably is ROTT masked gfx
				return EDF_TRUE;
			}
		}

		return EDF_FALSE;
	}
};

class RottLBMDataFormat : public EntryDataFormat
{
public:
	RottLBMDataFormat() : EntryDataFormat("img_rottlbm") {};
	~RottLBMDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		const uint8_t* data = mc.getData();

		// Check size
		if (mc.getSize() > 800)
		{
			if (data[0] == 0x40 && data[1] == 0x01 && data[2] == 0xC8 && data[3] == 0x00)
			{
				// May be ROTT LBM
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class RottRawDataFormat : public EntryDataFormat
{
public:
	RottRawDataFormat() : EntryDataFormat("img_rottraw") {};
	~RottRawDataFormat() {}

	/* How many format does ROTT need? This is just like the raw data plus header
	 * format from the Doom alpha, except that it's column-major instead of row-major.
	 */
	int isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() < sizeof(patch_header_t))
			return EDF_FALSE;

		const uint8_t* data = mc.getData();
		const patch_header_t* header = (const patch_header_t*)data;

		// Check header values are 'sane'
		if (!(header->height > 0 && header->height < 4096 &&
		        header->width > 0 && header->width < 4096 &&
		        header->top > -2000 && header->top < 2000 &&
		        header->left > -2000 && header->left < 2000))
			return EDF_FALSE;

		// Check the size matches
		if (mc.getSize() != (sizeof(patch_header_t) + (header->width * header->height)))
			return EDF_FALSE;

		return EDF_TRUE;
	}
};

class RottPicDataFormat : public EntryDataFormat
{
public:
	RottPicDataFormat() : EntryDataFormat("img_rottpic") {};
	~RottPicDataFormat() {}

	// Yet another ROTT image format. Cheesus.
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 8)
			return EDF_FALSE;

		const uint8_t* data = mc.getData();
		if (data[0] && data[1] && (size - 4 == (data[0] * data[1] * 4)) && data[size-2] == 0 && data[size-1] == 0)
			return EDF_TRUE;
		return EDF_FALSE;
	}
};

class WolfPicDataFormat : public EntryDataFormat
{
public:
	WolfPicDataFormat() : EntryDataFormat("img_wolfpic") {};
	~WolfPicDataFormat() {}

	// Wolf picture format
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 4)
			return EDF_FALSE;
		if ((4 + (READ_L16(mc, 0)*READ_L16(mc, 2))) != mc.getSize())
			return EDF_FALSE;

		return EDF_TRUE;
	}
};

class WolfSpriteDataFormat : public EntryDataFormat
{
public:
	WolfSpriteDataFormat() : EntryDataFormat("img_wolfsprite") {};
	~WolfSpriteDataFormat() {}

	// Wolf picture format
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size < 8 || size > 4228)
			return EDF_FALSE;
		if (mc[0] > 63 || mc[1] || mc[2] > 64 || mc[1] >= mc [2] || mc[3])
			return EDF_FALSE;

		return EDF_TRUE;
	}
};

// Dark Forces uses the "Jedi engine" so its data formats are named jedi too.
class JediBMFormat : public EntryDataFormat
{
public:
	JediBMFormat() : EntryDataFormat("img_jedi_bm") {};
	~JediBMFormat() {}

	// Jedi engine bitmap format
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size > 32)
		{
			if (mc[0] == 'B' && mc[1] == 'M' && mc[2] == ' ' && mc[3] == 0x1E
			        && READ_L16(mc, 4) != 0 && READ_L16(mc, 6) != 0
			        && mc[14] < 3 && mc[15] == 0)
			{
				// Check that padding is left alone
				for (int i = 20; i < 32; ++i)
					if (mc[i]) return EDF_FALSE;
				// Probably okay
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class JediFMEFormat : public EntryDataFormat
{
public:
	JediFMEFormat() : EntryDataFormat("img_jedi_fme") {};
	~JediFMEFormat() {}

	// Jedi engine frame format
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size > 64)
		{
			// The only constants we have is that byte 8 is either 1 or 0 (h-flip)
			// and byte 12 is always 32 (secondary header after the first header)
			// Then in the secondary header, byte 40 is 1 or 0 (compression) and
			// bytes 44-47 correspond to the little-endian size of the file - 32.
			if (mc[8] < 2 && mc[12] == 32 && (mc[9]|mc[10]|mc[11]|mc[13]|mc[14]|mc[15]) == 0 && mc[40] < 2)
			{
				// Check that padding is left alone
				for (int i = 16; i < 32; ++i)
					if (mc[i]) return EDF_FALSE;
				for (int i = 16; i < 32; ++i)
					if (mc[i]) return EDF_FALSE;
				// Check size if compressed:
				if (mc[40] == 1)
				{
					if (READ_L32(mc, 44) != size - 32)
						return EDF_FALSE;
					// Check some padding
					for (int i = 41; i < 44; ++i)
						if (mc[i]) return EDF_FALSE;
					for (int i = 48; i < 56; ++i)
						if (mc[i]) return EDF_FALSE;
				}
				else
				{
					// All that should be zero if uncompressed
					for (int i = 41; i < 56; ++i)
						if (mc[i]) return EDF_FALSE;
				}
				// Probably okay
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class JediWAXFormat : public EntryDataFormat
{
public:
	JediWAXFormat() : EntryDataFormat("img_jedi_wax") {};
	~JediWAXFormat() {}

	// Jedi engine wax format
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size > 460)
		{
			// Constant identifier 00 10 01 00 *or* 00 00 01 00
			if (mc[0] == 0 && (mc[1] == 16 || mc[1] == 0) && mc[2] == 1 && mc[3] == 0)
			{
				// The numbers of sequences, frames and cells should not exceed 255.
				// Also check that padding is left alone.
				for (int i = 5; i < 8; ++i)
					if (mc[i]) return EDF_FALSE;
				for (int i = 9; i < 12; ++i)
					if (mc[i]) return EDF_FALSE;
				for (int i = 13; i < 32; ++i)
					if (mc[i]) return EDF_FALSE;
				// Probably okay
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};


class Font0DataFormat : public EntryDataFormat
{
public:
	Font0DataFormat() : EntryDataFormat("font_doom_alpha") {};
	~Font0DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() <= 0x302)
			return EDF_FALSE;

		const uint16_t* gfx_data = (const uint16_t*) mc.getData();

		size_t height = wxINT16_SWAP_ON_BE(gfx_data[0]);

		size_t datasize = mc.getSize() - 0x302;
		if (height == 0 || datasize % height)
			return EDF_FALSE;

		// It seems okay so far. Check that one
		// character does start at offset 0x302.
		// The offsets are themselves between
		// offsets 0x102 and 0x302. Halved for int16_t.
		for (size_t i = 0x81; i < 0x181; ++i)
			if (gfx_data[i] == wxINT16_SWAP_ON_BE(0x302))
				return EDF_TRUE;

		// Doesn't seem to be such a file after all.
		return EDF_FALSE;
	}
};

class Font1DataFormat : public EntryDataFormat
{
public:
	Font1DataFormat() : EntryDataFormat("font_zd_console") {};
	~Font1DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for FON1 header
			if (mc[0] == 'F' && mc[1] == 'O' && mc[2] == 'N' && mc[3] == '1')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class Font2DataFormat : public EntryDataFormat
{
public:
	Font2DataFormat() : EntryDataFormat("font_zd_big") {};
	~Font2DataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for FON2 header
			if (mc[0] == 'F' && mc[1] == 'O' && mc[2] == 'N' && mc[3] == '2')
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class BMFontDataFormat : public EntryDataFormat
{
public:
	BMFontDataFormat() : EntryDataFormat("font_bmf") {};
	~BMFontDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		// Check size
		if (mc.getSize() > 4)
		{
			// Check for BMF header
			if (mc[0] == 0xE1 && mc[1] == 0xE6 && mc[2] == 0xD5 && mc[3] == 0x1A)
				return EDF_TRUE;
		}
		return EDF_FALSE;
	}
};

class FontWolfDataFormat : public EntryDataFormat
{
public:
	FontWolfDataFormat() : EntryDataFormat("font_wolf") {};
	~FontWolfDataFormat() {}

	int isThisFormat(MemChunk& mc)
	{
		if (mc.getSize() <= 0x302)
			return EDF_FALSE;

		const uint16_t* gfx_data = (const uint16_t*) mc.getData();

		size_t height = wxINT16_SWAP_ON_BE(gfx_data[0]);

		size_t datasize = mc.getSize() - 0x302;
		if (height == 0 || datasize % height)
			return EDF_FALSE;

		// It seems okay so far. Check that one
		// character does start at offset 0x302.
		// The offsets are themselves between
		// offsets 0x002 and 0x202. Halved for int16_t.
		for (size_t i = 0x01; i < 0x101; ++i)
			if (gfx_data[i] == wxINT16_SWAP_ON_BE(0x302))
				return EDF_TRUE;

		// Doesn't seem to be such a file after all.
		return EDF_FALSE;
	}
};

class JediFNTFormat : public EntryDataFormat
{
public:
	JediFNTFormat() : EntryDataFormat("font_jedi_fnt") {};
	~JediFNTFormat() {}

	// Jedi engine fnt format
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size > 35)
		{
			// Constant identifier FNT\15, height should be greater than 0,
			// last should be greater than first.
			if (mc[0] == 'F' && mc[1] == 'N' && mc[2] == 'T' && mc[3] == 0x15
			        && mc[4] > 0 && mc[8] <= mc[9])
			{
				// Check that padding is left alone.
				for (int i = 10; i < 32; ++i)
					if (mc[i]) return EDF_FALSE;
				// Probably okay
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};

class JediFONTFormat : public EntryDataFormat
{
public:
	JediFONTFormat() : EntryDataFormat("font_jedi_font") {};
	~JediFONTFormat() {}

	// Jedi engine font format
	int isThisFormat(MemChunk& mc)
	{
		size_t size = mc.getSize();
		if (size > 16)
		{
			// Numchar should be greater than 0, width should be multiple of 8,
			// height should be greater than 0, padding should be null.
			if (READ_L16(mc, 2) >= 0 && (READ_L16(mc, 4)%8) == 0 &&
			        READ_L16(mc, 6) >= 0 && READ_L16(mc, 10) == 0)
			{
				size_t numchr = READ_L16(mc, 2);
				if (size < 16 + numchr)
					return EDF_FALSE;
				// Also check that character width never exceeds max width.
				for (size_t i = 12; i < 12 + numchr; ++i)
					if (mc[i] > READ_L16(mc, 4)) return EDF_FALSE;
				// Check that there are enough data to cover all characters and the header
				size_t neededbytes = 12 + numchr + ((numchr * READ_L16(mc, 6)) * (READ_L16(mc, 4)>>3));
				if (size != neededbytes)
					return EDF_FALSE;
				// Probably okay
				return EDF_TRUE;
			}
		}
		return EDF_FALSE;
	}
};



#endif //IMAGEFORMATS_H
