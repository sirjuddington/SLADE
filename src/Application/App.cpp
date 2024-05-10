
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    App.cpp
// Description: The app namespace, with various general application related
//              functions
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryDataFormat.h"
#include "Archive/EntryType/EntryType.h"
#include "Database/Database.h"
#include "Game/Game.h"
#include "Game/SpecialPreset.h"
#include "General/Clipboard.h"
#include "General/ColourConfiguration.h"
#include "General/Console.h"
#include "General/Executables.h"
#include "General/KeyBind.h"
#include "General/ResourceManager.h"
#include "General/SAction.h"
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "Graphics/Palette/PaletteManager.h"
#include "Graphics/SImage/SIFormat.h"
#include "Library/Library.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/NodeBuilders.h"
#include "OpenGL/GLTexture.h"
#include "SLADEWxApp.h"
#include "Scripting/Lua.h"
#include "Scripting/ScriptManager.h"
#include "TextEditor/TextLanguage.h"
#include "TextEditor/TextStyle.h"
#include "UI/Dialogs/SetupWizard/SetupWizardDialog.h"
#include "UI/SBrush.h"
#include "UI/State.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include <FreeImage.h>
#include <dumb.h>
#include <filesystem>
#ifdef __WXOSX__
#include <ApplicationServices/ApplicationServices.h>
#endif

#ifndef _WIN32
#undef _WINDOWS_ // Undefine _WINDOWS_ that has been defined by FreeImage
#endif

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::app
{
wxStopWatch     timer;
int             temp_fail_count = 0;
bool            init_ok         = false;
bool            exiting         = false;
std::thread::id main_thread_id;

// Version
Version version_num{ 3, 3, 0, 1000 };

// Directory paths
string dir_data;
string dir_user;
string dir_app;
string dir_res;
string dir_temp;
#ifdef WIN32
string dir_separator = "\\";
#else
string dir_separator = "/";
#endif

// App objects (managers, etc.)
Console         console_main;
PaletteManager  palette_manager;
ArchiveManager  archive_manager;
Clipboard       clip_board;
ResourceManager resource_manager;
} // namespace slade::app

CVAR(Int, temp_location, 0, CVar::Flag::Save)
CVAR(String, temp_location_custom, "", CVar::Flag::Save)


// ----------------------------------------------------------------------------
//
// app::Version Struct Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Compares with another version [rhs].
// Returns 0 if equal, 1 if this is newer or -1 if this is older
// ----------------------------------------------------------------------------
int app::Version::cmp(const Version& rhs) const
{
	if (major == rhs.major)
	{
		if (minor == rhs.minor)
		{
			if (revision == rhs.revision)
			{
				if (beta == rhs.beta)
					return 0;
				if (beta == 0 && rhs.beta > 0)
					return 1;
				if (beta > 0 && rhs.beta == 0)
					return -1;

				return beta > rhs.beta ? 1 : -1;
			}

			return revision > rhs.revision ? 1 : -1;
		}

		return minor > rhs.minor ? 1 : -1;
	}

	return major > rhs.major ? 1 : -1;
}

// ----------------------------------------------------------------------------
// Returns a string representation of the version (eg. "3.2.1 beta 4")
// ----------------------------------------------------------------------------
string app::Version::toString() const
{
	auto vers = fmt::format("{}.{}.{}", major, minor, revision);
	if (beta > 999)
		vers += " alpha";
	else if (beta > 0)
		vers += fmt::format(" beta {}", beta);
	return vers;
}


