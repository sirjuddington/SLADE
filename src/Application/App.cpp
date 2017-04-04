
#include "Main.h"
#include "App.h"

namespace App
{
	wxStopWatch	timer;
	int			temp_fail_count = 0;

	// Directory paths
	string	dir_data = "";
	string	dir_user = "";
	string	dir_app = "";
	string	dir_res = "";
#ifdef WIN32
	string	dir_separator = "\\";
#else
	string	dir_separator = "/";
#endif
}

CVAR(Int, temp_location, 0, CVAR_SAVE)
CVAR(String, temp_location_custom, "", CVAR_SAVE)

namespace App
{
	/* initDirectories
	 * Checks for and creates necessary application directories. Returns
	 * true if all directories existed and were created successfully if
	 * needed, false otherwise
	 *******************************************************************/
	bool initDirectories()
	{
		// If we're passed in a INSTALL_PREFIX (from CMAKE_INSTALL_PREFIX),
		// use this for the installation prefix
#if defined(__WXGTK__) && defined(INSTALL_PREFIX)
		wxStandardPaths::Get().SetInstallPrefix(INSTALL_PREFIX);
#endif//defined(__UNIX__) && defined(INSTALL_PREFIX)

		// Setup app dir
		dir_app = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();

		// Check for portable install
		if (wxFileExists(path("portable", Dir::Executable)))
		{
			// Setup portable user/data dirs
			dir_data = dir_app;
			dir_res = dir_app;
			dir_user = dir_app + dir_separator + "config";
		}
		else
		{
			// Setup standard user/data dirs
			dir_user = wxStandardPaths::Get().GetUserDataDir();
			dir_data = wxStandardPaths::Get().GetDataDir();
			dir_res = wxStandardPaths::Get().GetResourcesDir();
		}

		// Create user dir if necessary
		if (!wxDirExists(dir_user))
		{
			if (!wxMkdir(dir_user))
			{
				wxMessageBox(S_FMT("Unable to create user directory \"%s\"", dir_user), "Error", wxICON_ERROR);
				return false;
			}
		}

		// Check data dir
		if (!wxDirExists(dir_data))
			dir_data = dir_app;	// Use app dir if data dir doesn't exist

		// Check res dir
		if (!wxDirExists(dir_res))
			dir_res = dir_app;	// Use app dir if res dir doesn't exist

		return true;
	}
}

bool App::init()
{
	if (!initDirectories())
		return false;

	return true;
}

long App::runTimer()
{
	return timer.Time();
}

/* App::path
 * Prepends an application-related path to a filename,
 * App::Dir::Data: SLADE application data directory (for SLADE.pk3)
 * App::Dir::User: User configuration and resources directory
 * App::Dir::Executable: Directory of the SLADE executable
 * App::Dir::Temp: Temporary files directory
 *******************************************************************/
string App::path(string filename, Dir dir)
{
	if (dir == Dir::Data)
		return dir_data + dir_separator + filename;
	if (dir == Dir::User)
		return dir_user + dir_separator + filename;
	if (dir == Dir::Executable)
		return dir_app + dir_separator + filename;
	if (dir == Dir::Resources)
		return dir_res + dir_separator + filename;
	if (dir == Dir::Temp)
	{
		// Get temp path
		string dir_temp;
		if (temp_location == 0)
			dir_temp = wxStandardPaths::Get().GetTempDir().Append(dir_separator).Append("SLADE3");
		else if (temp_location == 1)
			dir_temp = dir_app + dir_separator + "temp";
		else
			dir_temp = temp_location_custom;

		// Create folder if necessary
		if (!wxDirExists(dir_temp) && temp_fail_count < 2)
		{
			if (!wxMkdir(dir_temp))
			{
				LOG_MESSAGE(1, "Unable to create temp directory \"%s\"", dir_temp);
				temp_fail_count++;
				return path(filename, dir);
			}
		}

		return dir_temp + dir_separator + filename;
	}

	return filename;
}
