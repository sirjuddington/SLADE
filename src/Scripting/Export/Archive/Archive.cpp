
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archive.cpp
// Description: Functions to export Archive and its subclasses to lua using sol3
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
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Scripting/Export/Export.h"
#include "Scripting/LuaBridge.h"
#include "Utility/StringUtils.h"

using namespace slade;

namespace slade::lua
{
// -----------------------------------------------------------------------------
// Returns a vector of all entries in the archive [self]
// -----------------------------------------------------------------------------
static vector<shared_ptr<ArchiveEntry>> archiveAllEntries(const Archive& self)
{
	vector<shared_ptr<ArchiveEntry>> list;
	self.putEntryTreeAsList(list);
	return list;
}

// -----------------------------------------------------------------------------
// Creates a new directory in archive [self] at [path].
// Returns the created directory or nullptr if the archive doesn't support them
// -----------------------------------------------------------------------------
static shared_ptr<ArchiveDir> archiveCreateDir(Archive& self, string_view path)
{
	if (self.formatInfo().supports_dirs)
		return self.createDir(path);
	else
		return {};
}

// -----------------------------------------------------------------------------
// Creates a new entry in archive [self] at [full_path],[position].
// Returns the created entry
// -----------------------------------------------------------------------------
static shared_ptr<ArchiveEntry> archiveCreateEntry(Archive& self, string_view full_path, int position)
{
	auto dir = self.dirAtPath(strutil::beforeLast(full_path, '/'));
	return self.addNewEntry(strutil::afterLast(full_path, '/'), position, dir)->getShared();
}

// -----------------------------------------------------------------------------
// Creates a new entry in archive [self] with [name] in namespace [ns].
// Returns the created entry
// -----------------------------------------------------------------------------
static shared_ptr<ArchiveEntry> archiveCreateEntryInNamespace(Archive& self, string_view name, string_view ns)
{
	return self.addNewEntry(name, ns)->getShared();
}

// -----------------------------------------------------------------------------
// Wrapper for Archive::findFirst that returns a shared pointer
// -----------------------------------------------------------------------------
static shared_ptr<ArchiveEntry> archiveFindFirst(Archive& self, ArchiveSearchOptions& opt)
{
	auto found = self.findFirst(opt);
	return found ? found->getShared() : nullptr;
}

// -----------------------------------------------------------------------------
// Wrapper for Archive::findLast that returns a shared pointer
// -----------------------------------------------------------------------------
static shared_ptr<ArchiveEntry> archiveFindLast(Archive& self, ArchiveSearchOptions& opt)
{
	auto found = self.findLast(opt);
	return found ? found->getShared() : nullptr;
}

// -----------------------------------------------------------------------------
// Wrapper for Archive::findAll that returns shared pointers
// -----------------------------------------------------------------------------
static vector<shared_ptr<ArchiveEntry>> archiveFindAll(Archive& self, ArchiveSearchOptions& opt)
{
	auto found = self.findAll(opt);

	vector<shared_ptr<ArchiveEntry>> found_shared;
	found_shared.reserve(found.size());
	for (const auto& entry : found)
		found_shared.push_back(entry->getShared());

	return found_shared;
}

// -----------------------------------------------------------------------------
// Registers the Archive type with lua
// -----------------------------------------------------------------------------
void registerArchive(lua_State* lua)
{
	// Create Archive type, no constructor
	auto lua_archive = luabridge::getGlobalNamespace(lua).beginClass<Archive>("Archive", luabridge::visibleMetatables);
	lua_archive.addConstructorFrom<shared_ptr<Archive>, void(string_view)>();

	// Properties
	// -------------------------------------------------------------------------
	lua_archive.addProperty("filename", [](const Archive& self) { return self.filename(); });
	lua_archive.addProperty("entries", archiveAllEntries);
	lua_archive.addProperty("rootDir", &Archive::rootDir);
	lua_archive.addProperty("format", &Archive::formatInfo);

	// Functions
	// -------------------------------------------------------------------------
	lua_archive.addFunction("FilenameNoPath", [](const Archive& self) { return self.filename(false); });
	lua_archive.addFunction("EntryAtPath", &Archive::entryAtPathShared);
	lua_archive.addFunction("DirAtPath", [](const Archive& self, const string& path) { return self.dirAtPath(path); });
	lua_archive.addFunction("CreateEntry", &archiveCreateEntry);
	lua_archive.addFunction("CreateEntryInNamespace", &archiveCreateEntryInNamespace);
	lua_archive.addFunction("CreateDir", &archiveCreateDir);
	lua_archive.addFunction("RemoveEntry", [](Archive& self, ArchiveEntry* entry) { return self.removeEntry(entry); });
	lua_archive.addFunction("RenameEntry", &Archive::renameEntry);
	lua_archive.addFunction(
		"Save",
		[](Archive& self) { return std::make_tuple(self.save(), global::error); },
		[](Archive& self, const string& filename) { return std::make_tuple(self.save(filename), global::error); });
	lua_archive.addFunction("FindFirst", &archiveFindFirst);
	lua_archive.addFunction("FindLast", &archiveFindLast);
	lua_archive.addFunction("FindAll", &archiveFindAll);
}
} // namespace slade::lua
