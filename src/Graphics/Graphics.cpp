
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Graphics.cpp
// Description: Various gfx-namespace functions for working with graphic data
//              and formats (eg. PNG)
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Graphics.h"
#include "Archive/ArchiveEntry.h"
#include "General/Misc.h"
#include "Graphics/GameFormats.h"
#include "SImage/SIFormat.h"
#include "Utility/Memory.h"

using namespace slade;

// -----------------------------------------------------------------------------
//
// Local Functions
//
// -----------------------------------------------------------------------------
namespace slade::gfx
{
// Define some png chunk structures
struct Ihdr
{
	uint32_t id;
	uint32_t width;
	uint32_t height;
	uint8_t  cinfo[5];
};

struct GrabChunk
{
	char    name[4];
	int32_t xoff;
	int32_t yoff;
};

struct AlphChunk
{
	char name[4];
};

struct TrnsChunk
{
	char    name[4];
	uint8_t entries[256];
};

struct TransChunk
{
	char name[5];
};

struct ChunkSize
{
	uint32_t size;
};

// -----------------------------------------------------------------------------
// Sets the offsets to [xoff,yoff] in the given DoomGfx formatted [data]
// -----------------------------------------------------------------------------
bool setOffsetsDoomGfx(SeekableData& data, int xoff, int yoff)
{
	// Get patch header
	PatchHeader header;
	data.seekFromStart(0);
	data.read(&header, 8);

	if (header.left == xoff && header.top == yoff)
		return false;

	// Apply new offsets
	header.left = wxINT16_SWAP_ON_BE((int16_t)xoff);
	header.top  = wxINT16_SWAP_ON_BE((int16_t)yoff);

	// Write new header to entry
	data.seekFromStart(0);
	data.write(&header, 8);

	return true;
}

// -----------------------------------------------------------------------------
// Sets the offsets to [xoff,yoff] in the given DoomGfxAlpha formatted [data]
// -----------------------------------------------------------------------------
bool setOffsetsDoomAlphaGfx(SeekableData& data, int xoff, int yoff)
{
	// Get patch header
	data.seekFromStart(0);
	OldPatchHeader header;
	data.read(&header, 4);

	if (header.left == xoff && header.top == yoff)
		return false;

	// Apply new offsets
	header.left = (int8_t)xoff;
	header.top  = (int8_t)yoff;

	// Write new header to entry
	data.seekFromStart(0);
	data.write(&header, 4);

	return true;
}
} // namespace slade::gfx


// -----------------------------------------------------------------------------
//
// Gfx Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the dimensions of the PNG image in [png_data]
// -----------------------------------------------------------------------------
Vec2i gfx::pngGetSize(const MemChunk& png_data)
{
	if (png_data.size() < 24)
		return { 0, 0 };

	// Read width and height from IHDR chunk
	const Ihdr* ihdr = (Ihdr*)(png_data.data() + 12);
	uint32_t    w    = wxINT32_SWAP_ON_LE(ihdr->width);
	uint32_t    h    = wxINT32_SWAP_ON_LE(ihdr->height);

	return { static_cast<int>(w), static_cast<int>(h) };
}

// -----------------------------------------------------------------------------
// Returns the offsets (grAb chunk) of the PNG image in [png_data].
// Will return an empty std::optional if the png has no grAb chunk.
// -----------------------------------------------------------------------------
std::optional<Vec2i> gfx::pngGetgrAb(const MemChunk& png_data)
{
	// Find existing grAb chunk
	for (uint32_t a = 0; a < png_data.size(); a++)
	{
		// Check for 'grAb' header
		if (png_data[a] == 'g' && png_data[a + 1] == 'r' && png_data[a + 2] == 'A' && png_data[a + 3] == 'b')
		{
			int x = memory::readB32(png_data.data(), a + 4);
			int y = memory::readB32(png_data.data(), a + 8);
			return { Vec2i(x, y) };
		}

		// Stop when we get to the 'IDAT' chunk
		if (png_data[a] == 'I' && png_data[a + 1] == 'D' && png_data[a + 2] == 'A' && png_data[a + 3] == 'T')
			break;
	}

	return {};
}

