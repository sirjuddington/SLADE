
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Export.h"
#include "Graphics/Palette/Palette.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
#include "Scripting/Lua.h"
#include "Scripting/LuaBridge.h"

using namespace slade;

// ReSharper disable CppParameterMayBeConstPtrOrRef


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
static void registerImageConvertOptionsType(lua_State* lua)
{
	auto lua_copt = luabridge::getGlobalNamespace(lua).beginClass<SIFormat::ConvertOptions>("ImageConvertOptions");
	lua_copt.addConstructor<void()>();

	// Constants
	// -------------------------------------------------------------------------
	ADD_CLASS_CONSTANT(lua_copt, "MASK_NONE", SIFormat::Mask::None);
	ADD_CLASS_CONSTANT(lua_copt, "MASK_COLOUR", SIFormat::Mask::Colour);
	ADD_CLASS_CONSTANT(lua_copt, "MASK_ALPHA", SIFormat::Mask::Alpha);
	ADD_CLASS_CONSTANT(lua_copt, "MASK_BRIGHTNESS", SIFormat::Mask::Brightness);

	// Properties
	// -------------------------------------------------------------------------
	using CO = SIFormat::ConvertOptions;
	lua_copt.addProperty("paletteCurrent", &CO::pal_current, &CO::pal_current);
	lua_copt.addProperty("paletteTarget", &CO::pal_target, &CO::pal_target);
	lua_copt.addProperty("maskSource", &CO::mask_source, &CO::mask_source);
	lua_copt.addProperty("maskColour", &CO::mask_colour, &CO::mask_colour);
	lua_copt.addProperty("alphaThreshold", &CO::alpha_threshold, &CO::alpha_threshold);
	lua_copt.addProperty("transparency", &CO::transparency, &CO::transparency);
	lua_copt.addProperty("pixelFormat", &CO::col_format, &CO::col_format);
}

// -----------------------------------------------------------------------------
// Registers the ImageFormat (SIFormat) type with lua
// -----------------------------------------------------------------------------
static void registerImageFormatType(lua_State* lua)
{
	auto lua_iformat = luabridge::getGlobalNamespace(lua).beginClass<SIFormat>("ImageFormat");

	// Constants
	// -------------------------------------------------------------------------
	ADD_CLASS_CONSTANT(lua_iformat, "WRITABLE_NO", SIFormat::Writable::No);
	ADD_CLASS_CONSTANT(lua_iformat, "WRITABLE_YES", SIFormat::Writable::Yes);
	ADD_CLASS_CONSTANT(lua_iformat, "WRITABLE_NEEDS_CONVERSION", SIFormat::Writable::Convert);

	// Properties
	// -------------------------------------------------------------------------
	lua_iformat.addProperty("id", &SIFormat::id);
	lua_iformat.addProperty("name", &SIFormat::name);
	lua_iformat.addProperty("extension", &SIFormat::extension);

	// Functions
	// -------------------------------------------------------------------------
	lua_iformat.addFunction("IsThisFormat", &SIFormat::isThisFormat);
	lua_iformat.addFunction("CanWrite", &SIFormat::canWrite);
	lua_iformat.addFunction("CanWritePixelFormat", &SIFormat::canWriteType);
	lua_iformat.addFunction("ConvertWritable", &SIFormat::convertWritable);
	lua_iformat.addFunction(
		"LoadImage",
		&SIFormat::loadImage,
		[](SIFormat& self, SImage& image, MemChunk& data) { self.loadImage(image, data); });
	lua_iformat.addFunction(
		"SaveImage",
		&SIFormat::saveImage,
		[](SIFormat& self, SImage& image, MemChunk& out) { self.saveImage(image, out); },
		[](SIFormat& self, SImage& image, MemChunk& out, Palette* pal) { self.saveImage(image, out, pal); });
}

