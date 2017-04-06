/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    EntryType.cpp
 * Description: Entry Type detection system
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "MainEditor/MainEditor.h"
#include "EntryType.h"
#include "Utility/Tokenizer.h"
#include "General/Console/Console.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/ZipArchive.h"
#include "MainEditor/BinaryControlLump.h"
#include "Utility/Parser.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
vector<EntryType*>	entry_types;		// The big list of all entry types
vector<string>		entry_categories;	// All entry type categories

// Special entry types
EntryType			etype_unknown;	// The default, 'unknown' entry type
EntryType			etype_folder;	// Folder entry type
EntryType			etype_marker;	// Marker entry type
EntryType			etype_map;		// Map marker type


/*******************************************************************
 * ENTRYTYPE CLASS FUNCTIONS
 *******************************************************************/

/* EntryType::EntryType
 * EntryType class constructor
 *******************************************************************/
EntryType::EntryType(string id)
{
	// Init info variables
	this->id = id;
	name = "Unknown";
	extension = "dat";
	icon = "default";
	editor = "default";
	reliability = 255;
	category = "Data";
	colour = COL_WHITE;

	// Init match criteria
	format = EntryDataFormat::anyFormat();
	size_limit[0] = -1;
	size_limit[1] = -1;
	detectable = true;
}

/* EntryType::~EntryType
 * EntryType class constructor
 *******************************************************************/
EntryType::~EntryType()
{
	match_extension.clear();
	match_name.clear();
	match_size.clear();
	match_archive.clear();
	size_multiple.clear();
}

/* EntryType::addToList
 * Adds the type to the list of entry types
 *******************************************************************/
void EntryType::addToList()
{
	entry_types.push_back(this);
	index = entry_types.size() - 1;
}

/* EntryType::dump
 * Dumps entry type info to the log
 *******************************************************************/
void EntryType::dump()
{
	LOG_MESSAGE(1, "Type %s \"%s\", format %s, extension %s", id, name, format->getId(),extension);
	LOG_MESSAGE(1, "Size limit: %d-%d", size_limit[0], size_limit[1]);

	for (size_t a = 0; a < match_archive.size(); a++)
		LOG_MESSAGE(1, "Match Archive: \"%s\"", match_archive[a]);

	for (size_t a = 0; a < match_extension.size(); a++)
		LOG_MESSAGE(1, "Match Extension: \"%s\"", match_extension[a]);

	for (size_t a = 0; a < match_name.size(); a++)
		LOG_MESSAGE(1, "Match Name: \"%s\"", match_name[a]);

	for (size_t a = 0; a < match_size.size(); a++)
		LOG_MESSAGE(1, "Match Size: %d", match_size[a]);

	for (size_t a = 0; a < size_multiple.size(); a++)
		LOG_MESSAGE(1, "Size Multiple: %d", size_multiple[a]);

	LOG_MESSAGE(1, "---");
}

/* EntryType::copyToType
 * Copies this entry type's info/properties to [target]
 *******************************************************************/
void EntryType::copyToType(EntryType* target)
{
	// Copy type attributes
	target->editor = editor;
	target->extension = extension;
	target->icon = icon;
	target->name = name;
	target->reliability = reliability;
	target->category = category;
	target->colour = colour;

	// Copy type match criteria
	target->format = format;
	target->size_limit[0] = size_limit[0];
	target->size_limit[1] = size_limit[1];
	target->section = section;
	target->match_extension = match_extension;
	target->match_name = match_name;
	target->match_size = match_size;
	target->match_extension = match_extension;
	target->match_archive = match_archive;

	// Copy extra properties
	extra.copyTo(target->extra);
}

/* EntryType::getFileFilterString
 * Returns a file filter string for this type:
 * "<type name> files (*.<type extension)|*.<type extension>"
 *******************************************************************/
string EntryType::getFileFilterString()
{
	string ret = name + " files (*.";
	ret += extension;
	ret += ")|*.";
	ret += extension;

	return ret;
}

