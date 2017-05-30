
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    ArchiveOperations.cpp
 * Description: Functions that perform specific operations on archives
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
#include "UI/WxStuff.h"
#include "ArchiveOperations.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/WadArchive.h"
#include "Graphics/CTexture/TextureXList.h"
#include "General/ResourceManager.h"
#include "Dialogs/ExtMessageDialog.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditor/SLADEMap/MapSide.h"
#include "MapEditor/SLADEMap/MapSector.h"
#include "MapEditor/SLADEMap/MapThing.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "General/Console/Console.h"



/*******************************************************************
 * VARIABLES
 *******************************************************************/
typedef std::map<string, int> StrIntMap;
typedef std::map<string, vector<ArchiveEntry*> > PathMap;
typedef std::map<int, vector<ArchiveEntry*> > CRCMap;


/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

/* ArchiveOperations::removeUnusedPatches
 * Removes any patches and associated entries from [archive] that
 * are not used in any texture definitions
 *******************************************************************/
bool ArchiveOperations::removeUnusedPatches(Archive* archive)
{
	if (!archive)
		return false;

	// Find PNAMES entry
	Archive::search_options_t opt;
	opt.match_type = EntryType::getType("pnames");
	ArchiveEntry* pnames = archive->findLast(opt);

	// Find TEXTUREx entries
	opt.match_type = EntryType::getType("texturex");
	vector<ArchiveEntry*> tx_entries = archive->findAll(opt);

	// Can't do anything without PNAMES/TEXTUREx
	if (!pnames || tx_entries.size() == 0)
		return false;

	// Open patch table
	PatchTable ptable;
	ptable.loadPNAMES(pnames, archive);

	// Open texturex entries to update patch usage
	vector<TextureXList*> tx_lists;
	for (unsigned a = 0; a < tx_entries.size(); a++)
	{
		TextureXList* texturex = new TextureXList();
		texturex->readTEXTUREXData(tx_entries[a], ptable);
		for (unsigned t = 0; t < texturex->nTextures(); t++)
			ptable.updatePatchUsage(texturex->getTexture(t));
		tx_lists.push_back(texturex);
	}

	// Go through patch table
	unsigned removed = 0;
	vector<ArchiveEntry*> to_remove;
	for (unsigned a = 0; a < ptable.nPatches(); a++)
	{
		patch_t& p = ptable.patch(a);

		// Check if used in any texture
		if (p.used_in.size() == 0)
		{
			// Unused

			// If its entry is in the archive, flag it to be removed
			ArchiveEntry* entry = theResourceManager->getPatchEntry(p.name, "patches", archive);
			if (entry && entry->getParent() == archive)
				to_remove.push_back(entry);

			// Update texturex list patch indices
			for (unsigned t = 0; t < tx_lists.size(); t++)
				tx_lists[t]->removePatch(p.name);

			// Remove the patch from the patch table
			LOG_MESSAGE(1, "Removed patch %s", p.name);
			removed++;
			ptable.removePatch(a--);
		}
	}

	// Remove unused patch entries
	for (unsigned a = 0; a < to_remove.size(); a++)
	{
		LOG_MESSAGE(1, "Removed entry %s", to_remove[a]->getName());
		archive->removeEntry(to_remove[a]);
	}

	// Write PNAMES changes
	ptable.writePNAMES(pnames);

	// Write TEXTUREx changes
	for (unsigned a = 0; a < tx_lists.size(); a++)
		tx_lists[a]->writeTEXTUREXData(tx_entries[a], ptable);

	// Cleanup
	for (unsigned a = 0; a < tx_lists.size(); a++)
		delete tx_lists[a];

	// Notify user
	wxMessageBox(S_FMT("Removed %d patches and %lu entries. See console log for details.", removed, to_remove.size()), "Removed Unused Patches", wxOK|wxICON_INFORMATION);

	return true;
}

/* ArchiveOperations::checkDuplicateEntryNames
 * Checks [archive] for multiple entries of the same name, and
 * displays a list of duplicate entry names if any are found
 *******************************************************************/
bool ArchiveOperations::checkDuplicateEntryNames(Archive* archive)
{
	StrIntMap map_namecounts;
	PathMap map_entries;

	// Get list of all entries in archive
	vector<ArchiveEntry*> entries;
	archive->getEntryTreeAsList(entries);

	// Go through list
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Skip directory entries
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Increment count for entry name
		map_namecounts[entries[a]->getPath(true)] += 1;

		// Enqueue entries
		map_entries[entries[a]->getName(true)].push_back(entries[a]);
	}

	// Generate string of duplicate entry names
	string dups;
	// Treeless archives such as WADs can just include a simple list of duplicated names and how often they appear
	if (archive->isTreeless())
	{
		StrIntMap::iterator i = map_namecounts.begin();
		while (i != map_namecounts.end())
		{
			if (i->second > 1)
			{
				string name = i->first;
				name.Remove(0, 1);
				dups += S_FMT("%s appears %d times\n", name, i->second);
			}
			i++;
		}
		// Hierarchized archives, however, need to compare only the name (not the whole path) and to display the full
		// path of each entry with a duplicated name, so that they might be found more easily than by having the user
		// recurse through the entire directory tree -- such a task is something a program should do instead.
	}
	else
	{
		PathMap::iterator i = map_entries.begin();
		while (i != map_entries.end())
		{
			if (i->second.size() > 1)
			{
				string name;
				dups += S_FMT("\n%i entries are named %s\t", i->second.size(), i->first);
				vector<ArchiveEntry*>::iterator j = i->second.begin();
				while (j != i->second.end())
				{
					name = (*j)->getPath(true); name.Remove(0, 1);
					dups += S_FMT("\t%s", name);
					++j;
				}
			}
			++i;
		}
	}

	// If no duplicates exist, do nothing
	if (dups.IsEmpty())
	{
		wxMessageBox("No duplicated entry names exist");
		return false;
	}

	// Display list of duplicate entry names
	ExtMessageDialog msg(theMainWindow, "Duplicate Entries");
	msg.setExt(dups);
	msg.setMessage("The following entry names are duplicated:");
	msg.ShowModal();

	return true;
}

/* ArchiveOperations::removeEntriesUnchangedFromIWAD
 * Compare the archive's entries with those sharing the same name
 * and namespace in the base resource archive, deleting duplicates
 *******************************************************************/
