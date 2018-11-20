#pragma once

/* Various little structures for binary control lumps. */

// If the compiler pads the structures, this completely breaks parsing.
#pragma pack(1)

/*******************************************************************
 * BOOM'S ANIMATED LUMP
 *******************************************************************/

enum AnimatedType
{
	ANIM_FLAT = 0,
	ANIM_TEXTURE = 1,
	ANIM_MASK = 1,
	ANIM_DECALS = 2, // ZDoom uses bit 1 to flag whether decals are allowed.
	ANIM_STOP = 255,
};

// The format of an entry in an ANIMATED lump
struct AnimatedEntry
{
	uint8_t		type;		// flat or texture, with or without decals
	char		last [9];	// first name in the animation
	char		first[9];	// last name in the animation
	uint32_t	speed;		// amount of tics between two frames
};

/*******************************************************************
 * BOOM'S SWITCHES LUMP
 *******************************************************************/

enum SwitchesType
{
	SWCH_STOP = wxINT16_SWAP_ON_BE(0),
	SWCH_DEMO = wxINT16_SWAP_ON_BE(1),
	SWCH_FULL = wxINT16_SWAP_ON_BE(2),
	SWCH_COMM = wxINT16_SWAP_ON_BE(3),
	SWCH_OOPS = wxINT16_SWAP_ON_BE(4),
};

struct SwitchesEntry
{
	char		off[9];
	char		on[9];
	uint16_t	type;
};

// Can restore default packing now
#pragma pack()