/* EntryType::isThisType
 * Returns true if [entry] matches the EntryType's criteria, false
 * otherwise
 *******************************************************************/
int EntryType::isThisType(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return EDF_FALSE;

	// Check type is detectable
	if (!detectable)
		return EDF_FALSE;

	// Check min size
	if (size_limit[0] >= 0 && entry->getSize() < (unsigned)size_limit[0])
		return EDF_FALSE;

	// Check max size
	if (size_limit[1] >= 0 && entry->getSize() > (unsigned)size_limit[1])
		return EDF_FALSE;

	// Check for archive match if needed
	if (!match_archive.empty())
	{
		bool match = false;
		for (size_t a = 0; a < match_archive.size(); a++)
		{
			if (entry->getParent() && entry->getParent()->getFormat() == match_archive[a])
			{
				match = true;
				break;
			}
		}
		if (!match)
			return EDF_FALSE;
	}

	// Check for size match if needed
	if (!match_size.empty())
	{
		bool match = false;
		for (size_t a = 0; a < match_size.size(); a++)
		{
			if (entry->getSize() == match_size[a])
			{
				match = true;
				break;
			}
		}

		if (!match)
			return EDF_FALSE;
	}

	// Check for data format match if needed
	int r = EDF_TRUE;
	if (format == EntryDataFormat::textFormat())
	{
		// Hack for identifying ACS script sources despite DB2 apparently appending
		// two null bytes to them, which make the memchr test fail.
		size_t end = entry->getSize() - 1;
		if (end > 3) end -= 2;
		// Text is a special case, as other data formats can sometimes be detected as 'text',
		// we'll only check for it if text data is specified in the entry type
		if (entry->getSize() > 0 && memchr(entry->getData(), 0, end) != NULL)
			return EDF_FALSE;
	}
	else if (format != EntryDataFormat::anyFormat() && entry->getSize() > 0)
	{
		r = format->isThisFormat(entry->getMCData());
		if (r == EDF_FALSE)
			return EDF_FALSE;
	}

	// Check for size multiple match if needed
	if (!size_multiple.empty())
	{
		bool match = false;
		size_t size_multiple_size = size_multiple.size();
		for (size_t a = 0; a < size_multiple_size; a++)
		{
			if (entry->getSize() % size_multiple[a] == 0)
			{
				match = true;
				break;
			}
		}

		if (!match)
			return EDF_FALSE;
	}

	// If both names and extensions are defined, and the type only needs one
	// of the two, not both, take it into account.
	bool extorname = false;
	bool matchedname = false;
	if (matchextorname && !match_name.empty() && !match_extension.empty())
		extorname = true;

	// Entry name related stuff
	if (!match_name.empty() || !match_extension.empty())
	{
		// Get entry name (lowercase), find extension separator
		string fn = entry->getUpperName();
		size_t ext_sep = fn.find_first_of('.', 0);

		// Check for name match if needed
		if (!match_name.empty())
		{
			string name = fn;
			if (ext_sep != wxString::npos)
				name = fn.Left(ext_sep);

			bool match = false;
			size_t match_name_size = match_name.size();
			for (size_t a = 0; a < match_name_size; a++)
			{
				if (name.Matches(match_name[a]))
				{
					match = true;
					break;
				}
			}

			if (!match && !extorname)
				return EDF_FALSE;
			else matchedname = match;
		}

		// Check for extension match if needed
		if (!match_extension.empty())
		{
			bool match = false;
			if (ext_sep != wxString::npos)
			{
				string ext = fn.Mid(ext_sep+1);
				size_t match_extension_size = match_extension.size();
				for (size_t a = 0; a < match_extension_size; a++)
				{
					if (ext == match_extension[a])
					{
						match = true;
						break;
					}
				}
			}

			if (!match && !(extorname && matchedname))
				return EDF_FALSE;
		}
	}

	//// Entry name related stuff
//   if (match_name.size() > 0 || match_extension.size() > 0) {
//       // Get full entry name as filename (only do this if absolutely necessary, wxFileName stuff is slow)
//       wxFileName fn(entry->getName());

//       // Check for name match if needed
//       if (match_name.size() > 0) {
//           string name = fn.GetName().Lower();
//           bool match = false;
//           for (size_t a = 0; a < match_name.size(); a++) {
//               if (name.Matches(match_name[a])) {
//                   match = true;
//                   break;
//               }
//           }

//           if (!match && !extorname)
//               return EDF_FALSE;
//           else matchedname = match;
//       }

//       // Check for extension match if needed
//       if (match_extension.size() > 0) {
//           bool match = false;
//           for (size_t a = 0; a < match_extension.size(); a++) {
//               if (!fn.GetExt().CmpNoCase(match_extension[a])) {
//                   match = true;
//                   break;
//               }
//           }

//           if (!match && !(extorname && matchedname))
//               return EDF_FALSE;
//       }
//   }

	// Check for entry section match if needed
	if (!section.empty())
	{
		// Check entry is part of an archive (if not it can't be in a section)
		if (!entry->getParent())
			return EDF_FALSE;

		string e_section = entry->getParent()->detectNamespace(entry);

		//if (e_section != section)
		//	return EDF_FALSE;
		r = EDF_FALSE;
		for (auto ns : section)
			if (S_CMPNOCASE(ns, e_section))
				r = EDF_TRUE;
	}

	// Passed all checks, so we have a match
	return r;
}