void ArchiveOperations::removeEntriesUnchangedFromIWAD(Archive* archive)
{
	// Do nothing if there is no base resource archive,
	// or if the archive *is* the base resource archive.
	Archive* bra = theArchiveManager->baseResourceArchive();
	if (bra == NULL || bra == archive || archive == NULL)
		return;

	// Get list of all entries in archive
	vector<ArchiveEntry*> entries;
	archive->getEntryTreeAsList(entries);

	// Init search options
	Archive::search_options_t search;
	ArchiveEntry* other = NULL;
	string dups = "";
	size_t count = 0;

	// Go through list
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Skip directory entries
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Skip markers
		if (entries[a]->getType() == EntryType::mapMarkerType() || entries[a]->getSize() == 0)
			continue;

		// Now, let's look for a counterpart in the IWAD
		search.match_namespace = archive->detectNamespace(entries[a]);
		search.match_name = entries[a]->getName();
		other = bra->findLast(search);

		// If there is one, and it is identical, remove it
		if (other != NULL && (other->getMCData().crc() == entries[a]->getMCData().crc()))
		{
			++count;
			dups += S_FMT("%s\n", search.match_name);
			archive->removeEntry(entries[a]);
			entries[a] = NULL;
		}
	}


	// If no duplicates exist, do nothing
	if (count == 0)
	{
		wxMessageBox("No duplicated entries exist");
		return;
	}

	string message = S_FMT("The following %d entr%s duplicated from the base resource archive and deleted:",
						   count, (count > 1) ? "ies were" : "y was");

	// Display list of deleted duplicate entries
	ExtMessageDialog msg(theMainWindow, (count > 1) ? "Deleted Entries" : "Deleted Entry");
	msg.setExt(dups);
	msg.setMessage(message);
	msg.ShowModal();
}

/* ArchiveOperations::checkDuplicateEntryContent
 * Checks [archive] for multiple entries with the same data, and
 * displays a list of the duplicate entries' names if any are found
 *******************************************************************/
bool ArchiveOperations::checkDuplicateEntryContent(Archive* archive)
{
	CRCMap map_entries;

	// Get list of all entries in archive
	vector<ArchiveEntry*> entries;
	archive->getEntryTreeAsList(entries);
	string dups = "";

	// Go through list
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Skip directory entries
		if (entries[a]->getType() == EntryType::folderType())
			continue;

		// Skip markers
		if (entries[a]->getType() == EntryType::mapMarkerType() || entries[a]->getSize() == 0)
			continue;

		// Enqueue entries
		map_entries[entries[a]->getMCData().crc()].push_back(entries[a]);
	}

	// Now iterate through the dupes to list the name of the duplicated entries
	CRCMap::iterator i = map_entries.begin();
	while (i != map_entries.end())
	{
		if (i->second.size() > 1)
		{
			string name = i->second[0]->getPath(true); name.Remove(0, 1);
			dups += S_FMT("\n%s\t(%8x) duplicated by", name, i->first);
			vector<ArchiveEntry*>::iterator j = i->second.begin() + 1;
			while (j != i->second.end())
			{
				name = (*j)->getPath(true); name.Remove(0, 1);
				dups += S_FMT("\t%s", name);
				++j;
			}
		}
		++i;
	}

	// If no duplicates exist, do nothing
	if (dups.IsEmpty())
	{
		wxMessageBox("No duplicated entry data exist");
		return false;
	}

	// Display list of duplicate entry names
	ExtMessageDialog msg(theMainWindow, "Duplicate Entries");
	msg.setExt(dups);
	msg.setMessage("The following entry data are duplicated:");
	msg.ShowModal();

	return true;
}



// Hardcoded doom defaults for now
int n_tex_anim = 13;
string tex_anim_start[] =
{
	"BLODGR1",
	"SLADRIP1",
	"BLODRIP1",
	"FIREWALA",
	"GSTFONT1",
	"FIRELAV3",
	"FIREMAG1",
	"FIREBLU1",
	"ROCKRED1",
	"BFALL1",
	"SFALL1",
	"WFALL1",
	"DBRAIN1",
};
string tex_anim_end[] =
{
	"BLODGR4",
	"SLADRIP3",
	"BLODRIP4",
	"FIREWALL",
	"GSTFONT3",
	"FIRELAVA",
	"FIREMAG3",
	"FIREBLU2",
	"ROCKRED3",
	"BFALL4",
	"SFALL4",
	"WFALL4",
	"DBRAIN4",
};

int n_flat_anim = 9;
string flat_anim_start[] =
{
	"NUKAGE1",
	"FWATER1",
	"SWATER1",
	"LAVA1",
	"BLOOD1",
	"RROCK05",
	"SLIME01",
	"SLIME05",
	"SLIME09",
};
string flat_anim_end[] =
{
	"NUKAGE3",
	"FWATER4",
	"SWATER4",
	"LAVA4",
	"BLOOD3",
	"RROCK08",
	"SLIME04",
	"SLIME08",
	"SLIME12",
};

