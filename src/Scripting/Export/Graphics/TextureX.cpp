
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
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
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
// Returns all the patches in PatchTable [self] as a vector of strings
// -----------------------------------------------------------------------------
vector<string> patchTablePatches(PatchTable& self)
{
	vector<string> patches;
	for (const auto& patch : self.patches())
		patches.push_back(patch.name);
	return patches;
}

// -----------------------------------------------------------------------------
// Registers the PatchTable type with lua
// -----------------------------------------------------------------------------
void registerPatchTableType(sol::state& lua)
{
	auto lua_ptable = lua.new_usertype<PatchTable>("PatchTable", "new", sol::constructors<PatchTable()>());

	// Properties
	// -------------------------------------------------------------------------
	lua_ptable.set("patches", sol::property(&patchTablePatches));
	lua_ptable.set("parent", sol::property(&PatchTable::parent));

	// Functions
	// -------------------------------------------------------------------------
	lua_ptable.set_function("Patch", [](PatchTable& self, unsigned index) { return self.patch(index).name; });
	lua_ptable.set_function(
		"PatchEntry",
		sol::overload(
			[](PatchTable& self, int index) { return self.patchEntry(index); },
			sol::resolve<ArchiveEntry*(string_view) const>(&PatchTable::patchEntry)));
	lua_ptable.set_function("RemovePatch", [](PatchTable& self, int index) { self.removePatch(index); });
	lua_ptable.set_function(
		"ReplacePatch", [](PatchTable& self, int index, string_view name) { self.replacePatch(index, name); });
	lua_ptable.set_function("AddPatch", &PatchTable::addPatch);
	lua_ptable.set_function(
		"LoadPNAMES", [](PatchTable& self, ArchiveEntry* pnames) { return self.loadPNAMES(pnames); });
	lua_ptable.set_function("WritePNAMES", &PatchTable::writePNAMES);
}

// -----------------------------------------------------------------------------
// Wrapper for TextureXList::addTexture that returns a pointer to the added
// texture
// -----------------------------------------------------------------------------
CTexture* addTexture(TextureXList& self, string_view name, bool extended, int position)
{
	auto       tex     = std::make_unique<CTexture>(name, extended);
	const auto tex_ptr = tex.get();
	self.addTexture(std::move(tex), position);
	return tex_ptr;
}

// -----------------------------------------------------------------------------
// Registers the TextureXList type with lua
// -----------------------------------------------------------------------------
void registerTextureXListType(sol::state& lua)
{
	auto lua_txlist = lua.new_usertype<TextureXList>(
		"TextureXList", "new", sol::constructors<TextureXList(), TextureXList(TextureXList::Format)>());

	// Constants
	// -------------------------------------------------------------------------
	lua_txlist.set("FORMAT_NORMAL", sol::property([]() { return TextureXList::Format::Normal; }));
	lua_txlist.set("FORMAT_STRIFE11", sol::property([]() { return TextureXList::Format::Strife11; }));
	lua_txlist.set("FORMAT_NAMELESS", sol::property([]() { return TextureXList::Format::Nameless; }));
	lua_txlist.set("FORMAT_TEXTURES", sol::property([]() { return TextureXList::Format::Textures; }));
	lua_txlist.set("FORMAT_JAGUAR", sol::property([]() { return TextureXList::Format::Jaguar; }));

	// Properties
	// -------------------------------------------------------------------------
	lua_txlist.set("textures", sol::property(&TextureXList::textures));
	lua_txlist.set("format", sol::property(&TextureXList::format));
	lua_txlist.set("formatString", sol::property(&TextureXList::textureXFormatString));

	// Functions
	// -------------------------------------------------------------------------
	lua_txlist.set_function("Texture", sol::resolve<CTexture*(string_view) const>(&TextureXList::texture));
	lua_txlist.set_function("TextureIndex", &TextureXList::textureIndex);
	lua_txlist.set_function(
		"AddTexture",
		sol::overload(
			&addTexture,
			[](TextureXList& self, string_view name) { return addTexture(self, name, false, 0); },
			[](TextureXList& self, string_view name, bool extended) { return addTexture(self, name, extended, 0); }));
	lua_txlist.set_function("RemoveTexture", [](TextureXList& self, int index) { self.removeTexture(index); });
	lua_txlist.set_function("SwapTextures", &TextureXList::swapTextures);
	lua_txlist.set_function("Clear", [](TextureXList& self) { self.clear(); });
	lua_txlist.set_function("RemovePatch", &TextureXList::removePatch);
	lua_txlist.set_function("ReadTEXTUREXData", &TextureXList::readTEXTUREXData);
	lua_txlist.set_function("WriteTEXTUREXData", &TextureXList::writeTEXTUREXData);
	lua_txlist.set_function("ReadTEXTURESData", &TextureXList::readTEXTURESData);
	lua_txlist.set_function("WriteTEXTURESData", &TextureXList::writeTEXTURESData);
	lua_txlist.set_function("ConvertToTEXTURES", &TextureXList::convertToTEXTURES);
	lua_txlist.set_function("FindErrors", &TextureXList::findErrors);
}
} // namespace slade::lua
