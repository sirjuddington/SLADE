
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveFormat.cpp
// Description: ArchiveFormat enum, ArchiveFormatInfo struct and related
//              functions for dealing with archive formats
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
#include "ArchiveFormat.h"
#include "Utility/JsonUtils.h"
#include "Utility/Named.h"
#include "Utility/StringUtils.h"

using namespace slade;
using namespace archive;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::archive
{
vector<Named<ArchiveFormat>> formats = { { "adat", ArchiveFormat::ADat }, { "bsp", ArchiveFormat::Bsp },
										 { "bz2", ArchiveFormat::Bz2 },   { "chasm_bin", ArchiveFormat::ChasmBin },
										 { "dat", ArchiveFormat::Dat },   { "folder", ArchiveFormat::Dir },
										 { "disk", ArchiveFormat::Disk }, { "gob", ArchiveFormat::Gob },
										 { "grp", ArchiveFormat::Grp },   { "gzip", ArchiveFormat::GZip },
										 { "hog", ArchiveFormat::Hog },   { "lfd", ArchiveFormat::Lfd },
										 { "lib", ArchiveFormat::Lib },   { "pak", ArchiveFormat::Pak },
										 { "pod", ArchiveFormat::Pod },   { "res", ArchiveFormat::Res },
										 { "rff", ArchiveFormat::Rff },   { "sin", ArchiveFormat::SiN },
										 { "tar", ArchiveFormat::Tar },   { "wad", ArchiveFormat::Wad },
										 { "wadj", ArchiveFormat::WadJ }, { "wad2", ArchiveFormat::Wad2 },
										 { "wolf", ArchiveFormat::Wolf }, { "zip", ArchiveFormat::Zip } };

std::map<ArchiveFormat, ArchiveFormatInfo> format_info;
} // namespace slade::archive


// -----------------------------------------------------------------------------
//
// Archive Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Reads archive formats configuration json from [mc]
// -----------------------------------------------------------------------------
bool archive::loadFormatInfo(const MemChunk& mc)
{
	if (auto j = jsonutil::parse(mc); !j.is_discarded())
	{
		for (auto& [id, j_info] : j.items())
		{
			ArchiveFormatInfo fmt{ id };

			fmt.name                  = j_info["name"];
			fmt.supports_dirs         = j_info.value("supports_dirs", false);
			fmt.names_extensions      = j_info.value("names_extensions", false);
			fmt.max_name_length       = j_info.value("max_name_length", -1);
			fmt.entry_format          = j_info["entry_format"];
			fmt.prefer_uppercase      = j_info.value("prefer_uppercase", false);
			fmt.create                = j_info.value("create", false);
			fmt.allow_duplicate_names = j_info.value("allow_duplicate_names", false);

			if (j_info.contains("extensions"))
				for (auto& j_ext : j_info["extensions"])
					fmt.extensions.emplace_back(j_ext["extension"], j_ext["name"]);

			auto format = formatFromId(fmt.id);
			if (format == ArchiveFormat::Unknown)
			{
				log::error("Unknown archive format id \"{}\" in archive_formats.json", fmt.id);
				continue;
			}

			format_info[format] = fmt;
		}
	}

	// Add builtin 'folder' format
	ArchiveFormatInfo fmt_folder("folder");
	fmt_folder.name                  = "Folder";
	fmt_folder.names_extensions      = true;
	fmt_folder.supports_dirs         = true;
	fmt_folder.allow_duplicate_names = false;
	format_info[ArchiveFormat::Dir]  = fmt_folder;

	// Add 'unknown' format
	ArchiveFormatInfo fmt_unknown("unknown");
	fmt_unknown.name                    = "Unknown";
	format_info[ArchiveFormat::Unknown] = fmt_unknown;

	return true;
}

// -----------------------------------------------------------------------------
// Returns a list of all loaded ArchiveFormatInfos
// -----------------------------------------------------------------------------
vector<ArchiveFormatInfo>& archive::allFormatsInfo()
{
	static vector<ArchiveFormatInfo> all_formats_vec;

	if (all_formats_vec.empty() && !format_info.empty())
	{
		for (auto [format, info] : format_info)
			all_formats_vec.push_back(info);
	}

	return all_formats_vec;
}

// -----------------------------------------------------------------------------
// Returns the ArchiveFormatInfo for [format]
// -----------------------------------------------------------------------------
const ArchiveFormatInfo& archive::formatInfo(ArchiveFormat format)
{
	return format_info[format];
}

// -----------------------------------------------------------------------------
// Returns the ArchiveFormatInfo for format [id]
// -----------------------------------------------------------------------------
const ArchiveFormatInfo& archive::formatInfoFromId(string_view id)
{
	for (auto& [format, info] : format_info)
		if (info.id == id)
			return info;

	return format_info[ArchiveFormat::Unknown];
}

// -----------------------------------------------------------------------------
// Returns the id string for [format]
// -----------------------------------------------------------------------------
string archive::formatId(ArchiveFormat format)
{
	for (const auto& [id, m_format] : formats)
		if (m_format == format)
			return id;

	return "unknown";
}

// -----------------------------------------------------------------------------
// Returns the format for the given [format_id_string]
// -----------------------------------------------------------------------------
ArchiveFormat archive::formatFromId(string_view format_id_string)
{
	for (const auto& [name, id] : formats)
		if (name == format_id_string)
			return id;

	return ArchiveFormat::Unknown;
}

// -----------------------------------------------------------------------------
// Returns the format that uses the given [extension]
// -----------------------------------------------------------------------------
ArchiveFormat archive::formatFromExtension(string_view extension)
{
	for (const auto& [format, info] : format_info)
	{
		for (const auto& ext : info.extensions)
		{
			if (strutil::equalCI(ext.first, extension))
				return format;
		}
	}

	return ArchiveFormat::Unknown;
}
