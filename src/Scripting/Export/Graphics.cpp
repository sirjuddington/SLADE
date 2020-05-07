
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Graphics.cpp
// Description: Functions to export graphics-related types and namespaces to lua
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
#include "General/Misc.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
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
// Registers the ImageConvertOptions (SIFormat::ConvertOptions) type with lua
// -----------------------------------------------------------------------------
void registerImageConvertOptionsType(sol::state& lua)
{
	auto lua_copt = lua.new_usertype<SIFormat::ConvertOptions>(
		"ImageConvertOptions", "new", sol::constructors<SIFormat::ConvertOptions()>());

	// Constants
	// -------------------------------------------------------------------------
	lua_copt["MASK_NONE"]       = sol::property([]() { return SIFormat::Mask::None; });
	lua_copt["MASK_COLOUR"]     = sol::property([]() { return SIFormat::Mask::Colour; });
	lua_copt["MASK_ALPHA"]      = sol::property([]() { return SIFormat::Mask::Alpha; });
	lua_copt["MASK_BRIGHTNESS"] = sol::property([]() { return SIFormat::Mask::Brightness; });

	// Properties
	// -------------------------------------------------------------------------
	lua_copt["paletteCurrent"] = &SIFormat::ConvertOptions::pal_current;
	lua_copt["paletteTarget"]  = &SIFormat::ConvertOptions::pal_target;
	lua_copt["maskSource"]     = &SIFormat::ConvertOptions::mask_source;
	lua_copt["maskColour"]     = &SIFormat::ConvertOptions::mask_colour;
	lua_copt["alphaThreshold"] = &SIFormat::ConvertOptions::alpha_threshold;
	lua_copt["transparency"]   = &SIFormat::ConvertOptions::transparency;
	lua_copt["pixelFormat"]    = &SIFormat::ConvertOptions::col_format;
}

// -----------------------------------------------------------------------------
// Registers the ImageFormat (SIFormat) type with lua
// -----------------------------------------------------------------------------
void registerImageFormatType(sol::state& lua)
{
	auto lua_iformat = lua.new_usertype<SIFormat>("ImageFormat", "new", sol::no_constructor);

	// Constants
	// -------------------------------------------------------------------------
	lua_iformat["WRITABLE_NO"]               = sol::property([]() { return SIFormat::Writable::No; });
	lua_iformat["WRITABLE_YES"]              = sol::property([]() { return SIFormat::Writable::Yes; });
	lua_iformat["WRITABLE_NEEDS_CONVERSION"] = sol::property([]() { return SIFormat::Writable::Convert; });

	// Properties
	// -------------------------------------------------------------------------
	lua_iformat["id"]        = sol::property(&SIFormat::id);
	lua_iformat["name"]      = sol::property(&SIFormat::name);
	lua_iformat["extension"] = sol::property(&SIFormat::extension);

	// Functions
	// -------------------------------------------------------------------------
	lua_iformat["IsThisFormat"]        = &SIFormat::isThisFormat;
	lua_iformat["CanWrite"]            = &SIFormat::canWrite;
	lua_iformat["CanWritePixelFormat"] = &SIFormat::canWriteType;
	lua_iformat["ConvertWritable"]     = &SIFormat::convertWritable;
	lua_iformat["LoadImage"]           = sol::overload(
        &SIFormat::loadImage, [](SIFormat& self, SImage& image, MemChunk& data) { self.loadImage(image, data); });
	lua_iformat["SaveImage"] = sol::overload(
		&SIFormat::saveImage,
		[](SIFormat& self, SImage& image, MemChunk& out) { self.saveImage(image, out); },
		[](SIFormat& self, SImage& image, MemChunk& out, Palette* pal) { self.saveImage(image, out, pal); });
}

// -----------------------------------------------------------------------------
// Registers the ImageDrawOptions (SImage::DrawProps) type with lua
// -----------------------------------------------------------------------------
void registerImageDrawOptionsType(sol::state& lua)
{
	auto lua_idopt = lua.new_usertype<SImage::DrawProps>("ImageDrawOptions", sol::constructors<SImage::DrawProps()>());

	// Properties
	// -------------------------------------------------------------------------
	lua_idopt["blend"]           = &SImage::DrawProps::blend;
	lua_idopt["alpha"]           = &SImage::DrawProps::alpha;
	lua_idopt["keepSourceAlpha"] = &SImage::DrawProps::src_alpha;
}

// -----------------------------------------------------------------------------
// Disambiguates the SImage::SetPixel function (colour version) for exporting to
// lua (as Image.SetPixelColour)
// -----------------------------------------------------------------------------
bool imageSetPixelCol(SImage& self, int x, int y, const ColRGBA& colour, Palette* pal)
{
	return self.setPixel(x, y, colour, pal);
}

