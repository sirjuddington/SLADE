
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "GenLineSpecial.h"


// ----------------------------------------------------------------------------
//
// Defines
//
// ----------------------------------------------------------------------------
#define GenFloorBase			0x6000
#define GenCeilingBase			0x4000
#define GenDoorBase				0x3c00
#define GenLockedBase			0x3800
#define GenLiftBase				0x3400
#define GenStairsBase			0x3000
#define GenCrusherBase			0x2F80

#define TriggerType				0x0007
#define TriggerTypeShift		0

#define FloorCrush				0x1000
#define FloorChange				0x0c00
#define FloorTarget				0x0380
#define FloorDirection			0x0040
#define FloorModel				0x0020
#define FloorSpeed				0x0018

#define FloorCrushShift			12
#define FloorChangeShift		10
#define FloorTargetShift		7
#define FloorDirectionShift		6
#define FloorModelShift			5
#define FloorSpeedShift			3

#define CeilingCrush			0x1000
#define CeilingChange			0x0c00
#define CeilingTarget			0x0380
#define CeilingDirection		0x0040
#define CeilingModel			0x0020
#define CeilingSpeed			0x0018

#define CeilingCrushShift		12
#define CeilingChangeShift		10
#define CeilingTargetShift		7
#define CeilingDirectionShift	6
#define CeilingModelShift		5
#define CeilingSpeedShift		3

#define LiftTarget				0x0300
#define LiftDelay				0x00c0
#define LiftMonster				0x0020
#define LiftSpeed				0x0018

#define LiftTargetShift			8
#define LiftDelayShift			6
#define LiftMonsterShift		5
#define LiftSpeedShift			3

#define StairIgnore				0x0200
#define StairDirection			0x0100
#define StairStep				0x00c0
#define StairMonster			0x0020
#define StairSpeed				0x0018

#define StairIgnoreShift		9
#define StairDirectionShift		8
#define StairStepShift			6
#define StairMonsterShift		5
#define StairSpeedShift			3

#define CrusherSilent			0x0040
#define CrusherMonster			0x0020
#define CrusherSpeed			0x0018

#define CrusherSilentShift		6
#define CrusherMonsterShift		5
#define CrusherSpeedShift		3

#define DoorDelay				0x0300
#define DoorMonster				0x0080
#define DoorKind				0x0060
#define DoorSpeed				0x0018

#define DoorDelayShift			8
#define DoorMonsterShift		7
#define DoorKindShift			5
#define DoorSpeedShift			3

#define LockedNKeys				0x0200
#define LockedKey				0x01c0
#define LockedKind				0x0020
#define LockedSpeed				0x0018

#define LockedNKeysShift		9
#define LockedKeyShift			6
#define LockedKindShift			5
#define LockedSpeedShift		3

// ----------------------------------------------------------------------------
//
// BoomGenLineSpecial Namespace
//
// ----------------------------------------------------------------------------
namespace BoomGenLineSpecial
{
	static const char* Triggers[]=
	{"W1","WR","S1","SR","G1","GR","D1","DR",};

	static const char* FloorTargets[]=
	{"to Highest N Floor","to Lowest N Floor","to Next N Floor","to Lowest N Ceiling","to Ceiling","by Lower Tex","24 Units","32 Units",};

	static const char* Directions[]=
	{"Down","Up",};

	static const char* Speeds[]=
	{"Slow","Normal","Fast","Turbo",};

	static const char* Changers[]=
	{"","Zero Type/Copy Tex","Copy Tex","Copy Type/Copy Tex",};

	static const char* Models[]=
	{"Trigger","Numeric",};

	static const char* Crushers[]=
	{"","Cr",};

	static const char* CeilingTargets[]=
	{"to Highest N Ceiling","to Lowest N Ceiling","to Next N Ceiling","to Lowest N Floor","to Floor","by Upper Tex","24 Units","32 Units",};

