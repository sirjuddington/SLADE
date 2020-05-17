
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
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
#include "Archive/ArchiveManager.h"
#include "General/UI.h"
#include "Utility/StringUtils.h"
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
struct Icon
{
	struct Image
	{
		wxImage       wx_image;
		ArchiveEntry* resource_entry = nullptr;
	};

	string name;
	Image  i16;
	Image  i24;
	Image  i32;
};

vector<Icon>   icons_general;
vector<Icon>   icons_text_editor;
vector<Icon>   icons_entry;
wxBitmap       icon_empty;
vector<string> iconsets_entry;
vector<string> iconsets_general;
} // namespace slade::icons


// -----------------------------------------------------------------------------
//
// Icons Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::icons
{
// -----------------------------------------------------------------------------
// Returns a list of all icons of [type]
// -----------------------------------------------------------------------------
vector<Icon>& iconList(Type type)
{
	if (type == Entry)
		return icons_entry;
	else if (type == TextEditor)
		return icons_text_editor;
	else
		return icons_general;
}

// -----------------------------------------------------------------------------
// Loads all icons in [dir] to the list for [type]
// -----------------------------------------------------------------------------
bool loadIconsDir(Type type, ArchiveDir* dir)
{
	if (!dir)
		return false;

	// Check for icon set dirs
	for (const auto& subdir : dir->subdirs())
	{
		if (subdir->name() != "16" && subdir->name() != "24" && subdir->name() != "32")
		{
			if (type == General)
				iconsets_general.push_back(subdir->name());
			else if (type == Entry)
				iconsets_entry.push_back(subdir->name());
		}
	}

	// Get icon set dir
	string icon_set_dir = "Default";
	if (type == Entry)
		icon_set_dir = iconset_entry_list;
	if (type == General)
		icon_set_dir = iconset_general;
	if (icon_set_dir != "Default")
		if (auto subdir = dir->subdir(icon_set_dir))
			dir = subdir.get();

	auto& icons = iconList(type);

	// Load 16x16 icons (these must exist)
	auto* dir_16 = dir->subdir("16").get();
	if (!dir_16)
	{
		log::error("Error loading icons, no 16x16 dir exists for set \"{}\" (in {})", icon_set_dir, dir->path());
		return false;
	}
	for (const auto& entry : dir_16->allEntries())
	{
		// Ignore anything not png format
		if (!strutil::endsWithCI(entry->name(), ".png"))
		{
			log::warning("Invalid 16x16 image format for icon \"{}\", must be png", entry->nameNoExt());
			continue;
		}

		// Load 16x16 icon image
		Icon n_icon;
		auto stream = wxMemoryInputStream(entry->rawData(), entry->size());
		if (!n_icon.i16.wx_image.LoadFile(stream, wxBITMAP_TYPE_PNG))
		{
			log::warning("Unable to load 16x16 image for icon \"{}\" (is it not png format?)", entry->nameNoExt());
			continue;
		}

		// Add the icon
		n_icon.name               = entry->nameNoExt();
		n_icon.i16.resource_entry = entry.get();
		icons.push_back(n_icon);
	}

	// Load 24x24 icons
	if (auto* dir_24 = dir->subdir("24").get())
	{
		for (const auto& entry : dir_24->allEntries())
		{
			// Ignore anything not png format
			if (!strutil::endsWithCI(entry->name(), ".png"))
			{
				log::warning("Invalid 24x24 image format for icon \"{}\", must be png", entry->nameNoExt());
				continue;
			}

			// Find existing icon
			auto name = entry->nameNoExt();
			for (auto& icon : icons)
			{
				if (icon.name == name)
				{
					// Load 24x24 icon image
					auto stream = wxMemoryInputStream(entry->rawData(), entry->size());
					if (!icon.i24.wx_image.LoadFile(stream, wxBITMAP_TYPE_PNG))
						log::warning(
							"Unable to load 24x24 image for icon \"{}\" (is it not png format?)", entry->nameNoExt());
					icon.i24.resource_entry = entry.get();

					break;
				}
			}
		}
	}

	// Load 32x32 icons
	if (auto* dir_32 = dir->subdir("32").get())
	{
		for (const auto& entry : dir_32->allEntries())
		{
			// Ignore anything not png format
			if (!strutil::endsWithCI(entry->name(), ".png"))
			{
				log::warning("Invalid 32x32 image format for icon \"{}\", must be png", entry->nameNoExt());
				continue;
			}

			// Find existing icon
			auto name = entry->nameNoExt();
			for (auto& icon : icons)
			{
				if (icon.name == name)
				{
					// Load 32x32 icon image
					auto stream = wxMemoryInputStream(entry->rawData(), entry->size());
					if (!icon.i32.wx_image.LoadFile(stream, wxBITMAP_TYPE_PNG))
						log::warning(
							"Unable to load 32x32 image for icon \"{}\" (is it not png format?)", entry->nameNoExt());
					icon.i32.resource_entry = entry.get();

					break;
				}
			}
		}
	}

	// Generate any missing large icons
	for (auto& icon : icons)
	{
		if (!icon.i24.wx_image.IsOk())
		{
			icon.i24.resource_entry = icon.i16.resource_entry;
			icon.i24.wx_image       = icon.i16.wx_image.Copy();
			icon.i24.wx_image.Rescale(24, 24, wxIMAGE_QUALITY_BICUBIC);
		}

		if (!icon.i32.wx_image.IsOk())
		{
			icon.i32.resource_entry = icon.i16.resource_entry;
			icon.i32.wx_image       = icon.i16.wx_image.Copy();
			icon.i32.wx_image.Rescale(32, 32, wxIMAGE_QUALITY_BICUBIC);
		}
	}

	return true;
}

const Icon::Image& validImageForSize(const Icon& icon_def, int size)
{
	if (size <= 16)
		return icon_def.i16;

	if (size <= 24)
	{
		if (icon_def.i24.wx_image.IsOk())
			return icon_def.i24;
		else
			return icon_def.i16;
	}

	// Size >= 25
	if (icon_def.i32.wx_image.IsOk())
		return icon_def.i32;

	return icon_def.i16;
}
} // namespace slade::icons

