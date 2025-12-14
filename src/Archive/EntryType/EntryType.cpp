
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    EntryType.cpp
// Description: Entry Type detection system
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
#include "EntryType.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveManager.h"
#include "EntryDataFormat.h"
#include "General/Console.h"
#include "MainEditor/MainEditor.h"
#include "Utility/Colour.h"
#include "Utility/FileUtils.h"
#include "Utility/JsonUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
vector<unique_ptr<EntryType>> entry_types;      // The big list of all entry types
vector<string>                entry_categories; // All entry type categories

// Special entry types
EntryType* etype_unknown = nullptr; // The default, 'unknown' entry type
EntryType* etype_folder  = nullptr; // Folder entry type
EntryType* etype_marker  = nullptr; // Marker entry type
EntryType* etype_map     = nullptr; // Map marker type
} // namespace


// -----------------------------------------------------------------------------
//
// EntryType Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// EntryType class constructor
// -----------------------------------------------------------------------------
EntryType::EntryType(string_view id) : id_{ id }, colour_{ ColRGBA::WHITE }, format_{ EntryDataFormat::anyFormat() } {}

// -----------------------------------------------------------------------------
// Returns the id of the type's data format
// -----------------------------------------------------------------------------
const string& EntryType::formatId() const
{
	return format_->id();
}

// -----------------------------------------------------------------------------
// Dumps entry type info to the log
// -----------------------------------------------------------------------------
void EntryType::dump() const
{
	log::info("Type {} \"{}\", format {}, extension {}", id_, name_, format_->id(), extension_);
	log::info("Size limit: {}-{}", size_limit_[0], size_limit_[1]);

	for (const auto& a : match_archive_)
		log::info("Match Archive: \"{}\"", a);

	for (const auto& a : match_extension_)
		log::info("Match Extension: \"{}\"", a);

	for (const auto& a : match_name_)
		log::info("Match Name: \"{}\"", a);

	for (int a : match_size_)
		log::info("Match Size: {}", a);

	for (int a : size_multiple_)
		log::info("Size Multiple: {}", a);

	log::info("---");
}

// -----------------------------------------------------------------------------
// Copies this entry type's info/properties to [target]
// -----------------------------------------------------------------------------
void EntryType::copyToType(EntryType& target) const
{
	// Copy type attributes
	target.editor_      = editor_;
	target.extension_   = extension_;
	target.icon_        = icon_;
	target.name_        = name_;
	target.reliability_ = reliability_;
	target.category_    = category_;
	target.colour_      = colour_;

	// Copy type match criteria
	target.format_          = format_;
	target.size_limit_[0]   = size_limit_[0];
	target.size_limit_[1]   = size_limit_[1];
	target.section_         = section_;
	target.match_extension_ = match_extension_;
	target.match_name_      = match_name_;
	target.match_size_      = match_size_;
	target.match_extension_ = match_extension_;
	target.match_archive_   = match_archive_;

	// Copy extra properties
	target.extra_ = extra_;
}

// -----------------------------------------------------------------------------
// Returns a file filter string for this type:
// "<type name> files (*.<type extension)|*.<type extension>"
// -----------------------------------------------------------------------------
string EntryType::fileFilterString() const
{
	return fmt::format("{0} files (*.{1})|*.{1}", name_, extension_);
}

