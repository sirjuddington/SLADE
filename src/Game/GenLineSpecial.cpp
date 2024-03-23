
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    GenLineSpecial.cpp
// Description: Stuff for handling Boom generalised line specials
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
#include "GenLineSpecial.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Values
//
// -----------------------------------------------------------------------------
namespace
{
enum
{
	GenFloorBase   = 0x6000,
	GenCeilingBase = 0x4000,
	GenDoorBase    = 0x3c00,
	GenLockedBase  = 0x3800,
	GenLiftBase    = 0x3400,
	GenStairsBase  = 0x3000,
	GenCrusherBase = 0x2F80
};

enum
{
	TriggerType      = 0x0007,
	TriggerTypeShift = 0
};

enum
{
	FloorCrush     = 0x1000,
	FloorChange    = 0x0c00,
	FloorTarget    = 0x0380,
	FloorDirection = 0x0040,
	FloorModel     = 0x0020,
	FloorSpeed     = 0x0018
};

enum
{
	FloorCrushShift     = 12,
	FloorChangeShift    = 10,
	FloorTargetShift    = 7,
	FloorDirectionShift = 6,
	FloorModelShift     = 5,
	FloorSpeedShift     = 3
};

enum
{
	CeilingCrush     = 0x1000,
	CeilingChange    = 0x0c00,
	CeilingTarget    = 0x0380,
	CeilingDirection = 0x0040,
	CeilingModel     = 0x0020,
	CeilingSpeed     = 0x0018
};

enum
{
	CeilingCrushShift     = 12,
	CeilingChangeShift    = 10,
	CeilingTargetShift    = 7,
	CeilingDirectionShift = 6,
	CeilingModelShift     = 5,
	CeilingSpeedShift     = 3
};

enum
{
	LiftTarget  = 0x0300,
	LiftDelay   = 0x00c0,
	LiftMonster = 0x0020,
	LiftSpeed   = 0x0018
};

enum
{
	LiftTargetShift  = 8,
	LiftDelayShift   = 6,
	LiftMonsterShift = 5,
	LiftSpeedShift   = 3
};

enum
{
	StairIgnore    = 0x0200,
	StairDirection = 0x0100,
	StairStep      = 0x00c0,
	StairMonster   = 0x0020,
	StairSpeed     = 0x0018
};

enum
{
	StairIgnoreShift    = 9,
	StairDirectionShift = 8,
	StairStepShift      = 6,
	StairMonsterShift   = 5,
	StairSpeedShift     = 3
};

enum
{
	CrusherSilent  = 0x0040,
	CrusherMonster = 0x0020,
	CrusherSpeed   = 0x0018
};

enum
{
	CrusherSilentShift  = 6,
	CrusherMonsterShift = 5,
	CrusherSpeedShift   = 3
};

enum
{
	DoorDelay   = 0x0300,
	DoorMonster = 0x0080,
	DoorKind    = 0x0060,
	DoorSpeed   = 0x0018
};

enum
{
	DoorDelayShift   = 8,
	DoorMonsterShift = 7,
	DoorKindShift    = 5,
	DoorSpeedShift   = 3
};

enum
{
	LockedNKeys = 0x0200,
	LockedKey   = 0x01c0,
	LockedKind  = 0x0020,
	LockedSpeed = 0x0018
};

enum
{
	LockedNKeysShift = 9,
	LockedKeyShift   = 6,
	LockedKindShift  = 5,
	LockedSpeedShift = 3
};

const char* triggers[] = {
	"W1", "WR", "S1", "SR", "G1", "GR", "D1", "DR",
};

const char* floor_targets[] = {
	"to Highest N Floor", "to Lowest N Floor", "to Next N Floor", "to Lowest N Ceiling",
	"to Ceiling",         "by Lower Tex",      "24 Units",        "32 Units",
};

const char* directions[] = {
	"Down",
	"Up",
};

const char* speeds[] = {
	"Slow",
	"Normal",
	"Fast",
	"Turbo",
};

const char* changers[] = {
	"",
	"Zero Type/Copy Tex",
	"Copy Tex",
	"Copy Type/Copy Tex",
};

const char* models[] = {
	"Trigger",
	"Numeric",
};

const char* crushers[] = {
	"",
	"Cr",
};

const char* ceiling_targets[] = {
	"to Highest N Ceiling",
	"to Lowest N Ceiling",
	"to Next N Ceiling",
	"to Lowest N Floor",
	"to Floor",
	"by Upper Tex",
	"24 Units",
	"32 Units",
};

const char* doors1[] = {
	"OpnD",
	"Opn",
	"ClsD",
	"Cls",
};

const char* doors2[] = {
	"Cls",
	"",
	"Opn",
	"",
};

const char* delays[] = {
	"1",
	"4",
	"9",
	"30",
};

const char* locked_delays[] = {
	"4",
};

const char* locks[] = {
	"Any Key", "Red Card", "Blue Card", "Yellow Card", "Red Skull", "Blue Skull", "Yellow Skull", "All 6 Keys",
	"Any Key", "Red Key",  "Blue Key",  "Yellow Key",  "Red Key",   "Blue Key",   "Yellow Key",   "All 3 Keys",
};

const char* lift_targets[] = {
	"to Lowest N Floor",
	"to Next N Floor",
	"to Lowest N Ceiling",
	"Perpetual",
};

const char* lift_delays[] = {
	"1",
	"3",
	"5",
	"10",
};

const char* steps[] = {
	"4",
	"8",
	"16",
	"24",
};
} // namespace


