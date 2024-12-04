
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
#include "Scripting/Export/Export.h"
#include "Scripting/LuaBridge.h"

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
static bool imageSetPixelCol(SImage& self, int x, int y, const ColRGBA& colour, Palette* pal)
{
	return self.setPixel(x, y, colour, pal);
}

// -----------------------------------------------------------------------------
// Disambiguates the SImage::SetPixel function (index version) for exporting to
// lua (as Image.SetPixelIndex)
// -----------------------------------------------------------------------------
static bool imageSetPixelIndex(SImage& self, int x, int y, int index, int alpha)
{
	return self.setPixel(x, y, std::clamp<uint8_t>(index, 0, 255), std::clamp<uint8_t>(alpha, 0, 255));
}

// -----------------------------------------------------------------------------
// Disambiguates the SImage::ApplyTranslation function (Translation version) for
// exporting to lua (as Image.ApplyTranslation)
// -----------------------------------------------------------------------------
static bool imageApplyTranslation(SImage& self, Translation* trans, Palette* pal, bool truecolour)
{
	return self.applyTranslation(trans, pal, truecolour);
}

// -----------------------------------------------------------------------------
// Rearranges the parameters of SImage::drawImage so that the coordinates are
// first in the script version (makes it consistent with DrawPixel)
// -----------------------------------------------------------------------------
static bool imageDrawImage(
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
static std::tuple<bool, string> imageLoadEntry(SImage& self, ArchiveEntry* entry, int index)
{
	bool ok = misc::loadImageFromEntry(&self, entry, index);
	return std::make_tuple(ok, global::error);
}

// -----------------------------------------------------------------------------
// Registers the Image (SImage) type with lua
// -----------------------------------------------------------------------------
void registerImageType(lua_State* lua)
{
	auto lua_image = luabridge::getGlobalNamespace(lua).beginClass<SImage>("Image");
	lua_image.addConstructor<void(), void(SImage::Type)>();

	// Constants
	// -------------------------------------------------------------------------
	ADD_CLASS_CONSTANT(lua_image, "TYPE_PALMASK", SImage::Type::PalMask);
	ADD_CLASS_CONSTANT(lua_image, "TYPE_RGBA", SImage::Type::RGBA);
	ADD_CLASS_CONSTANT(lua_image, "TYPE_ALPHAMAP", SImage::Type::AlphaMap);
	ADD_CLASS_CONSTANT(lua_image, "SOURCE_BRIGHTNESS", SImage::AlphaSource::Brightness);
	ADD_CLASS_CONSTANT(lua_image, "SOURCE_ALPHA", SImage::AlphaSource::Alpha);

	// Properties
	// -------------------------------------------------------------------------
	lua_image.addProperty("type", &SImage::type);
	lua_image.addProperty("width", &SImage::width);
	lua_image.addProperty("height", &SImage::height);
	lua_image.addProperty("hasPalette", &SImage::hasPalette);
	lua_image.addProperty("palette", &SImage::palette, &SImage::setPalette);
	lua_image.addProperty("offsetX", [](SImage& self) { return self.offset().x; }, &SImage::setXOffset);
	lua_image.addProperty("offsetY", [](SImage& self) { return self.offset().y; }, &SImage::setYOffset);
	lua_image.addProperty("stride", &SImage::stride);
	lua_image.addProperty("bpp", &SImage::bpp);

	// Functions
	// -------------------------------------------------------------------------
	lua_image.addFunction("IsValid", &SImage::isValid);
	lua_image.addFunction(
		"PixelAt", &SImage::pixelAt, [](SImage& self, unsigned x, unsigned y) { return self.pixelAt(x, y); });
	lua_image.addFunction("PixelIndexAt", &SImage::pixelIndexAt);
	lua_image.addFunction("FindUnusedColour", &SImage::findUnusedColour);
	lua_image.addFunction("CountUniqueColours", &SImage::countColours);
	lua_image.addFunction("Clear", [](SImage& self) { self.clear(); });
	lua_image.addFunction(
		"Create",
		[](SImage& self, int w, int h, SImage::Type type, Palette* pal) { self.create(w, h, type, pal); },
		[](SImage& self, int w, int h, SImage::Type type) { self.create(w, h, type); });
	lua_image.addFunction("Copy", &SImage::copyImage);
	lua_image.addFunction("FillAlpha", &SImage::fillAlpha);
	lua_image.addFunction(
		"LoadData",
		[](SImage& self, MemChunk& mc) { self.open(mc); },
		[](SImage& self, MemChunk& mc, int index) { self.open(mc, index); },
		&SImage::open);
	lua_image.addFunction(
		"LoadEntry", [](SImage& self, ArchiveEntry* entry) { return imageLoadEntry(self, entry, 0); }, &imageLoadEntry);
	lua_image.addFunction(
		"WriteRGBAData", &SImage::putRGBAData, [](SImage& self, MemChunk& mc) { return self.putRGBAData(mc); });
	lua_image.addFunction(
		"WriteRGBData", &SImage::putRGBData, [](SImage& self, MemChunk& mc) { return self.putRGBData(mc); });
	lua_image.addFunction("WriteIndexedData", &SImage::putIndexedData);
	lua_image.addFunction("ConvertRGBA", &SImage::convertRGBA, [](SImage& self) { return self.convertRGBA(); });
	lua_image.addFunction(
		"ConvertIndexed", &SImage::convertPaletted, [](SImage& self, Palette* p) { return self.convertPaletted(p); });
	lua_image.addFunction(
		"ConvertAlphaMap",
		&SImage::convertAlphaMap,
		[](SImage& self, SImage::AlphaSource source) { return self.convertAlphaMap(source); });
	lua_image.addFunction("MaskFromColour", &SImage::maskFromColour);
	lua_image.addFunction("MaskFromBrightness", &SImage::maskFromBrightness);
	lua_image.addFunction(
		"SetPixelColour",
		[](SImage& self, int x, int y, ColRGBA& col) { return imageSetPixelCol(self, x, y, col, nullptr); },
		&imageSetPixelCol);
	lua_image.addFunction(
		"SetPixelIndex",
		[](SImage& self, int x, int y, int index) { return imageSetPixelIndex(self, x, y, index, 255); },
		&imageSetPixelIndex);
	lua_image.addFunction("Rotate", &SImage::rotate);
	lua_image.addFunction("MirrorVertical", [](SImage& self) { return self.mirror(true); });
	lua_image.addFunction("MirrorHorizontal", [](SImage& self) { return self.mirror(false); });
	lua_image.addFunction("Crop", &SImage::crop);
	lua_image.addFunction("Resize", &SImage::resize);
	lua_image.addFunction(
		"ApplyTranslation",
		[](SImage& self, Translation* t) { return imageApplyTranslation(self, t, nullptr, false); },
		[](SImage& self, Translation* t, Palette* p) { return imageApplyTranslation(self, t, p, false); },
		&imageApplyTranslation);
	lua_image.addFunction(
		"DrawPixel",
		&SImage::drawPixel,
		[](SImage& self, int x, int y, ColRGBA& col, SImage::DrawProps& props)
		{ return self.drawPixel(x, y, col, props, nullptr); });
	lua_image.addFunction(
		"DrawImage",
		&imageDrawImage,
		[](SImage& self, int x, int y, SImage& img, SImage::DrawProps& props, Palette* pal_src)
		{ return imageDrawImage(self, x, y, img, props, pal_src, nullptr); },
		[](SImage& self, int x, int y, SImage& img, SImage::DrawProps& props)
		{ return imageDrawImage(self, x, y, img, props, nullptr, nullptr); });
	lua_image.addFunction(
		"Colourise",
		[](SImage& self, ColRGBA& col, Palette* pal) { self.colourise(col, pal); },
		[](SImage& self, ColRGBA& col) { self.colourise(col); });
	lua_image.addFunction(
		"Tint",
		[](SImage& self, ColRGBA& col, float amount, Palette* pal) { self.tint(col, amount, pal); },
		[](SImage& self, ColRGBA& col, float amount) { self.tint(col, amount); });
	lua_image.addFunction("Trim", &SImage::adjust);
}
} // namespace slade::lua
