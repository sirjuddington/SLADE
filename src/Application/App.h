#pragma once

#include <thread>

class Console;
class PaletteManager;
class ArchiveManager;

namespace App
{
bool            isInitialised();
Console*        console();
PaletteManager* paletteManager();
long            runTimer();
bool            isExiting();
ArchiveManager& archiveManager();

bool init(vector<string>& args, double ui_scale = 1.);
void saveConfigFile();
void exit(bool save_config);

// Path related stuff
enum class Dir
{
	User,
	Data,
	Executable,
	Resources,
	Temp
};
string path(string filename, Dir dir);

// Platform and build options
enum Platform
{
	Windows,
	Linux,
	MacOS,
	Unknown
};
Platform     platform();
bool         useWebView();
bool         useSFMLRenderWindow();
const string getIcon();

std::thread::id mainThreadId();
} // namespace App