// -----------------------------------------------------------------------------
//
// genlinespecial Namespace
//
// -----------------------------------------------------------------------------
namespace slade::genlinespecial
{
// ------------------------------------------------------------------------
// Returns a string representation of the generalised line value [type]
// ------------------------------------------------------------------------
string parseLineType(int type)
{
	string type_string;

	// Floor type
	if (type >= 0x6000)
	{
		int trigger   = type & TriggerType;
		int speed     = (type & FloorSpeed) >> FloorSpeedShift;
		int direction = (type & FloorDirection) >> FloorDirectionShift;
		int target    = (type & FloorTarget) >> FloorTargetShift;
		int change    = (type & FloorChange) >> FloorChangeShift;
		int model     = (type & FloorModel) >> FloorModelShift;

		// Trigger
		type_string += triggers[trigger];
		if (change == 0 && model == 1)
			type_string += "M";

		type_string += " Floor ";

		// Direction, target, speed
		type_string += fmt::format("{} {} {}", directions[direction], floor_targets[target], speeds[speed]);

		// Change
		if (type & FloorChange)
			type_string += fmt::format(" {} ({})", changers[change], models[model]);

		// Crush
		if (type & FloorCrush)
			type_string += " Crushing";
	}

	// Ceiling type
	else if (type >= 0x4000)
	{
		int trigger   = type & TriggerType;
		int speed     = (type & CeilingSpeed) >> CeilingSpeedShift;
		int direction = (type & CeilingDirection) >> CeilingDirectionShift;
		int target    = (type & CeilingTarget) >> CeilingTargetShift;
		int change    = (type & CeilingChange) >> CeilingChangeShift;
		int model     = (type & CeilingModel) >> CeilingModelShift;

		// Trigger
		type_string += triggers[trigger];
		if (change == 0 && model == 1)
			type_string += "M";

		type_string += " Ceiling ";

		// Direction, target, speed
		type_string += fmt::format("{} {} {}", directions[direction], ceiling_targets[target], speeds[speed]);

		// Change
		if (type & CeilingChange)
			type_string += fmt::format(" {} ({})", changers[change], models[model]);

		// Crush
		if (type & CeilingCrush)
			type_string += " Crushing";
	}

	// Door type
	else if (type >= 0x3c00)
	{
		int trigger = type & TriggerType;
		int kind    = (type & DoorKind) >> DoorKindShift;
		int delay   = (type & DoorDelay) >> DoorDelayShift;
		int speed   = (type & DoorSpeed) >> DoorSpeedShift;

		// Trigger
		type_string += triggers[trigger];
		if (type & DoorMonster)
			type_string += "M";

		type_string += " Door ";

		// Door kind
		switch (kind)
		{
		case 0:  type_string += fmt::format("Open Wait {} Close", delays[delay]); break;
		case 1:  type_string += "Open Stay"; break;
		case 2:  type_string += fmt::format("Close Wait {} Open", delays[delay]); break;
		case 3:  type_string += "Close Stay"; break;
		default: break;
		}

		// Door speed
		type_string += fmt::format(" {}", speeds[speed]);
	}

	// Locked Door type
	else if (type >= 0x3800)
	{
		int trigger = type & TriggerType;
		int key     = (type & LockedKey) >> LockedKeyShift;
		int num     = (type & LockedNKeys) >> LockedNKeysShift;
		int kind    = (type & LockedKind) >> LockedKindShift;
		int speed   = (type & DoorSpeed) >> DoorSpeedShift;

		// Trigger
		type_string += triggers[trigger];

		type_string += " Door ";

		// Lock
		type_string += fmt::format("{} ", locks[num * 8 + key]);

		// Door kind
		switch (kind)
		{
		case 0:  type_string += "Open Wait 4 Close"; break;
		case 1:  type_string += "Open Stay"; break;
		case 2:  type_string += "Close Wait 4 Open"; break;
		case 3:  type_string += "Close Stay"; break;
		default: break;
		}

		// Door speed
		type_string += fmt::format(" {}", speeds[speed]);
	}

	// Lift type
	else if (type >= 0x3400)
	{
		int trigger = type & TriggerType;
		int target  = (type & LiftTarget) >> LiftTargetShift;
		int delay   = (type & LiftDelay) >> LiftDelayShift;
		int speed   = (type & LiftSpeed) >> LiftSpeedShift;

		// Trigger
		type_string += triggers[trigger];
		if (type & LiftMonster)
			type_string += "M";

		type_string += " Lift ";

		// Target
		type_string += lift_targets[target];

		// Delay
		type_string += fmt::format(" Delay {} ", lift_delays[delay]);

		// Speed
		type_string += speeds[speed];
	}

	// Stairs type
	else if (type >= 0x3000)
	{
		int trigger   = type & TriggerType;
		int direction = (type & StairDirection) >> StairDirectionShift;
		int step      = (type & StairStep) >> StairStepShift;
		int speed     = (type & StairSpeed) >> StairSpeedShift;

		// Trigger
		type_string += triggers[trigger];
		if (type & StairMonster)
			type_string += "M";

		// Direction, step height, speed
		type_string += fmt::format(" Stairs {} {} {}", directions[direction], steps[step], speeds[speed]);

		// Ignore
		if (type & StairIgnore)
			type_string += " Ignore Tex";
	}

	// Crusher type
	else if (type >= 0x2F80)
	{
		int trigger = type & TriggerType;
		int speed   = (type & CrusherSpeed) >> CrusherSpeedShift;

		// Trigger
		type_string += triggers[trigger];
		if (type & CrusherMonster)
			type_string += "M";

		// Speed
		type_string += fmt::format(" Crusher {}", speeds[speed]);

		// Silent
		if (type & CrusherSilent)
			type_string += " Silent";
	}

	return type_string;
}

// ------------------------------------------------------------------------
// Puts line type properties from [type] into [props]
// ------------------------------------------------------------------------
SpecialType getLineTypeProperties(int type, int* props)
{
	if (!props)
		return SpecialType::None;

	// Trigger always first
	props[0] = type & TriggerType;

	// Floor
	if (type >= 0x6000)
	{
		props[1] = (type & FloorSpeed) >> FloorSpeedShift;
		props[2] = (type & FloorModel) >> FloorModelShift;
		props[3] = (type & FloorDirection) >> FloorDirectionShift;
		props[4] = (type & FloorTarget) >> FloorTargetShift;
		props[5] = (type & FloorChange) >> FloorChangeShift;
		props[6] = (type & FloorCrush) >> FloorCrushShift;

		return SpecialType::Floor;
	}

	// Ceiling
	else if (type >= 0x4000)
	{
		props[1] = (type & CeilingSpeed) >> CeilingSpeedShift;
		props[2] = (type & CeilingModel) >> CeilingModelShift;
		props[3] = (type & CeilingDirection) >> CeilingDirectionShift;
		props[4] = (type & CeilingTarget) >> CeilingTargetShift;
		props[5] = (type & CeilingChange) >> CeilingChangeShift;
		props[6] = (type & CeilingCrush) >> CeilingCrushShift;

		return SpecialType::Ceiling;
	}

	// Door
	else if (type >= 0x3c00)
	{
		props[1] = (type & DoorSpeed) >> DoorSpeedShift;
		props[2] = (type & DoorKind) >> DoorKindShift;
		props[3] = (type & DoorMonster) >> DoorMonsterShift;
		props[4] = (type & DoorDelay) >> DoorDelayShift;

		return SpecialType::Door;
	}

	// Locked Door
	else if (type >= 0x3800)
	{
		props[1] = (type & LockedSpeed) >> LockedSpeedShift;
		props[2] = (type & LockedKind) >> LockedKindShift;
		props[3] = (type & LockedKey) >> LockedKeyShift;
		props[4] = (type & LockedNKeys) >> LockedNKeysShift;

		return SpecialType::LockedDoor;
	}

	// Lift
	else if (type >= 0x3400)
	{
		props[1] = (type & LiftSpeed) >> LiftSpeedShift;
		props[2] = (type & LiftMonster) >> LiftMonsterShift;
		props[3] = (type & LiftDelay) >> LiftDelayShift;
		props[4] = (type & LiftTarget) >> LiftTargetShift;

		return SpecialType::Lift;
	}

	// Stairs
	else if (type >= 0x3000)
	{
		props[1] = (type & StairSpeed) >> StairSpeedShift;
		props[2] = (type & StairMonster) >> StairMonsterShift;
		props[3] = (type & StairStep) >> StairStepShift;
		props[4] = (type & StairDirection) >> StairDirectionShift;
		props[5] = (type & StairIgnore) >> StairIgnoreShift;

		return SpecialType::Stairs;
	}

	// Crusher
	else if (type >= 0x2F80)
	{
		props[1] = (type & CrusherSpeed) >> CrusherSpeedShift;
		props[2] = (type & CrusherMonster) >> CrusherMonsterShift;
		props[3] = (type & CrusherSilent) >> CrusherSilentShift;

		return SpecialType::Crusher;
	}

	return SpecialType::None;
}

// ------------------------------------------------------------------------
// Returns a generalised special value from base type [type] and
// generalised properties [props]
// ------------------------------------------------------------------------
int generateSpecial(SpecialType type, const int* props)
{
	int special = 0;

	// Floor
	if (type == SpecialType::Floor)
	{
		special = GenFloorBase;
		special += (props[0] << TriggerTypeShift);
		special += (props[1] << FloorSpeedShift);
		special += (props[2] << FloorModelShift);
		special += (props[3] << FloorDirectionShift);
		special += (props[4] << FloorTargetShift);
		special += (props[5] << FloorChangeShift);
		special += (props[6] << FloorCrushShift);
	}

	// Ceiling
	else if (type == SpecialType::Ceiling)
	{
		special = GenCeilingBase;
		special += (props[0] << TriggerTypeShift);
		special += (props[1] << CeilingSpeedShift);
		special += (props[2] << CeilingModelShift);
		special += (props[3] << CeilingDirectionShift);
		special += (props[4] << CeilingTargetShift);
		special += (props[5] << CeilingChangeShift);
		special += (props[6] << CeilingCrushShift);
	}

	// Door
	else if (type == SpecialType::Door)
	{
		special = GenDoorBase;
		special += (props[0] << TriggerTypeShift);
		special += (props[1] << DoorSpeedShift);
		special += (props[2] << DoorKindShift);
		special += (props[3] << DoorMonsterShift);
		special += (props[4] << DoorDelayShift);
	}

	// Locked Door
	else if (type == SpecialType::LockedDoor)
	{
		special = GenLockedBase;
		special += (props[0] << TriggerTypeShift);
		special += (props[1] << LockedSpeedShift);
		special += (props[2] << LockedKindShift);
		special += (props[3] << LockedKeyShift);
		special += (props[4] << LockedNKeysShift);
	}

	// Lift
	else if (type == SpecialType::Lift)
	{
		special = GenLiftBase;
		special += (props[0] << TriggerTypeShift);
		special += (props[1] << LiftSpeedShift);
		special += (props[2] << LiftMonsterShift);
		special += (props[3] << LiftDelayShift);
		special += (props[4] << LiftTargetShift);
	}

	// Stairs
	else if (type == SpecialType::Stairs)
	{
		special = GenStairsBase;
		special += (props[0] << TriggerTypeShift);
		special += (props[1] << StairSpeedShift);
		special += (props[2] << StairMonsterShift);
		special += (props[3] << StairStepShift);
		special += (props[4] << StairDirectionShift);
		special += (props[5] << StairIgnoreShift);
	}

	// Crusher
	else if (type == SpecialType::Crusher)
	{
		special = GenCrusherBase;
		special += (props[0] << TriggerTypeShift);
		special += (props[1] << CrusherSpeedShift);
		special += (props[2] << CrusherMonsterShift);
		special += (props[3] << CrusherSilentShift);
	}

	return special;
}
} // namespace slade::genlinespecial