// ----------------------------------------------------------------------------
//
// App Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::app
{
// -----------------------------------------------------------------------------
// Checks for and creates necessary application directories. Returns true
// if all directories existed and were created successfully if needed,
// false otherwise
// -----------------------------------------------------------------------------
bool initDirectories()
{
	// If we're passed in a INSTALL_PREFIX (from CMAKE_INSTALL_PREFIX),
	// use this for the installation prefix
#if defined(__WXGTK__) && defined(INSTALL_PREFIX)
	wxStandardPaths::Get().SetInstallPrefix(INSTALL_PREFIX);
#endif // defined(__UNIX__) && defined(INSTALL_PREFIX)

	// Setup app dir
	dir_app = strutil::Path::pathOf(wxStandardPaths::Get().GetExecutablePath().ToStdString(), false);

	// Check for portable install
	if (wxFileExists(path("portable", Dir::Executable)))
	{
		// Setup portable user/data dirs
		dir_data = dir_app;
		dir_res  = dir_app;
		dir_user = dir_app + dir_separator + "config";
	}
	else
	{
		// Setup standard user/data dirs
		dir_user = wxStandardPaths::Get().GetUserDataDir();
		dir_data = wxStandardPaths::Get().GetDataDir();
		dir_res  = wxStandardPaths::Get().GetResourcesDir();
	}

	// Create user dir if necessary
	if (!wxDirExists(dir_user))
	{
		if (!wxMkdir(dir_user))
		{
			global::error = fmt::format("Unable to create user directory \"{}\"", dir_user);
			return false;
		}
	}

	// Create temp dir if necessary
	dir_temp = dir_user + dir_separator + "temp";
	if (!wxDirExists(dir_temp))
	{
		if (!wxMkdir(dir_temp))
		{
			global::error = fmt::format("Unable to create temp directory \"{}\"", dir_temp);
			return false;
		}
	}

	// Check data dir
	if (!wxDirExists(dir_data))
		dir_data = dir_app; // Use app dir if data dir doesn't exist

	// Check res dir
	if (!wxDirExists(dir_res))
		dir_res = dir_app; // Use app dir if res dir doesn't exist

	return true;
}

// -----------------------------------------------------------------------------
// Reads and parses the SLADE configuration file
// -----------------------------------------------------------------------------
void readConfigFile()
{
	// Open SLADE.cfg
	Tokenizer tz;
	if (!tz.openFile(path("slade3.cfg", Dir::User)))
		return;

	// Go through the file with the tokenizer
	while (!tz.atEnd())
	{
		// If we come across a 'cvars' token, read in the cvars section
		if (tz.advIf("cvars", 2))
		{
			// Keep reading name/value pairs until we hit the ending '}'
			while (!tz.checkOrEnd("}"))
			{
				if (tz.peek().quoted_string)
				{
					// String CVar values are written in UTF8
					auto val = wxString::FromUTF8(tz.peek().text.c_str());
					CVar::set(tz.current().text, val.ToStdString());
				}
				else
					CVar::set(tz.current().text, tz.peek().text);

				tz.adv(2);
			}

			tz.adv(); // Skip ending }
		}

		// Read base resource archive paths
		if (tz.advIf("base_resource_paths", 2))
		{
			while (!tz.checkOrEnd("}"))
			{
				auto path = wxString::FromUTF8(tz.current().text.c_str());
				archive_manager.addBaseResourcePath(wxutil::strToView(path));
				tz.adv();
			}

			tz.adv(); // Skip ending }
		}

		// Read (pre-3.3.0) recent files list
		if (tz.advIf("recent_files", 2))
			library::readPre330RecentFiles(tz);

		// Read keybinds
		if (tz.advIf("keys", 2))
			KeyBind::readBinds(tz);

		// Read nodebuilder paths
		if (tz.advIf("nodebuilder_paths", 2))
		{
			while (!tz.checkOrEnd("}"))
			{
				auto path = wxString::FromUTF8(tz.peek().text.c_str());
				nodebuilders::addBuilderPath(tz.current().text, wxutil::strToView(path));
				tz.adv(2);
			}

			tz.adv(); // Skip ending }
		}

		// Read game exe paths
		if (tz.advIf("executable_paths", 2))
		{
			while (!tz.checkOrEnd("}"))
			{
				auto path = wxString::FromUTF8(tz.peek().text.c_str());
				executables::setGameExePath(tz.current().text, wxutil::strToView(path));
				tz.adv(2);
			}

			tz.adv(); // Skip ending }
		}

		// Next token
		tz.adv();
	}
}

// -----------------------------------------------------------------------------
// Processes command line [args]
// -----------------------------------------------------------------------------
vector<string> processCommandLine(const vector<string>& args)
{
	vector<string> to_open;

	// Process command line args (except the first as it is normally the executable name)
	for (auto& arg : args)
	{
		// -nosplash: Disable splash window
		if (strutil::equalCI(arg, "-nosplash"))
			ui::enableSplash(false);

		// -debug: Enable debug mode
		else if (strutil::equalCI(arg, "-debug"))
		{
			global::debug = true;
			log::info("Debugging stuff enabled");
		}

		// Other (no dash), open as archive
		else if (!strutil::startsWith(arg, '-'))
			to_open.push_back(arg);

		// Unknown parameter
		else
			log::warning("Unknown command line parameter: \"{}\"", arg);
	}

	return to_open;
}
} // namespace slade::app

