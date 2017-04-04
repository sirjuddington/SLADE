#pragma once

class SAction;

namespace App
{
	bool	init();
	long	runTimer();

	// Path related stuff
	enum class Dir { User, Data, Executable, Resources, Temp };
	string path(string filename, Dir dir);
}

// TODO: Remove when split done
#include "SLADEWxApp.h"
