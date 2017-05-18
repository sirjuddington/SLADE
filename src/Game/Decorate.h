#pragma once

#include "ThingType.h"

class Archive;

namespace Game
{
	// This is used to have the same priority order as DB2
	// Idle, See, Inactive, Spawn, first defined
	enum StateSprites
	{
		FirstDefined = 1,
		Spawn,
		Inactive,
		See,
		Idle,
	};

	bool readDecorateDefs(Archive* archive, std::map<int, ThingType>& types);
}
