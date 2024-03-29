
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CTexture.cpp
// Description: Functions to export composite texture-related types to lua using
//              sol3
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
#include "Graphics/CTexture/CTexture.h"
#include "Archive/ArchiveEntry.h"
#include "Graphics/Translation.h"
#include "thirdparty/sol/sol.hpp"

using namespace slade;

namespace slade::lua
{
// -----------------------------------------------------------------------------
// Registers the CTPatch and CTPatchEx types with lua
// -----------------------------------------------------------------------------
void registerCTexturePatchTypes(sol::state& lua)
{
	// -------------------------------------------------------------------------
	// CTPatch
	// -------------------------------------------------------------------------
	auto lua_ctpatch = lua.new_usertype<slade::CTPatch>("CTPatch", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_ctpatch.set("name", sol::property(&CTPatch::name, &CTPatch::setName));
	lua_ctpatch.set("offsetX", sol::property(&CTPatch::xOffset, &CTPatch::setOffsetX));
	lua_ctpatch.set("offsetY", sol::property(&CTPatch::yOffset, &CTPatch::setOffsetY));
	/*lua_ctpatch["offset"] = sol::property(
		[](CTPatch& self) { return Vec2d(self.xOffset(), self.yOffset()); },
		[](CTPatch& self, double x, double y) {
			self.setOffsetX(x);
			self.setOffsetY(y);
		});*/

	// Functions
	// -------------------------------------------------------------------------
	lua_ctpatch.set_function("PatchEntry", &CTPatch::patchEntry);
	lua_ctpatch.set_function("AsExtended", [](CTPatch& self) { return dynamic_cast<CTPatchEx*>(&self); });


	// -------------------------------------------------------------------------
	// CTPatchEx
	// -------------------------------------------------------------------------
	auto lua_ctpatch_ex = lua.new_usertype<CTPatchEx>(
		"CTPatchEx", "new", sol::no_constructor, sol::base_classes, sol::bases<CTPatch>());

	// Constants
	// -------------------------------------------------------------------------
	lua_ctpatch_ex.set("TYPE_PATCH", sol::property([]() { return CTPatchEx::Type::Patch; }));
	lua_ctpatch_ex.set("TYPE_GRAPHIC", sol::property([]() { return CTPatchEx::Type::Graphic; }));
	lua_ctpatch_ex.set("BLENDTYPE_NONE", sol::property([]() { return CTPatchEx::BlendType::None; }));
	lua_ctpatch_ex.set("BLENDTYPE_TRANSLATION", sol::property([]() { return CTPatchEx::BlendType::Translation; }));
	lua_ctpatch_ex.set("BLENDTYPE_BLEND", sol::property([]() { return CTPatchEx::BlendType::Blend; }));
	lua_ctpatch_ex.set("BLENDTYPE_TINT", sol::property([]() { return CTPatchEx::BlendType::Tint; }));

	// Properties
	// -------------------------------------------------------------------------
	lua_ctpatch_ex.set("flipX", sol::property(&CTPatchEx::flipX, &CTPatchEx::setFlipX));
	lua_ctpatch_ex.set("flipY", sol::property(&CTPatchEx::flipY, &CTPatchEx::setFlipY));
	lua_ctpatch_ex.set("useOffsets", sol::property(&CTPatchEx::useOffsets, &CTPatchEx::setUseOffsets));
	lua_ctpatch_ex.set("rotation", sol::property(&CTPatchEx::rotation, &CTPatchEx::setRotation));
	lua_ctpatch_ex.set("colour", sol::property(&CTPatchEx::colour, &CTPatchEx::setColour));
	lua_ctpatch_ex.set("alpha", sol::property(&CTPatchEx::alpha, &CTPatchEx::setAlpha));
	lua_ctpatch_ex.set("style", sol::property(&CTPatchEx::style, &CTPatchEx::setStyle));
	lua_ctpatch_ex.set("blendType", sol::property(&CTPatchEx::blendType, &CTPatchEx::setBlendType));
	lua_ctpatch_ex.set("translation", &CTPatchEx::translation);

	// Functions
	// -------------------------------------------------------------------------
	lua_ctpatch_ex.set_function("AsText", &CTPatchEx::asText);
	// parse?
}

// -----------------------------------------------------------------------------
// Registers the CTexture type with lua
// -----------------------------------------------------------------------------
void registerCTextureType(sol::state& lua)
{
	auto lua_ctexture = lua.new_usertype<CTexture>("CTexture", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_ctexture.set("patches", sol::property(&CTexture::patches));
	lua_ctexture.set("name", sol::property(&CTexture::name, &CTexture::setName));
	lua_ctexture.set("width", sol::property(&CTexture::width, &CTexture::setWidth));
	lua_ctexture.set("height", sol::property(&CTexture::height, &CTexture::setHeight));
	lua_ctexture.set("scaleX", sol::property(&CTexture::scaleX, &CTexture::setScaleX));
	lua_ctexture.set("scaleY", sol::property(&CTexture::scaleY, &CTexture::setScaleY));
	lua_ctexture.set("offsetX", sol::property(&CTexture::offsetX, &CTexture::setOffsetX));
	lua_ctexture.set("offsetY", sol::property(&CTexture::offsetY, &CTexture::setOffsetY));
	lua_ctexture.set("worldPanning", sol::property(&CTexture::worldPanning, &CTexture::setWorldPanning));
	lua_ctexture.set("type", sol::property(&CTexture::type, &CTexture::setType));
	lua_ctexture.set("extended", sol::property(&CTexture::isExtended, &CTexture::setExtended));
	lua_ctexture.set("optional", sol::property(&CTexture::isOptional, &CTexture::setOptional));
	lua_ctexture.set("noDecals", sol::property(&CTexture::noDecals, &CTexture::setNoDecals));
	lua_ctexture.set("nullTexture", sol::property(&CTexture::nullTexture, &CTexture::setNullTexture));

	// Functions
	// -------------------------------------------------------------------------
	lua_ctexture.set_function(
		"CopyTexture",
		sol::overload(
			&CTexture::copyTexture, [](CTexture& self, const CTexture& tex) { return self.copyTexture(tex, false); }));
	lua_ctexture.set_function("Clear", &CTexture::clear);
	lua_ctexture.set_function(
		"AddPatch",
		sol::overload(
			&CTexture::addPatch,
			[](CTexture& self, string_view patch) { return self.addPatch(patch, 0, 0, 0); },
			[](CTexture& self, string_view patch, int x, int y) { return self.addPatch(patch, x, y, 0); }));
	lua_ctexture.set_function("RemovePatch", [](CTexture& self, int index) { return self.removePatch(index); });
	lua_ctexture.set_function("ReplacePatch", &CTexture::replacePatch);
	lua_ctexture.set_function(
		"DuplicatePatch",
		sol::overload(&CTexture::duplicatePatch, [](CTexture& self, int index) { self.duplicatePatch(index, 8, 8); }));
	lua_ctexture.set_function("SwapPatches", &CTexture::swapPatches);
	lua_ctexture.set_function("AsText", &CTexture::asText);
	lua_ctexture.set_function("ConvertExtended", &CTexture::convertExtended);
	lua_ctexture.set_function("ConvertRegular", &CTexture::convertRegular);
}
} // namespace slade::lua