struct texused_t
{
	bool used;
	texused_t() { used = false; }
};
WX_DECLARE_STRING_HASH_MAP(texused_t, TexUsedMap);
void ArchiveOperations::removeUnusedTextures(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// --- Build list of used textures ---
	TexUsedMap used_textures;
	int total_maps = 0;

	// Get all SIDEDEFS entries
	Archive::search_options_t opt;
	opt.match_type = EntryType::getType("map_sidedefs");
	vector<ArchiveEntry*> sidedefs = archive->findAll(opt);
	total_maps += sidedefs.size();

	// Go through and add used textures to list
	doomside_t sdef;
	string tex_lower, tex_middle, tex_upper;
	for (unsigned a = 0; a < sidedefs.size(); a++)
	{
		int nsides = sidedefs[a]->getSize() / 30;
		sidedefs[a]->seek(0, SEEK_SET);
		for (int s = 0; s < nsides; s++)
		{
			// Read side data
			sidedefs[a]->read(&sdef, 30);

			// Get textures
			tex_lower = wxString::FromAscii(sdef.tex_lower, 8);
			tex_middle = wxString::FromAscii(sdef.tex_middle, 8);
			tex_upper = wxString::FromAscii(sdef.tex_upper, 8);

			// Add to used textures list
			used_textures[tex_lower].used = true;
			used_textures[tex_middle].used = true;
			used_textures[tex_upper].used = true;
		}
	}

	// Get all TEXTMAP entries
	opt.match_name = "TEXTMAP";
	opt.match_type = EntryType::getType("udmf_textmap");
	vector<ArchiveEntry*> udmfmaps = archive->findAll(opt);
	total_maps += udmfmaps.size();

	// Go through and add used textures to list
	Tokenizer tz;
	tz.setSpecialCharacters("{};=");
	for (unsigned a = 0; a < udmfmaps.size(); a++)
	{
		// Open in tokenizer
		tz.openMem(udmfmaps[a]->getData(), udmfmaps[a]->getSize(), "UDMF TEXTMAP");

		// Go through text tokens
		string token = tz.getToken();
		while (!token.IsEmpty())
		{
			// Check for sidedef definition
			if (token == "sidedef")
			{
				tz.getToken();	// Skip {

				token = tz.getToken();
				while (token != "}")
				{
					// Check for texture property
					if (token == "texturetop" || token == "texturemiddle" || token == "texturebottom")
					{
						tz.getToken();	// Skip =
						used_textures[tz.getToken()].used = true;
					}

					token = tz.getToken();
				}
			}

			// Next token
			token = tz.getToken();
		}
	}

	// Check if any maps were found
	if (total_maps == 0)
		return;

	// Find all TEXTUREx entries
	opt.match_name = "";
	opt.match_type = EntryType::getType("texturex");
	vector<ArchiveEntry*> tx_entries = archive->findAll(opt);

	// Go through texture lists
	PatchTable ptable;	// Dummy patch table, patch info not needed here
	wxArrayString unused_tex;
	for (unsigned a = 0; a < tx_entries.size(); a++)
	{
		TextureXList txlist;
		txlist.readTEXTUREXData(tx_entries[a], ptable);

		// Go through textures
		bool anim = false;
		for (unsigned t = 1; t < txlist.nTextures(); t++)
		{
			string texname = txlist.getTexture(t)->getName();

			// Check for animation start
			for (int b = 0; b < n_tex_anim; b++)
			{
				if (texname == tex_anim_start[b])
				{
					anim = true;
					break;
				}
			}

			// Check for animation end
			bool thisend = false;
			for (int b = 0; b < n_tex_anim; b++)
			{
				if (texname == tex_anim_end[b])
				{
					anim = false;
					thisend = true;
					break;
				}
			}

			// Mark if unused and not part of an animation
			if (!used_textures[texname].used && !anim && !thisend)
				unused_tex.Add(txlist.getTexture(t)->getName());
		}
	}

	// Pop up a dialog with a checkbox list of unused textures
	wxMultiChoiceDialog dialog(theMainWindow, "The following textures are not used in any map,\nselect which textures to delete", "Delete Unused Textures", unused_tex);

	// Get base resource textures (if any)
	Archive* base_resource = theArchiveManager->baseResourceArchive();
	vector<ArchiveEntry*> base_tx_entries;
	if (base_resource)
		base_tx_entries = base_resource->findAll(opt);
	PatchTable pt_temp;
	TextureXList tx;
	for (unsigned a = 0; a < base_tx_entries.size(); a++)
		tx.readTEXTUREXData(base_tx_entries[a], pt_temp, true);
	vector<string> base_resource_textures;
	for (unsigned a = 0; a < tx.nTextures(); a++)
		base_resource_textures.push_back(tx.getTexture(a)->getName());

	// Determine which textures to check initially
	wxArrayInt selection;
	for (unsigned a = 0; a < unused_tex.size(); a++)
	{
		bool swtex = false;

		// Check for switch texture
		if (unused_tex[a].StartsWith("SW1"))
		{
			// Get counterpart switch name
			string swname = unused_tex[a];
			swname.Replace("SW1", "SW2", false);

			// Check if its counterpart is used
			if (used_textures[swname].used)
				swtex = true;
		}
		else if (unused_tex[a].StartsWith("SW2"))
		{
			// Get counterpart switch name
			string swname = unused_tex[a];
			swname.Replace("SW2", "SW1", false);

			// Check if its counterpart is used
			if (used_textures[swname].used)
				swtex = true;
		}

		// Check for base resource texture
		bool br_tex = false;
		for (unsigned b = 0; b < base_resource_textures.size(); b++)
		{
			if (base_resource_textures[b].CmpNoCase(unused_tex[a]) == 0)
			{
				LOG_MESSAGE(3, "Texture " + base_resource_textures[b] + " is in base resource");
				br_tex = true;
				break;
			}
		}

		if (!swtex && !br_tex)
			selection.Add(a);
	}
	dialog.SetSelections(selection);

	int n_removed = 0;
	if (dialog.ShowModal() == wxID_OK)
	{
		// Get selected textures
		selection = dialog.GetSelections();

		// Go through texture lists
		for (unsigned a = 0; a < tx_entries.size(); a++)
		{
			TextureXList txlist;
			txlist.readTEXTUREXData(tx_entries[a], ptable);

			// Go through selected textures to delete
			for (unsigned t = 0; t < selection.size(); t++)
			{
				// Get texture index
				int index = txlist.textureIndex(unused_tex[selection[t]]);

				// Delete it from the list (if found)
				if (index >= 0)
				{
					txlist.removeTexture(index);
					n_removed++;
				}
			}

			// Write texture list data back to entry
			txlist.writeTEXTUREXData(tx_entries[a], ptable);
		}
	}

	wxMessageBox(S_FMT("Removed %d unused textures", n_removed));
}

