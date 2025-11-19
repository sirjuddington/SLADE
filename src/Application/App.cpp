
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
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryDataFormat.h"
#include "Archive/EntryType/EntryType.h"
#include "Audio/MIDIPlayer.h"
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
#include "Graphics/Icons.h"
#include "Graphics/Palette/PaletteManager.h"
#include "Graphics/SImage/SIFormat.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/NodeBuilders.h"
#include "OpenGL/GLTexture.h"
#include "SLADEWxApp.h"
#include "Scripting/ScriptManager.h"
#include "Scripting/Scripting.h"
#include "TextEditor/TextLanguage.h"
#include "TextEditor/TextStyle.h"
#include "UI/Dialogs/SetupWizard/SetupWizardDialog.h"
#include "UI/SBrush.h"
#include "UI/State.h"
#include "UI/UI.h"
#include "Utility/FileUtils.h"
#include "Utility/JsonUtils.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include <dumb.h>
#ifdef __WXOSX__
#include <ApplicationServices/ApplicationServices.h>
#endif
#if defined(__WXMSW__) && wxCHECK_VERSION(3, 3, 0)
#include <wx/msw/darkmode.h>
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
CVAR(Int, win_darkmode, 1, CVar::Flag::Save)


#if defined(__WXMSW__) && wxCHECK_VERSION(3, 3, 0)
namespace slade::app
{
// ----------------------------------------------------------------------------
// SLADEDarkModeSettings Class
//
// Custom wxDarkModeSettings class to tweak some colours in dark mode
// ----------------------------------------------------------------------------
class SLADEDarkModeSettings : public wxDarkModeSettings
{
public:
	wxColour GetColour(wxSystemColour index) override
	{
		switch (index)
		{
		case wxSYS_COLOUR_HIGHLIGHT: return { wxS("#374F80") };
		default:                     return wxDarkModeSettings::GetColour(index);
		}
	}
};
} // namespace slade::app
#endif


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
	dir_app = strutil::Path::pathOf(wxStandardPaths::Get().GetExecutablePath().utf8_string(), false);

	// Check for portable install
	if (fileutil::fileExists(path("portable", Dir::Executable)))
	{
		// Setup portable user/data dirs
		dir_data = dir_app;
		dir_res  = dir_app;
		dir_user = dir_app + dir_separator + "config";
	}
	else
	{
		// Setup standard user/data dirs
		dir_user = wxStandardPaths::Get().GetUserDataDir().utf8_string();
		dir_data = wxStandardPaths::Get().GetDataDir().utf8_string();
		dir_res  = wxStandardPaths::Get().GetResourcesDir().utf8_string();
	}

	// Create user dir if necessary
	if (!fileutil::dirExists(dir_user))
	{
		if (!fileutil::createDir(dir_user))
		{
			wxMessageBox(WX_FMT("Unable to create user directory \"{}\"", dir_user), wxS("Error"), wxICON_ERROR);
			return false;
		}
	}

	// Create temp dir if necessary
	dir_temp = dir_user + dir_separator + "temp";
	if (!fileutil::dirExists(dir_temp))
	{
		if (!fileutil::createDir(dir_temp))
		{
			wxMessageBox(WX_FMT("Unable to create temp directory \"{}\"", dir_temp), wxS("Error"), wxICON_ERROR);
			return false;
		}
	}

	// Check data dir
	if (!fileutil::dirExists(dir_data))
		dir_data = dir_app; // Use app dir if data dir doesn't exist

	// Check res dir
	if (!fileutil::dirExists(dir_res))
		dir_res = dir_app; // Use app dir if res dir doesn't exist

	return true;
}

// -----------------------------------------------------------------------------
// Reads the pre-3.3.0 configuration file (slade3.cfg), for migration purposes
// -----------------------------------------------------------------------------
void readOldConfigFile()
{
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
					CVar::set(tz.current().text, val.utf8_string());
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
				archive_manager.addBaseResourcePath(tz.current().text);
				tz.adv();
			}

			tz.adv(); // Skip ending }
		}

		// Read keybinds
		if (tz.advIf("keys", 2))
			KeyBind::readOldBinds(tz);

		// Read nodebuilder paths
		if (tz.advIf("nodebuilder_paths", 2))
		{
			while (!tz.checkOrEnd("}"))
			{
				nodebuilders::addBuilderPath(tz.current().text, tz.peek().text);
				tz.adv(2);
			}

			tz.adv(); // Skip ending }
		}

		// Read game exe paths
		if (tz.advIf("executable_paths", 2))
		{
			while (!tz.checkOrEnd("}"))
			{
				executables::setGameExePath(tz.current().text, tz.peek().text);
				tz.adv(2);
			}

			tz.adv(); // Skip ending }
		}

		// Next token
		tz.adv();
	}
}