/* EntryType::readEntryTypeDefinition
 * Reads in a block of entry type definitions. Returns false if there
 * was a parsing error, true otherwise
 *******************************************************************/
bool EntryType::readEntryTypeDefinition(MemChunk& mc)
{
	// Parse the definition
	Parser p;
	p.parseText(mc);

	// Get entry_types tree
	ParseTreeNode* pt_etypes = (ParseTreeNode*)(p.parseTreeRoot()->getChild("entry_types"));

	// Check it exists
	if (!pt_etypes)
		return false;

	// Go through all parsed types
	for (unsigned a = 0; a < pt_etypes->nChildren(); a++)
	{
		// Get child as ParseTreeNode
		ParseTreeNode* typenode = (ParseTreeNode*)pt_etypes->getChild(a);

		// Create new entry type
		EntryType* ntype = new EntryType(typenode->getName().Lower());

		// Copy from existing type if inherited
		if (!typenode->getInherit().IsEmpty())
		{
			EntryType* parent_type = EntryType::getType(typenode->getInherit().Lower());

			if (parent_type != EntryType::unknownType())
				parent_type->copyToType(ntype);
			else
				LOG_MESSAGE(1, "Warning: Entry type %s inherits from unknown type %s", ntype->getId(), typenode->getInherit());
		}

		// Go through all parsed fields
		for (unsigned b = 0; b < typenode->nChildren(); b++)
		{
			// Get child as ParseTreeNode
			ParseTreeNode* fieldnode = (ParseTreeNode*)typenode->getChild(b);

			// Process it
			if (S_CMPNOCASE(fieldnode->getName(), "name"))  				// Name field
			{
				ntype->name = fieldnode->getStringValue();
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "detectable"))  		// Detectable field
			{
				ntype->detectable = fieldnode->getBoolValue();
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "export_ext"))  		// Export Extension field
			{
				ntype->extension = fieldnode->getStringValue();
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "format"))  			// Format field
			{
				string format_string = fieldnode->getStringValue();
				ntype->format = EntryDataFormat::getFormat(format_string);

				// Warn if undefined format
				if (ntype->format == EntryDataFormat::anyFormat())
					LOG_MESSAGE(1, "Warning: Entry type %s requires undefined format %s", ntype->getId(), format_string);
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "icon"))  			// Icon field
			{
				ntype->icon = fieldnode->getStringValue();
				if (ntype->icon.StartsWith("e_"))
					ntype->icon = ntype->icon.Mid(2);
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "editor"))  			// Editor field (to be removed)
			{
				ntype->editor = fieldnode->getStringValue();
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "section"))  		// Section field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->section.push_back(fieldnode->getStringValue(v).Lower());
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "match_ext"))  		// Match Extension field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->match_extension.push_back(fieldnode->getStringValue(v).Upper());
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "match_name"))  		// Match Name field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->match_name.push_back(fieldnode->getStringValue(v).Upper());
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "match_extorname"))  // Match name or extension
			{
				ntype->matchextorname = fieldnode->getBoolValue();
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "size"))  			// Size field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->match_size.push_back(fieldnode->getIntValue(v));
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "min_size"))  		// Min Size field
			{
				ntype->size_limit[0] = fieldnode->getIntValue();
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "max_size"))  		// Max Size field
			{
				ntype->size_limit[1] = fieldnode->getIntValue();
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "size_multiple"))  	// Size Multiple field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->size_multiple.push_back(fieldnode->getIntValue(v));
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "reliability"))  	// Reliability field
			{
				ntype->reliability = fieldnode->getIntValue();
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "match_archive"))  	// Archive field
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->match_archive.push_back(fieldnode->getStringValue(v).Lower());
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "extra"))  			// Extra properties
			{
				for (unsigned v = 0; v < fieldnode->nValues(); v++)
					ntype->extra.addFlag(fieldnode->getStringValue(v));
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "category"))  		// Type category
			{
				ntype->category = fieldnode->getStringValue();

				// Add to category list if needed
				bool exists = false;
				for (unsigned c = 0; c < entry_categories.size(); c++)
				{
					if (S_CMPNOCASE(entry_categories[c], ntype->category))
					{
						exists = true;
						break;
					}
				}
				if (!exists) entry_categories.push_back(ntype->category);
			}
			else if (S_CMPNOCASE(fieldnode->getName(), "image_format"))		// Image format hint
				ntype->extra["image_format"] = fieldnode->getStringValue(0);
			else if (S_CMPNOCASE(fieldnode->getName(), "colour"))  			// Colour
			{
				if (fieldnode->nValues() >= 3)
					ntype->colour = rgba_t(fieldnode->getIntValue(0), fieldnode->getIntValue(1), fieldnode->getIntValue(2));
				else
					LOG_MESSAGE(1, "Not enough colour components defined for entry type %s", ntype->getId());
			}
			else
			{
				// Unhandled properties can go into 'extra', only their first value is kept
				ntype->extra[fieldnode->getName()] = fieldnode->getStringValue();
			}
		}

		//ntype->dump();
		ntype->addToList();
	}

	return true;
}