void ArchiveOperations::removeUnusedFlats(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// --- Build list of used flats ---
	TexUsedMap used_textures;
	int total_maps = 0;

	// Get all SECTORS entries
	Archive::search_options_t opt;
	opt.match_type = EntryType::getType("map_sectors");
	vector<ArchiveEntry*> sectors = archive->findAll(opt);
	total_maps += sectors.size();

	// Go through and add used flats to list
	doomsector_t sec;
	string tex_floor, tex_ceil;
	for (unsigned a = 0; a < sectors.size(); a++)
	{
		int nsec = sectors[a]->getSize() / 26;
		sectors[a]->seek(0, SEEK_SET);
		for (int s = 0; s < nsec; s++)
		{
			// Read sector data
			sectors[a]->read(&sec, 26);

			// Get textures
			tex_floor = wxString::FromAscii(sec.f_tex, 8);
			tex_ceil = wxString::FromAscii(sec.c_tex, 8);

			// Add to used textures list
			used_textures[tex_floor].used = true;
			used_textures[tex_ceil].used = true;
		}
	}

	// Get all TEXTMAP entries
	opt.match_name = "TEXTMAP";
	opt.match_type = EntryType::getType("udmf_textmap");
	vector<ArchiveEntry*> udmfmaps = archive->findAll(opt);
	total_maps += udmfmaps.size();

	// Go through and add used flats to list
	Tokenizer tz;
	tz.setSpecialCharacters("{};=");
	for (unsigned a = 0; a < udmfmaps.size(); a++)
	{
		// Open in tokenizer
		tz.openMem(udmfmaps[a]->getData(), udmfmaps[a]->getSize(), "UDMF TEXTMAP");

		// Go through text tokens
		string token = tz.getToken();
		while (!token.IsEmpty())
		{
			// Check for sector definition
			if (token == "sector")
			{
				tz.getToken();	// Skip {

				token = tz.getToken();
				while (token != "}")
				{
					// Check for texture property
					if (token == "texturefloor" || token == "textureceiling")
					{
						tz.getToken();	// Skip =
						used_textures[tz.getToken()].used = true;
					}

					token = tz.getToken();
				}
			}

			// Next token
			token = tz.getToken();
		}
	}

	// Check if any maps were found
	if (total_maps == 0)
		return;

	// Find all flats
	opt.match_name = "";
	opt.match_namespace = "flats";
	opt.match_type = NULL;
	vector<ArchiveEntry*> flats = archive->findAll(opt);

	// Create list of all unused flats
	wxArrayString unused_tex;
	bool anim = false;
	for (unsigned a = 0; a < flats.size(); a++)
	{
		// Skip markers
		if (flats[a]->getSize() == 0)
			continue;

		// Check for animation start
		string flatname = flats[a]->getName(true);
		for (int b = 0; b < n_flat_anim; b++)
		{
			if (flatname == flat_anim_start[b])
			{
				anim = true;
				LOG_MESSAGE(1, "%s anim start", flatname);
				break;
			}
		}

		// Check for animation end
		bool thisend = false;
		for (int b = 0; b < n_flat_anim; b++)
		{
			if (flatname == flat_anim_end[b])
			{
				anim = false;
				thisend = true;
				LOG_MESSAGE(1, "%s anim end", flatname);
				break;
			}
		}

		// Add if not animated
		if (!used_textures[flatname].used && !anim && !thisend)
			unused_tex.Add(flatname);
	}

	// Pop up a dialog with a checkbox list of unused textures
	wxMultiChoiceDialog dialog(theMainWindow, "The following textures are not used in any map,\nselect which textures to delete", "Delete Unused Textures", unused_tex);

	// Select all flats initially
	wxArrayInt selection;
	for (unsigned a = 0; a < unused_tex.size(); a++)
		selection.push_back(a);
	dialog.SetSelections(selection);

	int n_removed = 0;
	if (dialog.ShowModal() == wxID_OK)
	{
		// Go through selected flats
		selection = dialog.GetSelections();
		opt.match_namespace = "flats";
		for (unsigned a = 0; a < selection.size(); a++)
		{
			opt.match_name = unused_tex[selection[a]];
			ArchiveEntry* entry = archive->findFirst(opt);
			archive->removeEntry(entry);
			n_removed++;
		}
	}

	wxMessageBox(S_FMT("Removed %d unused flats", n_removed));
}


CONSOLE_COMMAND(test_cleantex, 0, false)
{
	Archive* current = MainEditor::currentArchive();
	if (current) ArchiveOperations::removeUnusedTextures(current);
}

CONSOLE_COMMAND(test_cleanflats, 0, false)
{
	Archive* current = MainEditor::currentArchive();
	if (current) ArchiveOperations::removeUnusedFlats(current);
}

size_t replaceThingsDoom(ArchiveEntry* entry, int oldtype, int newtype)
{
	if (entry == NULL) return 0;

	size_t size = entry->getSize();
	size_t numthings = size / sizeof(doomthing_t);
	size_t changed = 0;

	doomthing_t* things = new doomthing_t[numthings];
	memcpy(things, entry->getData(), size);

	// Perform replacement
	for (size_t t = 0; t < numthings; ++t)
	{
		if (things[t].type == oldtype)
		{
			things[t].type = newtype;
			++changed;
		}
	}
	// Import the changes if needed
	if (changed > 0)
		entry->importMem(things, size);
	delete[] things;

	return changed;
}
size_t replaceThingsDoom64(ArchiveEntry* entry, int oldtype, int newtype)
{
	if (entry == NULL)
		return 0;

	size_t size = entry->getSize();
	size_t numthings = size / sizeof(doom64thing_t);
	size_t changed = 0;

	doom64thing_t* things = new doom64thing_t[numthings];
	memcpy(things, entry->getData(), size);

	// Perform replacement
	for (size_t t = 0; t < numthings; ++t)
	{
		if (things[t].type == oldtype)
		{
			things[t].type = newtype;
			++changed;
		}
	}
	// Import the changes if needed
	if (changed > 0)
		entry->importMem(things, size);
	delete[] things;

	return changed;
}
size_t replaceThingsHexen(ArchiveEntry* entry, int oldtype, int newtype)
{
	if (entry == NULL)
		return 0;

	size_t size = entry->getSize();
	size_t numthings = size / sizeof(hexenthing_t);
	size_t changed = 0;

	hexenthing_t* things = new hexenthing_t[numthings];
	memcpy(things, entry->getData(), size);

	// Perform replacement
	for (size_t t = 0; t < numthings; ++t)
	{
		if (things[t].type == oldtype)
		{
			things[t].type = newtype;
			++changed;
		}
	}
	// Import the changes if needed
	if (changed > 0)
		entry->importMem(things, size);
	delete[] things;

	return changed;
}
size_t replaceThingsUDMF(ArchiveEntry* entry, int oldtype, int newtype)
{
	if (entry == NULL) return 0;

	size_t changed = 0;
	// TODO: parse and replace code
	// Import the changes if needed
	if (changed > 0)
	{
		//entry->importMemChunk(mc);
	}
	return changed;
}
size_t ArchiveOperations::replaceThings(Archive* archive, int oldtype, int newtype)
{
	size_t changed = 0;
	// Check archive was given
	if (!archive)
		return changed;

	// Get all maps
	vector<Archive::mapdesc_t> maps = archive->detectMaps();
	string report = "";

	for (size_t a = 0; a < maps.size(); ++a)
	{
		size_t achanged = 0;
		// Is it an embedded wad?
		if (maps[a].archive)
		{
			// Attempt to open entry as wad archive
			Archive* temp_archive = new WadArchive();
			if (temp_archive->open(maps[a].head))
			{
				achanged = ArchiveOperations::replaceThings(temp_archive, oldtype, newtype);
				MemChunk mc;
				if (!(temp_archive->write(mc, true)))
				{
					achanged = 0;
				}
				else
				{
					temp_archive->close();
					if (!(maps[a].head->importMemChunk(mc)))
					{
						achanged = 0;
					}
				}
			}

			// Cleanup
			delete temp_archive;

		}
		else
		{
			// Find the map entry to modify
			ArchiveEntry* mapentry = maps[a].head;
			ArchiveEntry* things = NULL;
			if (maps[a].format == MAP_DOOM || maps[a].format == MAP_DOOM64 || maps[a].format == MAP_HEXEN)
			{
				while (mapentry && mapentry != maps[a].end)
				{
					if (mapentry->getType() == EntryType::getType("map_things"))
					{
						things = mapentry;
						break;
					}
					mapentry = mapentry->nextEntry();
				}
			}
			else if (maps[a].format == MAP_UDMF)
			{
				while (mapentry && mapentry != maps[a].end)
				{
					if (mapentry->getType() == EntryType::getType("udmf_textmap"))
					{
						things = mapentry;
						break;
					}
					mapentry = mapentry->nextEntry();
				}
			}

			// Did we get a map entry?
			if (things)
			{
				switch(maps[a].format)
				{
				case MAP_DOOM:
					achanged = replaceThingsDoom(things, oldtype, newtype);
					break;
				case MAP_HEXEN:
					achanged = replaceThingsHexen(things, oldtype, newtype);
					break;
				case MAP_DOOM64:
					achanged = replaceThingsDoom64(things, oldtype, newtype);
					break;
				case MAP_UDMF:
					achanged = replaceThingsUDMF(things, oldtype, newtype);
					break;
				default:
					LOG_MESSAGE(1, "Unknown map format for " + maps[a].head->getName());
					break;
				}
			}
		}
		report += S_FMT("%s:\t%i things changed\n", maps[a].head->getName(), achanged);
		changed += achanged;
	}
	LOG_MESSAGE(1, report);
	return changed;
}

