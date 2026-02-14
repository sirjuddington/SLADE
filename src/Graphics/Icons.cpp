
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    Icons.cpp
// Description: Functions to do with loading program icons from slade.pk3
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
#include "Icons.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "WxGfx.h"
#include <wx/mstream.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, iconset_general, "Default", CVar::Flag::Save)
CVAR(String, iconset_entry_list, "Default", CVar::Flag::Save)

namespace slade::icons
{
struct IconDef
{
	string        svg_data;
	string        svg_data_dark;
	ArchiveEntry* entry_png16 = nullptr;
	ArchiveEntry* entry_png32 = nullptr;
};

struct IconSet
{
	string                                 name;
	std::map<string, IconDef, std::less<>> icons;

	IconSet(string_view name) : name{ name } {}
};

bool            ui_icons_dark = false;
vector<IconSet> iconsets_entry;
vector<IconSet> iconsets_general;
IconSet         iconset_text_editor{ "Default" };
IconSet         iconset_ui_dark{ "Dark" };
IconSet         iconset_ui_light{ "Light" };
} // namespace slade::icons


// -----------------------------------------------------------------------------
//
// Icons Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::icons
{
// -----------------------------------------------------------------------------
// Returns the current icon set for [type]
// -----------------------------------------------------------------------------
const IconSet& currentIconSet(Type type)
{
	if (type == General || type == Any)
	{
		for (const auto& set : iconsets_general)
			if (set.name == iconset_general.value)
				return set;

		return iconsets_general[0];
	}

	if (type == Entry)
	{
		for (const auto& set : iconsets_entry)
			if (set.name == iconset_entry_list.value)
				return set;

		return iconsets_entry[0];
	}

	return iconsets_general[0];
}

// -----------------------------------------------------------------------------
// Returns the definition for icon [name] of [type], or nullptr if none found
// -----------------------------------------------------------------------------
const IconDef* iconDef(Type type, string_view name)
{
	if (type == General || type == Any)
	{
		// Find icon in current set
		const auto& set = currentIconSet(General);
		if (auto i = set.icons.find(name); i != set.icons.end())
			return &i->second;

		// If not found look in default set
		if (set.name != "Default")
		{
			const auto& default_set = iconsets_general[0];
			if (auto i = default_set.icons.find(name); i != default_set.icons.end())
				return &i->second;
		}
	}

	if (type == Entry || type == Any)
	{
		// Find icon in current set
		const auto& set = currentIconSet(Entry);
		if (auto i = set.icons.find(name); i != set.icons.end())
			return &i->second;

		// If not found look in default set
		if (set.name != "Default")
		{
			const auto& default_set = iconsets_entry[0];
			if (auto i = default_set.icons.find(name); i != default_set.icons.end())
				return &i->second;
		}
	}

	if (type == TextEditor || type == Any)
		if (auto i = iconset_text_editor.icons.find(name); i != iconset_text_editor.icons.end())
			return &i->second;

	return nullptr;
}

// -----------------------------------------------------------------------------
// Loads an SVG [svg_data] of [size] into a wxBitmap, with optional [padding]
// -----------------------------------------------------------------------------
wxBitmap loadSVGIcon(const string& svg_data, int size, const Point2i& padding)
{
	const auto img = wxgfx::createImageFromSVG(svg_data, size, size);

	// Add padding if needed
	if (padding.x > 0 || padding.y > 0)
	{
		wxImage padded(img.GetWidth() + padding.x * 2, img.GetHeight() + padding.y * 2);
		padded.SetMaskColour(0, 0, 0);
		padded.InitAlpha();
		padded.Paste(img, padding.x, padding.y);
		return { padded };
	}

	return { img };
}

// -----------------------------------------------------------------------------
// Loads a PNG icon of [size] using the given [icon] definition into a wxBitmap,
// with optional [padding]
// -----------------------------------------------------------------------------
wxBitmap loadPNGIcon(const IconDef& icon, int size, Point2i padding)
{
	// Check valid png icon definition
	if (!icon.entry_png16 && !icon.entry_png32)
		return wxNullBitmap;

	wxImage image;

	// Calculate image size to use (will be used to rescale if necessary)
	// Only allow 'regular' sizes to avoid excessively blurry scaling
	auto img_size = size;
	if (img_size < 24)
		img_size = 16;
	else if (img_size < 32)
		img_size = 24;
	else if (img_size < 48)
		img_size = 32;
	else if (img_size < 64)
		img_size = 48;
	else
		img_size = 64;

	// Get appropriate png entry to load
	ArchiveEntry* png_entry = nullptr;
	if (icon.entry_png16 && (img_size < 32 || !icon.entry_png32))
		png_entry = icon.entry_png16;
	else if (icon.entry_png32)
		png_entry = icon.entry_png32;

	if (!png_entry)
		return wxNullBitmap;

	// Load png to image
	auto stream = wxMemoryInputStream(png_entry->rawData(), png_entry->size());
	if (!image.LoadFile(stream, wxBITMAP_TYPE_PNG))
	{
		log::warning("Unable to load icon image \"{}\" (is it not png format?)", png_entry->path(true));
		return wxNullBitmap;
	}

	// Scale to desired size if necessary
	if (image.GetWidth() != size)
	{
		// Scale to calculated size
		image.Rescale(img_size, img_size, wxIMAGE_QUALITY_BICUBIC);

		// If requested size is still larger than the image, add extra padding
		if (image.GetWidth() < size)
		{
			padding.x += (size - img_size) / 2;
			padding.y += (size - img_size) / 2;
		}
	}

	// Add padding if needed
	if (padding.x > 0 || padding.y > 0)
	{
		wxImage padded(image.GetWidth() + padding.x * 2, image.GetHeight() + padding.y * 2);
		padded.SetMaskColour(0, 0, 0);
		padded.InitAlpha();
		padded.Paste(image, padding.x, padding.y);
		return { padded };
	}

	return { image };
}

// -----------------------------------------------------------------------------
// Reads an icon set from the given JSON object [j] using the resources in
// [res_archive]
// -----------------------------------------------------------------------------
IconSet readIconSet(const Json& j, const Archive& res_archive)
{
	IconSet set{ j.at("name").get<string>() };
	for (auto& [id, j_icon] : j["icons"].items())
	{
		IconDef idef;

		// SVG
		if (j_icon.contains("svg") || j_icon.contains("svg_light") || j_icon.contains("svg_dark"))
		{
			if (j_icon.contains("svg"))
			{
				// Single SVG
				auto* entry = res_archive.entryAtPath(j_icon["svg"].get<string>());
				if (entry)
					idef.svg_data = entry->data().asString();
				else
					log::error("Icon entry \"{}\" does not exist in slade.pk3", j_icon["svg"].get<string>());
			}
			else
			{
				// Light/Dark SVGs
				if (j_icon.contains("svg_light"))
				{
					auto* entry = res_archive.entryAtPath(j_icon["svg_light"].get<string>());
					if (entry)
						idef.svg_data = entry->data().asString();
					else
						log::error("Icon entry \"{}\" does not exist in slade.pk3", j_icon["svg_light"].get<string>());
				}
				if (j_icon.contains("svg_dark"))
				{
					auto* entry = res_archive.entryAtPath(j_icon["svg_dark"].get<string>());
					if (entry)
						idef.svg_data_dark = entry->data().asString();
					else
						log::error("Icon entry \"{}\" does not exist in slade.pk3", j_icon["svg_dark"].get<string>());
				}
			}

			// If only one svg entry is provided, use it for both light and dark
			if (idef.svg_data.empty() && !idef.svg_data_dark.empty())
				idef.svg_data = idef.svg_data_dark;
			if (idef.svg_data_dark.empty() && !idef.svg_data.empty())
				idef.svg_data_dark = idef.svg_data;
		}

		// PNG
		else if (j_icon.contains("png16") || j_icon.contains("png32"))
		{
			if (j_icon.contains("png16"))
			{
				idef.entry_png16 = res_archive.entryAtPath(j_icon["png16"].get<string>());
				if (!idef.entry_png16)
					log::error("Icon entry \"{}\" does not exist in slade.pk3", j_icon["png16"].get<string>());
			}
			if (j_icon.contains("png32"))
			{
				idef.entry_png32 = res_archive.entryAtPath(j_icon["png32"].get<string>());
				if (!idef.entry_png32)
					log::error("Icon entry \"{}\" does not exist in slade.pk3", j_icon["png32"].get<string>());
			}
		}

		set.icons[id] = idef;
	}

	return set;
}
} // namespace slade::icons

