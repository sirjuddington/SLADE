
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Graphics/Palette/Palette.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "Scripting/Lua.h"
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
// Writes a log [message] of [type]
// -----------------------------------------------------------------------------
void logMessage(string_view message, log::MessageType type = log::MessageType::Script)
{
	log::message(type, message);
}

// -----------------------------------------------------------------------------
// Switches to the tab for [archive], opening it if necessary
// -----------------------------------------------------------------------------
bool showArchive(Archive* archive)
{
	if (!archive)
		return false;

	maineditor::openArchiveTab(archive);
	return true;
}

// -----------------------------------------------------------------------------
// Returns a string_view of a MemChunk's data
// -----------------------------------------------------------------------------
string_view memChunkData(MemChunk& mc)
{
	return string_view{ reinterpret_cast<char*>(mc.data()), mc.size() };
}

// -----------------------------------------------------------------------------
// Reads data at [offset] in [self] as the type T.
// Returns a sol::object of either the value read or sol::nil if reading failed
// -----------------------------------------------------------------------------
template<typename T> sol::object memChunkRead(MemChunk& self, unsigned offset)
{
	// Need to check how slow sol::make_object is, if it's really slow it might be best
	// just to return 0 if the read fails, rather than nil
	T val;
	if (!self.read(offset, &val, sizeof(T)))
		return sol::make_object(lua::state().lua_state(), sol::lua_nil);
	return sol::make_object(lua::state().lua_state(), val);
}

// -----------------------------------------------------------------------------
// Reads a string in [self] beginning at [offset] up to [length] characters.
// If [null_terminated] is true, the read string will terminate at the first
// null (\0) character found after [offset] (but before [offset]+[length])
// -----------------------------------------------------------------------------
string memChunkReadString(MemChunk& self, unsigned offset, unsigned length, bool null_terminated)
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

	string str;
	str.resize(length, 0);
	return self.read(offset, str.data(), length) ? str : string{};
}

// -----------------------------------------------------------------------------
// Writes data of the type T at [offset] in [self].
// If [expand] is true, the MemChunk will be expanded if the written data goes
// past the end of the chunk
// -----------------------------------------------------------------------------
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
	lua_mc["SetData"]  = [](MemChunk& self, string_view data)
	{ self.importMem(reinterpret_cast<const uint8_t*>(data.data()), data.size()); };
	lua_mc["Clear"]  = &MemChunk::clear;
	lua_mc["Resize"] = &MemChunk::reSize;
	lua_mc["Copy"]   = sol::resolve<bool(const MemChunk&)>(&MemChunk::importMem);
	lua_mc["CopyTo"] = sol::overload(
		&MemChunk::exportMemChunk,
		[](MemChunk& self, MemChunk& mc) { return self.exportMemChunk(mc); },
		[](MemChunk& self, MemChunk& mc, int offset) { return self.exportMemChunk(mc, offset); });
	lua_mc["ImportFile"] = sol::overload(
		&MemChunk::importFile,
		[](MemChunk& self, string_view fn) { self.importFile(fn); },
		[](MemChunk& self, string_view fn, int offset) { self.importFile(fn, offset); });
	lua_mc["ExportFile"] = sol::overload(
		&MemChunk::exportFile,
		[](MemChunk& self, string_view fn) { self.exportFile(fn); },
		[](MemChunk& self, string_view fn, int offset) { self.exportFile(fn, offset); });
	lua_mc["FillData"]    = &MemChunk::fillData;
	lua_mc["WriteInt8"]   = &memChunkWrite<int8_t>;
	lua_mc["WriteUInt8"]  = &memChunkWrite<uint8_t>;
	lua_mc["WriteInt16"]  = &memChunkWrite<int16_t>;
	lua_mc["WriteUInt16"] = &memChunkWrite<uint16_t>;
	lua_mc["WriteInt32"]  = &memChunkWrite<int32_t>;
	lua_mc["WriteUInt32"] = &memChunkWrite<uint32_t>;
	lua_mc["WriteInt64"]  = &memChunkWrite<int64_t>;
	lua_mc["WriteUInt64"] = &memChunkWrite<uint64_t>;
	lua_mc["WriteString"] = [](MemChunk& self, int offset, string_view value, bool expand)
	{ self.write(offset, value.data(), value.size(), expand); };
	lua_mc["ReadInt8"]   = &memChunkRead<int8_t>;
	lua_mc["ReadUInt8"]  = &memChunkRead<uint8_t>;
	lua_mc["ReadInt16"]  = &memChunkRead<int16_t>;
	lua_mc["ReadUInt16"] = &memChunkRead<uint16_t>;
	lua_mc["ReadInt32"]  = &memChunkRead<int32_t>;
	lua_mc["ReadUInt32"] = &memChunkRead<uint32_t>;
	lua_mc["ReadInt64"]  = &memChunkRead<int64_t>;
	lua_mc["ReadUInt64"] = &memChunkRead<uint64_t>;
	lua_mc["ReadString"] = sol::overload(
		&memChunkReadString,
		[](MemChunk& self, unsigned offset, unsigned length)
		{ return memChunkReadString(self, offset, length, false); });
}

