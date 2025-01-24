
#include "Main.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "General/Misc.h"
#include "Scripting/Export/Export.h"
#include "Scripting/Lua.h"
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
static std::tuple<bool, string> entryImportMC(ArchiveEntry& self, const MemChunk& mc)
{
	return std::make_tuple(self.importMemChunk(mc), global::error);
}

static int entryImportDataLua(lua_State* L)
{
	try
	{
		auto entry = luabridge::get<ArchiveEntry*>(L, 1).value();

		// Second parameter can be either string or MemChunk (DataBlock)
		string_view     data_string;
		const MemChunk* data_mc = nullptr;
		try
		{
			data_string = luabridge::get<string_view>(L, 2).value();
		}
		catch (const std::exception&)
		{
			data_mc = luabridge::get<const MemChunk*>(L, 2).value();
		}

		bool result;
		if (data_mc)
			result = entry->importMemChunk(*data_mc);
		else
			result = entry->importMem(data_string.data(), data_string.size());

		luabridge::push(L, result).throw_on_error();
		luabridge::push(L, global::error).throw_on_error();

		return 2;
	}
	catch (const std::exception& e)
	{
		throw LuaException("Runtime", fmt::format("Error in ArchiveEntry.ImportData: {}", e.what()));
	}
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

static int entryImportFileLua(lua_State* L)
{
	try
	{
		auto entry  = luabridge::get<ArchiveEntry*>(L, 1).value();
		auto file   = luabridge::get<string_view>(L, 2).value();
		auto result = entry->importFile(file);
		luabridge::push(L, result).throw_on_error();
		luabridge::push(L, global::error).throw_on_error();
		return 2;
	}
	catch (const std::exception& e)
	{
		throw LuaException("Runtime", fmt::format("Error in ArchiveEntry.ImportFile: {}", e.what()));
	}
}

static int entryImportEntryLua(lua_State* L)
{
	try
	{
		auto entry  = luabridge::get<ArchiveEntry*>(L, 1).value();
		auto entry2 = luabridge::get<ArchiveEntry*>(L, 2).value();
		auto result = entry->importEntry(entry2);
		luabridge::push(L, result).throw_on_error();
		luabridge::push(L, global::error).throw_on_error();
		return 2;
	}
	catch (const std::exception& e)
	{
		throw LuaException("Runtime", fmt::format("Error in ArchiveEntry.ImportEntry: {}", e.what()));
	}
}

static int entryExportFileLua(lua_State* L)
{
	try
	{
		auto entry    = luabridge::get<ArchiveEntry*>(L, 1).value();
		auto filename = luabridge::get<string_view>(L, 2).value();
		auto result   = entry->exportFile(filename);
		luabridge::push(L, result).throw_on_error();
		luabridge::push(L, global::error).throw_on_error();
		return 2;
	}
	catch (const std::exception& e)
	{
		throw LuaException("Runtime", fmt::format("Error in ArchiveEntry.ExportFile: {}", e.what()));
	}
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
	lua_entry.addProperty("path", [](const ArchiveEntry& self) { return self.path(); });
	lua_entry.addProperty("type", &ArchiveEntry::type);
	lua_entry.addProperty("size", &ArchiveEntry::size);
	lua_entry.addProperty("index", [](ArchiveEntry& self) { return self.index(); });
	lua_entry.addProperty("crc32", [](const ArchiveEntry& self) { return misc::crc(self.rawData(), self.size()); });
	lua_entry.addProperty("data", &ArchiveEntry::data);
	lua_entry.addProperty(
		"parentArchive", [](const ArchiveEntry& self) { return app::archiveManager().shareArchive(self.parent()); });
	lua_entry.addProperty(
		"parentDir",
		[](const ArchiveEntry& self) { return self.parentDir() ? ArchiveDir::getShared(self.parentDir()) : nullptr; });

	// Functions
	// -------------------------------------------------------------------------
	lua_entry.addFunction(
		"FormattedName",
		[](const ArchiveEntry& self) { return formattedEntryName(self, true, true, false); },
		[](const ArchiveEntry& self, bool include_path) { return formattedEntryName(self, include_path, true, false); },
		[](const ArchiveEntry& self, bool include_path, bool include_extension)
		{ return formattedEntryName(self, include_path, include_extension, false); },
		&formattedEntryName);
	lua_entry.addFunction("FormattedSize", &ArchiveEntry::sizeString);
	lua_entry.addFunction("ImportFile", &entryImportFileLua);
	lua_entry.addFunction("ImportEntry", &entryImportEntryLua);
	lua_entry.addFunction("ImportData", &entryImportDataLua); //, &entryImportMCLua);
	lua_entry.addFunction("ExportFile", &entryExportFileLua);
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

	// lua_etype.addStaticFunction("Get", &EntryType::fromId); // Maybe replace Archives.EntryType with this?
}
} // namespace slade::lua
