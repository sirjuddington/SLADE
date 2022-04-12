#pragma once

namespace slade::genlinespecial
{
enum SpecialType
{
	// Special types
	None  = -1,
	Floor = 0,
	Ceiling,
	Door,
	LockedDoor,
	Lift,
	Stairs,
	Crusher
};

string      parseLineType(int type);
SpecialType getLineTypeProperties(int type, int* props);
int         generateSpecial(SpecialType type, const int* props);

} // namespace slade::genlinespecial
