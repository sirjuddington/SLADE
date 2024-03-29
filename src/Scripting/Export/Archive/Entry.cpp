
#include "Main.h"

#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Misc.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;

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
	lua_entry.set("name", sol::property([](ArchiveEntry& self) { return self.name(); }));
	lua_entry.set("path", sol::property([](ArchiveEntry& self) { return self.path(); }));
	lua_entry.set("type", sol::property(&ArchiveEntry::type));
	lua_entry.set("size", sol::property(&ArchiveEntry::size));
	lua_entry.set("index", sol::property(&ArchiveEntry::index));
	lua_entry.set("crc32", sol::property([](ArchiveEntry& self) { return misc::crc(self.rawData(), self.size()); }));
	lua_entry.set("data", sol::property([](ArchiveEntry& self) { return &self.data(); }));
	lua_entry.set("parentArchive", sol::property(&entryParent));
	lua_entry.set("parentDir", sol::property(&entryDir));

	// Functions
	// -------------------------------------------------------------------------
	lua_entry.set_function(
		"FormattedName",
		sol::overload(
			[](ArchiveEntry& self) { return formattedEntryName(self, true, true, false); },
			[](ArchiveEntry& self, bool include_path) { return formattedEntryName(self, include_path, true, false); },
			[](ArchiveEntry& self, bool include_path, bool include_extension)
			{ return formattedEntryName(self, include_path, include_extension, false); },
			&formattedEntryName));
	lua_entry.set_function("FormattedSize", &ArchiveEntry::sizeString);
	lua_entry.set_function(
		"ImportFile",
		[](ArchiveEntry& self, string_view filename)
		{ return std::make_tuple(self.importFile(filename), global::error); });
	lua_entry.set_function(
		"ImportEntry",
		[](ArchiveEntry& self, ArchiveEntry* entry)
		{ return std::make_tuple(self.importEntry(entry), global::error); });
	lua_entry.set_function("ImportData", sol::overload(&entryImportString, &entryImportMC));
	lua_entry.set_function(
		"ExportFile",
		[](ArchiveEntry& self, string_view filename)
		{ return std::make_tuple(self.exportFile(filename), global::error); });
	lua_entry.set_function("Rename", &entryRename);
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
	lua_etype.set("id", sol::property(&EntryType::id));
	lua_etype.set("name", sol::property(&EntryType::name));
	lua_etype.set("extension", sol::property(&EntryType::extension));
	lua_etype.set("formatId", sol::property(&EntryType::formatId));
	lua_etype.set("editor", sol::property(&EntryType::editor));
	lua_etype.set("category", sol::property(&EntryType::category));
}
} // namespace slade::lua
