
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
	lua_ptable["patches"] = sol::property(&patchTablePatches);
	lua_ptable["parent"]  = sol::property(&PatchTable::parent);

	// Functions
	// -------------------------------------------------------------------------
	lua_ptable["Patch"]      = [](PatchTable& self, unsigned index) { return self.patch(index).name; };
	lua_ptable["PatchEntry"] = sol::overload(
		[](PatchTable& self, int index) { return self.patchEntry(index); },
		sol::resolve<ArchiveEntry*(string_view) const>(&PatchTable::patchEntry));
	lua_ptable["RemovePatch"]  = [](PatchTable& self, int index) { self.removePatch(index); };
	lua_ptable["ReplacePatch"] = [](PatchTable& self, int index, string_view name) { self.replacePatch(index, name); };
	lua_ptable["AddPatch"]     = &PatchTable::addPatch;
	lua_ptable["LoadPNAMES"]   = [](PatchTable& self, ArchiveEntry* pnames) { return self.loadPNAMES(pnames); };
	lua_ptable["WritePNAMES"]  = &PatchTable::writePNAMES;
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
	lua_txlist["FORMAT_NORMAL"]   = sol::property([]() { return TextureXList::Format::Normal; });
	lua_txlist["FORMAT_STRIFE11"] = sol::property([]() { return TextureXList::Format::Strife11; });
	lua_txlist["FORMAT_NAMELESS"] = sol::property([]() { return TextureXList::Format::Nameless; });
	lua_txlist["FORMAT_TEXTURES"] = sol::property([]() { return TextureXList::Format::Textures; });
	lua_txlist["FORMAT_JAGUAR"]   = sol::property([]() { return TextureXList::Format::Jaguar; });

	// Properties
	// -------------------------------------------------------------------------
	lua_txlist["textures"]     = sol::property(&TextureXList::textures);
	lua_txlist["format"]       = sol::property(&TextureXList::format);
	lua_txlist["formatString"] = sol::property(&TextureXList::textureXFormatString);

	// Functions
	// -------------------------------------------------------------------------
	lua_txlist["Texture"]      = sol::resolve<CTexture*(string_view) const>(&TextureXList::texture);
	lua_txlist["TextureIndex"] = &TextureXList::textureIndex;
	lua_txlist["AddTexture"]   = sol::overload(
        &addTexture,
        [](TextureXList& self, string_view name) { return addTexture(self, name, false, 0); },
        [](TextureXList& self, string_view name, bool extended) { return addTexture(self, name, extended, 0); });
	lua_txlist["RemoveTexture"]     = [](TextureXList& self, int index) { self.removeTexture(index); };
	lua_txlist["SwapTextures"]      = &TextureXList::swapTextures;
	lua_txlist["Clear"]             = [](TextureXList& self) { self.clear(); };
	lua_txlist["RemovePatch"]       = &TextureXList::removePatch;
	lua_txlist["ReadTEXTUREXData"]  = &TextureXList::readTEXTUREXData;
	lua_txlist["WriteTEXTUREXData"] = &TextureXList::writeTEXTUREXData;
	lua_txlist["ReadTEXTURESData"]  = &TextureXList::readTEXTURESData;
	lua_txlist["WriteTEXTURESData"] = &TextureXList::writeTEXTURESData;
	lua_txlist["ConvertToTEXTURES"] = &TextureXList::convertToTEXTURES;
	lua_txlist["FindErrors"]        = &TextureXList::findErrors;
}
} // namespace slade::lua
