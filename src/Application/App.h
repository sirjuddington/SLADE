#pragma once

#include <thread>

class Console;
class PaletteManager;
class ArchiveManager;
class Clipboard;
class ResourceManager;

namespace App
{
bool             isInitialised();
Console*         console();
PaletteManager*  paletteManager();
long             runTimer();
bool             isExiting();
ArchiveManager&  archiveManager();
Clipboard&       clipboard();
ResourceManager& resources();

bool init(vector<wxString>& args, double ui_scale = 1.);
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

	int      cmp(const Version& rhs) const;
	wxString toString() const;
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
wxString path(const wxString& filename, Dir dir);

// Platform and build options
enum Platform
{
	Windows,
	Linux,
	MacOS,
	Unknown
};
Platform        platform();
bool            useWebView();
bool            useSFMLRenderWindow();
const wxString& iconFile();

std::thread::id mainThreadId();
} // namespace App