// -----------------------------------------------------------------------------
// Disambiguates the SImage::SetPixel function (index version) for exporting to
// lua (as Image.SetPixelIndex)
// -----------------------------------------------------------------------------
bool imageSetPixelIndex(SImage& self, int x, int y, int index, int alpha)
{
	return self.setPixel(x, y, index, alpha);
}

// -----------------------------------------------------------------------------
// Disambiguates the SImage::ApplyTranslation function (Translation version) for
// exporting to lua (as Image.ApplyTranslation)
// -----------------------------------------------------------------------------
bool imageApplyTranslation(SImage& self, Translation* trans, Palette* pal, bool truecolour)
{
	return self.applyTranslation(trans, pal, truecolour);
}

// -----------------------------------------------------------------------------
// Rearranges the parameters of SImage::drawImage so that the coordinates are
// first in the script version (makes it consistent with DrawPixel)
// -----------------------------------------------------------------------------
bool imageDrawImage(
	SImage&            self,
	int                x,
	int                y,
	SImage&            img,
	SImage::DrawProps& props,
	Palette*           pal_src,
	Palette*           pal_dest)
{
	return self.drawImage(img, x, y, props, pal_src, pal_dest);
}

// -----------------------------------------------------------------------------
// Loads data from [entry] into image [self]
// -----------------------------------------------------------------------------
std::tuple<bool, string> imageLoadEntry(SImage& self, ArchiveEntry* entry, int index)
{
	bool ok = misc::loadImageFromEntry(&self, entry, index);
	return std::make_tuple(ok, global::error);
}

