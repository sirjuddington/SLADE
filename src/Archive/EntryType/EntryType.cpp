
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/ZipArchive.h"
#include "General/Console.h"
#include "MainEditor/MainEditor.h"
#include "Utility/FileUtils.h"
#include "Utility/Parser.h"
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
			if (entry.parent() && entry.parent()->formatDesc().entry_format == a)
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
	bool extorname   = false;
	bool matchedname = false;
	if (match_ext_or_name_ && !match_name_.empty() && !match_extension_.empty())
		extorname = true;

	// Entry name related stuff
	if (!match_name_.empty() || !match_extension_.empty())
	{
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
void slade::EntryType::initTypes()
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
// Reads in a block of entry type definitions. Returns false if there was a
// parsing error, true otherwise
// -----------------------------------------------------------------------------
bool EntryType::readEntryTypeDefinitions(string_view definitions, string_view source)
{
	// Parse the definition
	const Parser p;
	p.parseText(definitions, source);

	// Get entry_types tree
	auto pt_etypes = p.parseTreeRoot()->childPTN("entry_types");

	// Check it exists
	if (!pt_etypes)
		return false;

	// Go through all parsed types
	for (unsigned a = 0; a < pt_etypes->nChildren(); a++)
	{
		// Get child as ParseTreeNode
		auto typenode = pt_etypes->childPTN(a);

		// Create new entry type
		auto ntype = std::make_unique<EntryType>(strutil::lower(typenode->name()));

		// Copy from existing type if inherited
		if (!typenode->inherit().empty())
		{
			auto parent_type = fromId(strutil::lower(typenode->inherit()));

			if (parent_type != etype_unknown)
				parent_type->copyToType(*ntype);
			else
				log::info("Warning: Entry type {} inherits from unknown type {}", ntype->id(), typenode->inherit());
		}

		// Go through all parsed fields
		for (unsigned b = 0; b < typenode->nChildren(); b++)
		{
			// Get child as ParseTreeNode
			auto fieldnode = typenode->childPTN(b);
			auto fn_name   = strutil::lower(fieldnode->name());

			// Process it
			if (fn_name == "name") // Name field
			{
				ntype->name_ = fieldnode->stringValue();
			}
			else if (fn_name == "detectable") // Detectable field
			{
				ntype->detectable_ = fieldnode->boolValue();
			}
			else if (fn_name == "export_ext") // Export Extension field
			{
				ntype->extension_ = fieldnode->stringValue();
			}
			else if (fn_name == "format") // Format field
			{
				auto format_string = fieldnode->stringValue();
				ntype->format_     = EntryDataFormat::format(format_string);

				// Warn if undefined format
				if (ntype->format_ == EntryDataFormat::anyFormat())
					log::warning("Entry type {} requires undefined format {}", ntype->id(), format_string);
			}
			else if (fn_name == "icon") // Icon field
			{
				ntype->icon_ = fieldnode->stringValue();
				if (strutil::startsWith(ntype->icon_, "e_"))
					ntype->icon_ = ntype->icon_.substr(2);
			}
			else if (fn_name == "editor") // Editor field (to be removed)
			{
				ntype->editor_ = fieldnode->stringValue();
			}
			else if (fn_name == "section") // Section field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->section_.emplace_back(strutil::lower(fieldnode->stringValue(v)));
			}
			else if (fn_name == "match_ext") // Match Extension field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->match_extension_.emplace_back(strutil::upper(fieldnode->stringValue(v)));
			}
			else if (fn_name == "match_name") // Match Name field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->match_name_.emplace_back(strutil::upper(fieldnode->stringValue(v)));
			}
			else if (fn_name == "match_extorname") // Match name or extension
			{
				ntype->match_ext_or_name_ = fieldnode->boolValue();
			}
			else if (fn_name == "size") // Size field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->match_size_.push_back(fieldnode->intValue(v));
			}
			else if (fn_name == "min_size") // Min Size field
			{
				ntype->size_limit_[0] = fieldnode->intValue();
			}
			else if (fn_name == "max_size") // Max Size field
			{
				ntype->size_limit_[1] = fieldnode->intValue();
			}
			else if (fn_name == "size_multiple") // Size Multiple field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->size_multiple_.push_back(fieldnode->intValue(v));
			}
			else if (fn_name == "reliability") // Reliability field
			{
				ntype->reliability_ = static_cast<uint8_t>(fieldnode->intValue());
			}
			else if (fn_name == "match_archive") // Archive field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->match_archive_.emplace_back(strutil::lower(fieldnode->stringValue(v)));
			}
			else if (fn_name == "extra") // Extra properties
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->extra_[fieldnode->stringValue(v)] = true;
			}
			else if (fn_name == "category") // Type category
			{
				ntype->category_ = fieldnode->stringValue();

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
			else if (fn_name == "image_format") // Image format hint
				ntype->extra_["image_format"] = fieldnode->stringValue(0);
			else if (fn_name == "colour") // Colour
			{
				if (fieldnode->nValues() >= 3)
					ntype->colour_ = ColRGBA(
						static_cast<uint8_t>(fieldnode->intValue(0)),
						static_cast<uint8_t>(fieldnode->intValue(1)),
						static_cast<uint8_t>(fieldnode->intValue(2)));
				else
					log::warning("Not enough colour components defined for entry type {}", ntype->id());
			}
			else
			{
				// Unhandled properties can go into 'extra', only their first value is kept
				ntype->extra_[fieldnode->name()] = fieldnode->stringValue();
			}
		}

		// ntype->dump();
		ntype->index_ = static_cast<int>(entry_types.size());
		entry_types.push_back(std::move(ntype));
	}

	return true;
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

	// Get entry types config
	auto etypes_cfg = res_archive->entryAtPath("config/entry_types.cfg");

	// Check it exists
	if (!etypes_cfg)
	{
		log::error("config/entry_types.cfg does not exist in slade.pk3");
		return false;
	}

	// Get full config as string (process #includes)
	string full_config;
	strutil::processIncludes(etypes_cfg, full_config);

	// Parse config
	auto etypes_read = readEntryTypeDefinitions(full_config, "entry_types.cfg");

	// Warn if no types were read (this shouldn't happen unless the resource archive is corrupted)
	if (!etypes_read)
		log::warning("No built-in entry types could be loaded from slade.pk3");

	// -------- READ CUSTOM TYPES ---------

	// If the directory doesn't exist create it
	auto path = app::path("entry_types", app::Dir::User);
	if (!fileutil::dirExists(path))
		fileutil::createDir(path);

	// Go through each file in the custom types directory
	for (const auto& file : fileutil::allFilesInDir(path, true, true))
	{
		// Load file data
		MemChunk mc;
		mc.importFile(file);

		// Parse file
		readEntryTypeDefinitions(mc.asString(), file);
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