// -----------------------------------------------------------------------------
// Returns true if the application has been initialised
// -----------------------------------------------------------------------------
bool app::isInitialised()
{
	return init_ok;
}

// -----------------------------------------------------------------------------
// Returns the global Console
// -----------------------------------------------------------------------------
Console* app::console()
{
	return &console_main;
}

// -----------------------------------------------------------------------------
// Returns the Palette Manager
// -----------------------------------------------------------------------------
PaletteManager* app::paletteManager()
{
	return &palette_manager;
}

// -----------------------------------------------------------------------------
// Returns the Archive Manager
// -----------------------------------------------------------------------------
ArchiveManager& app::archiveManager()
{
	return archive_manager;
}

// -----------------------------------------------------------------------------
// Returns the Clipboard
// -----------------------------------------------------------------------------
Clipboard& app::clipboard()
{
	return clip_board;
}

// -----------------------------------------------------------------------------
// Returns the Resource Manager
// -----------------------------------------------------------------------------
ResourceManager& app::resources()
{
	return resource_manager;
}

// -----------------------------------------------------------------------------
// Returns the program resource archive (ie. slade.pk3)
// -----------------------------------------------------------------------------
Archive* app::programResource()
{
	return archive_manager.programResourceArchive();
}

// -----------------------------------------------------------------------------
// Returns the number of ms elapsed since the application was started
// -----------------------------------------------------------------------------
long app::runTimer()
{
	return timer.Time();
}

// -----------------------------------------------------------------------------
// Returns true if the application is exiting
// -----------------------------------------------------------------------------
bool app::isExiting()
{
	return exiting;
}