// -----------------------------------------------------------------------------
// Registers the Image (SImage) type with lua
// -----------------------------------------------------------------------------
void registerImageType(sol::state& lua)
{
	auto lua_image = lua.new_usertype<SImage>("Image", "new", sol::constructors<SImage(), SImage(SImage::Type)>());

	// Constants
	// -------------------------------------------------------------------------
	lua_image["TYPE_PALMASK"]      = sol::property([]() { return SImage::Type::PalMask; });
	lua_image["TYPE_RGBA"]         = sol::property([]() { return SImage::Type::RGBA; });
	lua_image["TYPE_ALPHAMAP"]     = sol::property([]() { return SImage::Type::AlphaMap; });
	lua_image["SOURCE_BRIGHTNESS"] = sol::property([]() { return SImage::AlphaSource::Brightness; });
	lua_image["SOURCE_ALPHA"]      = sol::property([]() { return SImage::AlphaSource::Alpha; });

	// Properties
	// -------------------------------------------------------------------------
	lua_image["type"]       = sol::property(&SImage::type);
	lua_image["width"]      = sol::property(&SImage::width);
	lua_image["height"]     = sol::property(&SImage::height);
	lua_image["hasPalette"] = sol::property(&SImage::hasPalette);
	lua_image["palette"]    = sol::property(&SImage::palette, &SImage::setPalette);
	lua_image["offsetX"]    = sol::property([](SImage& self) { return self.offset().x; }, &SImage::setXOffset);
	lua_image["offsetY"]    = sol::property([](SImage& self) { return self.offset().y; }, &SImage::setYOffset);
	lua_image["stride"]     = sol::property(&SImage::stride);
	lua_image["bpp"]        = sol::property(&SImage::bpp);

	// Functions
	// -------------------------------------------------------------------------
	lua_image["IsValid"] = &SImage::isValid;
	lua_image["PixelAt"] = sol::overload(
		&SImage::pixelAt, [](SImage& self, unsigned x, unsigned y) { return self.pixelAt(x, y); });
	lua_image["PixelIndexAt"]       = &SImage::pixelIndexAt;
	lua_image["FindUnusedColour"]   = &SImage::findUnusedColour;
	lua_image["CountUniqueColours"] = &SImage::countColours;
	lua_image["Clear"]              = &SImage::clear;
	lua_image["Create"]             = sol::overload(
        [](SImage& self, int w, int h, SImage::Type type, Palette* pal) { self.create(w, h, type, pal); },
        [](SImage& self, int w, int h, SImage::Type type) { self.create(w, h, type); });
	lua_image["Copy"]      = &SImage::copyImage;
	lua_image["FillAlpha"] = &SImage::fillAlpha;
	lua_image["LoadData"]  = sol::overload(
        [](SImage& self, MemChunk& mc) { self.open(mc); },
        [](SImage& self, MemChunk& mc, int index) { self.open(mc, index); },
        &SImage::open);
	lua_image["LoadEntry"] = sol::overload(
		[](SImage& self, ArchiveEntry* entry) { return imageLoadEntry(self, entry, 0); }, &imageLoadEntry);
	lua_image["WriteRGBAData"] = sol::overload(
		&SImage::putRGBAData, [](SImage& self, MemChunk& mc) { return self.putRGBAData(mc); });
	lua_image["WriteRGBData"] = sol::overload(
		&SImage::putRGBData, [](SImage& self, MemChunk& mc) { return self.putRGBData(mc); });
	lua_image["WriteIndexedData"] = &SImage::putIndexedData;
	lua_image["ConvertRGBA"]    = sol::overload(&SImage::convertRGBA, [](SImage& self) { return self.convertRGBA(); });
	lua_image["ConvertIndexed"] = sol::overload(
		&SImage::convertPaletted, [](SImage& self, Palette* p) { return self.convertPaletted(p); });
	lua_image["ConvertAlphaMap"] = sol::overload(
		&SImage::convertAlphaMap,
		[](SImage& self, SImage::AlphaSource source) { return self.convertAlphaMap(source); });
	lua_image["MaskFromColour"]     = &SImage::maskFromColour;
	lua_image["MaskFromBrightness"] = &SImage::maskFromBrightness;
	lua_image["SetPixelColour"]     = sol::overload(
        [](SImage& self, int x, int y, ColRGBA& col) { return imageSetPixelCol(self, x, y, col, nullptr); },
        &imageSetPixelCol);
	lua_image["SetPixelIndex"] = sol::overload(
		[](SImage& self, int x, int y, int index) { return imageSetPixelIndex(self, x, y, index, 255); },
		&imageSetPixelIndex);
	lua_image["Rotate"]           = &SImage::rotate;
	lua_image["MirrorVertical"]   = [](SImage& self) { return self.mirror(true); };
	lua_image["MirrorHorizontal"] = [](SImage& self) { return self.mirror(false); };
	lua_image["Crop"]             = &SImage::crop;
	lua_image["Resize"]           = &SImage::resize;
	lua_image["ApplyTranslation"] = sol::overload(
		[](SImage& self, Translation* t) { return imageApplyTranslation(self, t, nullptr, false); },
		[](SImage& self, Translation* t, Palette* p) { return imageApplyTranslation(self, t, p, false); },
		&imageApplyTranslation);
	lua_image["DrawPixel"] = sol::overload(
		&SImage::drawPixel, [](SImage& self, int x, int y, ColRGBA& col, SImage::DrawProps& props) {
			return self.drawPixel(x, y, col, props, nullptr);
		});
	lua_image["DrawImage"] = sol::overload(
		&imageDrawImage,
		[](SImage& self, int x, int y, SImage& img, SImage::DrawProps& props, Palette* pal_src) {
			return imageDrawImage(self, x, y, img, props, pal_src, nullptr);
		},
		[](SImage& self, int x, int y, SImage& img, SImage::DrawProps& props) {
			return imageDrawImage(self, x, y, img, props, nullptr, nullptr);
		});
	lua_image["Colourise"] = sol::overload(
		[](SImage& self, ColRGBA& col, Palette* pal) { self.colourise(col, pal); },
		[](SImage& self, ColRGBA& col) { self.colourise(col); });
	lua_image["Tint"] = sol::overload(
		[](SImage& self, ColRGBA& col, float amount, Palette* pal) { self.tint(col, amount, pal); },
		[](SImage& self, ColRGBA& col, float amount) { self.tint(col, amount); });
	lua_image["Trim"] = &SImage::adjust;
}

// -----------------------------------------------------------------------------
// Loads raw rgb triplet [data] into palette [self]
// -----------------------------------------------------------------------------
bool paletteLoadData(Palette& self, string_view data)
{
	return self.loadMem((const uint8_t*)data.data(), data.size());
}

// -----------------------------------------------------------------------------
// Loads [data] in [format] to palette [self]
// -----------------------------------------------------------------------------
bool paletteLoadDataFormatted(Palette& self, string_view data, Palette::Format format)
{
	MemChunk mc{ (const uint8_t*)data.data(), (uint32_t)data.size() };
	return self.loadMem(mc, format);
}