// -----------------------------------------------------------------------------
// Loads all icons from slade.pk3 (in the icons/ dir)
// -----------------------------------------------------------------------------
bool icons::loadIcons()
{
	auto tempfile = app::path("sladetemp", app::Dir::Temp);

	// Get slade.pk3
	auto* res_archive = app::archiveManager().programResourceArchive();

	// Do nothing if it doesn't exist
	if (!res_archive)
		return false;

	// Get the icons directory of the archive
	auto* dir_icons = res_archive->dirAtPath("icons");

	// Load general icons
	iconsets_general.emplace_back("Default");
	loadIconsDir(General, dir_icons->subdir("general").get());

	// Load entry list icons
	iconsets_entry.emplace_back("Default");
	loadIconsDir(Entry, dir_icons->subdir("entry_list").get());

	// Load text editor icons
	loadIconsDir(TextEditor, dir_icons->subdir("text_editor").get());

	return true;
}

// -----------------------------------------------------------------------------
// Returns the icon matching [name] of [type] as a wxBitmap (for toolbars etc),
// or an empty bitmap if no icon was found.
// If [type] is less than 0, try all icon types.
// If [log_missing] is true, log an error message if the icon was not found
// -----------------------------------------------------------------------------
wxBitmap icons::getIcon(Type type, string_view name, int size, bool log_missing)
{
	// Check all types if [type] is < 0
	if (type == Any)
	{
		auto icon = getIcon(General, name, size, false);
		if (!icon.IsOk())
			icon = getIcon(Entry, name, size, false);
		if (!icon.IsOk())
			icon = getIcon(TextEditor, name, size, false);

		if (!icon.IsOk() && log_missing)
			log::warning(2, "Icon \"{}\" does not exist", name);

		return icon;
	}

	for (const auto& icon : iconList(type))
		if (icon.name == name)
			return wxBitmap(validImageForSize(icon, size).wx_image);

	if (log_missing)
		log::warning(2, "Icon \"{}\" does not exist", name);

	return wxNullBitmap;
}

// -----------------------------------------------------------------------------
// Returns the icon matching [name] of [type] as a wxBitmap (for toolbars etc)
// with [padding] pixels transparent border, or an empty bitmap if no icon was
// found.
// If [type] is less than 0, try all icon types.
// -----------------------------------------------------------------------------
wxBitmap icons::getPaddedIcon(Type type, string_view name, int size, int padding)
{
	// Check all types if [type] is < 0
	if (type == Any)
	{
		auto icon = getPaddedIcon(General, name, size, padding);
		if (!icon.IsOk())
			icon = getPaddedIcon(Entry, name, size, padding);
		if (!icon.IsOk())
			icon = getPaddedIcon(TextEditor, name, size, padding);

		return icon;
	}

	for (const auto& icon : iconList(type))
		if (icon.name == name)
		{
			const auto& image = validImageForSize(icon, size).wx_image;
			wxImage     padded(image.GetWidth() + padding * 2, image.GetHeight() + padding * 2);
			padded.SetMaskColour(0, 0, 0);
			padded.InitAlpha();
			padded.Paste(image, padding, padding);

			return wxBitmap(padded);
		}

	return wxNullBitmap;
}

// -----------------------------------------------------------------------------
// Returns the icon [name] of [type]
// -----------------------------------------------------------------------------
wxBitmap icons::getIcon(Type type, string_view name)
{
	return getIcon(type, name, 16 * ui::scaleFactor());
}

// -----------------------------------------------------------------------------
// Returns true if an icon with [name] of [type] exists
// -----------------------------------------------------------------------------
bool icons::iconExists(Type type, string_view name)
{
	// Check all types if [type] is < 0
	if (type == Any)
	{
		bool exists = iconExists(General, name);
		if (!exists)
			exists = iconExists(Entry, name);
		if (!exists)
			exists = iconExists(TextEditor, name);

		return exists;
	}

	auto& icons = iconList(type);
	for (const auto& icon : icons)
		if (icon.name == name)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Returns the resource entry for the icon matching [name] of [type], or nullptr
// if no matching icon was found.
// If [type] is less than 0, try all icon types.
// -----------------------------------------------------------------------------
ArchiveEntry* icons::getIconEntry(Type type, string_view name, int size)
{
	// Check all types if [type] is < 0
	if (type == Any)
	{
		auto* entry = getIconEntry(General, name, size);
		if (!entry)
			entry = getIconEntry(Entry, name, size);
		if (!entry)
			entry = getIconEntry(TextEditor, name, size);

		return entry;
	}

	auto& icons = iconList(type);
	for (const auto& icon : icons)
	{
		if (icon.name == name)
			return validImageForSize(icon, size).resource_entry;
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Exports icon [name] of [type] to a png image file at [path]
// -----------------------------------------------------------------------------
bool icons::exportIconPNG(Type type, string_view name, string_view path)
{
	auto& icons = iconList(type);

	for (auto& icon : icons)
	{
		if (icon.name == name)
			return icon.i16.resource_entry->exportFile(path);
	}

	return false;
}

// -----------------------------------------------------------------------------
// Returns a list of currently available icon sets for [type]
// -----------------------------------------------------------------------------
vector<string> icons::iconSets(Type type)
{
	if (type == General)
		return iconsets_general;
	else if (type == Entry)
		return iconsets_entry;

	return {};
}
