#pragma once

class Console;

namespace App
{
	bool		isInitialised();
	Console*	console();
	long		runTimer();

	bool	init();
	void	saveConfigFile();
	void	exit(bool save_config);

	// Path related stuff
	enum class Dir { User, Data, Executable, Resources, Temp };
	string path(string filename, Dir dir);
}
