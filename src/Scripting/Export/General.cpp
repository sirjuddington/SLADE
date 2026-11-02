
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Export.h"
#include "Geometry/Plane.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapEditor.h"
#include "Scripting/LuaBridge.h"
#include "Scripting/Scripting.h"
#include "Utility/Colour.h"

using namespace slade;

// ReSharper disable CppParameterMayBeConstPtrOrRef


// -----------------------------------------------------------------------------
//
// Scripting Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::scripting
{
// -----------------------------------------------------------------------------
// Writes a log [message] of [type]
// -----------------------------------------------------------------------------
static void logMessage(string_view message, log::MessageType type = log::MessageType::Script)
{
	log::message(type, message);
}

// -----------------------------------------------------------------------------
// Switches to the tab for [archive], opening it if necessary
// -----------------------------------------------------------------------------
static bool showArchive(const Archive* archive)
{
	if (!archive)
		return false;

	maineditor::openArchiveTab(archive);
	return true;
}

// -----------------------------------------------------------------------------
// Returns a string_view of a MemChunk's data
// -----------------------------------------------------------------------------
static string_view memChunkData(const MemChunk& mc)
{
	return string_view{ reinterpret_cast<const char*>(mc.data()), mc.size() };
}

// -----------------------------------------------------------------------------
// Reads data at [offset] in [self] as the type T.
// Returns a sol::object of either the value read or sol::nil if reading failed
// -----------------------------------------------------------------------------
template<typename T> static std::optional<T> memChunkRead(const MemChunk& self, unsigned offset)
{
	// Need to check how slow sol::make_object is, if it's really slow it might be best
	// just to return 0 if the read fails, rather than nil
	T val;
	if (!self.read(offset, &val, sizeof(T)))
		return {};
	return val;
}

// -----------------------------------------------------------------------------
// Reads a string in [self] beginning at [offset] up to [length] characters.
// If [null_terminated] is true, the read string will terminate at the first
// null (\0) character found after [offset] (but before [offset]+[length])
// -----------------------------------------------------------------------------
static string memChunkReadString(const MemChunk& self, unsigned offset, unsigned length, bool null_terminated)
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
template<typename T> static bool memChunkWrite(MemChunk& self, unsigned offset, T value, bool expand)
{
	return self.write(offset, &value, sizeof(T), expand);
}

// -----------------------------------------------------------------------------
// Registers the DataBlock (MemChunk) type with lua
// -----------------------------------------------------------------------------
static void registerMemChunkType(lua_State* lua)
{
	auto lua_mc = luabridge::getGlobalNamespace(lua).beginClass<MemChunk>("DataBlock");

	// Constructors
	lua_mc.addConstructor<void(), void(uint32_t)>();

	// Properties
	// -------------------------------------------------------------------------
	lua_mc.addProperty("size", &MemChunk::size);
	lua_mc.addProperty("crc", &MemChunk::crc);

	// Functions
	// -------------------------------------------------------------------------
	lua_mc.addFunction("AsString", memChunkData);
	lua_mc.addFunction(
		"SetData",
		[](MemChunk& self, string_view data)
		{ self.importMem(reinterpret_cast<const uint8_t*>(data.data()), data.size()); });
	lua_mc.addFunction("Clear", &MemChunk::clear);
	lua_mc.addFunction("Resize", &MemChunk::reSize);
	lua_mc.addFunction("Copy", luabridge::overload<const MemChunk&>(&MemChunk::importMem));
	lua_mc.addFunction(
		"CopyTo",
		&MemChunk::exportMemChunk,
		[](MemChunk& self, MemChunk& mc) { return self.exportMemChunk(mc); },
		[](MemChunk& self, MemChunk& mc, int offset) { return self.exportMemChunk(mc, offset); });
	lua_mc.addFunction(
		"ImportFile",
		&MemChunk::importFile,
		[](MemChunk& self, string_view fn) { self.importFile(fn); },
		[](MemChunk& self, string_view fn, int offset) { self.importFile(fn, offset); });
	lua_mc.addFunction(
		"ExportFile",
		&MemChunk::exportFile,
		[](MemChunk& self, string_view fn) { return self.exportFile(fn); },
		[](MemChunk& self, string_view fn, int offset) { return self.exportFile(fn, offset); });
	lua_mc.addFunction("FillData", &MemChunk::fillData);
	lua_mc.addFunction("WriteInt8", &memChunkWrite<int8_t>);
	lua_mc.addFunction("WriteUInt8", &memChunkWrite<uint8_t>);
	lua_mc.addFunction("WriteInt16", &memChunkWrite<int16_t>);
	lua_mc.addFunction("WriteUInt16", &memChunkWrite<uint16_t>);
	lua_mc.addFunction("WriteInt32", &memChunkWrite<int32_t>);
	lua_mc.addFunction("WriteUInt32", &memChunkWrite<uint32_t>);
	lua_mc.addFunction("WriteInt64", &memChunkWrite<int64_t>);
	lua_mc.addFunction("WriteUInt64", &memChunkWrite<uint64_t>);
	lua_mc.addFunction(
		"WriteString",
		[](MemChunk& self, int offset, string_view value, bool expand)
		{ self.write(offset, value.data(), value.size(), expand); });
	lua_mc.addFunction("ReadInt8", &memChunkRead<int8_t>);
	lua_mc.addFunction("ReadUInt8", &memChunkRead<uint8_t>);
	lua_mc.addFunction("ReadInt16", &memChunkRead<int16_t>);
	lua_mc.addFunction("ReadUInt16", &memChunkRead<uint16_t>);
	lua_mc.addFunction("ReadInt32", &memChunkRead<int32_t>);
	lua_mc.addFunction("ReadUInt32", &memChunkRead<uint32_t>);
	lua_mc.addFunction("ReadInt64", &memChunkRead<int64_t>);
	lua_mc.addFunction("ReadUInt64", &memChunkRead<uint64_t>);
	lua_mc.addFunction(
		"ReadString",
		&memChunkReadString,
		[](const MemChunk& self, unsigned offset, unsigned length)
		{ return memChunkReadString(self, offset, length, false); });
}

// -----------------------------------------------------------------------------
// Returns the HSL components of the colour [self]
// -----------------------------------------------------------------------------
static int colourAsHSLLua(lua_State* L)
{
	try
	{
		auto self = luabridge::Stack<ColRGBA&>::get(L, 1).value();
		auto hsl  = colour::rgbToHsl(self);
		luabridge::push(L, hsl.h).throw_on_error();
		luabridge::push(L, hsl.s).throw_on_error();
		luabridge::push(L, hsl.l).throw_on_error();
		return 3;
	}
	catch (const std::exception& e)
	{
		throw LuaException("Runtime", fmt::format("Error in Colour.AsHSL: {}", e.what()));
	}
}
// static std::tuple<double, double, double> colourAsHSL(ColRGBA& self)
//{
//	auto hsl = colour::rgbToHsl(self);
//	return std::make_tuple(hsl.h, hsl.s, hsl.l);
// }

// -----------------------------------------------------------------------------
// Returns the LAB components of the colour [self]
// -----------------------------------------------------------------------------
static int colourAsLABLua(lua_State* L)
{
	try
	{
		auto self = luabridge::Stack<ColRGBA&>::get(L, 1).value();
		auto lab  = colour::rgbToLab(self);
		luabridge::push(L, lab.l).throw_on_error();
		luabridge::push(L, lab.a).throw_on_error();
		luabridge::push(L, lab.b).throw_on_error();
		return 3;
	}
	catch (const std::exception& e)
	{
		throw LuaException("Runtime", fmt::format("Error in Colour.AsLAB: {}", e.what()));
	}
}
// static std::tuple<double, double, double> colourAsLAB(ColRGBA& self)
//{
//	auto lab = colour::rgbToLab(self);
//	return std::make_tuple(lab.l, lab.a, lab.b);
// }

// -----------------------------------------------------------------------------
// Registers the Colour (ColRGBA) type with lua
// -----------------------------------------------------------------------------
static void registerColourType(lua_State* lua)
{
	auto lua_colour = luabridge::getGlobalNamespace(lua).beginClass<ColRGBA>("Colour");
	lua_colour.addConstructor<void(), void(u8, u8, u8), void(u8, u8, u8, u8)>();

	// Constants
	// -------------------------------------------------------------------------
	lua_colour.addStaticProperty("FORMAT_RGB", [] { return static_cast<int>(colour::StringFormat::RGB); });
	lua_colour.addStaticProperty("FORMAT_RGBA", [] { return static_cast<int>(colour::StringFormat::RGBA); });
	lua_colour.addStaticProperty("FORMAT_HEX", [] { return static_cast<int>(colour::StringFormat::HEX); });
	lua_colour.addStaticProperty("FORMAT_ZDOOM", [] { return static_cast<int>(colour::StringFormat::ZDoom); });

	// Properties
	// -------------------------------------------------------------------------
	lua_colour.addProperty("r", &ColRGBA::r, &ColRGBA::r);
	lua_colour.addProperty("g", &ColRGBA::g, &ColRGBA::g);
	lua_colour.addProperty("b", &ColRGBA::b, &ColRGBA::b);
	lua_colour.addProperty("a", &ColRGBA::a, &ColRGBA::a);
	lua_colour.addProperty("fr", &ColRGBA::fr);
	lua_colour.addProperty("fg", &ColRGBA::fg);
	lua_colour.addProperty("fb", &ColRGBA::fb);
	lua_colour.addProperty("fa", &ColRGBA::fa);

	// Functions
	// -------------------------------------------------------------------------
	lua_colour.addFunction("AsHSL", &colourAsHSLLua);
	lua_colour.addFunction("AsLAB", &colourAsLABLua);
	lua_colour.addFunction("AsString", [](ColRGBA& self, colour::StringFormat f) { return colour::toString(self, f); });
	lua_colour.addFunction(
		"FromHSL",
		[](ColRGBA& self, double h, double s, double l)
		{
			auto rgb = colour::hslToRgb({ h, s, l });
			self.r   = rgb.r;
			self.g   = rgb.g;
			self.b   = rgb.b;
		});
}

// -----------------------------------------------------------------------------
// Registers some misc. types with lua
// -----------------------------------------------------------------------------
void registerMiscTypes(lua_State* lua)
{
	// Point type
	luabridge::getGlobalNamespace(lua)
		.beginClass<Vec2d>("Point")
		.addProperty("x", &Vec2d::x, &Vec2d::x)
		.addProperty("y", &Vec2d::y, &Vec2d::y);

	// Colour type
	registerColourType(lua);

	// Plane type
	luabridge::getGlobalNamespace(lua)
		.beginClass<Plane>("Plane")
		.addProperty("a", &Plane::a, &Plane::a)
		.addProperty("b", &Plane::b, &Plane::b)
		.addProperty("c", &Plane::c, &Plane::c)
		.addProperty("d", &Plane::d, &Plane::d)
		.addFunction("HeightAt", luabridge::overload<const Vec2d&>(&Plane::heightAt));

	// MemChunk type
	registerMemChunkType(lua);
}

// -----------------------------------------------------------------------------
// Registers the App namespace with lua
// -----------------------------------------------------------------------------
void registerAppNamespace(lua_State* lua)
{
	auto app = luabridge::getGlobalNamespace(lua).beginNamespace("App");

	// Functions
	// -------------------------------------------------------------------------
	app.addFunction("LogMessage", [](string_view message) { logMessage(message, log::MessageType::Script); });
	app.addFunction("LogWarning", [](string_view message) { logMessage(message, log::MessageType::Warning); });
	app.addFunction("LogError", [](string_view message) { logMessage(message, log::MessageType::Error); });
	app.addFunction("CurrentArchive", &maineditor::currentArchive);
	app.addFunction("CurrentEntry", &maineditor::currentEntry);
	app.addFunction("CurrentEntrySelection", &maineditor::currentEntrySelection);
	app.addFunction("CurrentPalette", &maineditor::currentPalette, [] { return maineditor::currentPalette(); });
	app.addFunction("ShowArchive", &showArchive);
	app.addFunction("ShowEntry", &maineditor::openEntry);
	app.addFunction("MapEditor", &mapeditor::editContext);
}

} // namespace slade::scripting
