
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
// Registers the ArchiveFormatDesc type with lua
// -----------------------------------------------------------------------------
void registerArchiveFormat(sol::state& lua)
{
	// Create ArchiveFormat type, no constructor
	auto lua_archiveformat = lua.new_usertype<ArchiveFormatDesc>("ArchiveFormat", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_archiveformat.set("id", sol::readonly(&ArchiveFormatDesc::id));
	lua_archiveformat.set("name", sol::readonly(&ArchiveFormatDesc::name));
	lua_archiveformat.set("supportsDirs", sol::readonly(&ArchiveFormatDesc::supports_dirs));
	lua_archiveformat.set("hasExtensions", sol::readonly(&ArchiveFormatDesc::names_extensions));
	lua_archiveformat.set("maxNameLength", sol::readonly(&ArchiveFormatDesc::max_name_length));
	lua_archiveformat.set("entryFormat", sol::readonly(&ArchiveFormatDesc::entry_format));
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
	lua_search_opt.set("matchName", sol::property(&ArchiveSearchOptions::match_name));
	lua_search_opt.set("matchType", sol::property(&ArchiveSearchOptions::match_type));
	lua_search_opt.set("matchNamespace", sol::property(&ArchiveSearchOptions::match_namespace));
	lua_search_opt.set("dir", sol::property(&ArchiveSearchOptions::dir));
	lua_search_opt.set("ignoreExt", sol::property(&ArchiveSearchOptions::ignore_ext));
	lua_search_opt.set("searchSubdirs", sol::property(&ArchiveSearchOptions::search_subdirs));
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
	lua_dir.set("name", sol::property(&ArchiveDir::name));
	lua_dir.set("archive", sol::property(&ArchiveDir::archive));
	lua_dir.set("entries", sol::property(&ArchiveDir::entries));
	lua_dir.set("parent", sol::property(&ArchiveDir::parent));
	lua_dir.set("path", sol::property(&ArchiveDir::path));
	lua_dir.set("subDirectories", sol::property(&ArchiveDir::subdirs));
}

// -----------------------------------------------------------------------------
// Registers the Archives namespace with lua
// -----------------------------------------------------------------------------
void registerArchivesNamespace(sol::state& lua)
{
	auto archives = lua.create_table("Archives");

	archives.set_function(
		"All",
		sol::overload(
			[](bool res) { return app::archiveManager().allArchives(res); },
			[]() { return app::archiveManager().allArchives(false); }));
	archives.set_function(
		"Create",
		[](string_view format) { return std::make_tuple(app::archiveManager().newArchive(format), global::error); });
	archives.set_function(
		"OpenFile",
		[](string_view filename)
		{ return std::make_tuple(app::archiveManager().openArchive(filename), global::error); });
	archives.set_function(
		"Close",
		sol::overload(
			[](Archive* archive) { return app::archiveManager().closeArchive(archive); },
			[](int index) { return app::archiveManager().closeArchive(index); }));
	archives.set_function("CloseAll", []() { app::archiveManager().closeAll(); });
	archives.set_function("FileExtensionsString", []() { return app::archiveManager().getArchiveExtensionsString(); });
	archives.set_function("BaseResource", []() { return app::archiveManager().baseResourceArchive(); });
	archives.set_function("BaseResourcePaths", []() { return app::archiveManager().baseResourcePaths(); });
	archives.set_function("OpenBaseResource", [](int index) { return app::archiveManager().openBaseResource(index); });
	archives.set_function("ProgramResource", []() { return app::archiveManager().programResourceArchive(); });
	archives.set_function("RecentFiles", []() { return app::archiveManager().recentFiles(); });
	archives.set_function("Bookmarks", []() { return app::archiveManager().bookmarks(); });
	archives.set_function(
		"AddBookmark", [](ArchiveEntry* entry) { app::archiveManager().addBookmark(entry->getShared()); });
	archives.set_function("RemoveBookmark", [](ArchiveEntry* entry) { app::archiveManager().deleteBookmark(entry); });
	archives.set_function("EntryType", &EntryType::fromId);
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
