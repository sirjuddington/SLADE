
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
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "Export.h"
#include "Scripting/LuaBridge.h"

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
// Registers the ArchiveFormatInfo type with lua
// -----------------------------------------------------------------------------
static void registerArchiveFormat(lua_State* lua)
{
	// Create ArchiveFormat type, no constructor
	auto lua_archiveformat = luabridge::getGlobalNamespace(lua).beginClass<ArchiveFormatInfo>("ArchiveFormat");

	// Properties
	// -------------------------------------------------------------------------
	lua_archiveformat.addProperty("id", &ArchiveFormatInfo::id);
	lua_archiveformat.addProperty("name", &ArchiveFormatInfo::name);
	lua_archiveformat.addProperty("supportsDirs", &ArchiveFormatInfo::supports_dirs);
	lua_archiveformat.addProperty("hasExtensions", &ArchiveFormatInfo::names_extensions);
	lua_archiveformat.addProperty("maxNameLength", &ArchiveFormatInfo::max_name_length);
	lua_archiveformat.addProperty("entryFormat", &ArchiveFormatInfo::entry_format);
	// TODO: extensions - need to export key_value_t or do something custom
}

// -----------------------------------------------------------------------------
// Registers the ArchiveSearchOptions type with lua
// -----------------------------------------------------------------------------
static void registerArchiveSearchOptions(lua_State* lua)
{
	auto lua_search_opt = luabridge::getGlobalNamespace(lua).beginClass<ArchiveSearchOptions>("ArchiveSearchOptions");
	lua_search_opt.addConstructor<void()>();

	// Properties
	// -------------------------------------------------------------------------
	using ASO = ArchiveSearchOptions;
	lua_search_opt.addProperty("matchName", &ASO::match_name, &ASO::match_name);
	lua_search_opt.addProperty("matchType", &ASO::match_type, &ASO::match_type);
	lua_search_opt.addProperty("matchNamespace", &ASO::match_namespace, &ASO::match_namespace);
	lua_search_opt.addProperty("dir", &ASO::dir, &ASO::dir);
	lua_search_opt.addProperty("ignoreExt", &ASO::ignore_ext, &ASO::ignore_ext);
	lua_search_opt.addProperty("searchSubdirs", &ASO::search_subdirs, &ASO::search_subdirs);
}

// -----------------------------------------------------------------------------
// Registers the ArchiveDir type with lua
// -----------------------------------------------------------------------------
static void registerArchiveDir(lua_State* lua)
{
	// Create ArchiveDir type, no constructor
	auto lua_dir = luabridge::getGlobalNamespace(lua).beginClass<ArchiveDir>("ArchiveDir");

	// Properties
	// -------------------------------------------------------------------------
	lua_dir.addProperty("name", &ArchiveDir::name);
	lua_dir.addProperty("archive", &ArchiveDir::archive);
	lua_dir.addProperty("entries", &ArchiveDir::entries);
	lua_dir.addProperty("parent", &ArchiveDir::parent);
	lua_dir.addProperty("path", [](const ArchiveDir& self) { return self.path(); });
	lua_dir.addProperty("subDirectories", &ArchiveDir::subdirs);
}

// -----------------------------------------------------------------------------
// Registers the Archives namespace with lua
// -----------------------------------------------------------------------------
void registerArchivesNamespace(lua_State* lua)
{
	auto archives = luabridge::getGlobalNamespace(lua).beginNamespace("Archives");

	archives.addFunction(
		"All",
		[](bool res) { return app::archiveManager().allArchives(res); },
		[] { return app::archiveManager().allArchives(false); });
	archives.addFunction(
		"Create",
		[](string_view format) { return std::make_tuple(app::archiveManager().newArchive(format), global::error); });
	archives.addFunction(
		"OpenFile",
		[](string_view filename)
		{ return std::make_tuple(app::archiveManager().openArchive(filename), global::error); });
	archives.addFunction(
		"Close",
		[](Archive* archive) { return app::archiveManager().closeArchive(archive); },
		[](int index) { return app::archiveManager().closeArchive(index); });
	archives.addFunction("CloseAll", [] { app::archiveManager().closeAll(); });
	archives.addFunction("FileExtensionsString", [] { return app::archiveManager().getArchiveExtensionsString(); });
	archives.addFunction("BaseResource", [] { return app::archiveManager().baseResourceArchive(); });
	archives.addFunction("BaseResourcePaths", [] { return app::archiveManager().baseResourcePaths(); });
	archives.addFunction("OpenBaseResource", [](int index) { return app::archiveManager().openBaseResource(index); });
	archives.addFunction("ProgramResource", [] { return app::archiveManager().programResourceArchive(); });
	archives.addFunction("RecentFiles", [] { return app::archiveManager().recentFiles(); });
	archives.addFunction("Bookmarks", [] { return app::archiveManager().bookmarks(); });
	archives.addFunction(
		"AddBookmark", [](ArchiveEntry* entry) { app::archiveManager().addBookmark(entry->getShared()); });
	archives.addFunction("RemoveBookmark", [](ArchiveEntry* entry) { app::archiveManager().deleteBookmark(entry); });
	archives.addFunction("EntryType", &EntryType::fromId);
}

// -----------------------------------------------------------------------------
// Registers various Archive-related types with lua
// -----------------------------------------------------------------------------
void registerArchiveTypes(lua_State* lua)
{
	registerArchiveFormat(lua);
	registerArchiveSearchOptions(lua);
	registerArchive(lua);
	registerArchiveEntry(lua);
	registerEntryType(lua);
	registerArchiveDir(lua);
}

} // namespace slade::lua
