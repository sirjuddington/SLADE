#pragma once

namespace slade::gfx
{
// The header of a Doom-format gfx image
struct PatchHeader
{
	short width;
	short height;
	short left;
	short top;
};

// The header of an alpha/beta Doom-format gfx image
struct OldPatchHeader
{
	uint8_t width;
	uint8_t height;
	int8_t  left;
	int8_t  top;
};

// The header of a Jaguar Doom-format gfx image
struct JagPicHeader
{
	short width;
	short height;
	short depth;
	short palshift;
	short flags;
	char  padding[6];
};

// The header of a PSX Doom-format gfx image
struct PSXPicHeader
{
	short left;
	short top;
	short width;
	short height;
};

// The header of a rott-format gfx image
struct ROTTPatchHeader
{
	short origsize;
	short width;
	short height;
	short left;
	short top;
	// short	translevel; // Not all of them have that
};

// The header of a ZDoom imgz image
struct IMGZHeader
{
	uint8_t  magic[4];
	uint16_t width;
	uint16_t height;
	int16_t  left;
	int16_t  top;
	uint8_t  compression;
	uint8_t  reserved[11];
};
} // namespace slade::gfx