// -----------------------------------------------------------------------------
// Loads all icon definitions from slade.pk3 (icons.cfg)
// -----------------------------------------------------------------------------
bool icons::loadIcons()
{
	// Check for dark mode
#if defined(__WXMSW__) && !wxCHECK_VERSION(3, 3, 0)
	ui_icons_dark = false; // Force light theme icons in windows
#elif wxCHECK_VERSION(3, 1, 3)
	ui_icons_dark = wxSystemSettings::GetAppearance().IsDark();
#else
	auto fg   = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	auto fg_r = fg.Red();
	auto fg_g = fg.Green();
	auto fg_b = fg.Blue();
	auto bg   = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	auto bg_r = bg.Red();
	auto bg_g = bg.Green();
	auto bg_b = bg.Blue();
	wxColour::MakeGrey(&fg_r, &fg_g, &fg_b);
	wxColour::MakeGrey(&bg_r, &bg_g, &bg_b);
	log::info("DARK MODE CHECK: FG {} BG {}", fg_r, bg_r);
	ui_icons_dark = fg_r > bg_r;
#endif

	// Get slade.pk3
	auto* res_archive = app::programResource();
	if (!res_archive)
		return false;

	// Load General sets
	auto dir_general = res_archive->dirAtPath("config/icons/general");
	for (auto entry : dir_general->entries())
	{
		if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
			iconsets_general.push_back(readIconSet(j, *res_archive));
	}

	// Load Entry List sets
	auto dir_elist = res_archive->dirAtPath("config/icons/entry_list");
	for (auto entry : dir_elist->entries())
	{
		if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
			iconsets_entry.push_back(readIconSet(j, *res_archive));
	}

	// Load Text Editor icons
	auto dir_txed_icons = res_archive->dirAtPath("icons/text_editor");
	for (auto entry : dir_txed_icons->entries())
		iconset_text_editor.icons[string{ entry->nameNoExt() }].svg_data = entry->data().asString();

	// Load UI icons (light)
	auto* dir_ui_icons_light = res_archive->dirAtPath("icons/ui/light");
	for (auto& icon_entry : dir_ui_icons_light->entries())
		iconset_ui_light.icons[string{ icon_entry->nameNoExt() }].svg_data = icon_entry->data().asString();

	// Load UI icons (dark)
	auto* dir_ui_icons_dark = res_archive->dirAtPath("icons/ui/dark");
	for (auto& icon_entry : dir_ui_icons_dark->entries())
		iconset_ui_dark.icons[string{ icon_entry->nameNoExt() }].svg_data = icon_entry->data().asString();

	return true;
}

