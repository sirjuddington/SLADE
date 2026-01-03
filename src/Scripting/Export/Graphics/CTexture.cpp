
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Scripting/Export/Export.h"
#include "Scripting/LuaBridge.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Types
//
// -----------------------------------------------------------------------------
template<> struct luabridge::Stack<CTPatchEx::Type> : Enum<CTPatchEx::Type>
{
};
template<> struct luabridge::Stack<CTPatchEx::BlendType> : Enum<CTPatchEx::BlendType>
{
};


// -----------------------------------------------------------------------------
//
// Scripting Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::scripting
{
// -----------------------------------------------------------------------------
// Registers the CTPatch and CTPatchEx types with lua
// -----------------------------------------------------------------------------
void registerCTexturePatchTypes(lua_State* lua)
{
	// -------------------------------------------------------------------------
	// CTPatch
	// -------------------------------------------------------------------------
	auto lua_ctpatch = luabridge::getGlobalNamespace(lua).beginClass<CTPatch>("CTPatch");

	// Properties
	// -------------------------------------------------------------------------
	lua_ctpatch.addProperty("name", &CTPatch::name, &CTPatch::setName);
	lua_ctpatch.addProperty("offsetX", &CTPatch::xOffset, &CTPatch::setOffsetX);
	lua_ctpatch.addProperty("offsetY", &CTPatch::yOffset, &CTPatch::setOffsetY);
	/*lua_ctpatch["offset"] = sol::property(
		[](CTPatch& self) { return Vec2d(self.xOffset(), self.yOffset()); },
		[](CTPatch& self, double x, double y) {
			self.setOffsetX(x);
			self.setOffsetY(y);
		});*/

	// Functions
	// -------------------------------------------------------------------------
	lua_ctpatch.addFunction("PatchEntry", &CTPatch::patchEntry);
	lua_ctpatch.addFunction("AsExtended", [](CTPatch& self) { return dynamic_cast<CTPatchEx*>(&self); });


	// -------------------------------------------------------------------------
	// CTPatchEx
	// -------------------------------------------------------------------------
	auto lua_ctpatch_ex = luabridge::getGlobalNamespace(lua).deriveClass<CTPatchEx, CTPatch>("CTPatchEx");

	// Constants
	// -------------------------------------------------------------------------
	lua_ctpatch_ex.addStaticProperty("TYPE_PATCH", +[] { return CTPatchEx::Type::Patch; });
	lua_ctpatch_ex.addStaticProperty("TYPE_GRAPHIC", +[] { return CTPatchEx::Type::Graphic; });
	lua_ctpatch_ex.addStaticProperty("BLENDTYPE_NONE", +[] { return CTPatchEx::BlendType::None; });
	lua_ctpatch_ex.addStaticProperty("BLENDTYPE_TRANSLATION", +[] { return CTPatchEx::BlendType::Translation; });
	lua_ctpatch_ex.addStaticProperty("BLENDTYPE_BLEND", +[] { return CTPatchEx::BlendType::Blend; });
	lua_ctpatch_ex.addStaticProperty("BLENDTYPE_TINT", +[] { return CTPatchEx::BlendType::Tint; });

	// Properties
	// -------------------------------------------------------------------------
	lua_ctpatch_ex.addProperty("flipX", &CTPatchEx::flipX, &CTPatchEx::setFlipX);
	lua_ctpatch_ex.addProperty("flipY", &CTPatchEx::flipY, &CTPatchEx::setFlipY);
	lua_ctpatch_ex.addProperty("useOffsets", &CTPatchEx::useOffsets, &CTPatchEx::setUseOffsets);
	lua_ctpatch_ex.addProperty("rotation", &CTPatchEx::rotation, &CTPatchEx::setRotation);
	lua_ctpatch_ex.addProperty(
		"colour", &CTPatchEx::colour, [](CTPatchEx& self, const ColRGBA& col) { self.setColour(col); });
	lua_ctpatch_ex.addProperty("alpha", &CTPatchEx::alpha, &CTPatchEx::setAlpha);
	lua_ctpatch_ex.addProperty("style", &CTPatchEx::style, &CTPatchEx::setStyle);
	lua_ctpatch_ex.addProperty("blendType", &CTPatchEx::blendType, &CTPatchEx::setBlendType);
	lua_ctpatch_ex.addProperty("translation", &CTPatchEx::translation, &CTPatchEx::setTranslation);

	// Functions
	// -------------------------------------------------------------------------
	lua_ctpatch_ex.addFunction("AsText", &CTPatchEx::asText);
	// parse?
}

// -----------------------------------------------------------------------------
// Returns all the patches in CTexture [self] as a vector of raw pointers
// (luabridge doesn't support unique_ptr in vectors)
// -----------------------------------------------------------------------------
static vector<CTPatch*> cTexturePatches(const CTexture& self)
{
	vector<CTPatch*> patches;
	for (const auto& patch : self.patches())
		patches.push_back(patch.get());
	return patches;
}

// -----------------------------------------------------------------------------
// Registers the CTexture type with lua
// -----------------------------------------------------------------------------
void registerCTextureType(lua_State* lua)
{
	auto lua_ctexture = luabridge::getGlobalNamespace(lua).beginClass<CTexture>("CTexture");

	// Properties
	// -------------------------------------------------------------------------
	lua_ctexture.addProperty("patches", cTexturePatches);
	lua_ctexture.addProperty("name", &CTexture::name, &CTexture::setName);
	lua_ctexture.addProperty("width", &CTexture::width, &CTexture::setWidth);
	lua_ctexture.addProperty("height", &CTexture::height, &CTexture::setHeight);
	lua_ctexture.addProperty("scaleX", &CTexture::scaleX, &CTexture::setScaleX);
	lua_ctexture.addProperty("scaleY", &CTexture::scaleY, &CTexture::setScaleY);
	lua_ctexture.addProperty("offsetX", &CTexture::offsetX, &CTexture::setOffsetX);
	lua_ctexture.addProperty("offsetY", &CTexture::offsetY, &CTexture::setOffsetY);
	lua_ctexture.addProperty("worldPanning", &CTexture::worldPanning, &CTexture::setWorldPanning);
	lua_ctexture.addProperty("type", &CTexture::type, &CTexture::setType);
	lua_ctexture.addProperty("extended", &CTexture::isExtended, &CTexture::setExtended);
	lua_ctexture.addProperty("optional", &CTexture::isOptional, &CTexture::setOptional);
	lua_ctexture.addProperty("noDecals", &CTexture::noDecals, &CTexture::setNoDecals);
	lua_ctexture.addProperty("nullTexture", &CTexture::nullTexture, &CTexture::setNullTexture);

	// Functions
	// -------------------------------------------------------------------------
	lua_ctexture.addFunction(
		"CopyTexture",
		&CTexture::copyTexture,
		[](CTexture& self, const CTexture& tex) { return self.copyTexture(tex, false); });
	lua_ctexture.addFunction("Clear", &CTexture::clear);
	lua_ctexture.addFunction(
		"AddPatch",
		&CTexture::addPatch,
		[](CTexture& self, string_view patch) { return self.addPatch(patch, 0, 0, 0); },
		[](CTexture& self, string_view patch, int x, int y) { return self.addPatch(patch, x, y, 0); });
	lua_ctexture.addFunction("RemovePatch", [](CTexture& self, int index) { return self.removePatch(index); });
	lua_ctexture.addFunction("ReplacePatch", &CTexture::replacePatch);
	lua_ctexture.addFunction(
		"DuplicatePatch",
		&CTexture::duplicatePatch,
		[](CTexture& self, int index) { self.duplicatePatch(index, 8, 8); });
	lua_ctexture.addFunction("SwapPatches", &CTexture::swapPatches);
	lua_ctexture.addFunction("AsText", &CTexture::asText);
	lua_ctexture.addFunction("ConvertExtended", &CTexture::convertExtended);
	lua_ctexture.addFunction("ConvertRegular", &CTexture::convertRegular);
}
} // namespace slade::scripting