// -----------------------------------------------------------------------------
// Sets the offsets (grAb chunk) of the PNG image in [png_data] to [xoff,yoff].
// The grAb chunk will be added if one doesn't already exist.
// Returns false if the existing offsets were identical (and no change was made)
// -----------------------------------------------------------------------------
bool gfx::pngSetgrAb(MemChunk& png_data, int xoff, int yoff)
{
	// Find existing grAb chunk
	const auto* data       = png_data.data();
	uint32_t    grab_start = 0;
	int32_t     ox         = 0;
	int32_t     oy         = 0;
	for (uint32_t a = 0; a < png_data.size(); a++)
	{
		// Check for 'grAb' header
		if (data[a] == 'g' && data[a + 1] == 'r' && data[a + 2] == 'A' && data[a + 3] == 'b')
		{
			grab_start = a - 4;
			auto grab  = (const GrabChunk*)(data + a);
			ox         = wxINT32_SWAP_ON_LE(grab->xoff);
			oy         = wxINT32_SWAP_ON_LE(grab->yoff);
			break;
		}

		// Stop when we get to the 'IDAT' chunk
		if (data[a] == 'I' && data[a + 1] == 'D' && data[a + 2] == 'A' && data[a + 3] == 'T')
			break;
	}

	// Check if we need to write anything
	if (grab_start != 0 && ox == xoff && oy == yoff)
		return false;

	// Create new grAb chunk
	uint32_t  csize = wxUINT32_SWAP_ON_LE(8);
	GrabChunk gc    = { { 'g', 'r', 'A', 'b' }, wxINT32_SWAP_ON_LE(xoff), wxINT32_SWAP_ON_LE(yoff) };
	uint32_t  dcrc  = wxUINT32_SWAP_ON_LE(misc::crc((uint8_t*)&gc, 12));

	// Build new PNG from the original w/ the new grAb chunk
	MemChunk npng;
	uint32_t rest_start = 33;

	// Init new png data size
	if (grab_start == 0)
		npng.reSize(png_data.size() + 20);
	else
		npng.reSize(png_data.size());

	// Write PNG header and IHDR chunk
	npng.write(data, 33);

	// If no existing grAb chunk was found, write new one here
	if (grab_start == 0)
	{
		npng.write(&csize, 4);
		npng.write(&gc, 12);
		npng.write(&dcrc, 4);
	}
	else
	{
		// Otherwise write any other data before the existing grAb chunk
		uint32_t to_write = grab_start - 33;
		npng.write(data + 33, to_write);
		rest_start = grab_start + 20;

		// And now write the new grAb chunk
		npng.write(&csize, 4);
		npng.write(&gc, 12);
		npng.write(&dcrc, 4);
	}

	// Write the rest of the PNG data
	uint32_t to_write = png_data.size() - rest_start;
	npng.write(data + rest_start, to_write);

	// Load new png data
	png_data.importMem(npng);

	return true;
}

// -----------------------------------------------------------------------------
// Returns true if the PNG image in [png_data] has a tRNS chunk
// -----------------------------------------------------------------------------
bool gfx::pngGettRNS(const MemChunk& png_data)
{
	if (png_data.size() < 12)
		return false;

	// tRNS chunks are only valid for paletted PNGs, and the chunk must before the first IDAT.
	// Specs say it should be after a PLTE chunk, but that's not always the case (e.g., sgrna7a3.png).
	for (uint32_t a = 0; a < png_data.size(); a++)
	{
		// Check for 'tRNS' header
		if (png_data[a] == 't' && png_data[a + 1] == 'R' && png_data[a + 2] == 'N' && png_data[a + 3] == 'S')
			return true;

		// Stop when we get to the 'IDAT' chunk
		if (png_data[a] == 'I' && png_data[a + 1] == 'D' && png_data[a + 2] == 'A' && png_data[a + 3] == 'T')
			break;
	}
	return false;
}

