
#ifndef __ACTION_SPECIAL_H__
#define __ACTION_SPECIAL_H__

#include "Args.h"

// Tag types
enum
{
	AS_TT_NO = 0,
	AS_TT_SECTOR,
	AS_TT_LINE,
	AS_TT_THING,
	AS_TT_SECTOR_BACK,
	AS_TT_SECTOR_OR_BACK,
	AS_TT_SECTOR_AND_BACK,

	// Special handling for that one
	AS_TT_LINEID,
	AS_TT_LINEID_HI5,

	// Some more specific types
	AS_TT_1THING_2SECTOR,					// most ZDoom teleporters work like this
	AS_TT_1THING_3SECTOR,					// Teleport_NoFog & Thing_Destroy
	AS_TT_1THING_2THING,					// TeleportOther, NoiseAlert, Thing_Move, Thing_SetGoal
	AS_TT_1THING_4THING,					// Thing_ProjectileIntercept, Thing_ProjectileAimed
	AS_TT_1THING_2THING_3THING,				// TeleportGroup
	AS_TT_1SECTOR_2THING_3THING_5THING,		// TeleportInSector
	AS_TT_1LINEID_2LINE,					// Teleport_Line
	AS_TT_LINE_NEGATIVE,					// Scroll_Texture_Both
	AS_TT_4THING,							// ThrustThing
	AS_TT_5THING,							// Radius_Quake
	AS_TT_1LINE_2SECTOR,					// Sector_Attach3dMidtex
	AS_TT_1SECTOR_2SECTOR,					// Sector_SetLink
	AS_TT_1SECTOR_2SECTOR_3SECTOR_4SECTOR,	// Plane_Copy
	AS_TT_SECTOR_2IS3_LINE,					// Static_Init
	AS_TT_1SECTOR_2THING,					// PointPush_SetForce
};

class ParseTreeNode;
class ActionSpecial
{
	friend class GameConfiguration;
private:
	string	name;
	string	group;
	int		tagged;
	arg_t	args[5];

public:
	ActionSpecial(string name = "Unknown", string group = "");
	~ActionSpecial() {}

	void	copy(ActionSpecial* copy);

	string	getName() { return name; }
	string	getGroup() { return group; }
	int		needsTag() { return tagged; }
	arg_t&	getArg(int index) { if (index >= 0 && index < 5) return args[index]; else return args[0]; }

	void	setName(string name) { this->name = name; }
	void	setGroup(string group) { this->group = group; }
	void	setTagged(int tagged) { this->tagged = tagged; }

	string	getArgsString(int args[5]);

	void	reset();
	void	parse(ParseTreeNode* node);
	string	stringDesc();
};

#endif//__ACTION_SPECIAL_H__
