
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Image.cpp
// Description: Functions to export gfx translation-related types to lua using
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
#include "Graphics/Translation.h"
#include "Scripting/Export/Export.h"
#include "Scripting/LuaBridge.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Types
//
// -----------------------------------------------------------------------------
template<> struct luabridge::Stack<TransRange::Type> : Enum<TransRange::Type>
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
// Registers the TransRange type (and TransRange* sub-types) with lua
// -----------------------------------------------------------------------------
static void registerTranslationRangeTypes(lua_State* lua)
{
	// -------------------------------------------------------------------------
	// TransRange
	// -------------------------------------------------------------------------
	auto lua_trange = luabridge::getGlobalNamespace(lua).beginClass<TransRange>("TransRange");

	// Properties
	// -------------------------------------------------------------------------
	lua_trange.addProperty("type", &TransRange::type);
	lua_trange.addProperty("rangeStart", &TransRange::start, &TransRange::setStart);
	lua_trange.addProperty("rangeEnd", &TransRange::end, &TransRange::setEnd);

	// Functions
	// -------------------------------------------------------------------------
	lua_trange.addFunction("AsText", &TransRange::asText);
	lua_trange.addFunction("AsPaletteRange", [](TransRange& self) { return dynamic_cast<TransRangePalette*>(&self); });
	lua_trange.addFunction("AsColourRange", [](TransRange& self) { return dynamic_cast<TransRangeColour*>(&self); });
	lua_trange.addFunction("AsDesatRange", [](TransRange& self) { return dynamic_cast<TransRangeDesat*>(&self); });
	lua_trange.addFunction("AsBlendRange", [](TransRange& self) { return dynamic_cast<TransRangeBlend*>(&self); });
	lua_trange.addFunction("AsTintRange", [](TransRange& self) { return dynamic_cast<TransRangeTint*>(&self); });
	lua_trange.addFunction("AsSpecialRange", [](TransRange& self) { return dynamic_cast<TransRangeSpecial*>(&self); });


	// -------------------------------------------------------------------------
	// TransRangePalette
	// -------------------------------------------------------------------------
	auto lua_trange_pal = luabridge::getGlobalNamespace(lua).deriveClass<TransRangePalette, TransRange>(
		"TransRangePalette");

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_pal.addProperty("destStart", &TransRangePalette::dStart, &TransRangePalette::setDStart);
	lua_trange_pal.addProperty("destEnd", &TransRangePalette::dEnd, &TransRangePalette::setDEnd);


	// -------------------------------------------------------------------------
	// TransRangeColour
	// -------------------------------------------------------------------------
	auto lua_trange_col = luabridge::getGlobalNamespace(lua).deriveClass<TransRangeColour, TransRange>(
		"TransRangeColour");

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_col.addProperty("startColour", &TransRangeColour::startColour, &TransRangeColour::setStartColour);
	lua_trange_col.addProperty("endColour", &TransRangeColour::endColour, &TransRangeColour::setEndColour);


	// -------------------------------------------------------------------------
	// TransRangeDesat
	// -------------------------------------------------------------------------
	auto lua_trange_desat = luabridge::getGlobalNamespace(lua).deriveClass<TransRangeDesat, TransRange>(
		"TransRangeDesat");

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_desat.addProperty(
		"startR",
		[](TransRangeDesat& self) { return self.rgbStart().r; },
		[](TransRangeDesat& self, float r) { self.setRGBStart(r, self.rgbStart().g, self.rgbStart().b); });
	lua_trange_desat.addProperty(
		"startG",
		[](TransRangeDesat& self) { return self.rgbStart().g; },
		[](TransRangeDesat& self, float g) { self.setRGBStart(self.rgbStart().r, g, self.rgbStart().b); });
	lua_trange_desat.addProperty(
		"startB",
		[](TransRangeDesat& self) { return self.rgbStart().b; },
		[](TransRangeDesat& self, float b) { self.setRGBStart(self.rgbStart().r, self.rgbStart().g, b); });
	lua_trange_desat.addProperty(
		"endR",
		[](TransRangeDesat& self) { return self.rgbEnd().r; },
		[](TransRangeDesat& self, float r) { self.setRGBEnd(r, self.rgbEnd().g, self.rgbEnd().b); });
	lua_trange_desat.addProperty(
		"endG",
		[](TransRangeDesat& self) { return self.rgbEnd().g; },
		[](TransRangeDesat& self, float g) { self.setRGBEnd(self.rgbEnd().r, g, self.rgbEnd().b); });
	lua_trange_desat.addProperty(
		"endB",
		[](TransRangeDesat& self) { return self.rgbEnd().b; },
		[](TransRangeDesat& self, float b) { self.setRGBEnd(self.rgbEnd().r, self.rgbEnd().g, b); });

	// Functions
	// -------------------------------------------------------------------------
	lua_trange_desat.addFunction("SetStartRGB", &TransRangeDesat::setRGBStart);
	lua_trange_desat.addFunction("SetEndRGB", &TransRangeDesat::setRGBEnd);


	// -------------------------------------------------------------------------
	// TransRangeBlend
	// -------------------------------------------------------------------------
	auto lua_trange_blend = luabridge::getGlobalNamespace(lua).deriveClass<TransRangeBlend, TransRange>(
		"TransRangeBlend");

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_blend.addProperty("colour", &TransRangeBlend::colour, &TransRangeBlend::setColour);


	// -------------------------------------------------------------------------
	// TransRangeTint
	// -------------------------------------------------------------------------
	auto lua_trange_tint = luabridge::getGlobalNamespace(lua).deriveClass<TransRangeTint, TransRange>("TransRangeTint");

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_tint.addProperty("colour", &TransRangeTint::colour, &TransRangeTint::setColour);
	lua_trange_tint.addProperty("amount", &TransRangeTint::amount, &TransRangeTint::setAmount);


	// -------------------------------------------------------------------------
	// TransRangeSpecial
	// -------------------------------------------------------------------------
	auto lua_trange_special = luabridge::getGlobalNamespace(lua).deriveClass<TransRangeSpecial, TransRange>(
		"TransRangeSpecial");

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_special.addProperty("special", &TransRangeSpecial::special, &TransRangeSpecial::setSpecial);
}