CONSOLE_COMMAND(replacethings, 2, true)
{
	Archive* current = MainEditor::currentArchive();
	long oldtype, newtype;

	if (current && args[0].ToLong(&oldtype) && args[1].ToLong(&newtype))
	{
		ArchiveOperations::replaceThings(current, oldtype, newtype);
	}
}

CONSOLE_COMMAND(convertmapchex1to3, 0, false)
{
	Archive* current = MainEditor::currentArchive();
	long rep[23][2] = 
	{				//  #	Chex 1 actor			==>	Chex 3 actor			(unwanted replacement)
		{25, 78},	//  0	ChexTallFlower2			==> PropFlower1				(PropGlobeStand)
		{28, 79},	//  1	ChexTallFlower			==>	PropFlower2				(PropPhone)
		{30, 74},	//  2	ChexCavernStalagmite	==>	PropStalagmite			(PropPineTree)
		{31, 50},	//  3	ChexSubmergedPlant		==>	PropHydroponicPlant		(PropGreyRock)
		{32, 73},	//  4	ChexCavernColumn		==>	PropPillar				(PropBarrel)
		{34, 80},	//  5	ChexChemicalFlask		==>	PropBeaker				(PropCandlestick)
		{35, 36},	//  6	ChexGasTank				==>	PropOxygenTank			(PropCandelabra)
		{43, 9061},	//  7	ChexOrangeTree			==>	TreeOrange				(PropTorchTree)
		{45, 70},	//  8	ChexCivilian1			==>	PropCaptive1			(PropGreenTorch)
		{47, 9060},	//  9	ChexAppleTree			==>	TreeApple				(PropStalagtite)
		{54, 9058},	// 10	ChexBananaTree			==>	TreeBanana				(PropSpaceship -- must go before its own replacement)
		{48, 54},	// 11	ChexSpaceship			==>	PropSpaceship			(PropTechPillar -- must go after banana tree replacement)
		{55, 42},	// 12	ChexLightColumn			==>	LabCoil					(PropShortBlueTorch)
		{56, 26},	// 13	ChexCivilian2			==>	PropCaptive2			(PropShortGreenTorch)
		{57, 52},	// 14	ChexCivilian3			==>	PropCaptive3			(PropShortRedTorch)
		{3002, 58},	// 15	F.CycloptisCommonus		==>	F.CycloptisCommonusV3	(FlemoidusStridicus)
		{3003, 69},	// 16	Flembrane				==>	FlembraneV3				(FlemoidusMaximus)
		{33, 53},	// 17	ChexMineCart			==> PropBazoikCart			(none, but the sprite is modified otherwise)
		{27, 81},	// 18	"HeadOnAStick"			==> PropSmallBrush
		{53, 75},	// 19	"Meat5"					==> PropStalagtite2
		{49, 63},	// 20	Redundant bats
		{51, 59},	// 21	Redundant hanging plant #1
		{50, 61},	// 22	Redundant hanging plant #2
	};
	for (int i = 0; i < 23; ++i)
	{
		ArchiveOperations::replaceThings(current, rep[i][0], rep[i][1]);
	}
}

CONSOLE_COMMAND(convertmapchex2to3, 0, false)
{
	Archive* current = MainEditor::currentArchive();
	long rep[20][2] = 
	{
		{3001, 9057},	//  0	Quadrumpus
		{3002, 9050},	//  1	Larva
		{27, 81},		//  2	"HeadOnAStick"		==> PropSmallBrush
		{70, 49},		//  3	"BurningBarrel"		==> PropStool
		{36, 9055},		//  4	Chex Warrior 
		{52, 9054},		//  5	Tutanhkamen
		{53, 9053},		//  6	Ramses
		{30, 9052},		//  7	Thinker
		{31, 9051},		//  8	David
		{54, 76},		//  9	Triceratops
		{32, 23},		// 10	Chef -- replaced by a dead lost soul in Chex 3
		{33, 9056},		// 11	Big spoon
		{34, 35},		// 12	Street light
		{62, 9053},		// 13	Ramses again
		{56, 49},		// 14	Barstool again
		{57, 77},		// 15	T-rex		
		{49, 63},		// 16	Redundant bats
		{51, 59},		// 17	Redundant hanging plant #1
		{50, 61},		// 18	Redundant hanging plant #2
	};
	for (int i = 0; i < 19; ++i)
	{
		ArchiveOperations::replaceThings(current, rep[i][0], rep[i][1]);
	}
}

