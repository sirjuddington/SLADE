
#include "Main.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Misc.h"
#include "Scripting/Export/Export.h"
#include "Scripting/LuaBridge.h"

using namespace slade;

namespace slade::lua
{
// -----------------------------------------------------------------------------
// Returns the name of entry [self] with requested formatting:
// [include_path] - if true, include the path to the entry
// [include_extension] - if true, include the extension
// [name_uppercase] - if true, return the name in uppercase (except the path)
// -----------------------------------------------------------------------------
static string formattedEntryName(
	const ArchiveEntry& self,
	bool                include_path,
	bool                include_extension,
	bool                name_uppercase)
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
static std::tuple<bool, string> entryImportString(ArchiveEntry& self, const string& string)
{
	return std::make_tuple(self.importMem(string.data(), string.size()), global::error);
}

// -----------------------------------------------------------------------------
// Imports data from [mc] into entry [self]
// -----------------------------------------------------------------------------
static std::tuple<bool, string> entryImportMC(ArchiveEntry& self, MemChunk& mc)
{
	return std::make_tuple(self.importMemChunk(mc), global::error);
}

// -----------------------------------------------------------------------------
// Renames entry [self] to [new_name], using the parent archive's naming rules
// if it has one
// -----------------------------------------------------------------------------
static bool entryRename(ArchiveEntry& self, string_view new_name)
{
	if (self.parent())
		return self.parent()->renameEntry(&self, new_name);
	else
		return self.rename(new_name);
}

// -----------------------------------------------------------------------------
// Registers the ArchiveEntry type with lua
// -----------------------------------------------------------------------------
void registerArchiveEntry(lua_State* lua)
{
	// Create ArchiveEntry type, no constructor
	auto lua_entry = luabridge::getGlobalNamespace(lua).beginClass<ArchiveEntry>("ArchiveEntry");

	// Properties
	// -------------------------------------------------------------------------
	lua_entry.addProperty("name", &ArchiveEntry::name);
	lua_entry.addProperty("path", [](ArchiveEntry& self) { return self.path(); });
	lua_entry.addProperty("type", &ArchiveEntry::type);
	lua_entry.addProperty("size", &ArchiveEntry::size);
	lua_entry.addProperty("index", [](ArchiveEntry& self) { return self.index(); });
	lua_entry.addProperty("crc32", [](ArchiveEntry& self) { return misc::crc(self.rawData(), self.size()); });
	lua_entry.addProperty("data", &ArchiveEntry::data);
	lua_entry.addProperty(
		"parentArchive", [](ArchiveEntry& self) { return app::archiveManager().shareArchive(self.parent()); });
	lua_entry.addProperty(
		"parentDir",
		[](ArchiveEntry& self) { return self.parentDir() ? ArchiveDir::getShared(self.parentDir()) : nullptr; });

	// Functions
	// -------------------------------------------------------------------------
	lua_entry.addFunction(
		"FormattedName",
		[](ArchiveEntry& self) { return formattedEntryName(self, true, true, false); },
		[](ArchiveEntry& self, bool include_path) { return formattedEntryName(self, include_path, true, false); },
		[](ArchiveEntry& self, bool include_path, bool include_extension)
		{ return formattedEntryName(self, include_path, include_extension, false); },
		&formattedEntryName);
	lua_entry.addFunction("FormattedSize", &ArchiveEntry::sizeString);
	lua_entry.addFunction(
		"ImportFile",
		[](ArchiveEntry& self, string_view filename)
		{ return std::make_tuple(self.importFile(filename), global::error); });
	lua_entry.addFunction(
		"ImportEntry",
		[](ArchiveEntry& self, ArchiveEntry* entry)
		{ return std::make_tuple(self.importEntry(entry), global::error); });
	lua_entry.addFunction("ImportData", &entryImportString, &entryImportMC);
	lua_entry.addFunction(
		"ExportFile",
		[](ArchiveEntry& self, string_view filename)
		{ return std::make_tuple(self.exportFile(filename), global::error); });
	lua_entry.addFunction("Rename", &entryRename);
}

// -----------------------------------------------------------------------------
// Registers the EntryType type with lua
// -----------------------------------------------------------------------------
void registerEntryType(lua_State* lua)
{
	// Create EntryType type, no constructor
	auto lua_etype = luabridge::getGlobalNamespace(lua).beginClass<EntryType>("EntryType");

	// Properties
	// -------------------------------------------------------------------------
	lua_etype.addProperty("id", &EntryType::id);
	lua_etype.addProperty("name", &EntryType::name);
	lua_etype.addProperty("extension", &EntryType::extension);
	lua_etype.addProperty("format", &EntryType::formatId);
	lua_etype.addProperty("editor", &EntryType::editor);
	lua_etype.addProperty("category", &EntryType::category);
}
} // namespace slade::lua