/* EntryType::loadEntryTypes
 * Loads all built-in and custom user entry types
 *******************************************************************/
bool EntryType::loadEntryTypes()
{
	EntryDataFormat* fmt_any = EntryDataFormat::anyFormat();

	// Setup unknown type
	etype_unknown.format = fmt_any;
	etype_unknown.icon = "unknown";
	etype_unknown.detectable = false;
	etype_unknown.reliability = 0;
	etype_unknown.addToList();

	// Setup folder type
	etype_folder.format = fmt_any;
	etype_folder.icon = "folder";
	etype_folder.name = "Folder";
	etype_folder.detectable = false;
	etype_folder.addToList();

	// Setup marker type
	etype_marker.format = fmt_any;
	etype_marker.icon = "marker";
	etype_marker.name = "Marker";
	etype_marker.detectable = false;
	etype_marker.category = "";		// No category, markers only appear when 'All' categories shown
	etype_marker.addToList();

	// Setup map marker type
	etype_map.format = fmt_any;
	etype_map.icon = "map";
	etype_map.name = "Map Marker";
	etype_map.category = "Maps";	// Should appear with maps
	etype_map.detectable = false;
	etype_map.colour = rgba_t(0, 255, 0);
	etype_map.addToList();

	// -------- READ BUILT-IN TYPES ---------

	// Get builtin entry types from resource archive
	Archive* res_archive = theArchiveManager->programResourceArchive();

	// Check resource archive exists
	if (!res_archive)
	{
		LOG_MESSAGE(1, "Error: No resource archive open!");
		return false;
	}

	// Get entry types directory
	ArchiveTreeNode* et_dir = res_archive->getDir("config/entry_types/");

	// Check it exists
	if (!et_dir)
	{
		LOG_MESSAGE(1, "Error: config/entry_types does not exist in slade.pk3");
		return false;
	}

	// Read in each file in the directory
	bool etypes_read = false;
	unsigned int et_dir_numEntries = et_dir->numEntries();
	for (unsigned a = 0; a < et_dir_numEntries; a++)
	{
		if (readEntryTypeDefinition(et_dir->getEntry(a)->getMCData()))
			etypes_read = true;
	}

	// Warn if no types were read (this shouldn't happen unless the resource archive is corrupted)
	if (!etypes_read)
		LOG_MESSAGE(1, "Warning: No built-in entry types could be loaded from slade.pk3");

	// -------- READ CUSTOM TYPES ---------

	// If the directory doesn't exist create it
	if (!wxDirExists(App::path("entry_types", App::Dir::User)))
		wxMkdir(App::path("entry_types", App::Dir::User));

	// Open the custom palettes directory
	wxDir res_dir;
	res_dir.Open(App::path("entry_types", App::Dir::User));

	// Go through each file in the directory
	string filename = wxEmptyString;
	bool files = res_dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
	while (files)
	{
		// Load file data
		MemChunk mc;
		mc.importFile(res_dir.GetName() + "/" + filename);

		// Parse file
		readEntryTypeDefinition(mc);

		// Next file
		files = res_dir.GetNext(&filename);
	}

	return true;
}