// -----------------------------------------------------------------------------
// Returns true if [entry] matches the EntryType's criteria, false otherwise
// -----------------------------------------------------------------------------
int EntryType::isThisType(ArchiveEntry& entry) const
{
	// Check type is detectable
	if (!detectable_)
		return EntryDataFormat::MATCH_FALSE;

	// Check min size
	if (size_limit_[0] >= 0 && entry.size() < static_cast<unsigned>(size_limit_[0]))
		return EntryDataFormat::MATCH_FALSE;

	// Check max size
	if (size_limit_[1] >= 0 && entry.size() > static_cast<unsigned>(size_limit_[1]))
		return EntryDataFormat::MATCH_FALSE;

	// Check for archive match if needed
	if (!match_archive_.empty())
	{
		bool match = false;
		for (const auto& a : match_archive_)
		{
			if (entry.parent() && entry.parent()->formatInfo().entry_format == a)
			{
				match = true;
				break;
			}
		}
		if (!match)
			return EntryDataFormat::MATCH_FALSE;
	}

	// Check for size match if needed
	if (!match_size_.empty())
	{
		bool match = false;
		for (unsigned a : match_size_)
		{
			if (entry.size() == a)
			{
				match = true;
				break;
			}
		}

		if (!match)
			return EntryDataFormat::MATCH_FALSE;
	}

	// Check for data format match if needed
	int r = EntryDataFormat::MATCH_TRUE;
	if (format_ == EntryDataFormat::textFormat())
	{
		// Hack for identifying ACS script sources despite DB2 apparently appending
		// two null bytes to them, which make the memchr test fail.
		size_t end = entry.size() - 1;
		if (end > 3)
			end -= 2;
		// Text is a special case, as other data formats can sometimes be detected as 'text',
		// we'll only check for it if text data is specified in the entry type
		if (entry.size() > 0 && memchr(entry.rawData(), 0, end) != nullptr)
			return EntryDataFormat::MATCH_FALSE;
	}
	else if (format_ != EntryDataFormat::anyFormat() && entry.size() > 0)
	{
		r = format_->isThisFormat(entry.data());
		if (r == EntryDataFormat::MATCH_FALSE)
			return EntryDataFormat::MATCH_FALSE;
	}

	// Check for size multiple match if needed
	if (!size_multiple_.empty())
	{
		bool         match              = false;
		const size_t size_multiple_size = size_multiple_.size();
		for (size_t a = 0; a < size_multiple_size; a++)
		{
			if (entry.size() % size_multiple_[a] == 0)
			{
				match = true;
				break;
			}
		}

		if (!match)
			return EntryDataFormat::MATCH_FALSE;
	}

	// If both names and extensions are defined, and the type only needs one
	// of the two, not both, take it into account.
	bool extorname = false;
	if (match_ext_or_name_ && !match_name_.empty() && !match_extension_.empty())
		extorname = true;

	// Entry name related stuff
	if (!match_name_.empty() || !match_extension_.empty())
	{
		bool matchedname = false;

		// Get entry name (lowercase), find extension separator
		const string_view fn      = entry.upperName();
		const size_t      ext_sep = fn.find_last_of('.');

		// Check for name match if needed
		if (!match_name_.empty())
		{
			auto name = fn;
			if (ext_sep != string::npos)
				name = fn.substr(0, ext_sep);

			// If we are matching 8 characters or less, only check the first 8 characters of the entry name
			if (match_name_.size() <= 8 && name.size() > 8)
				name = name.substr(0, 8);

			bool match = false;
			for (const auto& match_name : match_name_)
			{
				if (strutil::matches(name, match_name))
				{
					match = true;
					break;
				}
			}

			if (!match && !extorname)
				return EntryDataFormat::MATCH_FALSE;
			else
				matchedname = match;
		}

		// Check for extension match if needed
		if (!match_extension_.empty())
		{
			bool match = false;
			if (ext_sep != string::npos)
			{
				const auto ext = fn.substr(ext_sep + 1);
				for (const auto& match_ext : match_extension_)
				{
					if (ext == match_ext)
					{
						match = true;
						break;
					}
				}
			}

			if (!match && !(extorname && matchedname))
				return EntryDataFormat::MATCH_FALSE;
		}
	}

	// Check for entry section match if needed
	if (!section_.empty())
	{
		// Check entry is part of an archive (if not it can't be in a section)
		if (!entry.parent())
			return EntryDataFormat::MATCH_FALSE;

		const auto e_section = entry.parent()->detectNamespace(&entry);

		r = EntryDataFormat::MATCH_FALSE;
		for (const auto& ns : section_)
			if (strutil::equalCI(ns, e_section))
				r = EntryDataFormat::MATCH_TRUE;
	}

	// Passed all checks, so we have a match
	return r;
}

