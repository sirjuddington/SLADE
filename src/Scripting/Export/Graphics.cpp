
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
#include "thirdparty/sol/sol.hpp"

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
void registerImageConvertOptionsType(sol::state& lua)
{
	auto lua_copt = lua.new_usertype<SIFormat::ConvertOptions>(
		"ImageConvertOptions", "new", sol::constructors<SIFormat::ConvertOptions()>());

	// Constants
	// -------------------------------------------------------------------------
	lua_copt.set("MASK_NONE", sol::property([]() { return SIFormat::Mask::None; }));
	lua_copt.set("MASK_COLOUR", sol::property([]() { return SIFormat::Mask::Colour; }));
	lua_copt.set("MASK_ALPHA", sol::property([]() { return SIFormat::Mask::Alpha; }));
	lua_copt.set("MASK_BRIGHTNESS", sol::property([]() { return SIFormat::Mask::Brightness; }));

	// Properties
	// -------------------------------------------------------------------------
	lua_copt.set("paletteCurrent", &SIFormat::ConvertOptions::pal_current);
	lua_copt.set("paletteTarget", &SIFormat::ConvertOptions::pal_target);
	lua_copt.set("maskSource", &SIFormat::ConvertOptions::mask_source);
	lua_copt.set("maskColour", &SIFormat::ConvertOptions::mask_colour);
	lua_copt.set("alphaThreshold", &SIFormat::ConvertOptions::alpha_threshold);
	lua_copt.set("transparency", &SIFormat::ConvertOptions::transparency);
	lua_copt.set("pixelFormat", &SIFormat::ConvertOptions::col_format);
}

// -----------------------------------------------------------------------------
// Registers the ImageFormat (SIFormat) type with lua
// -----------------------------------------------------------------------------
void registerImageFormatType(sol::state& lua)
{
	auto lua_iformat = lua.new_usertype<SIFormat>("ImageFormat", "new", sol::no_constructor);

	// Constants
	// -------------------------------------------------------------------------
	lua_iformat.set("WRITABLE_NO", sol::property([]() { return SIFormat::Writable::No; }));
	lua_iformat.set("WRITABLE_YES", sol::property([]() { return SIFormat::Writable::Yes; }));
	lua_iformat.set("WRITABLE_NEEDS_CONVERSION", sol::property([]() { return SIFormat::Writable::Convert; }));

	// Properties
	// -------------------------------------------------------------------------
	lua_iformat.set("id", sol::property(&SIFormat::id));
	lua_iformat.set("name", sol::property(&SIFormat::name));
	lua_iformat.set("extension", sol::property(&SIFormat::extension));

	// Functions
	// -------------------------------------------------------------------------
	lua_iformat.set_function("IsThisFormat", &SIFormat::isThisFormat);
	lua_iformat.set_function("CanWrite", &SIFormat::canWrite);
	lua_iformat.set_function("CanWritePixelFormat", &SIFormat::canWriteType);
	lua_iformat.set_function("ConvertWritable", &SIFormat::convertWritable);
	lua_iformat.set_function(
		"LoadImage",
		sol::overload(
			&SIFormat::loadImage, [](SIFormat& self, SImage& image, MemChunk& data) { self.loadImage(image, data); }));
	lua_iformat.set_function(
		"SaveImage",
		sol::overload(
			&SIFormat::saveImage,
			[](SIFormat& self, SImage& image, MemChunk& out) { self.saveImage(image, out); },
			[](SIFormat& self, SImage& image, MemChunk& out, Palette* pal) { self.saveImage(image, out, pal); }));
}

// -----------------------------------------------------------------------------
// Registers the ImageDrawOptions (SImage::DrawProps) type with lua
// -----------------------------------------------------------------------------
void registerImageDrawOptionsType(sol::state& lua)
{
	auto lua_idopt = lua.new_usertype<SImage::DrawProps>("ImageDrawOptions", sol::constructors<SImage::DrawProps()>());

	// Properties
	// -------------------------------------------------------------------------
	lua_idopt.set("blend", &SImage::DrawProps::blend);
	lua_idopt.set("alpha", &SImage::DrawProps::alpha);
	lua_idopt.set("keepSourceAlpha", &SImage::DrawProps::src_alpha);
}

