
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Lua.cpp
// Description: Lua scripting system (nothing much here yet)
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#define SOL_CHECK_ARGUMENTS 1
#include "External/sol/sol.hpp"
#include "General/Console/Console.h"
#include "Lua.h"
#include "Utility/SFileDialog.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/All.h"
#include "MainEditor/MainEditor.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace Lua
{
	sol::state	lua;
	wxWindow*	current_window = nullptr;
}


// ----------------------------------------------------------------------------
// wxString support for Sol
//
// See: https://github.com/ThePhD/sol2/issues/140
// ----------------------------------------------------------------------------
namespace sol
{
	namespace stack
	{
		template<>
		struct pusher<wxString>
		{
			static int push(lua_State* L, const wxString& str)
			{
				return stack::push(L, CHR(str));
			}
		};

		template<>
		struct getter<wxString>
		{
			static wxString get(lua_State *L, int index, record &tracking)
			{
				tracking.use(1);
				const char* luastr = stack::get<const char *>(L, index);
				return wxString::FromUTF8(luastr);
			}
		};
	}

	template<>
	struct lua_type_of<wxString> : std::integral_constant<type, type::string> {};
}


// ----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// ----------------------------------------------------------------------------
namespace Lua
{
	// --- Functions ---

	// logMessage: Prints a log message
	void logMessage(const char* message)
	{
		Log::message(Log::MessageType::Script, message);
	}

	// Show a message box
	void messageBox(const string& title, const string& message)
	{
		wxMessageBox(message, title, 5L, current_window);
	}

	// Prompt for a string
	string promptString(const string& title, const string& message, const string& default_value)
	{
		return wxGetTextFromUser(message, title, default_value, current_window);
	}

	// Prompt for a number
	int promptNumber(
		const string& title,
		const string& message,
		int default_value,
		int min,
		int max)
	{
		return (int)wxGetNumberFromUser(message, "", title, default_value, min, max);
	}

	// Prompt for a yes/no answer
	bool promptYesNo(const string& title, const string& message)
	{
		return (wxMessageBox(message, title, wxYES_NO | wxICON_QUESTION) == wxYES);
	}

	// Browse for a single file
	string browseFile(const string& title, const string& extensions, const string& filename)
	{
		SFileDialog::fd_info_t inf;
		SFileDialog::openFile(inf, title, extensions, current_window, filename);
		return inf.filenames.empty() ? "" : inf.filenames[0];
	}

	// Browse for multiple files
	vector<string> browseFiles(const string& title, const string& extensions)
	{
		SFileDialog::fd_info_t inf;
		vector<string> filenames;
		if (SFileDialog::openFiles(inf, title, extensions, current_window))
			filenames.assign(inf.filenames.begin(), inf.filenames.end());
		return filenames;
	}


	// --- Function & Type Registration ---

	void registerGlobalFunctions()
	{
		sol::table slade = lua["slade"];
		slade.set_function("logMessage", &logMessage);
		slade.set_function("messageBox", &messageBox);
		slade.set_function("promptString", &promptString);
		slade.set_function("promptNumber", &promptNumber);
		slade.set_function("promptYesNo", &promptYesNo);
		slade.set_function("browseFile", &browseFile);
		slade.set_function("browseFiles", &browseFiles);
		slade.set_function("archiveManager", &ArchiveManager::getInstance);
		slade.set_function("currentArchive", &MainEditor::currentArchive);
		slade.set_function("currentEntry", &MainEditor::currentEntry);
		slade.set_function("currentEntrySelection", &MainEditor::currentEntrySelection);
	}

	void registerArchiveManager()
	{
		lua.new_usertype<ArchiveManager>(
			"ArchiveManager",

			// No constructor
			"new", sol::no_constructor,

			// Functions
			"numArchives",					&ArchiveManager::numArchives,
			"openFile",						&ArchiveManager::luaOpenFile,
			"closeAll",						&ArchiveManager::closeAll,
			"getArchive",					sol::resolve<Archive*(int)>(&ArchiveManager::getArchive),
			"closeArchive",					sol::resolve<bool(Archive*)>(&ArchiveManager::closeArchive),
			"getArchiveExtensionsString",	&ArchiveManager::getArchiveExtensionsString
		);
	}