// -----------------------------------------------------------------------------
// Initialises built-in entry types (ie. types not defined in configs)
// -----------------------------------------------------------------------------
void EntryType::initTypes()
{
	const auto fmt_any = EntryDataFormat::anyFormat();

	// Setup unknown type
	auto et_unknown          = std::make_unique<EntryType>("unknown");
	et_unknown->format_      = fmt_any;
	et_unknown->icon_        = "unknown";
	et_unknown->detectable_  = false;
	et_unknown->reliability_ = 0;
	etype_unknown            = et_unknown.get();
	etype_unknown->index_    = static_cast<int>(entry_types.size());
	entry_types.push_back(std::move(et_unknown));

	// Setup folder type
	auto et_folder         = std::make_unique<EntryType>("folder");
	et_folder->format_     = fmt_any;
	et_folder->icon_       = "folder";
	et_folder->name_       = "Folder";
	et_folder->detectable_ = false;
	etype_folder           = et_folder.get();
	etype_folder->index_   = static_cast<int>(entry_types.size());
	entry_types.push_back(std::move(et_folder));

	// Setup marker type
	auto et_marker         = std::make_unique<EntryType>("marker");
	et_marker->format_     = fmt_any;
	et_marker->icon_       = "marker";
	et_marker->name_       = "Marker";
	et_marker->detectable_ = false;
	et_marker->category_   = ""; // No category, markers only appear when 'All' categories shown
	etype_marker           = et_marker.get();
	et_marker->index_      = static_cast<int>(entry_types.size());
	entry_types.push_back(std::move(et_marker));

	// Setup map marker type
	auto et_map         = std::make_unique<EntryType>("map");
	et_map->format_     = fmt_any;
	et_map->icon_       = "map";
	et_map->name_       = "Map Marker";
	et_map->category_   = "Maps"; // Should appear with maps
	et_map->detectable_ = false;
	et_map->colour_     = ColRGBA(0, 255, 0);
	etype_map           = et_map.get();
	etype_map->index_   = static_cast<int>(entry_types.size());
	entry_types.push_back(std::move(et_map));
}

// -----------------------------------------------------------------------------
// Reads entry type definitions from a json object [j]
// -----------------------------------------------------------------------------
void EntryType::readEntryTypes(Json& j)
{
	for (auto& [id, j_etype] : j.items())
	{
		// Create new entry type
		auto ntype = std::make_unique<EntryType>(strutil::lower(id));

		// Copy from existing type if inherited
		if (j_etype.contains("inherits"))
		{
			auto inherits    = j_etype["inherits"].get<string>();
			auto parent_type = fromId(strutil::lower(inherits));

			if (parent_type != etype_unknown)
				parent_type->copyToType(*ntype);
			else
				log::info("Warning: Entry type {} inherits from unknown type {}", ntype->id(), inherits);
		}

		// Read fields from json object
		ntype->name_              = j_etype.value("name", ntype->name_);
		ntype->detectable_        = j_etype.value("detectable", ntype->detectable_);
		ntype->extension_         = j_etype.value("export_ext", ntype->extension_);
		ntype->editor_            = j_etype.value("editor", ntype->editor_);
		ntype->reliability_       = j_etype.value("reliability", ntype->reliability_);
		ntype->section_           = j_etype.value("section", ntype->section_);
		ntype->match_ext_or_name_ = j_etype.value("match_extorname", ntype->match_ext_or_name_);
		ntype->match_name_        = j_etype.value("match_name", ntype->match_name_);
		ntype->match_extension_   = j_etype.value("match_ext", ntype->match_extension_);
		ntype->match_archive_     = j_etype.value("match_archive", ntype->match_archive_);
		ntype->match_size_        = j_etype.value("size", ntype->match_size_);
		ntype->size_limit_[0]     = j_etype.value("min_size", ntype->size_limit_[0]);
		ntype->size_limit_[1]     = j_etype.value("max_size", ntype->size_limit_[1]);
		ntype->size_multiple_     = j_etype.value("size_multiple", ntype->size_multiple_);

		// Format
		if (j_etype.contains("format"))
		{
			auto format_string = j_etype["format"].get<string>();
			ntype->format_     = EntryDataFormat::format(format_string);

			// Warn if undefined format
			if (ntype->format_ == EntryDataFormat::anyFormat())
				log::warning("Entry type {} requires undefined format {}", ntype->id(), format_string);
		}

		// Icon
		if (j_etype.contains("icon"))
		{
			ntype->icon_ = j_etype["icon"].get<string>();
			if (strutil::startsWith(ntype->icon_, "e_"))
				ntype->icon_ = ntype->icon_.substr(2);
		}

		// Colour
		if (j_etype.contains("colour"))
			ntype->colour_ = colour::fromString(j_etype.at("colour").get<string>());

		// Category
		if (j_etype.contains("category"))
		{
			ntype->category_ = j_etype["category"].get<string>();

			// Add to category list if needed
			bool exists = false;
			for (auto& category : entry_categories)
			{
				if (strutil::equalCI(category, ntype->category_))
				{
					exists = true;
					break;
				}
			}
			if (!exists)
				entry_categories.push_back(ntype->category_);
		}

		// Extra
		if (j_etype.contains("extra"))
			for (auto extra : j_etype["extra"])
				ntype->extra_[extra.get<string>()] = true;

		// Image format hint
		if (j_etype.contains("image_format"))
			ntype->extra_["image_format"] = j_etype["image_format"].get<string>();

		// Text editor language hint
		if (j_etype.contains("text_language"))
			ntype->extra_["text_language"] = j_etype["text_language"].get<string>();

		// Ensure correct casing
		for (auto& match_name : ntype->match_name_)
			strutil::upperIP(match_name);
		for (auto& match_ext : ntype->match_extension_)
			strutil::upperIP(match_ext);
		for (auto& match_archive : ntype->match_archive_)
			strutil::lowerIP(match_archive);
		for (auto& section : ntype->section_)
			strutil::lowerIP(section);

		entry_types.push_back(std::move(ntype));
	}
}