// -----------------------------------------------------------------------------
// Adds or removes the tRNS chunk in [png_data] depending on [value]
// -----------------------------------------------------------------------------
bool gfx::pngSettRNS(MemChunk& png_data, bool value)
{
	// tRNS chunks are only valid for paletted PNGs, and must be before the first IDAT.
	// Specs say they must be after PLTE chunk as well, so to play it safe, we'll insert
	// them just before the first IDAT.
	uint32_t trns_start = 0;
	uint32_t trns_size  = 0;
	uint32_t idat_start = 0;
	for (uint32_t a = 0; a < png_data.size(); a++)
	{
		// Check for 'tRNS' header
		if (png_data[a] == 't' && png_data[a + 1] == 'R' && png_data[a + 2] == 'N' && png_data[a + 3] == 'S')
		{
			trns_start = a - 4;
			auto trns  = (TransChunk*)(png_data.data() + a);
			trns_size  = 12 + memory::readB32(png_data.data(), a - 4);
		}

		// Stop when we get to the 'IDAT' chunk
		if (png_data[a] == 'I' && png_data[a + 1] == 'D' && png_data[a + 2] == 'A' && png_data[a + 3] == 'T')
		{
			idat_start = a - 4;
			break;
		}
	}

	// The IDAT chunk starts before the header is finished, this doesn't make sense, abort.
	if (idat_start < 33)
		return false;

	// We want to set tRNS, and it is already there: nothing to do.
	if (value && trns_start > 0)
		return false;

	// We want to unset tRNS, and it is already not there: nothing to do either.
	else if (!value && trns_start == 0)
		return false;

	// We want to set tRNS, which is missing: create it. We're just going to set index 0 to 0,
	// and leave the rest of the palette indices alone.
	else if (value && trns_start == 0)
	{
		// Build new PNG from the original w/ the new tRNS chunk
		MemChunk npng;

		// Init new png data size
		npng.reSize(png_data.size() + 13);

		// Write PNG header stuff up to the first IDAT chunk
		npng.write(png_data.data(), idat_start);

		// Create new tRNS chunk
		uint32_t   csize = wxUINT32_SWAP_ON_LE(1);
		TransChunk gc    = { { 't', 'R', 'N', 'S', '\0' } };
		uint32_t   dcrc  = wxUINT32_SWAP_ON_LE(misc::crc((uint8_t*)&gc, 5));

		// Write tRNS chunk
		npng.write(&csize, 4);
		npng.write(&gc, 5);
		npng.write(&dcrc, 4);

		// Write the rest of the PNG data
		uint32_t to_write = png_data.size() - idat_start;
		npng.write(png_data.data() + idat_start, to_write);

		// Load new png data to the entry
		png_data.importMem(npng);
	}

	// We want to unset tRNS, which is present: delete it.
	else if (!value && trns_start > 0)
	{
		// Build new PNG from the original without the tRNS chunk
		MemChunk npng;
		uint32_t rest_start = trns_start + trns_size;

		// Init new png data size
		npng.reSize(png_data.size() - trns_size);

		// Write PNG header and stuff up to tRNS start
		npng.write(png_data.data(), trns_start);

		// Write the rest of the PNG data
		uint32_t to_write = png_data.size() - rest_start;
		npng.write(png_data.data() + rest_start, to_write);

		// Load new png data to the entry
		png_data.importMem(npng);
	}

	// We don't know what we want, but it can't be good, so we do nothing.
	else
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Returns true if the PNG image in [png_data] has an alPh chunk
// -----------------------------------------------------------------------------
bool gfx::pngGetalPh(const MemChunk& png_data)
{
	if (png_data.size() < 12)
		return false;

	// Find existing alPh chunk
	for (uint32_t a = 0; a < png_data.size(); a++)
	{
		// Check for 'alPh' header
		if (png_data[a] == 'a' && png_data[a + 1] == 'l' && png_data[a + 2] == 'P' && png_data[a + 3] == 'h')
			return true;

		// Stop when we get to the 'IDAT' chunk
		if (png_data[a] == 'I' && png_data[a + 1] == 'D' && png_data[a + 2] == 'A' && png_data[a + 3] == 'T')
			break;
	}
	return false;
}

// -----------------------------------------------------------------------------
// Adds or removes the alPh chunk in [png_data] depending on [value]
// -----------------------------------------------------------------------------
bool gfx::pngSetalPh(MemChunk& png_data, bool value)
{
	// Find existing alPh chunk
	uint32_t alph_start = 0;
	for (uint32_t a = 0; a < png_data.size(); a++)
	{
		// Check for 'alPh' header
		if (png_data[a] == 'a' && png_data[a + 1] == 'l' && png_data[a + 2] == 'P' && png_data[a + 3] == 'h')
		{
			alph_start = a - 4;
			break;
		}

		// Stop when we get to the 'IDAT' chunk
		if (png_data[a] == 'I' && png_data[a + 1] == 'D' && png_data[a + 2] == 'A' && png_data[a + 3] == 'T')
			break;
	}

	// We want to set alPh, and it is already there: nothing to do.
	if (value && alph_start > 0)
		return false;

	// We want to unset alPh, and it is already not there: nothing to do either.
	else if (!value && alph_start == 0)
		return false;

	// We want to set alPh, which is missing: create it.
	else if (value && alph_start == 0)
	{
		// Build new PNG from the original w/ the new alPh chunk
		MemChunk npng;

		// Init new png data size
		npng.reSize(png_data.size() + 12);

		// Write PNG header and IHDR chunk
		npng.write(png_data.data(), 33);

		// Create new alPh chunk
		uint32_t  csize = wxUINT32_SWAP_ON_LE(0);
		AlphChunk gc    = { { 'a', 'l', 'P', 'h' } };
		uint32_t  dcrc  = wxUINT32_SWAP_ON_LE(misc::crc((uint8_t*)&gc, 4));

		// Create alPh chunk
		npng.write(&csize, 4);
		npng.write(&gc, 4);
		npng.write(&dcrc, 4);

		// Write the rest of the PNG data
		uint32_t to_write = png_data.size() - 33;
		npng.write(png_data.data() + 33, to_write);

		// Load new png data to the entry
		png_data.importMem(npng);
	}

	// We want to unset alPh, which is present: delete it.
	else if (!value && alph_start > 0)
	{
		// Build new PNG from the original without the alPh chunk
		MemChunk npng;
		uint32_t rest_start = alph_start + 12;

		// Init new png data size
		npng.reSize(png_data.size() - 12);

		// Write PNG info before alPh chunk
		npng.write(png_data.data(), alph_start);

		// Write the rest of the PNG data
		uint32_t to_write = png_data.size() - rest_start;
		npng.write(png_data.data() + rest_start, to_write);

		// Load new png data to the entry
		png_data.importMem(npng);
	}

	// We don't know what we want, but it can't be good, so we do nothing.
	else
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Calculates the offsets for a [width]x[height] graphic, for a sprite of [type]
// -----------------------------------------------------------------------------
Vec2i gfx::calculateOffsets(int width, int height, OffsetType type)
{
	switch (type)
	{
	case OffsetType::Monster: return { static_cast<int>(width * 0.5), height - 4 };
	case OffsetType::MonsterGL: return { static_cast<int>(width * 0.5), height };
	case OffsetType::Projectile: return { static_cast<int>(width * 0.5), static_cast<int>(height * 0.5) };
	case OffsetType::WeaponFull: return { -160 + static_cast<int>(width * 0.5), -200 + height };
	case OffsetType::WeaponDoom: return { -160 + static_cast<int>(width * 0.5), -200 + 32 + height };
	case OffsetType::WeaponHeretic: return { -160 + static_cast<int>(width * 0.5), -200 + 42 + height };
	case OffsetType::WeaponHexen: return { -160 + static_cast<int>(width * 0.5), -200 + 38 + height };
	default: return { 0, 0 };
	};
}

// -----------------------------------------------------------------------------
// Returns the offsets for the image in [img_data].
// Will return an empty std::optional if the data is not a format that supports
// offsets, or if no offsets are present in the image.
// -----------------------------------------------------------------------------
std::optional<Vec2i> gfx::getImageOffsets(MemChunk& img_data)
{
	// Detect image format if any
	auto* format = SIFormat::determineFormat(img_data);

	// Doom gfx format, normal and beta version.
	// Also arah format from alpha 0.2 because it uses the same header format.
	if (format->id() == "doom" || format->id() == "doom_beta" || format->id() == "doom_arah")
	{
		PatchHeader header;
		img_data.seekFromStart(0);
		img_data.read(&header, 8);

		return { Vec2i(header.left, header.top) };
	}

	// Doom alpha gfx format
	else if (format->id() == "doom_alpha")
	{
		OldPatchHeader header;
		img_data.seekFromStart(0);
		img_data.read(&header, 8);

		return { Vec2i(header.left, header.top) };
	}

	// PNG format
	else if (format->id() == "png")
		return pngGetgrAb(img_data);

	return {};
}

// -----------------------------------------------------------------------------
// Sets the offsets of the image in [img_data] to [xoff,yoff].
// Returns false if the image has no offsets, or the existing offsets were
// identical (and no change was made)
// -----------------------------------------------------------------------------
bool gfx::setImageOffsets(MemChunk& img_data, int xoff, int yoff)
{
	// Detect image format if any
	auto* format = SIFormat::determineFormat(img_data);

	// Doom gfx format, normal and beta version.
	// Also arah format from alpha 0.2 because it uses the same header format.
	if (format->id() == "doom" || format->id() == "doom_beta" || format->id() == "doom_arah")
		return setOffsetsDoomGfx(img_data, xoff, yoff);

	// Doom alpha gfx format
	else if (format->id() == "doom_alpha")
		return setOffsetsDoomAlphaGfx(img_data, xoff, yoff);

	// PNG format
	else if (format->id() == "png")
		return pngSetgrAb(img_data, xoff, yoff);

	// Unsupported format
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns true if the given [entry] supports offsets
// -----------------------------------------------------------------------------
bool gfx::supportsOffsets(const ArchiveEntry& entry)
{
	const auto* type = entry.type();
	if (type == nullptr)
		return false;

	// Check entry type
	auto& entryformat = type->formatId();
	return entryformat == "img_doom" || entryformat == "img_doom_arah" || entryformat == "img_doom_alpha"
		   || entryformat == "img_doom_beta" || entryformat == "img_png";
}