size_t replaceSpecialsDoom(ArchiveEntry* entry, int oldtype, int newtype, bool tag, int oldtag, int newtag)
{
	if (entry == NULL) return 0;

	size_t size = entry->getSize();
	size_t numlines = size / sizeof(doomline_t);
	size_t changed = 0;

	doomline_t* lines = new doomline_t[numlines];
	memcpy(lines, entry->getData(), size);

	// Perform replacement
	for (size_t l = 0; l < numlines; ++l)
	{
		if (lines[l].type == oldtype)
		{
			if (!tag || lines[l].sector_tag == oldtag)
			{
				lines[l].type = newtype;
				if (tag)
					lines[l].sector_tag = newtag;
				++changed;
			}
		}
	}
	// Import the changes if needed
	if (changed > 0)
		entry->importMem(lines, size);
	delete[] lines;

	return changed;
}
size_t replaceSpecialsDoom64(ArchiveEntry* entry, int oldtype, int newtype, bool tag, int oldtag, int newtag)
{
	return 0;
}
size_t replaceSpecialsHexen(ArchiveEntry* l_entry, ArchiveEntry* t_entry, int oldtype, int newtype,
							bool arg0, bool arg1, bool arg2, bool arg3, bool arg4,
							int oldarg0, int oldarg1, int oldarg2, int oldarg3, int oldarg4,
							int newarg0, int newarg1, int newarg2, int newarg3, int newarg4)
{
	if (l_entry == NULL && t_entry == NULL)
		return 0;

	size_t size = 0;
	size_t changed = 0;

	if (l_entry)
	{
		size = l_entry->getSize();
		size_t numlines = size / sizeof(hexenline_t);

		hexenline_t* lines = new hexenline_t[numlines];
		memcpy(lines, l_entry->getData(), size);
		size_t lchanged = 0;

		// Perform replacement
		for (size_t l = 0; l < numlines; ++l)
		{
			if (lines[l].type == oldtype)
			{
				if ((!arg0 || lines[l].args[0] == oldarg0) &&
						(!arg1 || lines[l].args[1] == oldarg1) &&
						(!arg2 || lines[l].args[2] == oldarg2) &&
						(!arg3 || lines[l].args[3] == oldarg3) &&
						(!arg4 || lines[l].args[4] == oldarg4))
				{
					lines[l].type = newtype;
					if (arg0) lines[l].args[0] = newarg0;
					if (arg1) lines[l].args[1] = newarg1;
					if (arg2) lines[l].args[2] = newarg2;
					if (arg3) lines[l].args[3] = newarg3;
					if (arg4) lines[l].args[4] = newarg4;
					++lchanged;
				}
			}
		}
		// Import the changes if needed
		if (lchanged > 0)
		{
			l_entry->importMem(lines, size);
			changed += lchanged;
		}
		delete[] lines;
	}

	if (t_entry)
	{
		size = t_entry->getSize();
		size_t numthings = size / sizeof(hexenthing_t);

		hexenthing_t* things = new hexenthing_t[numthings];
		memcpy(things, t_entry->getData(), size);
		size_t tchanged = 0;

		// Perform replacement
		for (size_t t = 0; t < numthings; ++t)
		{
			if (things[t].type == oldtype)
			{
				if ((!arg0 || things[t].args[0] == oldarg0) &&
						(!arg1 || things[t].args[1] == oldarg1) &&
						(!arg2 || things[t].args[2] == oldarg2) &&
						(!arg3 || things[t].args[3] == oldarg3) &&
						(!arg4 || things[t].args[4] == oldarg4))
				{
					things[t].type = newtype;
					if (arg0) things[t].args[0] = newarg0;
					if (arg1) things[t].args[1] = newarg1;
					if (arg2) things[t].args[2] = newarg2;
					if (arg3) things[t].args[3] = newarg3;
					if (arg4) things[t].args[4] = newarg4;
					++tchanged;
				}
			}
		}
		// Import the changes if needed
		if (tchanged > 0)
		{
			t_entry->importMem(things, size);
			changed += tchanged;
		}
		delete[] things;
	}

	return changed;
}
size_t replaceSpecialsUDMF(ArchiveEntry* entry, int oldtype, int newtype,
						   bool arg0, bool arg1, bool arg2, bool arg3, bool arg4,
						   int oldarg0, int oldarg1, int oldarg2, int oldarg3, int oldarg4,
						   int newarg0, int newarg1, int newarg2, int newarg3, int newarg4)
{
	if (entry == NULL) return 0;

	size_t changed = 0;
	// TODO: parse and replace code
	// Import the changes if needed
	if (changed > 0)
	{
		//entry->importMemChunk(mc);
	}
	return changed;
}
size_t ArchiveOperations::replaceSpecials(Archive* archive, int oldtype, int newtype, bool lines, bool things,
		bool arg0, int oldarg0, int newarg0,
		bool arg1, int oldarg1, int newarg1,
		bool arg2, int oldarg2, int newarg2,
		bool arg3, int oldarg3, int newarg3,
		bool arg4, int oldarg4, int newarg4)
{
	size_t changed = 0;
	// Check archive was given
	if (!archive)
		return changed;

	// Get all maps
	vector<Archive::mapdesc_t> maps = archive->detectMaps();
	string report = "";

	for (size_t a = 0; a < maps.size(); ++a)
	{
		size_t achanged = 0;
		// Is it an embedded wad?
		if (maps[a].archive)
		{
			// Attempt to open entry as wad archive
			Archive* temp_archive = new WadArchive();
			if (temp_archive->open(maps[a].head))
			{
				achanged = ArchiveOperations::replaceSpecials(temp_archive, oldtype, newtype, lines, things,
						   arg0, oldarg0, newarg0, arg1, oldarg1, newarg1, arg2, oldarg2, newarg2, arg3, oldarg3, newarg3, arg4, oldarg4, newarg4);
				MemChunk mc;
				if (!(temp_archive->write(mc, true)))
				{
					achanged = 0;
				}
				else
				{
					temp_archive->close();
					if (!(maps[a].head->importMemChunk(mc)))
					{
						achanged = 0;
					}
				}
			}

			// Cleanup
			delete temp_archive;

		}
		else
		{
			// Find the map entry to modify
			ArchiveEntry* mapentry = maps[a].head;
			ArchiveEntry* t_entry = NULL;
			ArchiveEntry* l_entry = NULL;
			if (maps[a].format == MAP_DOOM || maps[a].format == MAP_DOOM64 || maps[a].format == MAP_HEXEN)
			{
				while (mapentry && mapentry != maps[a].end)
				{
					if (things && mapentry->getType() == EntryType::getType("map_things"))
					{
						t_entry = mapentry;
						if (l_entry || !lines) break;
					}
					if (lines && mapentry->getType() == EntryType::getType("map_linedefs"))
					{
						l_entry = mapentry;
						if (t_entry || !things) break;
					}
					mapentry = mapentry->nextEntry();
				}
			}
			else if (maps[a].format == MAP_UDMF)
			{
				while (mapentry && mapentry != maps[a].end)
				{
					if (mapentry->getType() == EntryType::getType("udmf_textmap"))
					{
						l_entry = t_entry = mapentry;
						break;
					}
					mapentry = mapentry->nextEntry();
				}
			}

			// Did we get a map entry?
			if (l_entry || t_entry)
			{
				switch(maps[a].format)
				{
				case MAP_DOOM:
					if (arg1 || arg2 || arg3 || arg4) // Do nothing if Hexen specials are being modified
						break;
					achanged = replaceSpecialsDoom(l_entry, oldtype, newtype, arg0, oldarg0, newarg0);
					break;
				case MAP_HEXEN:
					if (oldtype > 255 || newtype > 255) // Do nothing if Doom specials are being modified
						break;
					achanged = replaceSpecialsHexen(l_entry, t_entry, oldtype, newtype, arg0, arg1, arg2, arg3, arg4,
													oldarg0, oldarg1, oldarg2, oldarg3, oldarg4, newarg0, newarg1, newarg2, newarg3, newarg4);
					break;
				case MAP_DOOM64:
					if (arg1 || arg2 || arg3 || arg4) // Do nothing if Hexen specials are being modified
						break;
					achanged = replaceSpecialsDoom64(l_entry, oldtype, newtype, arg0, oldarg0, newarg0);
					break;
				case MAP_UDMF:
					achanged = replaceSpecialsUDMF(l_entry, oldtype, newtype, arg0, arg1, arg2, arg3, arg4,
												   oldarg0, oldarg1, oldarg2, oldarg3, oldarg4, newarg0, newarg1, newarg2, newarg3, newarg4);
					break;
				default:
					LOG_MESSAGE(1, "Unknown map format for " + maps[a].head->getName());
					break;
				}
			}
		}
		report += S_FMT("%s:\t%i specials changed\n", maps[a].head->getName(), achanged);
		changed += achanged;
	}
	LOG_MESSAGE(1, report);
	return changed;
}