// -----------------------------------------------------------------------------
// Loads the icon [name] of [type] into a wxBitmapBundle of minimum [size], with
// optional [padding] (png icons only).
//
// NOTE: this does not use any kind of caching and will generate/load the icon
// from svg/png data each time
// -----------------------------------------------------------------------------
wxBitmapBundle icons::getIcon(Type type, string_view name, int size, const Point2i& padding)
{
	// Get icon definition
	const auto* icon_def = iconDef(type, name);
	if (!icon_def)
	{
		log::warning("Unknown icon \"{}\"", name);
		return wxNullBitmap;
	}

	// Check size
	if (size <= 0)
		size = 16;

	// If there is SVG data use that
	if (!icon_def->svg_data.empty())
		return wxBitmapBundle::FromSVG(
			ui_icons_dark ? icon_def->svg_data_dark.c_str() : icon_def->svg_data.c_str(), { size, size });

	// Otherwise load from png
	if (icon_def->entry_png16 || icon_def->entry_png32)
	{
		wxVector<wxBitmap> bitmaps;
		if (size <= 16)
			bitmaps.push_back(loadPNGIcon(*icon_def, 16, padding));
		if (size <= 24)
			bitmaps.push_back(loadPNGIcon(*icon_def, 24, padding));
		bitmaps.push_back(loadPNGIcon(*icon_def, 32, padding));
		return wxBitmapBundle::FromBitmaps(bitmaps);
	}

	return wxNullBitmap;
}

// -----------------------------------------------------------------------------
// Loads the interface icon [name] from [theme] into a wxBitmapBundle of minimum
// [size].
//
// NOTE: this does not use any kind of caching and will generate/load the icon
// from svg/png data each time
// -----------------------------------------------------------------------------
wxBitmapBundle icons::getInterfaceIcon(string_view name, int size, InterfaceTheme theme)
{
	// Get theme to use
	bool dark = false;
	switch (theme)
	{
	case System: dark = ui_icons_dark; break;
	case Light:  dark = false; break;
	case Dark:   dark = true; break;
	}

	// Get icon definition
	auto&    icon_set = dark ? iconset_ui_dark : iconset_ui_light;
	IconDef* icon_def = nullptr;
	if (auto i = icon_set.icons.find(name); i != icon_set.icons.end())
		icon_def = &i->second;
	if (!icon_def)
	{
		log::warning("Unknown interface icon \"{}\"", name);
		return wxNullBitmap;
	}

	// Check size
	if (size <= 0)
		size = 16;

	// If there is SVG data use that
	if (!icon_def->svg_data.empty())
		return wxBitmapBundle::FromSVG(icon_def->svg_data.c_str(), { size, size });

	return wxNullBitmap;
}

// -----------------------------------------------------------------------------
// Returns a list of all defined icon sets for [type]
// -----------------------------------------------------------------------------
vector<string> icons::iconSets(Type type)
{
	vector<string> sets;

	if (type == General)
		for (const auto& set : iconsets_general)
			sets.emplace_back(set.name);

	else if (type == Entry)
		for (const auto& set : iconsets_entry)
			sets.emplace_back(set.name);

	return sets;
}

// -----------------------------------------------------------------------------
// Returns true if [icon] of [type] exists
// -----------------------------------------------------------------------------
bool icons::iconExists(Type type, string_view name)
{
	return iconDef(type, name) != nullptr;
}
