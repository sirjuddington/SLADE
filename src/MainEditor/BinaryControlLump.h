#pragma once

/* Various little structures for binary control lumps. */

// If the compiler pads the structures, this completely breaks parsing.
#pragma pack(1)

namespace slade
{
/*******************************************************************
 * BOOM'S ANIMATED LUMP
 *******************************************************************/

namespace animtype
{
	static constexpr int FLAT    = 0;
	static constexpr int TEXTURE = 1;
	static constexpr int MASK    = 1;
	static constexpr int DECALS  = 2; // ZDoom uses bit 1 to flag whether decals are allowed.
	static constexpr int STOP    = 255;
} // namespace animtype

// The format of an entry in an ANIMATED lump
struct AnimatedEntry
{
	uint8_t  type;     // flat or texture, with or without decals
	char     last[9];  // first name in the animation
	char     first[9]; // last name in the animation
	uint32_t speed;    // amount of tics between two frames
};

/*******************************************************************
 * BOOM'S SWITCHES LUMP
 *******************************************************************/

namespace switchtype
{
	static constexpr int STOP = wxINT16_SWAP_ON_BE(0);
	static constexpr int DEMO = wxINT16_SWAP_ON_BE(1);
	static constexpr int FULL = wxINT16_SWAP_ON_BE(2);
	static constexpr int COMM = wxINT16_SWAP_ON_BE(3);
	static constexpr int OOPS = wxINT16_SWAP_ON_BE(4);
} // namespace switchtype

struct SwitchesEntry
{
	char     off[9];
	char     on[9];
	uint16_t type;
};
} // namespace slade

// Can restore default packing now
#pragma pack()
