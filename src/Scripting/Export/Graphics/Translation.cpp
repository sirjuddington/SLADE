
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
// Registers the TransRange type (and TransRange* sub-types) with lua
// -----------------------------------------------------------------------------
void registerTranslationRangeTypes(sol::state& lua)
{
	// -------------------------------------------------------------------------
	// TransRange
	// -------------------------------------------------------------------------
	auto lua_trange = lua.new_usertype<TransRange>("TransRange", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_trange.set("type", sol::property(&TransRange::type));
	lua_trange.set("rangeStart", sol::property(&TransRange::start, &TransRange::setStart));
	lua_trange.set("rangeEnd", sol::property(&TransRange::end, &TransRange::setEnd));

	// Functions
	// -------------------------------------------------------------------------
	lua_trange.set_function("AsText", &TransRange::asText);
	lua_trange.set_function("AsPaletteRange", [](TransRange& self) { return dynamic_cast<TransRangePalette*>(&self); });
	lua_trange.set_function("AsColourRange", [](TransRange& self) { return dynamic_cast<TransRangeColour*>(&self); });
	lua_trange.set_function("AsDesatRange", [](TransRange& self) { return dynamic_cast<TransRangeDesat*>(&self); });
	lua_trange.set_function("AsBlendRange", [](TransRange& self) { return dynamic_cast<TransRangeBlend*>(&self); });
	lua_trange.set_function("AsTintRange", [](TransRange& self) { return dynamic_cast<TransRangeTint*>(&self); });
	lua_trange.set_function("AsSpecialRange", [](TransRange& self) { return dynamic_cast<TransRangeSpecial*>(&self); });


	// -------------------------------------------------------------------------
	// TransRangePalette
	// -------------------------------------------------------------------------
	auto lua_trange_pal = lua.new_usertype<TransRangePalette>(
		"TransRangePalette", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_pal.set("destStart", sol::property(&TransRangePalette::dStart, &TransRangePalette::setDStart));
	lua_trange_pal.set("destEnd", sol::property(&TransRangePalette::dEnd, &TransRangePalette::setDEnd));


	// -------------------------------------------------------------------------
	// TransRangeColour
	// -------------------------------------------------------------------------
	auto lua_trange_col = lua.new_usertype<TransRangeColour>(
		"TransRangeColour", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_col.set("startColour", sol::property(&TransRangeColour::startColour, &TransRangeColour::setStartColour));
	lua_trange_col.set("endColour", sol::property(&TransRangeColour::endColour, &TransRangeColour::setEndColour));


	// -------------------------------------------------------------------------
	// TransRangeDesat
	// -------------------------------------------------------------------------
	auto lua_trange_desat = lua.new_usertype<TransRangeDesat>(
		"TransRangeDesat", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_desat.set(
		"startR",
		sol::property(
			[](TransRangeDesat& self) { return self.rgbStart().r; },
			[](TransRangeDesat& self, float r) { self.setRGBStart(r, self.rgbStart().g, self.rgbStart().b); }));
	lua_trange_desat.set(
		"startG",
		sol::property(
			[](TransRangeDesat& self) { return self.rgbStart().g; },
			[](TransRangeDesat& self, float g) { self.setRGBStart(self.rgbStart().r, g, self.rgbStart().b); }));
	lua_trange_desat.set(
		"startB",
		sol::property(
			[](TransRangeDesat& self) { return self.rgbStart().b; },
			[](TransRangeDesat& self, float b) { self.setRGBStart(self.rgbStart().r, self.rgbStart().g, b); }));
	lua_trange_desat.set(
		"endR",
		sol::property(
			[](TransRangeDesat& self) { return self.rgbEnd().r; },
			[](TransRangeDesat& self, float r) { self.setRGBEnd(r, self.rgbEnd().g, self.rgbEnd().b); }));
	lua_trange_desat.set(
		"endG",
		sol::property(
			[](TransRangeDesat& self) { return self.rgbEnd().g; },
			[](TransRangeDesat& self, float g) { self.setRGBEnd(self.rgbEnd().r, g, self.rgbEnd().b); }));
	lua_trange_desat.set(
		"endB",
		sol::property(
			[](TransRangeDesat& self) { return self.rgbEnd().b; },
			[](TransRangeDesat& self, float b) { self.setRGBEnd(self.rgbEnd().r, self.rgbEnd().g, b); }));

	// Functions
	// -------------------------------------------------------------------------
	lua_trange_desat.set_function("SetStartRGB", &TransRangeDesat::setRGBStart);
	lua_trange_desat.set_function("SetEndRGB", &TransRangeDesat::setRGBEnd);


	// -------------------------------------------------------------------------
	// TransRangeBlend
	// -------------------------------------------------------------------------
	auto lua_trange_blend = lua.new_usertype<TransRangeBlend>(
		"TransRangeBlend", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_blend.set("colour", sol::property(&TransRangeBlend::colour, &TransRangeBlend::setColour));


	// -------------------------------------------------------------------------
	// TransRangeTint
	// -------------------------------------------------------------------------
	auto lua_trange_tint = lua.new_usertype<TransRangeTint>(
		"TransRangeTint", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_tint.set("colour", sol::property(&TransRangeTint::colour, &TransRangeTint::setColour));
	lua_trange_tint.set("amount", sol::property(&TransRangeTint::amount, &TransRangeTint::setAmount));


	// -------------------------------------------------------------------------
	// TransRangeSpecial
	// -------------------------------------------------------------------------
	auto lua_trange_special = lua.new_usertype<TransRangeSpecial>(
		"TransRangeSpecial", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_special.set("special", sol::property(&TransRangeSpecial::special, &TransRangeSpecial::setSpecial));
}

// -----------------------------------------------------------------------------
// Adds a new translation range of [type] to the translation [self], with its
// initial range being from [range_start] to [range_end].
// Returns the TransRange that was added (cast to type T)
// -----------------------------------------------------------------------------
template<typename T> T* addTranslationRange(Translation& self, TransRange::Type type, int range_start, int range_end)
{
	return dynamic_cast<T*>(self.addRange(type, -1, range_start, range_end));
}

// -----------------------------------------------------------------------------
// Registers the Translation type with lua
// -----------------------------------------------------------------------------
void registerTranslationType(sol::state& lua)
{
	auto lua_translation = lua.new_usertype<Translation>("Translation", "new", sol::constructors<Translation()>());

	// Properties
	// -------------------------------------------------------------------------
	lua_translation.set("ranges", sol::property(&Translation::ranges));
	lua_translation.set("rangeCount", sol::property(&Translation::nRanges));
	lua_translation.set("standardName", sol::property(&Translation::builtInName, &Translation::setBuiltInName));
	lua_translation.set(
		"desatAmount", sol::property(&Translation::desaturationAmount, &Translation::setDesaturationAmount));

	// Constants
	// -------------------------------------------------------------------------
	lua_translation.set("RANGE_PALETTE", sol::property([]() { return TransRange::Type::Palette; }));
	lua_translation.set("RANGE_COLOUR", sol::property([]() { return TransRange::Type::Colour; }));
	lua_translation.set("RANGE_DESAT", sol::property([]() { return TransRange::Type::Desat; }));
	lua_translation.set("RANGE_BLEND", sol::property([]() { return TransRange::Type::Blend; }));
	lua_translation.set("RANGE_TINT", sol::property([]() { return TransRange::Type::Tint; }));
	lua_translation.set("RANGE_SPECIAL", sol::property([]() { return TransRange::Type::Special; }));

	// Functions
	// -------------------------------------------------------------------------
	lua_translation.set_function("Range", &Translation::range);
	lua_translation.set_function("Parse", &Translation::parse);
	lua_translation.set_function("AddRange", &Translation::parseRange);
	lua_translation.set_function(
		"AddPaletteRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangePalette>(self, TransRange::Type::Palette, range_start, range_end); });
	lua_translation.set_function(
		"AddColourRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeColour>(self, TransRange::Type::Colour, range_start, range_end); });
	lua_translation.set_function(
		"AddDesatRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeDesat>(self, TransRange::Type::Desat, range_start, range_end); });
	lua_translation.set_function(
		"AddBlendRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeBlend>(self, TransRange::Type::Blend, range_start, range_end); });
	lua_translation.set_function(
		"AddTintRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeTint>(self, TransRange::Type::Tint, range_start, range_end); });
	lua_translation.set_function(
		"AddSpecialRange",
		[](Translation& self, int range_start, int range_end)
		{ return addTranslationRange<TransRangeSpecial>(self, TransRange::Type::Special, range_start, range_end); });
	lua_translation.set_function("ReadTable", &Translation::read);
	lua_translation.set_function("AsText", &Translation::asText);
	lua_translation.set_function("Clear", &Translation::clear);
	lua_translation.set_function("Copy", &Translation::copy);
	lua_translation.set_function("IsEmpty", &Translation::isEmpty);
	lua_translation.set_function(
		"Translate",
		sol::overload(
			&Translation::translate, [](Translation& self, const ColRGBA& col) { return self.translate(col); }));
	lua_translation.set_function("RemoveRange", &Translation::removeRange);
	lua_translation.set_function("SwapRanges", &Translation::swapRanges);

	registerTranslationRangeTypes(lua);
}
} // namespace slade::lua
