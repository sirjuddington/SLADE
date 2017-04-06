
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    App.cpp
 * Description: The App namespace, with various general application
 *              related functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "Utility/Tokenizer.h"
#include "Archive/ArchiveManager.h"
#include "General/KeyBind.h"
#include "MapEditor/NodeBuilders.h"
#include "General/Executables.h"
#include "General/Misc.h"
#include "General/Lua.h"
#include "General/SAction.h"
#include "General/UI.h"
#include "UI/SplashWindow.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/Icons.h"
#include "OpenGL/Drawing.h"
#include "UI/TextEditor/TextLanguage.h"
#include "UI/TextEditor/TextStyle.h"
#include "General/ColourConfiguration.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/GameConfiguration/GameConfiguration.h"
#include "Dialogs/SetupWizard/SetupWizardDialog.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace App
{
	wxStopWatch	timer;
	int			temp_fail_count = 0;
	bool		init_ok = false;

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
CVAR(Bool, setup_wizard_run, false, CVAR_SAVE)


/*******************************************************************
 * APP NAMESPACE FUNCTIONS
 *******************************************************************/

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

	/* SLADEWxApp::readConfigFile
	 * Reads and parses the SLADE configuration file
	 *******************************************************************/
	void readConfigFile()
	{
		// Open SLADE.cfg
		Tokenizer tz;
		if (!tz.openFile(App::path("slade3.cfg", App::Dir::User)))
			return;

		// Go through the file with the tokenizer
		string token = tz.getToken();
		while (!tz.atEnd())
		{
			// If we come across a 'cvars' token, read in the cvars section
			if (!token.Cmp("cvars"))
			{
				token = tz.getToken();	// Skip '{'

										// Keep reading name/value pairs until we hit the ending '}'
				string cvar_name = tz.getToken();
				while (cvar_name.Cmp("}") && !tz.atEnd())
				{
					string cvar_val = tz.getToken();
					read_cvar(cvar_name, cvar_val);
					cvar_name = tz.getToken();
				}
			}

			// Read base resource archive paths
			if (!token.Cmp("base_resource_paths"))
			{
				// Skip {
				token = wxString::FromUTF8(UTF8(tz.getToken()));

				// Read paths until closing brace found
				token = tz.getToken();
				while (token.Cmp("}") && !tz.atEnd())
				{
					theArchiveManager->addBaseResourcePath(token);
					token = wxString::FromUTF8(UTF8(tz.getToken()));
				}
			}

			// Read recent files list
			if (token == "recent_files")
			{
				// Skip {
				token = tz.getToken();

				// Read files until closing brace found
				token = wxString::FromUTF8(UTF8(tz.getToken()));
				while (token != "}" && !tz.atEnd())
				{
					theArchiveManager->addRecentFile(token);
					token = wxString::FromUTF8(UTF8(tz.getToken()));
				}
			}

			// Read keybinds
			if (token == "keys")
			{
				token = tz.getToken();	// Skip {
				KeyBind::readBinds(tz);
			}

			// Read nodebuilder paths
			if (token == "nodebuilder_paths")
			{
				token = tz.getToken();	// Skip {

										// Read paths until closing brace found
				token = tz.getToken();
				while (token != "}" && !tz.atEnd())
				{
					string path = tz.getToken();
					NodeBuilders::addBuilderPath(token, path);
					token = tz.getToken();
				}
			}

			// Read game exe paths
			if (token == "executable_paths")
			{
				token = tz.getToken();	// Skip {

										// Read paths until closing brace found
				token = tz.getToken();
				while (token != "}" && !tz.atEnd())
				{
					if (token.length())
					{
						string path = tz.getToken();
						Executables::setGameExePath(token, path);
					}
					token = tz.getToken();
				}
			}

			// Read window size/position info
			if (token == "window_info")
			{
				token = tz.getToken();	// Skip {
				Misc::readWindowInfo(&tz);
			}

			// Get next token
			token = tz.getToken();
		}
	}
}

bool App::init()
{
	// Set locale to C so that the tokenizer will work properly
	// even in locales where the decimal separator is a comma.
	setlocale(LC_ALL, "C");

	// Init application directories
	if (!initDirectories())
		return false;

	// Init log
	Log::init();

	// Init keybinds
	KeyBind::initBinds();

	// Load configuration file
	Log::info("Loading configuration");
	readConfigFile();

	// Check that SLADE.pk3 can be found
	Log::info("Loading resources");
	theArchiveManager->init();
	if (!theArchiveManager->resArchiveOK())
	{
		wxMessageBox("Unable to find slade.pk3, make sure it exists in the same directory as the SLADE executable", "Error", wxICON_ERROR);
		return false;
	}

	// Init SActions
	SAction::initWxId(26000);
	SAction::initActions();

	// Init lua
	Lua::init();

	// Show splash screen
	SplashWindow::getInstance()->init(); // TODO: Move SplashWindow singleton to App namespace
	UI::showSplash("Starting up...");

	// Init SImage formats
	SIFormat::initFormats();

	// Load program icons
	Log::info("Loading icons");
	Icons::loadIcons();

	// Load program fonts
	Drawing::initFonts();

	// Load entry types
	Log::info("Loading entry types");
	EntryDataFormat::initBuiltinFormats();
	EntryType::loadEntryTypes();

	// Load text languages
	Log::info("Loading text languages");
	TextLanguage::loadLanguages();

	// Init text stylesets
	Log::info("Loading text style sets");
	StyleSet::loadResourceStyles();
	StyleSet::loadCustomStyles();

	// Init colour configuration
	Log::info("Loading colour configuration");
	ColourConfiguration::init();

	// Init nodebuilders
	NodeBuilders::init();

	// Init game executables
	Executables::init();

	// Init main editor
	MainEditor::init();

	// Init base resource
	Log::info("Loading base resource");
	theArchiveManager->initBaseResource();
	Log::info("Base resource loaded");

	// Init game configuration
	theGameConfiguration->init();

	// Show the main window
	MainEditor::windowWx()->Show(true);
	wxTheApp->SetTopWindow(MainEditor::windowWx());
	SplashWindow::getInstance()->SetParent(MainEditor::windowWx());
	SplashWindow::getInstance()->CentreOnParent();

	// Open any archives on the command line
	// argv[0] is normally the executable itself (i.e. SLADE.exe)
	// and opening it as an archive should not be attempted...
	for (int a = 1; a < wxTheApp->argc; a++)
		theArchiveManager->openArchive(wxTheApp->argv[a]);

	// Hide splash screen
	UI::hideSplash();

	init_ok = true;
	Log::info("SLADE Initialisation OK");

	// Show Setup Wizard if needed
	if (!setup_wizard_run)
	{
		SetupWizardDialog dlg(MainEditor::windowWx());
		dlg.ShowModal();
		setup_wizard_run = true;
		MainEditor::windowWx()->Update();
		MainEditor::windowWx()->Refresh();
	}

	return true;
}

bool App::isInitialised()
{
	return init_ok;
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
				Log::warning(S_FMT("Unable to create temp directory \"%s\"", dir_temp));
				temp_fail_count++;
				return path(filename, dir);
			}
		}

		return dir_temp + dir_separator + filename;
	}

	return filename;
}