/* EntryType::detectEntryType
 * Attempts to detect the given entry's type
 *******************************************************************/
bool EntryType::detectEntryType(ArchiveEntry* entry)
{
	// Do nothing if the entry is a folder or a map marker
	if (!entry || entry->getType() == &etype_folder || entry->getType() == &etype_map)
		return false;

	// If the entry's size is zero, set it to marker type
	if (entry->getSize() == 0)
	{
		entry->setType(&etype_marker);
		return true;
	}

	// Reset entry type
	entry->setType(&etype_unknown);

	// Go through all registered types
	size_t entry_types_size = entry_types.size();
	for (size_t a = 0; a < entry_types_size; a++)
	{
		// If the current type is more 'reliable' than this one, skip it
		if (entry->getTypeReliability() >= entry_types[a]->getReliability())
			continue;

		// Check for possible type match
		int r = entry_types[a]->isThisType(entry);
		if (r > 0)
		{
			// Type matches, set it
			entry->setType(entry_types[a], r);

			// No need to continue if the identification is 100% reliable
			if (entry->getTypeReliability() >= 255)
				return true;
		}
	}

	// Return t/f depending on if a matching type was found
	if (entry->getType() == &etype_unknown)
		return false;
	else
		return true;
}

/* EntryType::getType
 * Returns the entry type with the given id, or etype_unknown if no
 * id match is found
 *******************************************************************/
EntryType* EntryType::getType(string id)
{
	for (size_t a = 0; a < entry_types.size(); a++)
	{
		if (entry_types[a]->getId() == id)
			return entry_types[a];
	}

	return &etype_unknown;
}

/* EntryType::unknownType
 * Returns the global 'unknown' entry type
 *******************************************************************/
EntryType* EntryType::unknownType()
{
	return &etype_unknown;
}

/* EntryType::folderType
 * Returns the global 'folder' entry type
 *******************************************************************/
EntryType* EntryType::folderType()
{
	return &etype_folder;
}

/* EntryType::mapMarkerType
 * Returns the global 'map marker' entry type
 *******************************************************************/
EntryType* EntryType::mapMarkerType()
{
	return &etype_map;
}

/* EntryType::getIconList
 * Returns a list of icons for all entry types, organised by type
 * index
 *******************************************************************/
wxArrayString EntryType::getIconList()
{
	wxArrayString list;

	for (size_t a = 0; a < entry_types.size(); a++)
		list.Add(entry_types[a]->getIcon());

	return list;
}

