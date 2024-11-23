
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Utility/Named.h"
#include "Utility/Parser.h"
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
// Reads archive formats configuration file from [mc]
// -----------------------------------------------------------------------------
bool archive::loadFormatInfo(const MemChunk& mc)
{
	Parser parser;
	if (!parser.parseText(mc))
		return false;

	auto root         = parser.parseTreeRoot();
	auto formats_node = root->child("archive_formats");
	for (unsigned a = 0; a < formats_node->nChildren(); a++)
	{
		auto              fmt_desc = dynamic_cast<ParseTreeNode*>(formats_node->child(a));
		ArchiveFormatInfo fmt{ fmt_desc->name() };

		for (unsigned p = 0; p < fmt_desc->nChildren(); p++)
		{
			auto prop = fmt_desc->childPTN(p);

			// Format name
			if (prop->nameIsCI("name"))
				fmt.name = prop->stringValue();

			// Supports dirs
			else if (prop->nameIsCI("supports_dirs"))
				fmt.supports_dirs = prop->boolValue();

			// Entry names have extensions
			else if (prop->nameIsCI("names_extensions"))
				fmt.names_extensions = prop->boolValue();

			// Max entry name length
			else if (prop->nameIsCI("max_name_length"))
				fmt.max_name_length = prop->intValue();

			// Entry format (id)
			else if (prop->nameIsCI("entry_format"))
				fmt.entry_format = prop->stringValue();

			// Extensions
			else if (prop->nameIsCI("extensions"))
			{
				for (unsigned e = 0; e < prop->nChildren(); e++)
				{
					auto ext = prop->childPTN(e);
					fmt.extensions.emplace_back(ext->name(), ext->stringValue());
				}
			}

			// Prefer uppercase entry names
			else if (prop->nameIsCI("prefer_uppercase"))
				fmt.prefer_uppercase = prop->boolValue();

			// Can be created
			else if (prop->nameIsCI("create"))
				fmt.create = prop->boolValue();

			// Allow duplicate entry names (within same directory)
			else if (prop->nameIsCI("allow_duplicate_names"))
				fmt.allow_duplicate_names = prop->boolValue();
		}

		log::info(3, wxString::Format("Read archive format %s: \"%s\"", fmt.id, fmt.name));
		if (fmt.supports_dirs)
			log::info(3, "  Supports folders");
		if (fmt.names_extensions)
			log::info(3, "  Entry names have extensions");
		if (fmt.max_name_length >= 0)
			log::info(3, wxString::Format("  Max entry name length: %d", fmt.max_name_length));
		for (const auto& ext : fmt.extensions)
			log::info(3, wxString::Format(R"(  Extension "%s" = "%s")", ext.first, ext.second));

		auto format = formatFromId(fmt.id);
		if (format == ArchiveFormat::Unknown)
		{
			log::error("Unknown archive format id \"{}\" in archive_formats.cfg", fmt.id);
			continue;
		}

		format_info[format] = fmt;
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