// -----------------------------------------------------------------------------
// Adds a new translation range of [type] to the translation [self], with its
// initial range being from [range_start] to [range_end].
// Returns the TransRange that was added (cast to type T)
// -----------------------------------------------------------------------------
template<typename T>
static T* addTranslationRange(Translation& self, TransRange::Type type, int range_start, int range_end)
{
	return dynamic_cast<T*>(self.addRange(type, -1, range_start, range_end));
}

// -----------------------------------------------------------------------------
// Returns all the translation ranges in Translation [self] as a vector of
// raw pointers (luabridge doesn't support unique_ptr in vectors)
// -----------------------------------------------------------------------------
static vector<TransRange*> translationRanges(const Translation& self)
{
	vector<TransRange*> ranges;
	for (const auto& range : self.ranges())
		ranges.push_back(range.get());
	return ranges;
}

// -----------------------------------------------------------------------------
// Registers the Translation type with lua
// -----------------------------------------------------------------------------
void registerTranslationType(lua_State* lua)
{
	auto lua_translation = luabridge::getGlobalNamespace(lua).beginClass<Translation>("Translation");
	lua_translation.addConstructor<void()>();

	// Properties
	// -------------------------------------------------------------------------
	lua_translation.addProperty("ranges", translationRanges);
	lua_translation.addProperty("rangeCount", &Translation::nRanges);
	lua_translation.addProperty("standardName", &Translation::builtInName, &Translation::setBuiltInName);
	lua_translation.addProperty("desatAmount", &Translation::desaturationAmount, &Translation::setDesaturationAmount);

	// Constants
	// -------------------------------------------------------------------------
	lua_translation.addStaticProperty("RANGE_PALETTE", +[] { return TransRange::Type::Palette; });
	lua_translation.addStaticProperty("RANGE_COLOUR", +[] { return TransRange::Type::Colour; });
	lua_translation.addStaticProperty("RANGE_DESAT", +[] { return TransRange::Type::Desat; });
	lua_translation.addStaticProperty("RANGE_BLEND", +[] { return TransRange::Type::Blend; });
	lua_translation.addStaticProperty("RANGE_TINT", +[] { return TransRange::Type::Tint; });
	lua_translation.addStaticProperty("RANGE_SPECIAL", +[] { return TransRange::Type::Special; });

	// Functions
	// -------------------------------------------------------------------------
	lua_translation.addFunction("Range", &Translation::range);
	lua_translation.addFunction("Parse", &Translation::parse);
	lua_translation.addFunction("AddRange", &Translation::parseRange);
	lua_translation.addFunction(
		"AddPaletteRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangePalette>(self, TransRange::Type::Palette, range_start, range_end); });
	lua_translation.addFunction(
		"AddColourRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeColour>(self, TransRange::Type::Colour, range_start, range_end); });
	lua_translation.addFunction(
		"AddDesatRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeDesat>(self, TransRange::Type::Desat, range_start, range_end); });
	lua_translation.addFunction(
		"AddBlendRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeBlend>(self, TransRange::Type::Blend, range_start, range_end); });
	lua_translation.addFunction(
		"AddTintRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeTint>(self, TransRange::Type::Tint, range_start, range_end); });
	lua_translation.addFunction(
		"AddSpecialRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeSpecial>(self, TransRange::Type::Special, range_start, range_end); });
	lua_translation.addFunction(
		"ReadTable",
		[](Translation& self, const string& data) { self.read(reinterpret_cast<const uint8_t*>(data.data())); });
	lua_translation.addFunction("AsText", &Translation::asText);
	lua_translation.addFunction("Clear", &Translation::clear);
	lua_translation.addFunction("Copy", &Translation::copy);
	lua_translation.addFunction("IsEmpty", &Translation::isEmpty);
	lua_translation.addFunction(
		"Translate",
		&Translation::translate,
		[](Translation& self, const ColRGBA& col) { return self.translate(col); });
	lua_translation.addFunction("RemoveRange", &Translation::removeRange);
	lua_translation.addFunction("SwapRanges", &Translation::swapRanges);

	registerTranslationRangeTypes(lua);
}
} // namespace slade::lua