// -----------------------------------------------------------------------------
// Application initialisation
// -----------------------------------------------------------------------------
bool app::init(const vector<string>& args, double ui_scale)
{
	// Get the id of the current thread (should be the main one)
	main_thread_id = std::this_thread::get_id();

	// Set numeric locale to C so that the tokenizer will work properly
	// even in locales where the decimal separator is a comma.
	wxSetlocale(LC_NUMERIC, "C");

	// Init application directories
	if (!initDirectories())
		return false;

	// Init log
	log::init();

	// Init FreeImage
	FreeImage_Initialise();

	// Process the command line arguments
	auto paths_to_open = processCommandLine(args);

	// Init keybinds
	KeyBind::initBinds();

	// Load configuration file
	log::info("Loading configuration");
	readConfigFile();

	// Init entry types
	EntryDataFormat::initBuiltinFormats();
	EntryType::initTypes();

	// Check that SLADE.pk3 can be found
	log::info("Loading resources");
	archive_manager.init();
	if (!archive_manager.resArchiveOK())
	{
		global::error = "Unable to find slade.pk3, make sure it exists in the same directory as the SLADE executable";
		return false;
	}

	// Init database
	if (!database::init())
		return false;

	// Init library
	library::init();

	// Init SActions
	SAction::setBaseWxId(26000);
	SAction::initActions();

#ifndef NO_LUA
	// Init lua
	lua::init();
#endif

	// Init UI
	ui::init(ui_scale);

	// Show splash screen
	ui::showSplash("Starting up...");

	// Init palettes
	if (!palette_manager.init())
	{
		log::error("Failed to initialise palettes");
		return false;
	}

	// Init SImage formats
	SIFormat::initFormats();

	// Init brushes
	SBrush::initBrushes();

	// Load program icons
	log::info("Loading icons");
	icons::loadIcons();

	// Load entry types
	log::info("Loading entry types");
	EntryType::loadEntryTypes();

	// Load text languages
	log::info("Loading text languages");
	TextLanguage::loadLanguages();

	// Init text stylesets
	log::info("Loading text style sets");
	StyleSet::loadResourceStyles();
	StyleSet::loadCustomStyles();

	// Init colour configuration
	log::info("Loading colour configuration");
	colourconfig::init();

	// Init nodebuilders
	nodebuilders::init();

	// Init game executables
	executables::init();

	// Init main editor
	maineditor::init();

	// Init base resource
	log::info("Loading base resource");
	archive_manager.initBaseResource();
	log::info("Base resource loaded");

	// Init game configuration
	log::info("Loading game configurations");
	game::init();

#ifndef NO_LUA
	// Init script manager
	scriptmanager::init();
#endif

	// Show the main window
	maineditor::windowWx()->Show(true);
	wxGetApp().SetTopWindow(maineditor::windowWx());
	ui::showSplash("Starting up...", false, maineditor::windowWx());

	// Open any archives from the command line
	for (auto& path : paths_to_open)
		archive_manager.openArchive(path);

	// Hide splash screen
	ui::hideSplash();

	init_ok = true;
	log::info("SLADE Initialisation OK");

	// Show Setup Wizard if needed
	if (!ui::getStateBool("SetupWizardRun"))
	{
		SetupWizardDialog dlg(maineditor::windowWx());
		dlg.ShowModal();
		ui::saveStateBool("SetupWizardRun", true);
		maineditor::windowWx()->Update();
		maineditor::windowWx()->Refresh();
	}

// Show Accessibility Pop-Up on Mac if needed
#ifdef __WXOSX__
	CFStringRef     keys[]   = { kAXTrustedCheckOptionPrompt };
	CFTypeRef       values[] = { kCFBooleanTrue };
	CFDictionaryRef options  = CFDictionaryCreate(
        NULL,
        (const void**)&keys,
        (const void**)&values,
        sizeof(keys) / sizeof(keys[0]),
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);
	if (AXIsProcessTrustedWithOptions(options))
		CFRelease(options);
#endif

	return true;
}

// -----------------------------------------------------------------------------
// Saves the SLADE configuration file
// -----------------------------------------------------------------------------
void app::saveConfigFile()
{
	// Open SLADE.cfg for writing text
	wxFile file(app::path("slade3.cfg", app::Dir::User), wxFile::write);

	// Do nothing if it didn't open correctly
	if (!file.IsOpened())
		return;

	// Write cfg header
	file.Write("// ----------------------------------------------------\n");
	file.Write(wxString::Format("// SLADE v%s Configuration File\n", version_num.toString()));
	file.Write("// Don't edit this unless you know what you're doing\n");
	file.Write("// ----------------------------------------------------\n\n");

	// Write cvars
	file.Write(CVar::writeAll(), wxConvUTF8);

	// Write base resource archive paths
	file.Write("\nbase_resource_paths\n{\n");
	for (size_t a = 0; a < archive_manager.numBaseResourcePaths(); a++)
	{
		auto path = archive_manager.getBaseResourcePath(a);
		std::replace(path.begin(), path.end(), '\\', '/');
		file.Write(wxString::Format("\t\"%s\"\n", path), wxConvUTF8);
	}
	file.Write("}\n");

	// Write recent files
	// This isn't used in 3.3.0+, but we'll write them anyway for backwards-compatibility with previous versions
	// (will be removed eventually, perhaps in 3.4.0)
	auto recent_files = library::recentFiles();
	file.Write("\n// Recent Files (for backwards compatibility with pre-3.3.0 SLADE)\nrecent_files\n{\n");
	for (int i = recent_files.size() - 1; i >= 0; --i)
	{
		auto path = recent_files[i];
		std::replace(path.begin(), path.end(), '\\', '/');
		file.Write(wxString::Format("\t\"%s\"\n", path), wxConvUTF8);
	}
	file.Write("}\n");

	// Write keybinds
	file.Write("\nkeys\n{\n");
	file.Write(KeyBind::writeBinds());
	file.Write("}\n");

	// Write nodebuilder paths
	file.Write("\n");
	nodebuilders::saveBuilderPaths(file);

	// Write game exe paths
	file.Write("\nexecutable_paths\n{\n");
	file.Write(executables::writePaths());
	file.Write("}\n");

	// Close configuration file
	file.Write("\n// ----------------------------------------------------\n");
	file.Write("// End Configuration File\n");
	file.Write("// ----------------------------------------------------\n");
}