	void registerArchive()
	{
		lua.new_usertype<Archive>(
			"Archive",

			// No constructor
			"new", sol::no_constructor,

			// Functions
			"getFilename",				&Archive::getFilename,
			"allEntries",				&Archive::luaAllEntries,
			"getDir",					&Archive::luaGetDir,
			"createEntry",				&Archive::luaCreateEntry,
			"createEntryInNamespace",	&Archive::luaCreateEntryInNamespace,
			"removeEntry",				&Archive::removeEntry,
			"renameEntry",				&Archive::renameEntry
		);

		// Register all subclasses
		// (perhaps it'd be a good idea to make Archive not abstract and handle
		//  the format-specific stuff somewhere else, rather than in subclasses)
		lua.new_usertype<WadArchive>("WadArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<ZipArchive>("ZipArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<LibArchive>("LibArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<DatArchive>("DatArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<ResArchive>("ResArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<PakArchive>("PakArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<BSPArchive>("BSPArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<GrpArchive>("GrpArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<RffArchive>("RffArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<GobArchive>("GobArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<LfdArchive>("LfdArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<HogArchive>("HogArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<ADatArchive>("ADatArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<Wad2Archive>("Wad2Archive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<WadJArchive>("WadJArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<WolfArchive>("WolfArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<GZipArchive>("GZipArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<BZip2Archive>("BZip2Archive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<TarArchive>("TarArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<DiskArchive>("DiskArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<PodArchive>("PodArchive", sol::base_classes, sol::bases<Archive>());
		lua.new_usertype<ChasmBinArchive>("ChasmBinArchive", sol::base_classes, sol::bases<Archive>());
	}

	void registerArchiveEntry()
	{
		lua.new_usertype<ArchiveEntry>(
			"ArchiveEntry",

			// No constructor
			"new", sol::no_constructor,

			// Functions
			"getName",				&ArchiveEntry::getName,
			"getUpperName",			&ArchiveEntry::getUpperName,
			"getUpperNameNoExt",	&ArchiveEntry::getUpperNameNoExt,
			"getPath",				&ArchiveEntry::getPath,
			"getSizeString",		&ArchiveEntry::getSizeString,
			"getTypeString",		&ArchiveEntry::getTypeString,
			"getType",				&ArchiveEntry::getType
		);
	}
}

// ----------------------------------------------------------------------------
// Lua::init
//
// Initialises lua and registers functions
// ----------------------------------------------------------------------------
bool Lua::init()
{
	lua.open_libraries(sol::lib::base);

	// Register functions
	lua.create_named_table("slade");
	registerGlobalFunctions();
	registerArchiveManager();
	registerArchive();
	registerArchiveEntry();

	return true;
}

// ----------------------------------------------------------------------------
// Lua::close
//
// Close the lua state
// ----------------------------------------------------------------------------
void Lua::close()
{
}

// ----------------------------------------------------------------------------
// Lua::run
//
// Runs a lua script [program]
// ----------------------------------------------------------------------------
bool Lua::run(string program)
{
	auto result = lua.script(CHR(program), sol::simple_on_error);

	if (!result.valid())
	{
		sol::error error = result;
		string error_type = sol::to_string(result.status());
		string error_message = error.what();
		Log::error(S_FMT(
			"%s Error running Lua script: %s",
			CHR(error_type.MakeCapitalized()),
			CHR(error_message)
		));
		return false;
	}
	
	return true;
}

// ----------------------------------------------------------------------------
// Lua::runFile
//
// Runs a lua script from a text file [filename]
// ----------------------------------------------------------------------------
bool Lua::runFile(string filename)
{
	auto result = lua.script_file(CHR(filename), sol::simple_on_error);

	if (!result.valid())
	{
		sol::error error = result;
		string error_type = sol::to_string(result.status());
		string error_message = error.what();
		Log::error(S_FMT(
			"%s Error running Lua script: %s",
			CHR(error_type.MakeCapitalized()),
			CHR(error_message)
		));
		return false;
	}

	return true;
}


// ----------------------------------------------------------------------------
//
// Console Commands
//
// ----------------------------------------------------------------------------

CONSOLE_COMMAND(lua_exec, 1, true)
{
	Lua::run(args[0]);
}

CONSOLE_COMMAND(lua_execfile, 1, true)
{
	if (!wxFile::Exists(args[0]))
	{
		LOG_MESSAGE(1, "File \"%s\" does not exist", args[0]);
		return;
	}

	if (!Lua::runFile(args[0]))
		LOG_MESSAGE(1, "Error loading lua script file \"%s\"", args[0]);
}