// -----------------------------------------------------------------------------
// Registers the Palette type with lua
// -----------------------------------------------------------------------------
void registerPaletteType(sol::state& lua)
{
	auto lua_palette = lua.new_usertype<Palette>("Palette", "new", sol::constructors<Palette(), Palette(unsigned)>());

	// Properties
	// -------------------------------------------------------------------------
	lua_palette["colourCount"] = sol::property([](Palette& self) { return self.colours().size(); });

	// Constants
	// -------------------------------------------------------------------------
	lua_palette["FORMAT_RAW"]    = sol::property([]() { return Palette::Format::Raw; });
	lua_palette["FORMAT_IMAGE"]  = sol::property([]() { return Palette::Format::Image; });
	lua_palette["FORMAT_CSV"]    = sol::property([]() { return Palette::Format::CSV; });
	lua_palette["FORMAT_JASC"]   = sol::property([]() { return Palette::Format::JASC; });
	lua_palette["FORMAT_GIMP"]   = sol::property([]() { return Palette::Format::GIMP; });
	lua_palette["MATCH_DEFAULT"] = sol::property([]() { return Palette::ColourMatch::Default; });
	lua_palette["MATCH_OLD"]     = sol::property([]() { return Palette::ColourMatch::Old; });
	lua_palette["MATCH_RGB"]     = sol::property([]() { return Palette::ColourMatch::RGB; });
	lua_palette["MATCH_HSL"]     = sol::property([]() { return Palette::ColourMatch::HSL; });
	lua_palette["MATCH_C76"]     = sol::property([]() { return Palette::ColourMatch::C76; });
	lua_palette["MATCH_C94"]     = sol::property([]() { return Palette::ColourMatch::C94; });
	lua_palette["MATCH_C2K"]     = sol::property([]() { return Palette::ColourMatch::C2K; });

	// Functions
	// -------------------------------------------------------------------------
	lua_palette["Colour"]   = &Palette::colour;
	lua_palette["LoadData"] = sol::overload(&paletteLoadData, &paletteLoadDataFormatted);
	lua_palette["LoadFile"] = sol::overload(
		&Palette::loadFile, [](Palette& self, string_view file) { self.loadFile(file); });
	lua_palette["SaveFile"] = sol::overload(
		&Palette::saveFile, [](Palette& self, string_view file) { self.saveFile(file); });
	lua_palette["SetColour"]     = &Palette::setColour;
	lua_palette["SetColourR"]    = &Palette::setColourR;
	lua_palette["SetColourG"]    = &Palette::setColourG;
	lua_palette["SetColourB"]    = &Palette::setColourB;
	lua_palette["SetColourA"]    = &Palette::setColourA;
	lua_palette["CopyColours"]   = &Palette::copyPalette;
	lua_palette["FindColour"]    = &Palette::findColour;
	lua_palette["NearestColour"] = sol::overload(
		&Palette::nearestColour, [](Palette& self, const ColRGBA& col) { return self.nearestColour(col); });
	lua_palette["CountUniqueColours"] = &Palette::countColours;
	lua_palette["ApplyTranslation"]   = &Palette::applyTranslation;
	lua_palette["Colourise"]          = &Palette::colourise;
	lua_palette["Tint"]               = &Palette::tint;
	lua_palette["Saturate"]           = &Palette::saturate;
	lua_palette["Illuminate"]         = &Palette::illuminate;
	lua_palette["Shift"]              = &Palette::shift;
	lua_palette["Invert"]             = &Palette::invert;
	lua_palette["Gradient"] = [](Palette& self, const ColRGBA& startC, const ColRGBA& endC, int startI, int endI) {
		self.setGradient(startI, endI, startC, endC);
	};
}

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
	lua_trange["type"]       = sol::property(&TransRange::type);
	lua_trange["rangeStart"] = sol::property(&TransRange::start, &TransRange::setStart);
	lua_trange["rangeEnd"]   = sol::property(&TransRange::end, &TransRange::setEnd);

	// Functions
	// -------------------------------------------------------------------------
	lua_trange["AsText"]         = &TransRange::asText;
	lua_trange["AsPaletteRange"] = [](TransRange& self) { return dynamic_cast<TransRangePalette*>(&self); };
	lua_trange["AsColourRange"]  = [](TransRange& self) { return dynamic_cast<TransRangeColour*>(&self); };
	lua_trange["AsDesatRange"]   = [](TransRange& self) { return dynamic_cast<TransRangeDesat*>(&self); };
	lua_trange["AsBlendRange"]   = [](TransRange& self) { return dynamic_cast<TransRangeBlend*>(&self); };
	lua_trange["AsTintRange"]    = [](TransRange& self) { return dynamic_cast<TransRangeTint*>(&self); };
	lua_trange["AsSpecialRange"] = [](TransRange& self) { return dynamic_cast<TransRangeSpecial*>(&self); };


	// -------------------------------------------------------------------------
	// TransRangePalette
	// -------------------------------------------------------------------------
	auto lua_trange_pal = lua.new_usertype<TransRangePalette>(
		"TransRangePalette", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_pal["destStart"] = sol::property(&TransRangePalette::dStart, &TransRangePalette::setDStart);
	lua_trange_pal["destEnd"]   = sol::property(&TransRangePalette::dEnd, &TransRangePalette::setDEnd);


	// -------------------------------------------------------------------------
	// TransRangeColour
	// -------------------------------------------------------------------------
	auto lua_trange_col = lua.new_usertype<TransRangeColour>(
		"TransRangeColour", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_col["startColour"] = sol::property(&TransRangeColour::startColour, &TransRangeColour::setStartColour);
	lua_trange_col["endColour"]   = sol::property(&TransRangeColour::endColour, &TransRangeColour::setEndColour);


	// -------------------------------------------------------------------------
	// TransRangeDesat
	// -------------------------------------------------------------------------
	auto lua_trange_desat = lua.new_usertype<TransRangeDesat>(
		"TransRangeDesat", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_desat["startR"] = sol::property(
		[](TransRangeDesat& self) { return self.rgbStart().r; },
		[](TransRangeDesat& self, float r) { self.setRGBStart(r, self.rgbStart().g, self.rgbStart().b); });
	lua_trange_desat["startG"] = sol::property(
		[](TransRangeDesat& self) { return self.rgbStart().g; },
		[](TransRangeDesat& self, float g) { self.setRGBStart(self.rgbStart().r, g, self.rgbStart().b); });
	lua_trange_desat["startB"] = sol::property(
		[](TransRangeDesat& self) { return self.rgbStart().b; },
		[](TransRangeDesat& self, float b) { self.setRGBStart(self.rgbStart().r, self.rgbStart().g, b); });
	lua_trange_desat["endR"] = sol::property(
		[](TransRangeDesat& self) { return self.rgbEnd().r; },
		[](TransRangeDesat& self, float r) { self.setRGBEnd(r, self.rgbEnd().g, self.rgbEnd().b); });
	lua_trange_desat["endG"] = sol::property(
		[](TransRangeDesat& self) { return self.rgbEnd().g; },
		[](TransRangeDesat& self, float g) { self.setRGBEnd(self.rgbEnd().r, g, self.rgbEnd().b); });
	lua_trange_desat["endB"] = sol::property(
		[](TransRangeDesat& self) { return self.rgbEnd().b; },
		[](TransRangeDesat& self, float b) { self.setRGBEnd(self.rgbEnd().r, self.rgbEnd().g, b); });

	// Functions
	// -------------------------------------------------------------------------
	lua_trange_desat["SetStartRGB"] = &TransRangeDesat::setRGBStart;
	lua_trange_desat["SetEndRGB"]   = &TransRangeDesat::setRGBEnd;


	// -------------------------------------------------------------------------
	// TransRangeBlend
	// -------------------------------------------------------------------------
	auto lua_trange_blend = lua.new_usertype<TransRangeBlend>(
		"TransRangeBlend", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_blend["colour"] = sol::property(&TransRangeBlend::colour, &TransRangeBlend::setColour);


	// -------------------------------------------------------------------------
	// TransRangeTint
	// -------------------------------------------------------------------------
	auto lua_trange_tint = lua.new_usertype<TransRangeTint>(
		"TransRangeTint", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_tint["colour"] = sol::property(&TransRangeTint::colour, &TransRangeTint::setColour);
	lua_trange_tint["amount"] = sol::property(&TransRangeTint::amount, &TransRangeTint::setAmount);


	// -------------------------------------------------------------------------
	// TransRangeSpecial
	// -------------------------------------------------------------------------
	auto lua_trange_special = lua.new_usertype<TransRangeSpecial>(
		"TransRangeSpecial", "new", sol::no_constructor, sol::base_classes, sol::bases<TransRange>());

	// Properties
	// -------------------------------------------------------------------------
	lua_trange_special["special"] = sol::property(&TransRangeSpecial::special, &TransRangeSpecial::setSpecial);
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
	lua_translation["ranges"]       = sol::property(&Translation::ranges);
	lua_translation["rangeCount"]   = sol::property(&Translation::nRanges);
	lua_translation["standardName"] = sol::property(&Translation::builtInName, &Translation::setBuiltInName);
	lua_translation["desatAmount"]  = sol::property(
        &Translation::desaturationAmount, &Translation::setDesaturationAmount);

	// Constants
	// -------------------------------------------------------------------------
	lua_translation["RANGE_PALETTE"] = sol::property([]() { return TransRange::Type::Palette; });
	lua_translation["RANGE_COLOUR"]  = sol::property([]() { return TransRange::Type::Colour; });
	lua_translation["RANGE_DESAT"]   = sol::property([]() { return TransRange::Type::Desat; });
	lua_translation["RANGE_BLEND"]   = sol::property([]() { return TransRange::Type::Blend; });
	lua_translation["RANGE_TINT"]    = sol::property([]() { return TransRange::Type::Tint; });
	lua_translation["RANGE_SPECIAL"] = sol::property([]() { return TransRange::Type::Special; });

	// Functions
	// -------------------------------------------------------------------------
	lua_translation["Range"]           = [](Translation& self, unsigned index) { return self.range(index - 1); };
	lua_translation["Parse"]           = &Translation::parse;
	lua_translation["AddRange"]        = &Translation::parseRange;
	lua_translation["AddPaletteRange"] = [](Translation& self, int range_start, int range_end) {
		return addTranslationRange<TransRangePalette>(self, TransRange::Type::Palette, range_start, range_end);
	};
	lua_translation["AddColourRange"] = [](Translation& self, int range_start, int range_end) {
		return addTranslationRange<TransRangeColour>(self, TransRange::Type::Colour, range_start, range_end);
	};
	lua_translation["AddDesatRange"] = [](Translation& self, int range_start, int range_end) {
		return addTranslationRange<TransRangeDesat>(self, TransRange::Type::Desat, range_start, range_end);
	};
	lua_translation["AddBlendRange"] = [](Translation& self, int range_start, int range_end) {
		return addTranslationRange<TransRangeBlend>(self, TransRange::Type::Blend, range_start, range_end);
	};
	lua_translation["AddTintRange"] = [](Translation& self, int range_start, int range_end) {
		return addTranslationRange<TransRangeTint>(self, TransRange::Type::Tint, range_start, range_end);
	};
	lua_translation["AddSpecialRange"] = [](Translation& self, int range_start, int range_end) {
		return addTranslationRange<TransRangeSpecial>(self, TransRange::Type::Special, range_start, range_end);
	};
	lua_translation["ReadTable"] = &Translation::read;
	lua_translation["AsText"]    = &Translation::asText;
	lua_translation["Clear"]     = &Translation::clear;
	lua_translation["Copy"]      = &Translation::copy;
	lua_translation["IsEmpty"]   = &Translation::isEmpty;
	lua_translation["Translate"] = sol::overload(
		&Translation::translate, [](Translation& self, ColRGBA col) { return self.translate(col); });
	lua_translation["RemoveRange"] = [](Translation& self, int pos) { self.removeRange(pos - 1); };
	lua_translation["SwapRanges"]  = [](Translation& self, int pos1, int pos2) { self.swapRanges(pos1 - 1, pos2 - 1); };

	registerTranslationRangeTypes(lua);
}

// -----------------------------------------------------------------------------
// Registers the CTPatch and CTPatchEx types with lua
// -----------------------------------------------------------------------------
void registerCTexturePatchTypes(sol::state& lua)
{
	// -------------------------------------------------------------------------
	// CTPatch
	// -------------------------------------------------------------------------
	auto lua_ctpatch = lua.new_usertype<CTPatch>("CTPatch", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_ctpatch["name"]    = sol::property(&CTPatch::name, &CTPatch::setName);
	lua_ctpatch["offsetX"] = sol::property(&CTPatch::xOffset, &CTPatch::setOffsetX);
	lua_ctpatch["offsetY"] = sol::property(&CTPatch::yOffset, &CTPatch::setOffsetY);
	/*lua_ctpatch["offset"] = sol::property(
		[](CTPatch& self) { return Vec2d(self.xOffset(), self.yOffset()); },
		[](CTPatch& self, double x, double y) {
			self.setOffsetX(x);
			self.setOffsetY(y);
		});*/

	// Functions
	// -------------------------------------------------------------------------
	lua_ctpatch["PatchEntry"] = &CTPatch::patchEntry;
	lua_ctpatch["AsExtended"] = [](CTPatch& self) { return dynamic_cast<CTPatchEx*>(&self); };


	// -------------------------------------------------------------------------
	// CTPatchEx
	// -------------------------------------------------------------------------
	auto lua_ctpatch_ex = lua.new_usertype<CTPatchEx>(
		"CTPatchEx", "new", sol::no_constructor, sol::base_classes, sol::bases<CTPatch>());

	// Constants
	// -------------------------------------------------------------------------
	lua_ctpatch_ex["TYPE_PATCH"]            = sol::property([]() { return CTPatchEx::Type::Patch; });
	lua_ctpatch_ex["TYPE_GRAPHIC"]          = sol::property([]() { return CTPatchEx::Type::Graphic; });
	lua_ctpatch_ex["BLENDTYPE_NONE"]        = sol::property([]() { return CTPatchEx::BlendType::None; });
	lua_ctpatch_ex["BLENDTYPE_TRANSLATION"] = sol::property([]() { return CTPatchEx::BlendType::Translation; });
	lua_ctpatch_ex["BLENDTYPE_BLEND"]       = sol::property([]() { return CTPatchEx::BlendType::Blend; });
	lua_ctpatch_ex["BLENDTYPE_TINT"]        = sol::property([]() { return CTPatchEx::BlendType::Tint; });

	// Properties
	// -------------------------------------------------------------------------
	lua_ctpatch_ex["flipX"]       = sol::property(&CTPatchEx::flipX, &CTPatchEx::setFlipX);
	lua_ctpatch_ex["flipY"]       = sol::property(&CTPatchEx::flipY, &CTPatchEx::setFlipY);
	lua_ctpatch_ex["useOffsets"]  = sol::property(&CTPatchEx::useOffsets, &CTPatchEx::setUseOffsets);
	lua_ctpatch_ex["rotation"]    = sol::property(&CTPatchEx::rotation, &CTPatchEx::setRotation);
	lua_ctpatch_ex["colour"]      = sol::property(&CTPatchEx::colour, &CTPatchEx::setColour);
	lua_ctpatch_ex["alpha"]       = sol::property(&CTPatchEx::alpha, &CTPatchEx::setAlpha);
	lua_ctpatch_ex["style"]       = sol::property(&CTPatchEx::style, &CTPatchEx::setStyle);
	lua_ctpatch_ex["blendType"]   = sol::property(&CTPatchEx::blendType, &CTPatchEx::setBlendType);
	lua_ctpatch_ex["translation"] = &CTPatchEx::translation;

	// Functions
	// -------------------------------------------------------------------------
	lua_ctpatch_ex["AsText"] = &CTPatchEx::asText;
	// parse?
}

// -----------------------------------------------------------------------------
// Wrapper for CTexture::addPatch that changes the index to be 1-based
// -----------------------------------------------------------------------------
void cTextureAddPatch(CTexture& self, string_view patch, int x, int y, int index)
{
	self.addPatch(patch, x, y, index - 1);
}

// -----------------------------------------------------------------------------
// Wrapper for CTexture::duplicatePatch that changes the index to be 1-based
// -----------------------------------------------------------------------------
bool cTextureDuplicatePatch(CTexture& self, int index, int offset_x, int offset_y)
{
	return self.duplicatePatch(index - 1, offset_x, offset_y);
}

// -----------------------------------------------------------------------------
// Registers the CTexture type with lua
// -----------------------------------------------------------------------------
void registerCTextureType(sol::state& lua)
{
	auto lua_ctexture = lua.new_usertype<CTexture>("CTexture", "new", sol::no_constructor);

	// Properties
	// -------------------------------------------------------------------------
	lua_ctexture["patches"]      = sol::property(&CTexture::patches);
	lua_ctexture["name"]         = sol::property(&CTexture::name, &CTexture::setName);
	lua_ctexture["width"]        = sol::property(&CTexture::width, &CTexture::setWidth);
	lua_ctexture["height"]       = sol::property(&CTexture::height, &CTexture::setHeight);
	lua_ctexture["scaleX"]       = sol::property(&CTexture::scaleX, &CTexture::setScaleX);
	lua_ctexture["scaleY"]       = sol::property(&CTexture::scaleY, &CTexture::setScaleY);
	lua_ctexture["offsetX"]      = sol::property(&CTexture::offsetX, &CTexture::setOffsetX);
	lua_ctexture["offsetY"]      = sol::property(&CTexture::offsetY, &CTexture::setOffsetY);
	lua_ctexture["worldPanning"] = sol::property(&CTexture::worldPanning, &CTexture::setWorldPanning);
	lua_ctexture["type"]         = sol::property(&CTexture::type, &CTexture::setType);
	lua_ctexture["extended"]     = sol::property(&CTexture::isExtended, &CTexture::setExtended);
	lua_ctexture["optional"]     = sol::property(&CTexture::isOptional, &CTexture::setOptional);
	lua_ctexture["noDecals"]     = sol::property(&CTexture::noDecals, &CTexture::setNoDecals);
	lua_ctexture["nullTexture"]  = sol::property(&CTexture::nullTexture, &CTexture::setNullTexture);

	// Functions
	// -------------------------------------------------------------------------
	lua_ctexture["CopyTexture"] = sol::overload(
		&CTexture::copyTexture, [](CTexture& self, const CTexture& tex) { return self.copyTexture(tex, false); });
	lua_ctexture["Clear"]    = &CTexture::clear;
	lua_ctexture["AddPatch"] = sol::overload(
		&cTextureAddPatch,
		[](CTexture& self, string_view patch) { return cTextureAddPatch(self, patch, 0, 0, 0); },
		[](CTexture& self, string_view patch, int x, int y) { return cTextureAddPatch(self, patch, x, y, 0); });
	lua_ctexture["RemovePatch"]  = [](CTexture& self, int index) { return self.removePatch(index - 1); };
	lua_ctexture["ReplacePatch"] = [](CTexture& self, int index, string_view patch) {
		return self.replacePatch(index - 1, patch);
	};
	lua_ctexture["DuplicatePatch"] = sol::overload(
		&cTextureDuplicatePatch, [](CTexture& self, int index) { cTextureDuplicatePatch(self, index, 8, 8); });
	lua_ctexture["SwapPatches"]     = [](CTexture& self, int p1, int p2) { return self.swapPatches(p1 - 1, p2 - 1); };
	lua_ctexture["AsText"]          = &CTexture::asText;
	lua_ctexture["ConvertExtended"] = &CTexture::convertExtended;
	lua_ctexture["ConvertRegular"]  = &CTexture::convertRegular;
}

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
	lua_ptable["Patch"]      = [](PatchTable& self, unsigned index) { return self.patch(index - 1).name; };
	lua_ptable["PatchEntry"] = sol::overload(
		[](PatchTable& self, int index) { return self.patchEntry(index - 1); },
		sol::resolve<ArchiveEntry*(string_view)>(&PatchTable::patchEntry));
	lua_ptable["RemovePatch"]  = [](PatchTable& self, int index) { self.removePatch(index - 1); };
	lua_ptable["ReplacePatch"] = [](PatchTable& self, int index, string_view name) {
		self.replacePatch(index - 1, name);
	};
	lua_ptable["AddPatch"]    = &PatchTable::addPatch;
	lua_ptable["LoadPNAMES"]  = [](PatchTable& self, ArchiveEntry* pnames) { return self.loadPNAMES(pnames); };
	lua_ptable["WritePNAMES"] = &PatchTable::writePNAMES;
}

// -----------------------------------------------------------------------------
// Wrapper for TextureXList::addTexture that changes the index to be 1-based and
// returns a pointer to the added texture
// -----------------------------------------------------------------------------
CTexture* addTexture(TextureXList& self, string_view name, bool extended, int position)
{
	auto tex     = std::make_unique<CTexture>(name, extended);
	auto tex_ptr = tex.get();
	self.addTexture(std::move(tex), position - 1);
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
	lua_txlist["Texture"]      = sol::resolve<CTexture*(string_view)>(&TextureXList::texture);
	lua_txlist["TextureIndex"] = [](TextureXList& self, string_view name) { return self.textureIndex(name) + 1; };
	lua_txlist["AddTexture"]   = sol::overload(
        &addTexture,
        [](TextureXList& self, string_view name) { return addTexture(self, name, false, 0); },
        [](TextureXList& self, string_view name, bool extended) { return addTexture(self, name, extended, 0); });
	lua_txlist["RemoveTexture"]     = [](TextureXList& self, int index) { self.removeTexture(index - 1); };
	lua_txlist["SwapTextures"]      = [](TextureXList& self, int i1, int i2) { self.swapTextures(i1 - 1, i2 - 1); };
	lua_txlist["Clear"]             = [](TextureXList& self) { self.clear(); };
	lua_txlist["RemovePatch"]       = &TextureXList::removePatch;
	lua_txlist["ReadTEXTUREXData"]  = &TextureXList::readTEXTUREXData;
	lua_txlist["WriteTEXTUREXData"] = &TextureXList::writeTEXTUREXData;
	lua_txlist["ReadTEXTURESData"]  = &TextureXList::readTEXTURESData;
	lua_txlist["WriteTEXTURESData"] = &TextureXList::writeTEXTURESData;
	lua_txlist["ConvertToTEXTURES"] = &TextureXList::convertToTEXTURES;
	lua_txlist["FindErrors"]        = &TextureXList::findErrors;
}

// -----------------------------------------------------------------------------
// Registers all graphics-related types with lua
// -----------------------------------------------------------------------------
void registerGraphicsTypes(sol::state& lua)
{
	registerImageConvertOptionsType(lua);
	registerImageFormatType(lua);
	registerImageType(lua);
	registerTranslationType(lua);
	registerCTexturePatchTypes(lua);
	registerCTextureType(lua);
	registerPatchTableType(lua);
	registerTextureXListType(lua);
}

// -----------------------------------------------------------------------------
// Registers the Graphics function namespace with lua
// -----------------------------------------------------------------------------
void registerGraphicsNamespace(sol::state& lua)
{
	auto gfx = lua.create_named_table("Graphics");

	// Constants
	// -------------------------------------------------------------------------
	gfx["BLEND_NORMAL"]          = sol::property([]() { return SImage::BlendType::Normal; });
	gfx["BLEND_ADD"]             = sol::property([]() { return SImage::BlendType::Add; });
	gfx["BLEND_SUBTRACT"]        = sol::property([]() { return SImage::BlendType::Subtract; });
	gfx["BLEND_REVERSESUBTRACT"] = sol::property([]() { return SImage::BlendType::ReverseSubtract; });
	gfx["BLEND_MODULATE"]        = sol::property([]() { return SImage::BlendType::Modulate; });

	// Functions
	// -------------------------------------------------------------------------
	gfx["ImageFormat"]     = [](string_view id) { return SIFormat::getFormat(id); };
	gfx["AllImageFormats"] = []() {
		vector<SIFormat*> formats;
		SIFormat::putAllFormats(formats);
		return formats;
	};
	gfx["DetectImageFormat"] = [](MemChunk& mc) { return SIFormat::determineFormat(mc); };
}

} // namespace slade::lua
