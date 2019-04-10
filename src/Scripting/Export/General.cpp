
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Archive.cpp
// Description: Functions to export general/misc. types and namespaces to lua
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
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditContext.h"
#include "Scripting/Lua.h"
#include "thirdparty/sol/sol.hpp"


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace Lua
{
// -----------------------------------------------------------------------------
// Writes a log [message] of [type]
// -----------------------------------------------------------------------------
void logMessage(std::string_view message, Log::MessageType type = Log::MessageType::Script)
{
	Log::message(type, message);
}

// -----------------------------------------------------------------------------
// Switches to the tab for [archive], opening it if necessary
// -----------------------------------------------------------------------------
bool showArchive(Archive* archive)
{
	if (!archive)
		return false;

	MainEditor::openArchiveTab(archive);
	return true;
}

// -----------------------------------------------------------------------------
// Returns a string_view of a MemChunk's data
// -----------------------------------------------------------------------------
std::string_view memChunkData(MemChunk& mc)
{
	return std::string_view{ (char*)mc.data(), mc.size() };
}

template<typename T> sol::object memChunkRead(MemChunk& self, unsigned offset)
{
	// Need to check how slow sol::make_object is, if it's really slow it might be best
	// just to return 0 if the read fails, rather than nil
	T val;
	if (!self.read(offset, &val, sizeof(T)))
		return sol::make_object(Lua::state().lua_state(), sol::nil);
	return sol::make_object(Lua::state().lua_state(), val);
}

std::string memChunkReadString(MemChunk& self, unsigned offset, unsigned length, bool null_terminated)
{
	if (null_terminated)
	{
		for (unsigned p = offset; p < self.size(); ++p)
		{
			if (p == offset + length)
				break;

			if (self[p] == 0)
			{
				length = p - offset;
				break;
			}
		}
	}

	std::string str;
	str.resize(length, 0);
	return self.read(offset, str.data(), length) ? str : std::string{};
}

template<typename T> bool memChunkWrite(MemChunk& self, unsigned offset, T value, bool expand)
{
	return self.write(offset, &value, sizeof(T), expand);
}

// -----------------------------------------------------------------------------
// Registers the DataBlock (MemChunk) type with lua
// -----------------------------------------------------------------------------
void registerMemChunkType(sol::state& lua)
{
	auto lua_mc = lua.new_usertype<MemChunk>("DataBlock", sol::constructors<MemChunk(), MemChunk(uint32_t)>());

	// Properties
	// -------------------------------------------------------------------------
	lua_mc["size"] = sol::property(&MemChunk::size);
	lua_mc["crc"]  = sol::property(&MemChunk::crc);

	// Functions
	// -------------------------------------------------------------------------
	lua_mc["AsString"] = &memChunkData;
	lua_mc["SetData"]  = [](MemChunk& self, std::string_view data) {
        self.importMem((const uint8_t*)data.data(), data.size());
	};
	lua_mc["Clear"]  = &MemChunk::clear;
	lua_mc["Resize"] = &MemChunk::reSize;
	lua_mc["Copy"]   = sol::resolve<bool(const MemChunk&)>(&MemChunk::importMem);
	lua_mc["CopyTo"] = sol::overload(
		&MemChunk::exportMemChunk,
		[](MemChunk& self, MemChunk& mc) { return self.exportMemChunk(mc); },
		[](MemChunk& self, MemChunk& mc, int offset) { return self.exportMemChunk(mc, offset); });
	lua_mc["ImportFile"] = sol::overload(
		&MemChunk::importFile,
		[](MemChunk& self, std::string_view fn) { self.importFile(fn); },
		[](MemChunk& self, std::string_view fn, int offset) { self.importFile(fn, offset); });
	lua_mc["ExportFile"] = sol::overload(
		&MemChunk::exportFile,
		[](MemChunk& self, std::string_view fn) { self.exportFile(fn); },
		[](MemChunk& self, std::string_view fn, int offset) { self.exportFile(fn, offset); });
	lua_mc["FillData"]    = &MemChunk::fillData;
	lua_mc["WriteInt8"]   = &memChunkWrite<int8_t>;
	lua_mc["WriteUInt8"]  = &memChunkWrite<uint8_t>;
	lua_mc["WriteInt16"]  = &memChunkWrite<int16_t>;
	lua_mc["WriteUInt16"] = &memChunkWrite<uint16_t>;
	lua_mc["WriteInt32"]  = &memChunkWrite<int32_t>;
	lua_mc["WriteUInt32"] = &memChunkWrite<uint32_t>;
	lua_mc["WriteInt64"]  = &memChunkWrite<int64_t>;
	lua_mc["WriteUInt64"] = &memChunkWrite<uint64_t>;
	lua_mc["WriteString"] = [](MemChunk& self, int offset, std::string_view value, bool expand) {
		self.write(offset, value.data(), value.size(), expand);
	};
	lua_mc["ReadInt8"]   = &memChunkRead<int8_t>;
	lua_mc["ReadUInt8"]  = &memChunkRead<uint8_t>;
	lua_mc["ReadInt16"]  = &memChunkRead<int16_t>;
	lua_mc["ReadUInt16"] = &memChunkRead<uint16_t>;
	lua_mc["ReadInt32"]  = &memChunkRead<int32_t>;
	lua_mc["ReadUInt32"] = &memChunkRead<uint32_t>;
	lua_mc["ReadInt64"]  = &memChunkRead<int64_t>;
	lua_mc["ReadUInt64"] = &memChunkRead<uint64_t>;
	lua_mc["ReadString"] = sol::overload(&memChunkReadString, [](MemChunk& self, unsigned offset, unsigned length) {
		return memChunkReadString(self, offset, length, false);
	});
}

// -----------------------------------------------------------------------------
// Registers some misc. types with lua
// -----------------------------------------------------------------------------
void registerMiscTypes(sol::state& lua)
{
	// Point type
	auto lua_vec2f = lua.new_usertype<Vec2d>("Point", sol::constructors<Vec2d(), Vec2d(double, double)>());
	lua_vec2f["x"] = &Vec2d::x;
	lua_vec2f["y"] = &Vec2d::y;

	// Colour type
	auto lua_colour = lua.new_usertype<ColRGBA>(
		"Colour",
		sol::
			constructors<ColRGBA(), ColRGBA(uint8_t, uint8_t, uint8_t), ColRGBA(uint8_t, uint8_t, uint8_t, uint8_t)>());
	lua_colour["r"] = &ColRGBA::r;
	lua_colour["g"] = &ColRGBA::g;
	lua_colour["b"] = &ColRGBA::b;
	lua_colour["a"] = &ColRGBA::a;

	// Plane type
	auto lua_plane = lua.new_usertype<Plane>(
		"Plane", sol::constructors<Plane(), Plane(double, double, double, double)>());
	lua_plane["a"]        = &Plane::a;
	lua_plane["b"]        = &Plane::b;
	lua_plane["c"]        = &Plane::c;
	lua_plane["d"]        = &Plane::d;
	lua_plane["HeightAt"] = sol::resolve<double(Vec2d) const>(&Plane::heightAt);

	// MemChunk type
	registerMemChunkType(lua);
}

// -----------------------------------------------------------------------------
// Registers the App namespace with lua
// -----------------------------------------------------------------------------
void registerAppNamespace(sol::state& lua)
{
	auto app = lua.create_named_table("App");

	// Functions
	// -------------------------------------------------------------------------
	app["LogMessage"]            = [](std::string_view message) { logMessage(message, Log::MessageType::Script); };
	app["LogWarning"]            = [](std::string_view message) { logMessage(message, Log::MessageType::Warning); };
	app["LogError"]              = [](std::string_view message) { logMessage(message, Log::MessageType::Error); };
	app["CurrentArchive"]        = &MainEditor::currentArchive;
	app["CurrentEntry"]          = &MainEditor::currentEntry;
	app["CurrentEntrySelection"] = &MainEditor::currentEntrySelection;
	app["CurrentPalette"] = sol::overload(&MainEditor::currentPalette, []() { return MainEditor::currentPalette(); });
	app["ShowArchive"]    = &showArchive;
	app["ShowEntry"]      = &MainEditor::openEntry;
	app["MapEditor"]      = &MapEditor::editContext;
}

} // namespace Lua