// -----------------------------------------------------------------------------
// Application exit, shuts down and cleans everything up.
// If [save_config] is true, saves all configuration related files
// -----------------------------------------------------------------------------
void app::exit(bool save_config)
{
	exiting = true;

	if (save_config)
	{
		// Save configuration
		saveConfigFile();

		// Save text style configuration
		StyleSet::saveCurrent();

		// Save colour configuration
		MemChunk ccfg;
		colourconfig::writeConfiguration(ccfg);
		ccfg.exportFile(app::path("colours.cfg", app::Dir::User));

		// Save game exes
		wxFile f;
		f.Open(app::path("executables.cfg", app::Dir::User), wxFile::write);
		f.Write(executables::writeExecutables());
		f.Close();

		// Save custom special presets
		game::saveCustomSpecialPresets();

#ifndef NO_LUA
		// Save custom scripts
		scriptmanager::saveUserScripts();
#endif
	}

	// Close all open archives
	archive_manager.closeAll();

	// Clean up
	gl::Texture::clearAll();

	// Clear temp folder
	std::error_code error;
	for (auto& item : std::filesystem::directory_iterator{ app::path("", app::Dir::Temp) })
	{
		if (!item.is_regular_file())
			continue;

		if (!std::filesystem::remove(item, error))
			log::warning("Could not clean up temporary file \"{}\": {}", item.path().string(), error.message());
	}

#ifndef NO_LUA
	// Close lua
	lua::close();
#endif

	// Close DUMB
	dumb_exit();

	// Close program database
	database::close();

	// Exit wx Application
	wxGetApp().Exit();
}


// ----------------------------------------------------------------------------
// Returns the current version of SLADE
// ----------------------------------------------------------------------------
const app::Version& app::version()
{
	return version_num;
}

// -----------------------------------------------------------------------------
// Prepends an application-related path to a [filename]
//
// app::Dir::Data: SLADE application data directory (for SLADE.pk3)
// app::Dir::User: User configuration and resources directory
// app::Dir::Executable: Directory of the SLADE executable
// app::Dir::Temp: Temporary files directory
// -----------------------------------------------------------------------------
string app::path(string_view filename, Dir dir)
{
	switch (dir)
	{
	case Dir::User:       return fmt::format("{}{}{}", dir_user, dir_separator, filename);
	case Dir::Data:       return fmt::format("{}{}{}", dir_data, dir_separator, filename);
	case Dir::Executable: return fmt::format("{}{}{}", dir_app, dir_separator, filename);
	case Dir::Resources:  return fmt::format("{}{}{}", dir_res, dir_separator, filename);
	case Dir::Temp:       return fmt::format("{}{}{}", dir_temp, dir_separator, filename);
	default:              return string{ filename };
	}
}

app::Platform app::platform()
{
#ifdef __WXMSW__
	return Platform::Windows;
#elif __WXGTK__
	return Platform::Linux;
#elif __WXOSX__
	return Platform::MacOS;
#else
	return Platform::Unknown;
#endif
}

const string& app::iconFile()
{
	static string icon = "slade.ico";
	return icon;
}

bool app::isWin64Build()
{
#if defined(_WIN64)
	return true;
#else
	return false;
#endif
}

std::thread::id app::mainThreadId()
{
	return main_thread_id;
}


CONSOLE_COMMAND(setup_wizard, 0, false)
{
	SetupWizardDialog dlg(maineditor::windowWx());
	dlg.ShowModal();
}
