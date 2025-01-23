
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextureX.cpp
// Description: Functions to export TEXTUREx-related types to lua using sol3
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
#include "Archive/ArchiveEntry.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Scripting/Export/Export.h"
#include "Scripting/LuaBridge.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Types
//
// -----------------------------------------------------------------------------
template<> struct luabridge::Stack<TextureXList::Format> : Enum<TextureXList::Format>
{
};


// -----------------------------------------------------------------------------
//
// Lua Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::lua
{
// -----------------------------------------------------------------------------
// Returns all the patches in PatchTable [self] as a vector of strings
// -----------------------------------------------------------------------------
static vector<string> patchTablePatches(const PatchTable& self)
{
	vector<string> patches;
	for (const auto& patch : self.patches())
		patches.push_back(patch.name);
	return patches;
}

// -----------------------------------------------------------------------------
// Registers the PatchTable type with lua
// -----------------------------------------------------------------------------
void registerPatchTableType(lua_State* lua)
{
	auto lua_ptable = luabridge::getGlobalNamespace(lua).beginClass<PatchTable>("PatchTable");
	lua_ptable.addConstructor<void()>();

	// Properties
	// -------------------------------------------------------------------------
	lua_ptable.addProperty("patches", [](const PatchTable& self) { return patchTablePatches(self); });
	lua_ptable.addProperty("parent", &PatchTable::parent);

	// Functions
	// -------------------------------------------------------------------------
	lua_ptable.addFunction("Patch", [](PatchTable& self, unsigned index) { return self.patch(index).name; });
	lua_ptable.addFunction(
		"PatchEntry",
		[](PatchTable& self, int index) { return self.patchEntry(index); },
		luabridge::constOverload<string_view>(&PatchTable::patchEntry));
	lua_ptable.addFunction("RemovePatch", [](PatchTable& self, int index) { self.removePatch(index); });
	lua_ptable.addFunction(
		"ReplacePatch", [](PatchTable& self, int index, string_view name) { self.replacePatch(index, name); });
	lua_ptable.addFunction("AddPatch", &PatchTable::addPatch);
	lua_ptable.addFunction(
		"LoadPNAMES", [](PatchTable& self, ArchiveEntry* pnames) { return self.loadPNAMES(pnames); });
	lua_ptable.addFunction("WritePNAMES", &PatchTable::writePNAMES);
}

// -----------------------------------------------------------------------------
// Wrapper for TextureXList::addTexture that returns a pointer to the added
// texture
// -----------------------------------------------------------------------------
static CTexture* addTexture(TextureXList& self, string_view name, bool extended, int position)
{
	auto       tex     = std::make_unique<CTexture>(name, extended);
	const auto tex_ptr = tex.get();
	self.addTexture(std::move(tex), position);
	return tex_ptr;
}

// -----------------------------------------------------------------------------
// Registers the TextureXList type with lua
// -----------------------------------------------------------------------------
void registerTextureXListType(lua_State* lua)
{
	auto lua_txlist = luabridge::getGlobalNamespace(lua).beginClass<TextureXList>("TextureXList");
	lua_txlist.addConstructor<void(), void(TextureXList::Format)>();

	// Constants
	// -------------------------------------------------------------------------
	lua_txlist.addStaticProperty("FORMAT_NORMAL", +[] { return TextureXList::Format::Normal; });
	lua_txlist.addStaticProperty("FORMAT_STRIFE11", +[] { return TextureXList::Format::Strife11; });
	lua_txlist.addStaticProperty("FORMAT_NAMELESS", +[] { return TextureXList::Format::Nameless; });
	lua_txlist.addStaticProperty("FORMAT_TEXTURES", +[] { return TextureXList::Format::Textures; });
	lua_txlist.addStaticProperty("FORMAT_JAGUAR", +[] { return TextureXList::Format::Jaguar; });

	// Properties
	// -------------------------------------------------------------------------
	// lua_txlist.addProperty("textures", &TextureXList::textures);
	lua_txlist.addProperty("format", &TextureXList::format);
	lua_txlist.addProperty("formatString", &TextureXList::textureXFormatString);

	// Functions
	// -------------------------------------------------------------------------
	lua_txlist.addFunction("Texture", luabridge::constOverload<string_view>(&TextureXList::texture));
	lua_txlist.addFunction("TextureIndex", &TextureXList::textureIndex);
	lua_txlist.addFunction(
		"AddTexture",
		&addTexture,
		[](TextureXList& self, string_view name) { return addTexture(self, name, false, 0); },
		[](TextureXList& self, string_view name, bool extended) { return addTexture(self, name, extended, 0); });
	lua_txlist.addFunction("RemoveTexture", [](TextureXList& self, int index) { self.removeTexture(index); });
	lua_txlist.addFunction("SwapTextures", &TextureXList::swapTextures);
	lua_txlist.addFunction("Clear", [](TextureXList& self) { self.clear(); });
	lua_txlist.addFunction("RemovePatch", &TextureXList::removePatch);
	lua_txlist.addFunction("ReadTEXTUREXData", &TextureXList::readTEXTUREXData);
	lua_txlist.addFunction("WriteTEXTUREXData", &TextureXList::writeTEXTUREXData);
	lua_txlist.addFunction("ReadTEXTURESData", &TextureXList::readTEXTURESData);
	lua_txlist.addFunction("WriteTEXTURESData", &TextureXList::writeTEXTURESData);
	lua_txlist.addFunction("ConvertToTEXTURES", &TextureXList::convertToTEXTURES);
	lua_txlist.addFunction("FindErrors", &TextureXList::findErrors);
}
} // namespace slade::lua
