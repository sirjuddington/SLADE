
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
#include "Archive/Formats/All.h"
#include "Utility/StringUtils.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;

namespace slade::lua
{
// -----------------------------------------------------------------------------
// Returns a vector of all entries in the archive [self]
// -----------------------------------------------------------------------------
vector<shared_ptr<ArchiveEntry>> archiveAllEntries(Archive& self)
{
	vector<shared_ptr<ArchiveEntry>> list;
	self.putEntryTreeAsList(list);
	return list;
}

// -----------------------------------------------------------------------------
// Creates a new directory in archive [self] at [path].
// Returns the created directory or nullptr if the archive doesn't support them
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> archiveCreateDir(Archive& self, string_view path)
{
	if (self.formatDesc().supports_dirs)
		return self.createDir(path);
	else
		return {};
}

// -----------------------------------------------------------------------------
// Creates a new entry in archive [self] at [full_path],[position].
// Returns the created entry
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> archiveCreateEntry(Archive& self, string_view full_path, int position)
{
	auto dir = self.dirAtPath(strutil::beforeLast(full_path, '/'));
	return self.addNewEntry(strutil::afterLast(full_path, '/'), position, dir)->getShared();
}

// -----------------------------------------------------------------------------
// Creates a new entry in archive [self] with [name] in namespace [ns].
// Returns the created entry
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> archiveCreateEntryInNamespace(Archive& self, string_view name, string_view ns)
{
	return self.addNewEntry(name, ns)->getShared();
}

// -----------------------------------------------------------------------------
// Wrapper for Archive::findFirst that returns a shared pointer
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> archiveFindFirst(Archive& self, ArchiveSearchOptions& opt)
{
	auto found = self.findFirst(opt);
	return found ? found->getShared() : nullptr;
}

// -----------------------------------------------------------------------------
// Wrapper for Archive::findLast that returns a shared pointer
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> archiveFindLast(Archive& self, ArchiveSearchOptions& opt)
{
	auto found = self.findLast(opt);
	return found ? found->getShared() : nullptr;
}

// -----------------------------------------------------------------------------
// Wrapper for Archive::findAll that returns shared pointers
// -----------------------------------------------------------------------------
vector<shared_ptr<ArchiveEntry>> archiveFindAll(Archive& self, ArchiveSearchOptions& opt)
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
void registerArchive(sol::state& lua)
{
	// Create Archive type, no constructor
	auto lua_archive = lua.new_usertype<Archive>("Archive", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_archive["filename"] = sol::property([](Archive& self) { return self.filename(); });
	lua_archive["entries"]  = sol::property(&archiveAllEntries);
	lua_archive["rootDir"]  = sol::property(&Archive::rootDir);
	lua_archive["format"]   = sol::property(&Archive::formatDesc);

	// Functions
	// -------------------------------------------------------------------------
	lua_archive["FilenameNoPath"]         = [](Archive& self) { return self.filename(false); };
	lua_archive["EntryAtPath"]            = &Archive::entryAtPathShared;
	lua_archive["DirAtPath"]              = [](Archive& self, const string& path) { return self.dirAtPath(path); };
	lua_archive["CreateEntry"]            = &archiveCreateEntry;
	lua_archive["CreateEntryInNamespace"] = &archiveCreateEntryInNamespace;
	lua_archive["CreateDir"]              = &archiveCreateDir;
	lua_archive["RemoveEntry"]            = [](Archive& self, ArchiveEntry* entry) { return self.removeEntry(entry); };
	lua_archive["RenameEntry"]            = &Archive::renameEntry;
	lua_archive["Save"]                   = sol::overload(
        [](Archive& self) { return std::make_tuple(self.save(), global::error); },
        [](Archive& self, const string& filename) { return std::make_tuple(self.save(filename), global::error); });
	lua_archive["FindFirst"] = &archiveFindFirst;
	lua_archive["FindLast"]  = &archiveFindLast;
	lua_archive["FindAll"]   = &archiveFindAll;

	// Register all subclasses
	// (perhaps it'd be a good idea to make Archive not abstract and handle
	//  the format-specific stuff somewhere else, rather than in subclasses)
#define REGISTER_ARCHIVE(type) lua.new_usertype<type>(#type, sol::base_classes, sol::bases<Archive>())
	REGISTER_ARCHIVE(WadArchive);
	REGISTER_ARCHIVE(ZipArchive);
	REGISTER_ARCHIVE(LibArchive);
	REGISTER_ARCHIVE(DatArchive);
	REGISTER_ARCHIVE(ResArchive);
	REGISTER_ARCHIVE(PakArchive);
	REGISTER_ARCHIVE(BSPArchive);
	REGISTER_ARCHIVE(GrpArchive);
	REGISTER_ARCHIVE(RffArchive);
	REGISTER_ARCHIVE(GobArchive);
	REGISTER_ARCHIVE(LfdArchive);
	REGISTER_ARCHIVE(HogArchive);
	REGISTER_ARCHIVE(ADatArchive);
	REGISTER_ARCHIVE(Wad2Archive);
	REGISTER_ARCHIVE(WadJArchive);
	REGISTER_ARCHIVE(WolfArchive);
	REGISTER_ARCHIVE(GZipArchive);
	REGISTER_ARCHIVE(BZip2Archive);
	REGISTER_ARCHIVE(TarArchive);
	REGISTER_ARCHIVE(DiskArchive);
	REGISTER_ARCHIVE(PodArchive);
	REGISTER_ARCHIVE(ChasmBinArchive);
#undef REGISTER_ARCHIVE
}
} // namespace slade::lua
