#pragma once

namespace BoomGenLineSpecial
{
	enum
	{
		// Special types
		GS_FLOOR = 0,
		GS_CEILING,
		GS_DOOR,
		GS_LOCKED_DOOR,
		GS_LIFT,
		GS_STAIRS,
		GS_CRUSHER
	};

	string	parseLineType(int type);
	int		getLineTypeProperties(int type, int* props);
	int		generateSpecial(int type, int* props);
}