CONSOLE_COMMAND(replacespecials, 2, true)
{
	Archive* current = MainEditor::currentArchive();
	long oldtype, newtype;
	bool arg0 = false, arg1 = false, arg2 = false, arg3 = false, arg4 = false;
	long oldarg0, oldarg1, oldarg2, oldarg3, oldarg4;
	long newarg0, newarg1, newarg2, newarg3, newarg4;
	size_t fullarg = args.size();
	size_t oldtail = (fullarg / 2) - 1;
	size_t newtail = fullarg - 1;
	bool run = false;

	if (fullarg > 2 && (fullarg % 2 == 0))
	{
		switch (fullarg)
		{
		case 12:
			arg4 = args[oldtail--].ToLong(&oldarg4) && args[newtail--].ToLong(&newarg4);
		case 10:
			arg3 = args[oldtail--].ToLong(&oldarg3) && args[newtail--].ToLong(&newarg3);
		case  8:
			arg2 = args[oldtail--].ToLong(&oldarg2) && args[newtail--].ToLong(&newarg2);
		case  6:
			arg1 = args[oldtail--].ToLong(&oldarg1) && args[newtail--].ToLong(&newarg1);
		case  4:
			arg0 = args[oldtail--].ToLong(&oldarg0) && args[newtail--].ToLong(&newarg0);
		case  2:
			run  = args[oldtail--].ToLong(&oldtype) && args[newtail--].ToLong(&newtype);
			break;
		default:
			LOG_MESSAGE(1, "Invalid number of arguments: %d", fullarg);
		}
	}

	if (current && run)
	{
		ArchiveOperations::replaceSpecials(current, oldtype, newtype, true, true,
										   arg0, oldarg0, newarg0, arg1, oldarg1, newarg1, arg2, oldarg2, newarg2,
										   arg3, oldarg3, newarg3, arg4, oldarg4, newarg4);
	}
}