// -----------------------------------------------------------------------------
// Loads all built-in and custom user entry types
// -----------------------------------------------------------------------------
bool EntryType::loadEntryTypes()
{
	// Get builtin entry types from resource archive
	const auto res_archive = app::archiveManager().programResourceArchive();

	// Check resource archive exists
	if (!res_archive)
	{
		log::error("No resource archive open!");
		return false;
	}

	// Get entry types config dir
	auto etypes_cfg_dir = res_archive->dirAtPath("config/entry_types");
	if (!etypes_cfg_dir)
		return false;

	// Parse each json file in the config dir
	for (auto entry : etypes_cfg_dir->entries())
		if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
			readEntryTypes(j);

	// -------- READ CUSTOM TYPES ---------

	// If the directory doesn't exist create it
	auto path = app::path("entry_types", app::Dir::User);
	if (!fileutil::dirExists(path))
		fileutil::createDir(path);

	// Go through each file in the custom types directory
	for (const auto& file : fileutil::allFilesInDir(path, true, true))
	{
		// Parse file
		try
		{
			if (auto j = jsonutil::parseFile(file); !j.is_discarded())
				readEntryTypes(j);
		}
		catch (const std::exception& e)
		{
			log::error("Error parsing entry type file {}: {}", file, e.what());
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Attempts to detect the given entry's type
// -----------------------------------------------------------------------------
bool EntryType::detectEntryType(ArchiveEntry& entry)
{
	// Do nothing if the entry is a folder or a map marker
	if (entry.type() == etype_folder || entry.type() == etype_map)
		return false;

	// If the entry's size is zero, set it to marker type
	if (entry.size() == 0)
	{
		entry.setType(etype_marker);
		return true;
	}

	// Reset entry type
	entry.setType(etype_unknown);

	// Go through all registered types
	const size_t entry_types_size = entry_types.size();
	for (size_t a = 0; a < entry_types_size; a++)
	{
		// If the current type is more 'reliable' than this one, skip it
		if (entry.typeReliability() >= entry_types[a]->reliability())
			continue;

		// Check for possible type match
		const int r = entry_types[a]->isThisType(entry);
		if (r > 0)
		{
			// Type matches, set it
			entry.setType(entry_types[a].get(), r);

			// No need to continue if the identification is 100% reliable
			if (entry.typeReliability() >= 255)
				return true;
		}
	}

	// Return t/f depending on if a matching type was found
	return entry.type() != etype_unknown;
}

// -----------------------------------------------------------------------------
// Returns the entry type with the given id, or etype_unknown if no id match is
// found
// -----------------------------------------------------------------------------
EntryType* EntryType::fromId(string_view id)
{
	for (const auto& type : entry_types)
		if (type->id_ == id)
			return type.get();

	return etype_unknown;
}

// -----------------------------------------------------------------------------
// Returns the global 'unknown' entry type
// -----------------------------------------------------------------------------
EntryType* EntryType::unknownType()
{
	return etype_unknown;
}

// -----------------------------------------------------------------------------
// Returns the global 'folder' entry type
// -----------------------------------------------------------------------------
EntryType* EntryType::folderType()
{
	return etype_folder;
}

// -----------------------------------------------------------------------------
// Returns the global 'map marker' entry type
// -----------------------------------------------------------------------------
EntryType* EntryType::mapMarkerType()
{
	return etype_map;
}

// -----------------------------------------------------------------------------
// Returns a list of icons for all entry types, organised by type index
// -----------------------------------------------------------------------------
vector<string> EntryType::iconList()
{
	vector<string> list;

	list.reserve(entry_types.size());
	for (auto& entry_type : entry_types)
		list.push_back(entry_type->icon());

	return list;
}

// -----------------------------------------------------------------------------
// Returns a list of all entry types
// -----------------------------------------------------------------------------
vector<EntryType*> EntryType::allTypes()
{
	vector<EntryType*> etypes;
	etypes.reserve(entry_types.size());
	for (const auto& type : entry_types)
		etypes.push_back(type.get());
	return etypes;
}

// -----------------------------------------------------------------------------
// Returns a list of all entry type categories
// -----------------------------------------------------------------------------
vector<string> EntryType::allCategories()
{
	return entry_categories;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Command to attempt to detect the currently selected entries as the given
// type id. Lists all type ids if no parameters given
// -----------------------------------------------------------------------------
CONSOLE_COMMAND(type, 0, true)
{
	auto all_types = EntryType::allTypes();
	if (args.empty())
	{
		// List existing types and their IDs
		string       listing   = "List of entry types:\n\t";
		const string separator = "]\n\t";
		const string colon     = ": ";
		const string paren     = " [";
		for (size_t a = 3; a < all_types.size(); a++)
		{
			listing += all_types[a]->name();
			listing += paren;
			listing += all_types[a]->id();
			listing += colon;
			listing += all_types[a]->formatId();
			listing += separator;
		}
		log::info(listing);
	}
	else
	{
		// Find type by id or first matching format
		auto desttype = EntryType::unknownType();
		bool match    = false;

		// Use true unknown type rather than map marker...
		if (strutil::equalCI(args[0], "unknown") || strutil::equalCI(args[0], "none")
			|| strutil::equalCI(args[0], "any"))
			match = true;

		// Find actual format
		else
			for (size_t a = 3; a < all_types.size(); a++)
			{
				if (strutil::equalCI(args[0], all_types[a]->formatId())
					|| strutil::equalCI(args[0], all_types[a]->id()))
				{
					desttype = all_types[a];
					match    = true;
					break;
				}
			}
		if (!match)
		{
			log::info("Type {} does not exist (use \"type\" without parameter for a list)", args[0]);
			return;
		}

		// Allow to force type change even if format checks fails (use at own risk!)
		int  force = !(args.size() < 2 || strutil::equalCI(args[1], "force"));
		auto meep  = maineditor::currentEntrySelection();
		if (meep.empty())
		{
			log::info("No entry selected");
			return;
		}

		EntryDataFormat* foo = nullptr;
		if (desttype != EntryType::unknownType())
		{
			// Check if format corresponds to entry
			foo = EntryDataFormat::format(desttype->formatId());
			if (foo)
				log::info("Identifying as {}", desttype->name());
			else
				log::info("No data format for this type!");
		}
		else
			force = true; // Always force the unknown type

		for (auto& b : meep)
		{
			int okay = false;
			if (foo)
			{
				okay = foo->isThisFormat(b->data());
				if (okay)
					log::info("{}: Identification successful ({}/255)", b->name(), okay);
				else
					log::info("{}: Identification failed", b->name());
			}

			// Change type
			if (force || okay)
			{
				b->setType(desttype, okay);
				log::info("{}: Type changed.", b->name());
			}
		}
	}
}

CONSOLE_COMMAND(size, 0, true)
{
	const auto meep = maineditor::currentEntry();
	if (!meep)
	{
		log::info("No entry selected");
		return;
	}
	log::info("{}: {} bytes", meep->name(), meep->size());
}
