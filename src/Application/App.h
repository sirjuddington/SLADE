#pragma once

class Console;
class PaletteManager;

namespace App
{
	bool			isInitialised();
	Console*		console();
	PaletteManager*	paletteManager();
	long			runTimer();

	bool	init();
	void	saveConfigFile();
	void	exit(bool save_config);

	// Path related stuff
	enum class Dir { User, Data, Executable, Resources, Temp };
	string path(string filename, Dir dir);
}