bool replaceTextureString(char* str, string oldtex, string newtex)
{
	bool go = true;
	for (unsigned c = 0; c < oldtex.Length(); ++c)
	{
		if (str[c] != oldtex[c] && oldtex[c] != '?' && oldtex[c] != '*')
			go = false;
		if (oldtex[c] == '*')
			break;
	}
	if (go)
	{
		for (unsigned i = 0; i < 8; ++i)
		{
			if (i < newtex.Len())
			{
				// Keep the rest of the name as-is?
				if (newtex[i] == '*') break;
				// Keep just this character as-is?
				if (newtex[i] == '?') continue;
				// Else, copy the character
				str[i] = newtex[i];
			}
			else
				str[i] = 0;
		}
	}
	return go;
}
size_t replaceFlatsDoomHexen(ArchiveEntry* entry, string oldtex, string newtex, bool floor, bool ceiling)
{
	if (entry == NULL) return 0;

	size_t size = entry->getSize();
	size_t numsectors = size / sizeof(doomsector_t);
	bool fchanged, cchanged;
	size_t changed = 0;

	doomsector_t* sectors = new doomsector_t[numsectors];
	memcpy(sectors, entry->getData(), size);

	// Perform replacement
	for (size_t s = 0; s < numsectors; ++s)
	{
		fchanged = cchanged = false;
		if (floor)
			fchanged = replaceTextureString(sectors[s].f_tex, oldtex, newtex);
		if (ceiling)
			cchanged = replaceTextureString(sectors[s].c_tex, oldtex, newtex);
		if (fchanged || cchanged)
			++changed;
	}
	// Import the changes if needed
	if (changed > 0)
		entry->importMem(sectors, size);
	delete[] sectors;

	return changed;
}
size_t replaceWallsDoomHexen(ArchiveEntry* entry, string oldtex, string newtex, bool lower, bool middle, bool upper)
{
	if (entry == NULL) return 0;

	size_t size = entry->getSize();
	size_t numsides = size / sizeof(doomside_t);
	bool lchanged, mchanged, uchanged;
	size_t changed = 0;

	doomside_t* sides = new doomside_t[numsides];
	memcpy(sides, entry->getData(), size);
	char compare[9]; compare[8] = 0;

	// Perform replacement
	for (size_t s = 0; s < numsides; ++s)
	{
		lchanged = mchanged = uchanged = false;
		if (lower)
			lchanged = replaceTextureString(sides[s].tex_lower, oldtex, newtex);
		if (middle)
			mchanged = replaceTextureString(sides[s].tex_middle, oldtex, newtex);
		if (upper)
			uchanged = replaceTextureString(sides[s].tex_upper, oldtex, newtex);
		if (lchanged || mchanged || uchanged)
			++changed;
	}
	// Import the changes if needed
	if (changed > 0)
		entry->importMem(sides, size);
	delete[] sides;

	return changed;
}
size_t replaceFlatsDoom64(ArchiveEntry* entry, string oldtex, string newtex, bool floor, bool ceiling)
{
	if (entry == NULL) return 0;

	size_t size = entry->getSize();
	size_t numsectors = size / sizeof(doom64sector_t);
	bool fchanged, cchanged;
	size_t changed = 0;

	uint16_t oldhash = theResourceManager->getTextureHash(oldtex);
	uint16_t newhash = theResourceManager->getTextureHash(newtex);

	doom64sector_t* sectors = new doom64sector_t[numsectors];
	memcpy(sectors, entry->getData(), size);

	// Perform replacement
	for (size_t s = 0; s < numsectors; ++s)
	{
		fchanged = cchanged = false;
		if (floor && oldhash == sectors[s].f_tex)
		{
			sectors[s].f_tex = newhash;
			fchanged = true;
		}
		if (ceiling && oldhash == sectors[s].c_tex)
		{
			sectors[s].c_tex = newhash;
			cchanged = true;
		}
		if (fchanged || cchanged)
			++changed;
	}
	// Import the changes if needed
	if (changed > 0)
		entry->importMem(sectors, size);
	delete[] sectors;

	return changed;
}
size_t replaceWallsDoom64(ArchiveEntry* entry, string oldtex, string newtex, bool lower, bool middle, bool upper)
{
	if (entry == NULL) return 0;

	size_t size = entry->getSize();
	size_t numsides = size / sizeof(doom64side_t);
	bool lchanged, mchanged, uchanged;
	size_t changed = 0;

	uint16_t oldhash = theResourceManager->getTextureHash(oldtex);
	uint16_t newhash = theResourceManager->getTextureHash(newtex);

	doom64side_t* sides = new doom64side_t[numsides];
	memcpy(sides, entry->getData(), size);

	// Perform replacement
	for (size_t s = 0; s < numsides; ++s)
	{
		lchanged = mchanged = uchanged = false;
		if (lower && oldhash == sides[s].tex_lower)
		{
			sides[s].tex_lower = newhash;
			lchanged = true;
		}
		if (middle && oldhash == sides[s].tex_middle)
		{
			sides[s].tex_middle = newhash;
			mchanged = true;
		}
		if (upper && oldhash == sides[s].tex_upper)
		{
			sides[s].tex_upper = newhash;
			uchanged = true;
		}
		if (lchanged || mchanged || uchanged)
			++changed;
	}
	// Import the changes if needed
	if (changed > 0)
		entry->importMem(sides, size);
	delete[] sides;

	return changed;
}
size_t replaceTexturesUDMF(ArchiveEntry* entry, string oldtex, string newtex, bool floor, bool ceiling, bool lower, bool middle, bool upper)
{
	if (entry == NULL) return 0;

	size_t changed = 0;
	// TODO: parse and replace code
	// Import the changes if needed
	if (changed > 0)
	{
		//entry->importMemChunk(mc);
	}
	return changed;
}
size_t ArchiveOperations::replaceTextures(Archive* archive, string oldtex, string newtex, bool floor, bool ceiling, bool lower, bool middle, bool upper)
{
	size_t changed = 0;
	// Check archive was given
	if (!archive)
		return changed;

	// Get all maps
	vector<Archive::mapdesc_t> maps = archive->detectMaps();
	string report = "";

	for (size_t a = 0; a < maps.size(); ++a)
	{
		size_t achanged = 0;
		// Is it an embedded wad?
		if (maps[a].archive)
		{
			// Attempt to open entry as wad archive
			Archive* temp_archive = new WadArchive();
			if (temp_archive->open(maps[a].head))
			{
				achanged = ArchiveOperations::replaceTextures(temp_archive, oldtex, newtex, floor, ceiling, lower, middle, upper);
				MemChunk mc;
				if (!(temp_archive->write(mc, true)))
				{
					achanged = 0;
				}
				else
				{
					temp_archive->close();
					if (!(maps[a].head->importMemChunk(mc)))
					{
						achanged = 0;
					}
				}
			}

			// Cleanup
			delete temp_archive;

		}
		else
		{
			// Find the map entry to modify
			ArchiveEntry* mapentry = maps[a].head;
			ArchiveEntry* sectors = NULL;
			ArchiveEntry* sides = NULL;
			if (maps[a].format == MAP_DOOM || maps[a].format == MAP_DOOM64 || maps[a].format == MAP_HEXEN)
			{
				while (mapentry && mapentry != maps[a].end)
				{
					if ((floor || ceiling) && (mapentry->getType() == EntryType::getType("map_sectors")))
					{
						sectors = mapentry;
						if (sides || !(lower || middle || upper)) break;
					}
					if ((lower || middle || upper) && (mapentry->getType() == EntryType::getType("map_sidedefs")))
					{
						sides = mapentry;
						if (sectors || !(floor || ceiling)) break;
					}
					mapentry = mapentry->nextEntry();
				}
			}
			else if (maps[a].format == MAP_UDMF)
			{
				while (mapentry && mapentry != maps[a].end)
				{
					if (mapentry->getType() == EntryType::getType("udmf_textmap"))
					{
						sectors = sides = mapentry;
						break;
					}
					mapentry = mapentry->nextEntry();
				}
			}

			// Did we get a map entry?
			if (sectors || sides)
			{
				switch(maps[a].format)
				{
				case MAP_DOOM:
				case MAP_HEXEN:
					achanged = 0;
					if (sectors)
						achanged += replaceFlatsDoomHexen(sectors, oldtex, newtex, floor, ceiling);
					if (sides)
						achanged += replaceWallsDoomHexen(sides, oldtex, newtex, lower, middle, upper);
					break;
				case MAP_DOOM64:
					achanged = 0;
					if (sectors)
						achanged += replaceFlatsDoom64(sectors, oldtex, newtex, floor, ceiling);
					if (sides)
						achanged += replaceWallsDoom64(sides, oldtex, newtex, lower, middle, upper);
					break;
				case MAP_UDMF:
					achanged = replaceTexturesUDMF(sectors, oldtex, newtex, floor, ceiling, lower, middle, upper);
					break;
				default:
					LOG_MESSAGE(1, "Unknown map format for " + maps[a].head->getName());
					break;
				}
			}
		}
		report += S_FMT("%s:\t%i elements changed\n", maps[a].head->getName(), achanged);
		changed += achanged;
	}
	LOG_MESSAGE(1, report);
	return changed;
}

CONSOLE_COMMAND(replacetextures, 2, true)
{
	Archive* current = MainEditor::currentArchive();

	if (current)
	{
		ArchiveOperations::replaceTextures(current, args[0], args[1], true, true, true, true, true);
	}
}