	static const char* Doors1[]=
	{"OpnD","Opn","ClsD","Cls",};

	static const char* Doors2[]=
	{"Cls","","Opn","",};

	static const char* Delays[]=
	{"1","4","9","30",};

	static const char* LockedDelays[]=
	{"4",};

	static const char* Locks[]=
	{
		"Any Key","Red Card","Blue Card","Yellow Card","Red Skull","Blue Skull","Yellow Skull","All 6 Keys",
		"Any Key","Red Key","Blue Key","Yellow Key","Red Key","Blue Key","Yellow Key","All 3 Keys",
	};

	static const char* LiftTargets[]=
	{"to Lowest N Floor","to Next N Floor","to Lowest N Ceiling","Perpetual",};

	static const char* LiftDelays[]=
	{"1","3","5","10",};

	static const char* Steps[]=
	{"4","8","16","24",};

	// ------------------------------------------------------------------------
	// parseLineType
	//
	// Returns a string representation of the generalised line value [type]
	// ------------------------------------------------------------------------
	string parseLineType(int type)
	{
		string type_string;

		// Floor type
		if (type>=0x6000)
		{
			int trigger = type & TriggerType;
			int speed = (type & FloorSpeed) >> FloorSpeedShift;
			int direction = (type & FloorDirection) >> FloorDirectionShift;
			int target = (type & FloorTarget) >> FloorTargetShift;
			int change = (type & FloorChange) >> FloorChangeShift;
			int model = (type & FloorModel) >> FloorModelShift;

			// Trigger
			type_string += Triggers[trigger];
			if (change == 0 && model == 1)
				type_string += "M";

			type_string += " Floor ";

			// Direction, target, speed
			type_string += S_FMT("%s %s %s", Directions[direction], FloorTargets[target], Speeds[speed]);

			// Change
			if (type & FloorChange)
				type_string += S_FMT(" %s (%s)", Changers[change], Models[model]);

			// Crush
			if (type & FloorCrush)
				type_string += " Crushing";
		}

		// Ceiling type
		else if (type>=0x4000)
		{
			int trigger = type & TriggerType;
			int speed = (type & CeilingSpeed) >> CeilingSpeedShift;
			int direction = (type & CeilingDirection) >> CeilingDirectionShift;
			int target = (type & CeilingTarget) >> CeilingTargetShift;
			int change = (type & CeilingChange) >> CeilingChangeShift;
			int model = (type & CeilingModel) >> CeilingModelShift;

			// Trigger
			type_string += Triggers[trigger];
			if (change == 0 && model == 1)
				type_string += "M";

			type_string += " Ceiling ";

			// Direction, target, speed
			type_string += S_FMT("%s %s %s", Directions[direction], CeilingTargets[target], Speeds[speed]);

			// Change
			if (type & CeilingChange)
				type_string += S_FMT(" %s (%s)", Changers[change], Models[model]);

			// Crush
			if (type & CeilingCrush)
				type_string += " Crushing";
		}

		// Door type
		else if (type>=0x3c00)
		{
			int trigger = type & TriggerType;
			int kind = (type & DoorKind) >> DoorKindShift;
			int delay = (type & DoorDelay) >> DoorDelayShift;
			int speed = (type & DoorSpeed) >> DoorSpeedShift;

			// Trigger
			type_string += Triggers[trigger];
			if (type & DoorMonster)
				type_string += "M";

			type_string += " Door ";

			// Door kind
			switch (kind)
			{
			case 0: type_string += S_FMT("Open Wait %s Close", Delays[delay]); break;
			case 1: type_string += "Open Stay"; break;
			case 2: type_string += S_FMT("Close Wait %s Open", Delays[delay]); break;
			case 3: type_string += "Close Stay"; break;
			default: break;
			}

			// Door speed
			type_string += S_FMT(" %s", Speeds[speed]);
		}

		// Locked Door type
		else if (type>=0x3800)
		{
			int trigger = type & TriggerType;
			int key = (type & LockedKey) >> LockedKeyShift;
			int num = (type & LockedNKeys) >> LockedNKeysShift;
			int kind = (type & LockedKind) >> LockedKindShift;
			int speed = (type & DoorSpeed) >> DoorSpeedShift;

			// Trigger
			type_string += Triggers[trigger];

			type_string += " Door ";

			// Lock
			type_string += S_FMT("%s ", Locks[num*8+key]);

			// Door kind
			switch (kind)
			{
			case 0: type_string += "Open Wait 4 Close"; break;
			case 1: type_string += "Open Stay"; break;
			case 2: type_string += "Close Wait 4 Open"; break;
			case 3: type_string += "Close Stay"; break;
			default: break;
			}

			// Door speed
			type_string += S_FMT(" %s", Speeds[speed]);
		}

		// Lift type
		else if (type>=0x3400)
		{
			int trigger = type & TriggerType;
			int target = (type & LiftTarget) >> LiftTargetShift;
			int delay = (type & LiftDelay) >> LiftDelayShift;
			int speed = (type & LiftSpeed) >> LiftSpeedShift;

			// Trigger
			type_string += Triggers[trigger];
			if (type & LiftMonster)
				type_string += "M";

			type_string += " Lift ";

			// Target
			type_string += LiftTargets[target];

			// Delay
			type_string += S_FMT(" Delay %s ", LiftDelays[delay]);

			// Speed
			type_string += Speeds[speed];
		}

		// Stairs type
		else if (type>=0x3000)
		{
			int trigger = type & TriggerType;
			int direction = (type & StairDirection) >> StairDirectionShift;
			int step = (type & StairStep) >> StairStepShift;
			int speed = (type & StairSpeed) >> StairSpeedShift;

			// Trigger
			type_string += Triggers[trigger];
			if (type & StairMonster)
				type_string += "M";

			// Direction, step height, speed
			type_string += S_FMT(" Stairs %s %s %s", Directions[direction], Steps[step], Speeds[speed]);

			// Ignore
			if (type & StairIgnore)
				type_string += " Ignore Tex";
		}

		// Crusher type
		else if (type>=0x2F80)
		{
			int trigger = type & TriggerType;
			int speed = (type & CrusherSpeed) >> CrusherSpeedShift;

			// Trigger
			type_string += Triggers[trigger];
			if (type & CrusherMonster)
				type_string += "M";

			// Speed
			type_string += S_FMT(" Crusher %s", Speeds[speed]);

			// Silent
			if (type & CrusherSilent)
				type_string += " Silent";
		}

		return type_string;
	}