/* EntryType::cleanupEntryTypes
 * Clears all defined entry types
 *******************************************************************/
void EntryType::cleanupEntryTypes()
{
	for (size_t a = 3; a < entry_types.size(); a++)
	{
		EntryType* e = entry_types[a];
		if (e != &etype_unknown && e != &etype_folder && e != &etype_marker && e != &etype_map)
			delete entry_types[a];
	}
}

/* EntryType::allTypes
 * Returns a list of all entry types
 *******************************************************************/
vector<EntryType*> EntryType::allTypes()
{
	return entry_types;
}

/* EntryType::allCategories
 * Returns a list of all entry type categories
 *******************************************************************/
vector<string> EntryType::allCategories()
{
	return entry_categories;
}


/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

// Command to attempt to detect the currently selected entries
// as the given type id. Lists all type ids if no parameters given
CONSOLE_COMMAND (type, 0, true)
{
	vector<EntryType*> all_types = EntryType::allTypes();
	if (args.size() == 0)
	{
		// List existing types and their IDs
		string listing = "List of entry types:\n\t";
		string separator = "]\n\t"; string colon = ": "; string paren = " [";
		for (size_t a = 3; a < all_types.size(); a++)
		{
			listing += all_types[a]->getName();
			listing += paren;
			listing += all_types[a]->getId();
			listing += colon;
			listing += all_types[a]->getFormat();
			listing += separator;
		}
		LOG_MESSAGE(1, listing);
	}
	else
	{
		// Find type by id or first matching format
		EntryType* desttype = EntryType::unknownType();
		bool match = false;

		// Use true unknown type rather than map marker...
		if (!args[0].CmpNoCase("unknown") || !args[0].CmpNoCase("none") || !args[0].CmpNoCase("any"))
			match = true;

		// Find actual format
		else for (size_t a = 3; a < all_types.size(); a++)
			{
				if (!args[0].CmpNoCase(all_types[a]->getFormat()) || !args[0].CmpNoCase(all_types[a]->getId()))
				{
					desttype = all_types[a];
					match = true;
					break;
				}
			}
		if (!match)
		{
			LOG_MESSAGE(1, "Type %s does not exist (use \"type\" without parameter for a list)", args[0].mb_str());
			return;
		}

		// Allow to force type change even if format checks fails (use at own risk!)
		int okay = 0, force = !(args.size() < 2 || args[1].CmpNoCase("force"));
		vector<ArchiveEntry*> meep = MainEditor::currentEntrySelection();
		if (meep.size() == 0)
		{
			LOG_MESSAGE(1, "No entry selected");
			return;
		}

		EntryDataFormat* foo = NULL;
		if (desttype != EntryType::unknownType())
		{
			// Check if format corresponds to entry
			foo = EntryDataFormat::getFormat(desttype->getFormat());
			if (foo)
				LOG_MESSAGE(1, "Identifying as %s", desttype->getName().mb_str());
			else LOG_MESSAGE(1, "No data format for this type!");
		}
		else force = true; // Always force the unknown type

		for (size_t b = 0; b < meep.size(); ++b)
		{
			okay = false;
			if (foo)
			{
				okay = foo->isThisFormat(meep[b]->getMCData());
				if (okay) LOG_MESSAGE(1, "%s: Identification successful (%i/255)", meep[b]->getName().mb_str(), okay);
				else LOG_MESSAGE(1, "%s: Identification failed", meep[b]->getName().mb_str());
			}

			// Change type
			if (force || okay)
			{
				meep[b]->setType(desttype, okay);
				LOG_MESSAGE(1, "%s: Type changed.", meep[b]->getName().mb_str());
			}
		}
	}
}

CONSOLE_COMMAND (size, 0, true)
{
	ArchiveEntry* meep = MainEditor::currentEntry();
	if (!meep)
	{
		LOG_MESSAGE(1, "No entry selected");
		return;
	}
	LOG_MESSAGE(1, "%s: %i bytes", meep->getName().mb_str(), meep->getSize());
}
