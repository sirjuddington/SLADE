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

// Version
struct Version
{
	unsigned long major    = 0;
	unsigned long minor    = 0;
	unsigned long revision = 0;
	unsigned long beta     = 0;

	Version(unsigned long major = 0, unsigned long minor = 0, unsigned long revision = 0, unsigned long beta = 0) :
		major{ major },
		minor{ minor },
		revision{ revision },
		beta{ beta }
	{
	}

	int cmp(const Version& rhs) const;
	string toString() const;
};
const Version& version();

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