	// ------------------------------------------------------------------------
	// getLineTypeProperties
	//
	// Puts line type properties from [type] into [props]
	// ------------------------------------------------------------------------
	int getLineTypeProperties(int type, int* props)
	{
		if (!props)
			return -1;

		// Trigger always first
		props[0] = type & TriggerType;

		// Floor
		if (type>=0x6000)
		{
			props[1] = (type & FloorSpeed) >> FloorSpeedShift;
			props[2] = (type & FloorModel) >> FloorModelShift;
			props[3] = (type & FloorDirection) >> FloorDirectionShift;
			props[4] = (type & FloorTarget) >> FloorTargetShift;
			props[5] = (type & FloorChange) >> FloorChangeShift;
			props[6] = (type & FloorCrush) >> FloorCrushShift;

			return GS_FLOOR;
		}

		// Ceiling
		else if (type>=0x4000)
		{
			props[1] = (type & CeilingSpeed) >> CeilingSpeedShift;
			props[2] = (type & CeilingModel) >> CeilingModelShift;
			props[3] = (type & CeilingDirection) >> CeilingDirectionShift;
			props[4] = (type & CeilingTarget) >> CeilingTargetShift;
			props[5] = (type & CeilingChange) >> CeilingChangeShift;
			props[6] = (type & CeilingCrush) >> CeilingCrushShift;

			return GS_CEILING;
		}

		// Door
		else if (type>=0x3c00)
		{
			props[1] = (type & DoorSpeed) >> DoorSpeedShift;
			props[2] = (type & DoorKind) >> DoorKindShift;
			props[3] = (type & DoorMonster) >> DoorMonsterShift;
			props[4] = (type & DoorDelay) >> DoorDelayShift;

			return GS_DOOR;
		}

		// Locked Door
		else if (type>=0x3800)
		{
			props[1] = (type & LockedSpeed) >> LockedSpeedShift;
			props[2] = (type & LockedKind) >> LockedKindShift;
			props[3] = (type & LockedKey) >> LockedKeyShift;
			props[4] = (type & LockedNKeys) >> LockedNKeysShift;

			return GS_LOCKED_DOOR;
		}

		// Lift
		else if (type>=0x3400)
		{
			props[1] = (type & LiftSpeed) >> LiftSpeedShift;
			props[2] = (type & LiftMonster) >> LiftMonsterShift;
			props[3] = (type & LiftDelay) >> LiftDelayShift;
			props[4] = (type & LiftTarget) >> LiftTargetShift;

			return GS_LIFT;
		}

		// Stairs
		else if (type>=0x3000)
		{
			props[1] = (type & StairSpeed) >> StairSpeedShift;
			props[2] = (type & StairMonster) >> StairMonsterShift;
			props[3] = (type & StairStep) >> StairStepShift;
			props[4] = (type & StairDirection) >> StairDirectionShift;
			props[5] = (type & StairIgnore) >> StairIgnoreShift;

			return GS_STAIRS;
		}

		// Crusher
		else if (type>=0x2F80)
		{
			props[1] = (type & CrusherSpeed) >> CrusherSpeedShift;
			props[2] = (type & CrusherMonster) >> CrusherMonsterShift;
			props[3] = (type & CrusherSilent) >> CrusherSilentShift;

			return GS_CRUSHER;
		}

		return -1;
	}