// -----------------------------------------------------------------------------
// Loads raw rgb triplet [data] into palette [self]
// -----------------------------------------------------------------------------
bool paletteLoadData(Palette& self, string_view data)
{
	return self.loadMem(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

// -----------------------------------------------------------------------------
// Loads [data] in [format] to palette [self]
// -----------------------------------------------------------------------------
bool paletteLoadDataFormatted(Palette& self, string_view data, Palette::Format format)
{
	MemChunk mc{ reinterpret_cast<const uint8_t*>(data.data()), static_cast<uint32_t>(data.size()) };
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
	lua_palette.set("colourCount", sol::property([](Palette& self) { return self.colours().size(); }));

	// Constants
	// -------------------------------------------------------------------------
	lua_palette.set("FORMAT_RAW", sol::property([]() { return Palette::Format::Raw; }));
	lua_palette.set("FORMAT_IMAGE", sol::property([]() { return Palette::Format::Image; }));
	lua_palette.set("FORMAT_CSV", sol::property([]() { return Palette::Format::CSV; }));
	lua_palette.set("FORMAT_JASC", sol::property([]() { return Palette::Format::JASC; }));
	lua_palette.set("FORMAT_GIMP", sol::property([]() { return Palette::Format::GIMP; }));
	lua_palette.set("MATCH_DEFAULT", sol::property([]() { return Palette::ColourMatch::Default; }));
	lua_palette.set("MATCH_OLD", sol::property([]() { return Palette::ColourMatch::Old; }));
	lua_palette.set("MATCH_RGB", sol::property([]() { return Palette::ColourMatch::RGB; }));
	lua_palette.set("MATCH_HSL", sol::property([]() { return Palette::ColourMatch::HSL; }));
	lua_palette.set("MATCH_C76", sol::property([]() { return Palette::ColourMatch::C76; }));
	lua_palette.set("MATCH_C94", sol::property([]() { return Palette::ColourMatch::C94; }));
	lua_palette.set("MATCH_C2K", sol::property([]() { return Palette::ColourMatch::C2K; }));

	// Functions
	// -------------------------------------------------------------------------
	lua_palette.set_function("Colour", &Palette::colour);
	lua_palette.set_function("LoadData", sol::overload(&paletteLoadData, &paletteLoadDataFormatted));
	lua_palette.set_function(
		"LoadFile",
		sol::overload(&Palette::loadFile, [](Palette& self, string_view file) { return self.loadFile(file); }));
	lua_palette.set_function(
		"SaveFile",
		sol::overload(&Palette::saveFile, [](Palette& self, string_view file) { return self.saveFile(file); }));
	lua_palette.set_function("SetColour", &Palette::setColour);
	lua_palette.set_function("SetColourR", &Palette::setColourR);
	lua_palette.set_function("SetColourG", &Palette::setColourG);
	lua_palette.set_function("SetColourB", &Palette::setColourB);
	lua_palette.set_function("SetColourA", &Palette::setColourA);
	lua_palette.set_function("CopyColours", &Palette::copyPalette);
	lua_palette.set_function("FindColour", &Palette::findColour);
	lua_palette.set_function(
		"NearestColour",
		sol::overload(
			&Palette::nearestColour, [](Palette& self, const ColRGBA& col) { return self.nearestColour(col); }));
	lua_palette.set_function("CountUniqueColours", &Palette::countColours);
	lua_palette.set_function("ApplyTranslation", &Palette::applyTranslation);
	lua_palette.set_function("Colourise", &Palette::colourise);
	lua_palette.set_function("Tint", &Palette::tint);
	lua_palette.set_function("Saturate", &Palette::saturate);
	lua_palette.set_function("Illuminate", &Palette::illuminate);
	lua_palette.set_function("Shift", &Palette::shift);
	lua_palette.set_function("Invert", &Palette::invert);
	lua_palette.set_function(
		"Gradient",
		[](Palette& self, const ColRGBA& startC, const ColRGBA& endC, int startI, int endI)
		{ self.setGradient(startI, endI, startC, endC); });
}

// -----------------------------------------------------------------------------
// Registers all graphics-related types with lua
// -----------------------------------------------------------------------------
void registerGraphicsTypes(sol::state& lua)
{
	registerImageConvertOptionsType(lua);
	registerImageFormatType(lua);
	registerImageDrawOptionsType(lua);
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
sol::table getImageInfo(MemChunk& data, int index)
{
	auto* format = SIFormat::determineFormat(data);
	auto  info   = format->info(data, index);

	return lua::state().create_table_with(
		"width",
		info.width,
		"height",
		info.height,
		"type",
		info.colformat,
		"format",
		info.format,
		"imageCount",
		info.numimages,
		"offsetX",
		info.offset_x,
		"offsetY",
		info.offset_y,
		"hasPalette",
		info.has_palette);
}

// -----------------------------------------------------------------------------
// Registers the Graphics function namespace with lua
// -----------------------------------------------------------------------------
void registerGraphicsNamespace(sol::state& lua)
{
	auto gfx = lua.create_named_table("Graphics");

	// Constants
	// -------------------------------------------------------------------------
	gfx.set("BLEND_NORMAL", sol::property([]() { return SImage::BlendType::Normal; }));
	gfx.set("BLEND_ADD", sol::property([]() { return SImage::BlendType::Add; }));
	gfx.set("BLEND_SUBTRACT", sol::property([]() { return SImage::BlendType::Subtract; }));
	gfx.set("BLEND_REVERSESUBTRACT", sol::property([]() { return SImage::BlendType::ReverseSubtract; }));
	gfx.set("BLEND_MODULATE", sol::property([]() { return SImage::BlendType::Modulate; }));

	// Functions
	// -------------------------------------------------------------------------
	gfx.set_function("ImageFormat", [](string_view id) { return SIFormat::getFormat(id); });
	gfx.set_function(
		"AllImageFormats",
		[]()
		{
			vector<SIFormat*> formats;
			SIFormat::putAllFormats(formats);
			return formats;
		});
	gfx.set_function("DetectImageFormat", [](MemChunk& mc) { return SIFormat::determineFormat(mc); });
	gfx.set_function(
		"GetImageInfo", sol::overload(&getImageInfo, [](MemChunk& data) { return getImageInfo(data, 0); }));
}

} // namespace slade::lua
