
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Image.cpp
// Description: Functions to export the Image (SImage) type to lua using sol3
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
#include "General/Misc.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SImage.h"
#include "Utility/ColRGBA.h"
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
	return self.setPixel(x, y, std::clamp<uint8_t>(index, 0, 255), std::clamp<uint8_t>(alpha, 0, 255));
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
	lua_image["Clear"]              = sol::resolve<void()>(&SImage::clear);
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
		&SImage::drawPixel,
		[](SImage& self, int x, int y, ColRGBA& col, SImage::DrawProps& props)
		{ return self.drawPixel(x, y, col, props, nullptr); });
	lua_image["DrawImage"] = sol::overload(
		&imageDrawImage,
		[](SImage& self, int x, int y, SImage& img, SImage::DrawProps& props, Palette* pal_src)
		{ return imageDrawImage(self, x, y, img, props, pal_src, nullptr); },
		[](SImage& self, int x, int y, SImage& img, SImage::DrawProps& props)
		{ return imageDrawImage(self, x, y, img, props, nullptr, nullptr); });
	lua_image["Colourise"] = sol::overload(
		[](SImage& self, ColRGBA& col, Palette* pal) { self.colourise(col, pal); },
		[](SImage& self, ColRGBA& col) { self.colourise(col); });
	lua_image["Tint"] = sol::overload(
		[](SImage& self, ColRGBA& col, float amount, Palette* pal) { self.tint(col, amount, pal); },
		[](SImage& self, ColRGBA& col, float amount) { self.tint(col, amount); });
	lua_image["Trim"] = &SImage::adjust;
}
} // namespace slade::lua
