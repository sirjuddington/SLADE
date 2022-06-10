
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archive.cpp
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
#include "Archive/Archive.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/All.h"
#include "General/Misc.h"
#include "Library/Library.h"
#include "Utility/StringUtils.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::lua
{
// -----------------------------------------------------------------------------
// Returns the name of entry [self] with requested formatting:
// [include_path] - if true, include the path to the entry
// [include_extension] - if true, include the extension
// [name_uppercase] - if true, return the name in uppercase (except the path)
// -----------------------------------------------------------------------------
string formattedEntryName(const ArchiveEntry& self, bool include_path, bool include_extension, bool name_uppercase)
{
	string name;
	if (include_path)
		name = self.path();
	if (name_uppercase)
		name.append(include_extension ? self.upperName() : self.upperNameNoExt());
	else
		name.append(include_extension ? self.name() : self.nameNoExt());
	return name;
}

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
shared_ptr<ArchiveEntry> archiveFindFirst(Archive& self, Archive::SearchOptions& opt)
{
	auto found = self.findFirst(opt);
	return found ? found->getShared() : nullptr;
}

// -----------------------------------------------------------------------------
// Wrapper for Archive::findLast that returns a shared pointer
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> archiveFindLast(Archive& self, Archive::SearchOptions& opt)
{
	auto found = self.findLast(opt);
	return found ? found->getShared() : nullptr;
}

// -----------------------------------------------------------------------------
// Wrapper for Archive::findAll that returns shared pointers
// -----------------------------------------------------------------------------
vector<shared_ptr<ArchiveEntry>> archiveFindAll(Archive& self, Archive::SearchOptions& opt)
{
	auto found = self.findAll(opt);

	vector<shared_ptr<ArchiveEntry>> found_shared;
	found_shared.reserve(found.size());
	for (const auto& entry : found)
		found_shared.push_back(entry->getShared());

	return found_shared;
}

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
	auto lua_search_opt = lua.new_usertype<Archive::SearchOptions>(
		"ArchiveSearchOptions", "new", sol::constructors<Archive::SearchOptions()>());

	// Properties
	// -------------------------------------------------------------------------
	lua_search_opt["matchName"]      = sol::property(&Archive::SearchOptions::match_name);
	lua_search_opt["matchType"]      = sol::property(&Archive::SearchOptions::match_type);
	lua_search_opt["matchNamespace"] = sol::property(&Archive::SearchOptions::match_namespace);
	lua_search_opt["dir"]            = sol::property(&Archive::SearchOptions::dir);
	lua_search_opt["ignoreExt"]      = sol::property(&Archive::SearchOptions::ignore_ext);
	lua_search_opt["searchSubdirs"]  = sol::property(&Archive::SearchOptions::search_subdirs);
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

// -----------------------------------------------------------------------------
// Imports data from [string] into entry [self]
// -----------------------------------------------------------------------------
std::tuple<bool, string> entryImportString(ArchiveEntry& self, const string& string)
{
	return std::make_tuple(self.importMem(string.data(), string.size()), global::error);
}

// -----------------------------------------------------------------------------
// Imports data from [mc] into entry [self]
// -----------------------------------------------------------------------------
std::tuple<bool, string> entryImportMC(ArchiveEntry& self, MemChunk& mc)
{
	return std::make_tuple(self.importMemChunk(mc), global::error);
}

// -----------------------------------------------------------------------------
// Renames entry [self] to [new_name], using the parent archive's naming rules
// if it has one
// -----------------------------------------------------------------------------
bool entryRename(ArchiveEntry& self, string_view new_name)
{
	if (self.parent())
		return self.parent()->renameEntry(&self, new_name);
	else
		return self.rename(new_name);
}

// -----------------------------------------------------------------------------
// Returns entry [self]'s parent archive as a shared pointer if possible
// -----------------------------------------------------------------------------
shared_ptr<Archive> entryParent(ArchiveEntry& self)
{
	return app::archiveManager().shareArchive(self.parent());
}

// -----------------------------------------------------------------------------
// Returns entry [self]'s parent directory as a shared pointer if possible
// -----------------------------------------------------------------------------
shared_ptr<ArchiveDir> entryDir(ArchiveEntry& self)
{
	if (self.parentDir())
		return ArchiveDir::getShared(self.parentDir());

	return nullptr;
}

// -----------------------------------------------------------------------------
// Registers the ArchiveEntry type with lua
// -----------------------------------------------------------------------------
void registerArchiveEntry(sol::state& lua)
{
	// Create ArchiveEntry type, no constructor
	auto lua_entry = lua.new_usertype<ArchiveEntry>("ArchiveEntry", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_entry["name"]  = sol::property([](ArchiveEntry& self) { return self.name(); });
	lua_entry["path"]  = sol::property([](ArchiveEntry& self) { return self.path(); });
	lua_entry["type"]  = sol::property(&ArchiveEntry::type);
	lua_entry["size"]  = sol::property(&ArchiveEntry::size);
	lua_entry["index"] = sol::property(&ArchiveEntry::index);
	lua_entry["crc32"] = sol::property([](ArchiveEntry& self) { return misc::crc(self.rawData(), self.size()); });
	lua_entry["data"]  = sol::property([](ArchiveEntry& self) { return &self.data(); });
	lua_entry["parentArchive"] = sol::property(&entryParent);
	lua_entry["parentDir"]     = sol::property(&entryDir);

	// Functions
	// -------------------------------------------------------------------------
	lua_entry["FormattedName"] = sol::overload(
		[](ArchiveEntry& self) { return formattedEntryName(self, true, true, false); },
		[](ArchiveEntry& self, bool include_path) { return formattedEntryName(self, include_path, true, false); },
		[](ArchiveEntry& self, bool include_path, bool include_extension)
		{ return formattedEntryName(self, include_path, include_extension, false); },
		&formattedEntryName);
	lua_entry["FormattedSize"] = &ArchiveEntry::sizeString;
	lua_entry["ImportFile"]    = [](ArchiveEntry& self, string_view filename)
	{ return std::make_tuple(self.importFile(filename), global::error); };
	lua_entry["ImportEntry"] = [](ArchiveEntry& self, ArchiveEntry* entry)
	{ return std::make_tuple(self.importEntry(entry), global::error); };
	lua_entry["ImportData"] = sol::overload(&entryImportString, &entryImportMC);
	lua_entry["ExportFile"] = [](ArchiveEntry& self, string_view filename)
	{ return std::make_tuple(self.exportFile(filename), global::error); };
	lua_entry["Rename"] = &entryRename;
}

// -----------------------------------------------------------------------------
// Registers the ArchiveDir type with lua
// -----------------------------------------------------------------------------
void registerArchiveTreeNode(sol::state& lua)
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
// Registers the EntryType type with lua
// -----------------------------------------------------------------------------
void registerEntryType(sol::state& lua)
{
	// Create EntryType type, no constructor
	auto lua_etype = lua.new_usertype<EntryType>("EntryType", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_etype["id"]        = sol::property(&EntryType::id);
	lua_etype["name"]      = sol::property(&EntryType::name);
	lua_etype["extension"] = sol::property(&EntryType::extension);
	lua_etype["formatId"]  = sol::property(&EntryType::formatId);
	lua_etype["editor"]    = sol::property(&EntryType::editor);
	lua_etype["category"]  = sol::property(&EntryType::category);
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
	archives["RecentFiles"]          = []() { return library::recentFiles(); };
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
	registerArchiveTreeNode(lua);
}

} // namespace slade::lua