// -----------------------------------------------------------------------------
// Reads and parses the SLADE configuration file
// -----------------------------------------------------------------------------
void readConfigFile()
{
	// Open config JSON file
	SFile file(path("config.json", Dir::User));
	if (!file.isOpen())
	{
		// If it doesn't exist, try reading the old pre-3.3.0 config file
		readOldConfigFile();
		return;
	}

	auto j = jsonutil::parseFile(file);
	if (j.is_discarded())
		return;

	// CVars
	if (j.contains("cvars"))
	{
		for (auto& [key, value] : j["cvars"].items())
		{
			if (value.is_string())
				CVar::set(key, value);
			else
				CVar::set(key, to_string(value));
		}
	}

	// Base resource archive paths
	if (j.contains("base_resource_paths"))
		for (const auto& path : j["base_resource_paths"])
			archive_manager.addBaseResourcePath(path.get<string>());

	// Nodebuilder paths
	if (j.contains("nodebuilder_paths"))
		for (auto& [builder, path] : j["nodebuilder_paths"].items())
			nodebuilders::addBuilderPath(builder, path.get<string>());

	// Game executable paths
	if (j.contains("executable_paths"))
		for (const auto& [exe, path] : j["executable_paths"].items())
			executables::setGameExePath(exe, path.get<string>());
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
bool app::init(const vector<string>& args)
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
		wxMessageBox(
			wxS("Unable to find slade.pk3, make sure it exists in the same directory as the "
				"SLADE executable"),
			wxS("Error"),
			wxICON_ERROR);
		return false;
	}

	// Init database
	if (!database::init())
		return false;

	// Init SActions
	SAction::setBaseWxId(26000);
	SAction::initActions();

#ifndef NO_LUA
	// Init lua
	scripting::init();
#endif

	// Enable dark mode in Windows if requested and supported
#if defined(__WXMSW__) && wxCHECK_VERSION(3, 3, 0)
	if (win_darkmode > 0)
		wxTheApp->MSWEnableDarkMode(
			win_darkmode > 1 ? wxApp::DarkMode_Always : wxApp::DarkMode_Auto, new SLADEDarkModeSettings());
#endif

	// Init UI
	ui::init();

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
	if (!ui::getStateBool(ui::SETUP_WIZARD_RUN))
	{
		SetupWizardDialog dlg(maineditor::windowWx());
		dlg.ShowModal();
		ui::saveStateBool(ui::SETUP_WIZARD_RUN, true);
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
	// Open config file for writing
	SFile config_json(path("config.json", Dir::User), SFile::Mode::Write);
	if (!config_json.isOpen())
		return;

	// Build JSON object
	Json j;

	// CVars
	auto& j_cvars = j["cvars"];
	for (auto cvar : CVar::allCvars())
	{
		if (cvar->flags & CVar::Flag::Save)
		{
			switch (cvar->type)
			{
			case CVar::Type::Integer: j_cvars[cvar->name] = cvar->getValue().Int; break;
			case CVar::Type::Boolean: j_cvars[cvar->name] = cvar->getValue().Bool; break;
			case CVar::Type::Float:   j_cvars[cvar->name] = cvar->getValue().Float; break;
			case CVar::Type::String:  j_cvars[cvar->name] = dynamic_cast<CStringCVar*>(cvar)->value; break;
			default:                  break;
			}
		}
	}

	// Base resource archive paths
	for (size_t a = 0; a < archive_manager.numBaseResourcePaths(); a++)
	{
		auto path = archive_manager.getBaseResourcePath(a);
		std::replace(path.begin(), path.end(), '\\', '/');
		j["base_resource_paths"].push_back(path);
	}

	// Nodebuilder paths
	nodebuilders::writeBuilderPaths(j);

	// Game exe paths
	executables::writePaths(j);


	// Write JSON to file
	config_json.writeStr(j.dump(2));
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

		// Save keybinds
		KeyBind::saveBinds();

		// Save text style configuration
		StyleSet::saveCurrent();

		// Save colour configuration
		colourconfig::writeConfiguration(path("colours.json", Dir::User));

		// Save game exes
		executables::writeExecutables(path("executables.json", Dir::User));

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
	audio::resetMIDIPlayer();

	// Clear temp folder
	auto temp_files = fileutil::allFilesInDir(path("", Dir::Temp), true, true);
	for (const auto& file : temp_files)
	{
		if (!fileutil::removeFile(file))
			log::warning("Could not clean up temporary file \"{}\"", file);
	}

#ifndef NO_LUA
	// Close lua
	scripting::close();
#endif

	// Close DUMB
	dumb_exit();

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
	return Windows;
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

bool app::isDarkTheme()
{
	return wxSystemSettings::GetAppearance().IsDark();
}


CONSOLE_COMMAND(setup_wizard, 0, false)
{
	SetupWizardDialog dlg(maineditor::windowWx());
	dlg.ShowModal();
}