// -----------------------------------------------------------------------------
// Returns the HSL components of the colour [self]
// -----------------------------------------------------------------------------
std::tuple<double, double, double> colourAsHSL(ColRGBA& self)
{
	auto hsl = self.asHSL();
	return std::make_tuple(hsl.h, hsl.s, hsl.l);
}

// -----------------------------------------------------------------------------
// Returns the LAB components of the colour [self]
// -----------------------------------------------------------------------------
std::tuple<double, double, double> colourAsLAB(ColRGBA& self)
{
	auto lab = self.asLAB();
	return std::make_tuple(lab.l, lab.a, lab.b);
}

// -----------------------------------------------------------------------------
// Registers the Colour (ColRGBA) type with lua
// -----------------------------------------------------------------------------
void registerColourType(sol::state& lua)
{
	auto lua_colour = lua.new_usertype<ColRGBA>(
		"Colour", sol::constructors<ColRGBA(), ColRGBA(u8, u8, u8), ColRGBA(u8, u8, u8, u8)>());

	// Constants
	// -------------------------------------------------------------------------
	lua_colour["FORMAT_RGB"]   = sol::property([]() { return ColRGBA::StringFormat::RGB; });
	lua_colour["FORMAT_RGBA"]  = sol::property([]() { return ColRGBA::StringFormat::RGBA; });
	lua_colour["FORMAT_HEX"]   = sol::property([]() { return ColRGBA::StringFormat::HEX; });
	lua_colour["FORMAT_ZDOOM"] = sol::property([]() { return ColRGBA::StringFormat::ZDoom; });

	// Properties
	// -------------------------------------------------------------------------
	lua_colour["r"]  = &ColRGBA::r;
	lua_colour["g"]  = &ColRGBA::g;
	lua_colour["b"]  = &ColRGBA::b;
	lua_colour["a"]  = &ColRGBA::a;
	lua_colour["fr"] = sol::property(&ColRGBA::fr);
	lua_colour["fg"] = sol::property(&ColRGBA::fg);
	lua_colour["fb"] = sol::property(&ColRGBA::fb);
	lua_colour["fa"] = sol::property(&ColRGBA::fa);

	// Functions
	// -------------------------------------------------------------------------
	lua_colour["AsHSL"]    = &colourAsHSL;
	lua_colour["AsLAB"]    = &colourAsLAB;
	lua_colour["AsString"] = &ColRGBA::toString;
	lua_colour["FromHSL"]  = sol::resolve<void(double, double, double)>(&ColRGBA::fromHSL);
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
	registerColourType(lua);

	// Plane type
	auto lua_plane = lua.new_usertype<Plane>(
		"Plane", sol::constructors<Plane(), Plane(double, double, double, double)>());
	lua_plane["a"]        = &Plane::a;
	lua_plane["b"]        = &Plane::b;
	lua_plane["c"]        = &Plane::c;
	lua_plane["d"]        = &Plane::d;
	lua_plane["HeightAt"] = sol::resolve<double(const Vec2d&) const>(&Plane::heightAt);

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
	app["LogMessage"]            = [](string_view message) { logMessage(message, log::MessageType::Script); };
	app["LogWarning"]            = [](string_view message) { logMessage(message, log::MessageType::Warning); };
	app["LogError"]              = [](string_view message) { logMessage(message, log::MessageType::Error); };
	app["CurrentArchive"]        = &maineditor::currentArchive;
	app["CurrentEntry"]          = &maineditor::currentEntry;
	app["CurrentEntrySelection"] = &maineditor::currentEntrySelection;
	app["CurrentPalette"] = sol::overload(&maineditor::currentPalette, []() { return maineditor::currentPalette(); });
	app["ShowArchive"]    = &showArchive;
	app["ShowEntry"]      = &maineditor::openEntry;
	app["MapEditor"]      = &mapeditor::editContext;
}

} // namespace slade::lua