// -----------------------------------------------------------------------------
// Registers the ImageDrawOptions (SImage::DrawProps) type with lua
// -----------------------------------------------------------------------------
static void registerImageDrawOptionsType(lua_State* lua)
{
	auto lua_idopt = luabridge::getGlobalNamespace(lua).beginClass<SImage::DrawProps>("ImageDrawOptions");
	lua_idopt.addConstructor<void()>();

	// Properties
	// -------------------------------------------------------------------------
	using DP = SImage::DrawProps;
	lua_idopt.addProperty("blend", &DP::blend, &DP::blend);
	lua_idopt.addProperty("alpha", &DP::alpha, &DP::alpha);
	lua_idopt.addProperty("keepSourceAlpha", &DP::src_alpha, &DP::src_alpha);
}

// -----------------------------------------------------------------------------
// Loads raw rgb triplet [data] into palette [self]
// -----------------------------------------------------------------------------
static bool paletteLoadData(Palette& self, string_view data)
{
	return self.loadMem(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

// -----------------------------------------------------------------------------
// Loads [data] in [format] to palette [self]
// -----------------------------------------------------------------------------
static bool paletteLoadDataFormatted(Palette& self, string_view data, Palette::Format format)
{
	MemChunk mc{ reinterpret_cast<const uint8_t*>(data.data()), static_cast<uint32_t>(data.size()) };
	return self.loadMem(mc, format);
}

// -----------------------------------------------------------------------------
// Registers the Palette type with lua
// -----------------------------------------------------------------------------
static void registerPaletteType(lua_State* lua)
{
	auto lua_palette = luabridge::getGlobalNamespace(lua).beginClass<Palette>("Palette");
	lua_palette.addConstructor<void(), void(unsigned)>();

	// Properties
	// -------------------------------------------------------------------------
	lua_palette.addProperty("colourCount", [](Palette& self) { return self.colours().size(); });

	// Constants
	// -------------------------------------------------------------------------
	ADD_CLASS_CONSTANT(lua_palette, "FORMAT_RAW", Palette::Format::Raw);
	ADD_CLASS_CONSTANT(lua_palette, "FORMAT_IMAGE", Palette::Format::Image);
	ADD_CLASS_CONSTANT(lua_palette, "FORMAT_CSV", Palette::Format::CSV);
	ADD_CLASS_CONSTANT(lua_palette, "FORMAT_JASC", Palette::Format::JASC);
	ADD_CLASS_CONSTANT(lua_palette, "FORMAT_GIMP", Palette::Format::GIMP);
	ADD_CLASS_CONSTANT(lua_palette, "MATCH_DEFAULT", Palette::ColourMatch::Default);
	ADD_CLASS_CONSTANT(lua_palette, "MATCH_OLD", Palette::ColourMatch::Old);
	ADD_CLASS_CONSTANT(lua_palette, "MATCH_RGB", Palette::ColourMatch::RGB);
	ADD_CLASS_CONSTANT(lua_palette, "MATCH_HSL", Palette::ColourMatch::HSL);
	ADD_CLASS_CONSTANT(lua_palette, "MATCH_C76", Palette::ColourMatch::C76);
	ADD_CLASS_CONSTANT(lua_palette, "MATCH_C94", Palette::ColourMatch::C94);
	ADD_CLASS_CONSTANT(lua_palette, "MATCH_C2K", Palette::ColourMatch::C2K);

	// Functions
	// -------------------------------------------------------------------------
	lua_palette.addFunction("Colour", &Palette::colour);
	lua_palette.addFunction("LoadData", &paletteLoadData, &paletteLoadDataFormatted);
	lua_palette.addFunction(
		"LoadFile", &Palette::loadFile, [](Palette& self, string_view file) { return self.loadFile(file); });
	lua_palette.addFunction(
		"SaveFile", &Palette::saveFile, [](Palette& self, string_view file) { return self.saveFile(file); });
	lua_palette.addFunction("SetColour", &Palette::setColour);
	lua_palette.addFunction("SetColourR", &Palette::setColourR);
	lua_palette.addFunction("SetColourG", &Palette::setColourG);
	lua_palette.addFunction("SetColourB", &Palette::setColourB);
	lua_palette.addFunction("SetColourA", &Palette::setColourA);
	lua_palette.addFunction("CopyColours", &Palette::copyPalette);
	lua_palette.addFunction("FindColour", &Palette::findColour);
	lua_palette.addFunction(
		"NearestColour",
		&Palette::nearestColour,
		[](Palette& self, const ColRGBA& col) { return self.nearestColour(col); });
	lua_palette.addFunction("CountUniqueColours", &Palette::countColours);
	lua_palette.addFunction("ApplyTranslation", &Palette::applyTranslation);
	lua_palette.addFunction("Colourise", &Palette::colourise);
	lua_palette.addFunction("Tint", &Palette::tint);
	lua_palette.addFunction("Saturate", &Palette::saturate);
	lua_palette.addFunction("Illuminate", &Palette::illuminate);
	lua_palette.addFunction("Shift", &Palette::shift);
	lua_palette.addFunction("Invert", &Palette::invert);
	lua_palette.addFunction(
		"Gradient",
		[](Palette& self, const ColRGBA& startC, const ColRGBA& endC, int startI, int endI)
		{ self.setGradient(startI, endI, startC, endC); });
}

// -----------------------------------------------------------------------------
// Registers all graphics-related types with lua
// -----------------------------------------------------------------------------
void registerGraphicsTypes(lua_State* lua)
{
	registerImageConvertOptionsType(lua);
	registerImageFormatType(lua);
	registerImageDrawOptionsType(lua);
	registerPaletteType(lua);
	registerImageType(lua);
	registerTranslationType(lua);
	registerCTexturePatchTypes(lua);
	registerCTextureType(lua);
	registerPatchTableType(lua);
	registerTextureXListType(lua);
}

// -----------------------------------------------------------------------------
// Returns a table with info (SImage::Info struct) about the image in [data]
// -----------------------------------------------------------------------------
static luabridge::LuaRef getImageInfo(MemChunk& data, int index)
{
	auto* format = SIFormat::determineFormat(data);
	auto  info   = format->info(data, index);

	auto table          = luabridge::newTable(state());
	table["width"]      = info.width;
	table["height"]     = info.height;
	table["type"]       = info.colformat;
	table["format"]     = info.format;
	table["imageCount"] = info.numimages;
	table["offsetX"]    = info.offset_x;
	table["offsetY"]    = info.offset_y;
	table["hasPalette"] = info.has_palette;

	return table;
}

// -----------------------------------------------------------------------------
// Registers the Graphics function namespace with lua
// -----------------------------------------------------------------------------
void registerGraphicsNamespace(lua_State* lua)
{
	auto gfx = luabridge::getGlobalNamespace(lua).beginNamespace("Graphics");

	// Constants
	// -------------------------------------------------------------------------
	ADD_NS_CONSTANT(gfx, "BLEND_NORMAL", SImage::BlendType::Normal);
	ADD_NS_CONSTANT(gfx, "BLEND_ADD", SImage::BlendType::Add);
	ADD_NS_CONSTANT(gfx, "BLEND_SUBTRACT", SImage::BlendType::Subtract);
	ADD_NS_CONSTANT(gfx, "BLEND_REVERSESUBTRACT", SImage::BlendType::ReverseSubtract);
	ADD_NS_CONSTANT(gfx, "BLEND_MODULATE", SImage::BlendType::Modulate);

	// Functions
	// -------------------------------------------------------------------------
	gfx.addFunction("ImageFormat", [](string_view id) { return SIFormat::getFormat(id); });
	gfx.addFunction(
		"AllImageFormats",
		[]
		{
			vector<SIFormat*> formats;
			SIFormat::putAllFormats(formats);
			return formats;
		});
	gfx.addFunction("DetectImageFormat", [](MemChunk& mc) { return SIFormat::determineFormat(mc); });
	gfx.addFunction("GetImageInfo", &getImageInfo, [](MemChunk& data) { return getImageInfo(data, 0); });
}
} // namespace slade::lua
