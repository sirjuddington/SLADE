
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Lua.cpp
// Description: Lua scripting system
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
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/All.h"
#include "External/sol/sol.hpp"
#include "Game/Configuration.h"
#include "Game/ThingType.h"
#include "General/Console/Console.h"
#include "Lua.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "Utility/SFileDialog.h"
#include "General/Misc.h"


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

	// Get the global error message
	string globalError()
	{
		return Global::error;
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

	// Switch to the tab for [archive], opening it if necessary
	bool showArchive(Archive* archive)
	{
		if (!archive)
			return false;

		MainEditor::openArchiveTab(archive);
		return true;
	}


	// --- Function & Type Registration ---

	void registerGlobalFunctions()
	{
		sol::table slade = lua["slade"];
		slade.set_function("logMessage",			&logMessage);
		slade.set_function("globalError",			&globalError);
		slade.set_function("messageBox",			&messageBox);
		slade.set_function("promptString",			&promptString);
		slade.set_function("promptNumber",			&promptNumber);
		slade.set_function("promptYesNo",			&promptYesNo);
		slade.set_function("browseFile",			&browseFile);
		slade.set_function("browseFiles",			&browseFiles);
		slade.set_function("currentArchive",		&MainEditor::currentArchive);
		slade.set_function("currentEntry",			&MainEditor::currentEntry);
		slade.set_function("currentEntrySelection",	&MainEditor::currentEntrySelection);
		slade.set_function("showArchive",			&showArchive);
		slade.set_function("showEntry",				&MainEditor::openEntry);
		slade.set_function("mapEditor",				&MapEditor::editContext);
	}

	void registerGameFunctions()
	{
		sol::table game = lua.create_table("game");
		game.set_function("thingType", [&](int type) { return Game::configuration().thingType(type); });
	}

	void registerArchiveManager()
	{
		sol::table archives = lua.create_table("archives");

		archives.set_function("numArchives", [&]() { return App::archiveManager().numArchives(); });
		archives.set_function(
			"openFile",
			[&](const char* filename) { return App::archiveManager().openArchive(filename); }
		);
		archives.set_function("closeAll", [&]() { App::archiveManager().closeAll(); });
		archives.set_function("getArchive", [&](int index) { return App::archiveManager().getArchive(index); });
		archives.set_function(
			"closeArchive",
			[&](Archive* archive) { return App::archiveManager().closeArchive(archive); }
		);
		archives.set_function(
			"fileExtensionsString",
			[&]() { return App::archiveManager().getArchiveExtensionsString(); }
		);
	}

	void registerArchive()
	{
		lua.new_usertype<Archive>(
			"Archive",

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"filename",	sol::property([](Archive& self) { return self.filename(); }),
			"entries",	sol::property(&Archive::luaAllEntries),
			"rootDir",	sol::property(&Archive::rootDir),

			// Functions
			"filenameNoPath",			[](Archive& self) { return self.filename(false); },
			"entryAtPath",				&Archive::entryAtPath,
			"dirAtPath",				&Archive::luaGetDir,
			"createEntry",				&Archive::luaCreateEntry,
			"createEntryInNamespace",	&Archive::luaCreateEntryInNamespace,
			"removeEntry",				&Archive::removeEntry,
			"renameEntry",				&Archive::renameEntry
		);

		// Register all subclasses
		// (perhaps it'd be a good idea to make Archive not abstract and handle
		//  the format-specific stuff somewhere else, rather than in subclasses)
		//#define REGISTER_ARCHIVE(type) lua.new_usertype<type>(#type, sol::base_classes, sol::bases<Archive>());
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

			// Properties
			"name",		sol::property([](ArchiveEntry& self) { return self.getName(); }),
			"path",		sol::property([](ArchiveEntry& self) { return self.getPath(); }),
			"type",		sol::property(&ArchiveEntry::getType),
			"size",		sol::property(&ArchiveEntry::getSize),
			"index",	sol::property([](ArchiveEntry& self) { return self.getParentDir()->entryIndex(&self); }),

			// Functions
			"formattedName",
			[](ArchiveEntry& self, bool include_path, bool include_extension, bool name_uppercase)
			{
				string name;
				if (include_path)
					name = self.getPath();
				if (name_uppercase)
					name += include_extension ? self.getUpperName() : self.getUpperNameNoExt();
				else
					name += self.getName(!include_extension);
				return name;
			},
			"formattedSize",	&ArchiveEntry::getSizeString,
			"crc32",			[](ArchiveEntry& self) { return Misc::crc(self.getData(), self.getSize()); }
		);
	}

	void registerArchiveTreeNode()
	{
		lua.new_usertype<ArchiveTreeNode>(
			"ArchiveDir",

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"name",		sol::property(&ArchiveTreeNode::getName),
			"archive",	sol::property(&ArchiveTreeNode::archive),
			"entries",	sol::property(&ArchiveTreeNode::luaGetEntries),
			"parent",	sol::property([](ArchiveTreeNode& self) { return (ArchiveTreeNode*)self.getParent(); }),
			"path",		sol::property(&ArchiveTreeNode::getPath)
			// subDirectories
		);
	}

	void registerEntryType()
	{
		lua.new_usertype<EntryType>(
			"EntryType",

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"id",			sol::property(&EntryType::getId),
			"name",			sol::property(&EntryType::getName),
			"extension",	sol::property(&EntryType::getExtension),
			"formatId",		sol::property(&EntryType::getFormat),
			"editor",		sol::property(&EntryType::getEditor),
			"category",		sol::property(&EntryType::getCategory)
		);
	}

	void registerSLADEMap()
	{
		lua.new_usertype<SLADEMap>(
			"Map",

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"name",				sol::property(&SLADEMap::mapName),
			"udmfNamespace",	sol::property(&SLADEMap::udmfNamespace),
			"vertices",			sol::property(&SLADEMap::vertices),
			"linedefs",			sol::property(&SLADEMap::lines),
			"sidedefs",			sol::property(&SLADEMap::sides),
			"sectors",			sol::property(&SLADEMap::sectors),
			"things",			sol::property(&SLADEMap::things),

			// Functions
			"numVertices",	&SLADEMap::nVertices,
			"numLines",		&SLADEMap::nLines,
			"numSides",		&SLADEMap::nSides,
			"numSectors",	&SLADEMap::nSectors,
			"numThings",	&SLADEMap::nThings
		);
	}

	void registerItemSelection()
	{
		lua.new_usertype<ItemSelection>(
			"ItemSelection",

			// No constructor
			"new", sol::no_constructor,

			// Functions
			"selectedVertices",	&ItemSelection::selectedVertices,
			"selectedLines",	&ItemSelection::selectedLines,
			"selectedSectors",	&ItemSelection::selectedSectors,
			"selectedThings",	&ItemSelection::selectedThings
		);
	}

	void registerMapEditor()
	{
		registerItemSelection();

		// MapEditor enums
		lua.create_named_table("mapEditor");
		lua.new_enum(
			"Mode",
			"Vertices",	MapEditor::Mode::Vertices,
			"Lines",	MapEditor::Mode::Lines,
			"Sectors",	MapEditor::Mode::Sectors,
			"Things",	MapEditor::Mode::Things,
			"Visual",	MapEditor::Mode::Visual
		);
		lua.new_enum(
			"SectorMode",
			"Both",		MapEditor::SectorMode::Both,
			"Floor",	MapEditor::SectorMode::Floor,
			"Ceiling",	MapEditor::SectorMode::Ceiling
		);

		lua.new_usertype<MapEditContext>(
			"MapEditor",

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"editMode",			sol::property(&MapEditContext::editMode),
			"sectorEditMode",	sol::property(&MapEditContext::sectorEditMode),
			"gridSize",			sol::property(&MapEditContext::gridSize),
			"selection",		sol::property(&MapEditContext::selection),
			"map",				sol::property(&MapEditContext::map)
		);
	}

	void registerMapVertex()
	{
		lua.new_usertype<MapVertex>(
			"MapVertex",
			sol::base_classes, sol::bases<MapObject>(),

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"x",	&MapVertex::xPos,
			"y",	&MapVertex::yPos
		);
	}

	void registerMapLine()
	{
		lua.new_usertype<MapLine>(
			"MapLine",
			sol::base_classes, sol::bases<MapObject>(),

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"x1",		sol::property(&MapLine::x1),
			"y1",		sol::property(&MapLine::y1),
			"x2",		sol::property(&MapLine::x2),
			"y2",		sol::property(&MapLine::y2),
			"vertex1",	sol::property(&MapLine::v1),
			"vertex2",	sol::property(&MapLine::v2),
			"side1",	sol::property(&MapLine::s1),
			"side2",	sol::property(&MapLine::s2),
			"special",	sol::property(&MapLine::getSpecial),

			// Functions
			"length",		&MapLine::getLength,
			"frontSector",	&MapLine::frontSector,
			"backSector",	&MapLine::backSector
		);
	}

	void registerMapSide()
	{
		lua.new_usertype<MapSide>(
			"MapSide",
			sol::base_classes, sol::bases<MapObject>(),

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"sector",			sol::property(&MapSide::getSector),
			"line",				sol::property(&MapSide::getParentLine),
			"textureBottom",	sol::property(&MapSide::getTexLower),
			"textureMiddle",	sol::property(&MapSide::getTexMiddle),
			"textureTop",		sol::property(&MapSide::getTexUpper),
			"offsetX",			sol::property(&MapSide::getOffsetX),
			"offsetY",			sol::property(&MapSide::getOffsetY)
		);
	}

	void registerMapSector()
	{
		lua.new_usertype<MapSector>(
			"MapSector",
			sol::base_classes, sol::bases<MapObject>(),

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"textureFloor",		sol::property(&MapSector::getFloorTex),
			"textureCeiling",	sol::property(&MapSector::getCeilingTex),
			"heightFloor",		sol::property(&MapSector::getFloorHeight),
			"heightCeiling",	sol::property(&MapSector::getCeilingHeight),
			"lightLevel",		sol::property(&MapSector::getLightLevel),
			"special",			sol::property(&MapSector::getSpecial),
			"id",				sol::property(&MapSector::getTag)
		);
	}

	void registerMapThing()
	{
		lua.new_usertype<MapThing>(
			"MapThing",
			sol::base_classes, sol::bases<MapObject>(),

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"x",		sol::property(&MapThing::xPos),
			"y",		sol::property(&MapThing::yPos),
			"type",		sol::property(&MapThing::getType),
			"angle",	sol::property(&MapThing::getAngle)
		);
		
		// //dukglue_register_method(context, &MapThing::s_TypeInfo,	"typeInfo");
	}

	void registerMapObject()
	{
		lua.new_usertype<MapObject>(
			"MapObject",

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"index", sol::property(&MapObject::getIndex),
			"typeName", sol::property(&MapObject::getTypeName),

			// Functions
			"hasProperty",			&MapObject::hasProp,
			"boolProperty",			&MapObject::boolProperty,
			"intProperty",			&MapObject::intProperty,
			"floatProperty",		&MapObject::floatProperty,
			"stringProperty",		&MapObject::stringProperty,
			"setBoolProperty",		&MapObject::luaSetBoolProperty,
			"setIntProperty",		&MapObject::luaSetIntProperty,
			"setFloatProperty",		&MapObject::luaSetFloatProperty,
			"setStringProperty",	&MapObject::luaSetStringProperty
		);

		registerMapVertex();
		registerMapLine();
		registerMapSide();
		registerMapSector();
		registerMapThing();
	}

	void registerThingType()
	{
		lua.new_usertype<Game::ThingType>(
			"ThingType",

			// No constructor
			"new", sol::no_constructor,

			// Properties
			"name",			sol::property(&Game::ThingType::name),
			"group",		sol::property(&Game::ThingType::group),
			"radius",		sol::property(&Game::ThingType::radius),
			"height",		sol::property(&Game::ThingType::height),
			"scaleY",		sol::property(&Game::ThingType::scaleY),
			"scaleX",		sol::property(&Game::ThingType::scaleX),
			"angled",		sol::property(&Game::ThingType::angled),
			"hanging",		sol::property(&Game::ThingType::hanging),
			"fullbright",	sol::property(&Game::ThingType::fullbright),
			"decoration",	sol::property(&Game::ThingType::decoration),
			"solid",		sol::property(&Game::ThingType::solid),
			//"tagged",		sol::property(&Game::ThingType::needsTag),
			"sprite",		sol::property(&Game::ThingType::sprite),
			"icon",			sol::property(&Game::ThingType::icon),
			"translation",	sol::property(&Game::ThingType::translation),
			"palette",		sol::property(&Game::ThingType::palette)
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
	lua.open_libraries(sol::lib::base, sol::lib::string);

	// Register functions
	lua.create_named_table("slade");
	registerGlobalFunctions();
	registerGameFunctions();

	// Register classes
	registerArchiveManager();
	registerArchive();
	registerArchiveEntry();
	registerEntryType();
	registerArchiveTreeNode();
	registerSLADEMap();
	registerMapEditor();
	registerMapObject();
	registerThingType();

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