	// ------------------------------------------------------------------------
	// generateSpecial
	//
	// Returns a generalised special value from base type [type] and
	// generalised properties [props]
	// ------------------------------------------------------------------------
	int generateSpecial(int type, int* props)
	{
		int special = 0;

		// Floor
		if (type == GS_FLOOR)
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
		else if (type == GS_CEILING)
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
		else if (type == GS_DOOR)
		{
			special = GenDoorBase;
			special += (props[0] << TriggerTypeShift);
			special += (props[1] << DoorSpeedShift);
			special += (props[2] << DoorKindShift);
			special += (props[3] << DoorMonsterShift);
			special += (props[4] << DoorDelayShift);
		}

		// Locked Door
		else if (type == GS_LOCKED_DOOR)
		{
			special = GenLockedBase;
			special += (props[0] << TriggerTypeShift);
			special += (props[1] << LockedSpeedShift);
			special += (props[2] << LockedKindShift);
			special += (props[3] << LockedKeyShift);
			special += (props[4] << LockedNKeysShift);
		}

		// Lift
		else if (type == GS_LIFT)
		{
			special = GenLiftBase;
			special += (props[0] << TriggerTypeShift);
			special += (props[1] << LiftSpeedShift);
			special += (props[2] << LiftMonsterShift);
			special += (props[3] << LiftDelayShift);
			special += (props[4] << LiftTargetShift);
		}

		// Stairs
		else if (type == GS_STAIRS)
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
		else if (type == GS_CRUSHER)
		{
			special = GenCrusherBase;
			special += (props[0] << TriggerTypeShift);
			special += (props[1] << CrusherSpeedShift);
			special += (props[2] << CrusherMonsterShift);
			special += (props[3] << CrusherSilentShift);
		}

		return special;
	}
}
