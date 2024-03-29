
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archives.cpp
// Description: Functions to export Archive-related types and namespaces to lua
//              using sol3
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
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "Export.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;

// ReSharper disable CppParameterMayBeConstPtrOrRef


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::lua
{
// -----------------------------------------------------------------------------
// Registers the ArchiveFormat type with lua
// -----------------------------------------------------------------------------
void registerArchiveFormat(sol::state& lua)
{
	// Create ArchiveFormat type, no constructor
	auto lua_archiveformat = lua.new_usertype<ArchiveFormat>("ArchiveFormat", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_archiveformat["id"]            = sol::readonly(&ArchiveFormat::id);
	lua_archiveformat["name"]          = sol::readonly(&ArchiveFormat::name);
	lua_archiveformat["supportsDirs"]  = sol::readonly(&ArchiveFormat::supports_dirs);
	lua_archiveformat["hasExtensions"] = sol::readonly(&ArchiveFormat::names_extensions);
	lua_archiveformat["maxNameLength"] = sol::readonly(&ArchiveFormat::max_name_length);
	lua_archiveformat["entryFormat"]   = sol::readonly(&ArchiveFormat::entry_format);
	// TODO: extensions - need to export key_value_t or do something custom
}

// -----------------------------------------------------------------------------
// Registers the ArchiveSearchOptions type with lua
// -----------------------------------------------------------------------------
void registerArchiveSearchOptions(sol::state& lua)
{
	auto lua_search_opt = lua.new_usertype<ArchiveSearchOptions>(
		"ArchiveSearchOptions", "new", sol::constructors<ArchiveSearchOptions()>());

	// Properties
	// -------------------------------------------------------------------------
	lua_search_opt["matchName"]      = sol::property(&ArchiveSearchOptions::match_name);
	lua_search_opt["matchType"]      = sol::property(&ArchiveSearchOptions::match_type);
	lua_search_opt["matchNamespace"] = sol::property(&ArchiveSearchOptions::match_namespace);
	lua_search_opt["dir"]            = sol::property(&ArchiveSearchOptions::dir);
	lua_search_opt["ignoreExt"]      = sol::property(&ArchiveSearchOptions::ignore_ext);
	lua_search_opt["searchSubdirs"]  = sol::property(&ArchiveSearchOptions::search_subdirs);
}

// -----------------------------------------------------------------------------
// Registers the ArchiveDir type with lua
// -----------------------------------------------------------------------------
void registerArchiveDir(sol::state& lua)
{
	// Create ArchiveDir type, no constructor
	auto lua_dir = lua.new_usertype<ArchiveDir>("ArchiveDir", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_dir["name"]           = sol::property(&ArchiveDir::name);
	lua_dir["archive"]        = sol::property(&ArchiveDir::archive);
	lua_dir["entries"]        = sol::property(&ArchiveDir::entries);
	lua_dir["parent"]         = sol::property(&ArchiveDir::parent);
	lua_dir["path"]           = sol::property(&ArchiveDir::path);
	lua_dir["subDirectories"] = sol::property(&ArchiveDir::subdirs);
}

// -----------------------------------------------------------------------------
// Registers the Archives namespace with lua
// -----------------------------------------------------------------------------
void registerArchivesNamespace(sol::state& lua)
{
	auto archives = lua.create_table("Archives");

	archives["All"] = sol::overload(
		[](bool res) { return app::archiveManager().allArchives(res); },
		[]() { return app::archiveManager().allArchives(false); });
	archives["Create"] = [](string_view format)
	{ return std::make_tuple(app::archiveManager().newArchive(format), global::error); };
	archives["OpenFile"] = [](string_view filename)
	{ return std::make_tuple(app::archiveManager().openArchive(filename), global::error); };
	archives["Close"] = sol::overload(
		[](Archive* archive) { return app::archiveManager().closeArchive(archive); },
		[](int index) { return app::archiveManager().closeArchive(index); });
	archives["CloseAll"]             = []() { app::archiveManager().closeAll(); };
	archives["FileExtensionsString"] = []() { return app::archiveManager().getArchiveExtensionsString(); };
	archives["BaseResource"]         = []() { return app::archiveManager().baseResourceArchive(); };
	archives["BaseResourcePaths"]    = []() { return app::archiveManager().baseResourcePaths(); };
	archives["OpenBaseResource"]     = [](int index) { return app::archiveManager().openBaseResource(index); };
	archives["ProgramResource"]      = []() { return app::archiveManager().programResourceArchive(); };
	archives["RecentFiles"]          = []() { return app::archiveManager().recentFiles(); };
	archives["Bookmarks"]            = []() { return app::archiveManager().bookmarks(); };
	archives["AddBookmark"]    = [](ArchiveEntry* entry) { app::archiveManager().addBookmark(entry->getShared()); };
	archives["RemoveBookmark"] = [](ArchiveEntry* entry) { app::archiveManager().deleteBookmark(entry); };
	archives["EntryType"]      = &EntryType::fromId;
}

// -----------------------------------------------------------------------------
// Registers various Archive-related types with lua
// -----------------------------------------------------------------------------
void registerArchiveTypes(sol::state& lua)
{
	registerArchiveFormat(lua);
	registerArchiveSearchOptions(lua);
	registerArchive(lua);
	registerArchiveEntry(lua);
	registerEntryType(lua);
	registerArchiveDir(lua);
}

} // namespace slade::lua
