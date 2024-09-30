
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveOperations.cpp
// Description: Functions that perform specific operations on archives
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
#include "ArchiveOperations.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryType.h"
#include "Archive/Formats/DirArchiveHandler.h"
#include "Archive/MapDesc.h"
#include "General/Console.h"
#include "General/ResourceManager.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "SLADEMap/MapFormat/Doom32XMapFormat.h"
#include "SLADEMap/MapFormat/Doom64MapFormat.h"
#include "SLADEMap/MapFormat/DoomMapFormat.h"
#include "SLADEMap/MapFormat/HexenMapFormat.h"
#include "UI/Dialogs/ExtMessageDialog.h"
#include "UI/UI.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
typedef std::map<wxString, int>                   StrIntMap;
typedef std::map<wxString, vector<ArchiveEntry*>> PathMap;
typedef std::map<int, vector<ArchiveEntry*>>      CRCMap;


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_dir_ignore_hidden)


// -----------------------------------------------------------------------------
//
// ArchiveOperations Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Saves [archive] to disk, returns true on success
// -----------------------------------------------------------------------------
bool archiveoperations::save(Archive& archive)
{
	if (!archive.canSave())
		return false;

	// Check if the file has been modified on disk
	if (archive.format() != ArchiveFormat::Dir
		&& fileutil::fileModifiedTime(archive.filename()) > archive.fileModifiedTime())
	{
		if (wxMessageBox(
				wxString::Format(
					"The file %s has been modified on disk since the archive was last saved, are you sure you want "
					"to continue with saving?",
					archive.filename(false)),
				"File Modified",
				wxICON_WARNING | wxYES_NO)
			== wxNO)
			return false;
	}

	// Save the archive if possible
	auto time = wxDateTime::GetTimeNow();
	if (!archive.save())
	{
		// If there was an error pop up a message box
		wxMessageBox(wxString::Format("Error: %s", global::error), "Error", wxICON_ERROR);
		return false;
	}

	// Check if there were issues saving directory
	if (archive.format() == ArchiveFormat::Dir)
	{
		auto* dir_archive = dynamic_cast<DirArchiveHandler*>(&archive.formatHandler());
		if (dir_archive->saveErrorsOccurred())
		{
			auto   messages = log::since(time);
			string msg_log_str;
			for (const auto* msg : messages)
				msg_log_str += msg->formattedMessageLine() + "\n";

			ExtMessageDialog dlg(maineditor::windowWx(), "Directory Save Issues");
			dlg.CenterOnParent();
			dlg.setMessage("Some issues occurred while saving changes to the filesystem, see details below:");
			dlg.setExt(msg_log_str);
			dlg.ShowModal();
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Saves [archive] to disk under a different filename, opens a file dialog to
// select the new name/path.
// Returns false on error or if the dialog was cancelled, true otherwise
// -----------------------------------------------------------------------------
bool archiveoperations::saveAs(Archive& archive)
{
	// Popup file save dialog
	if (auto filename = filedialog::saveFile(
			"Save Archive " + archive.filename(false) + " As", archive.fileExtensionString(), maineditor::windowWx());
		!filename.empty())
	{
		// Save the archive
		if (!archive.save(filename))
		{
			// If there was an error pop up a message box
			wxMessageBox(wxString::Format("Error: %s", global::error), "Error", wxICON_ERROR);
			return false;
		}

		// Add recent file
		app::archiveManager().addRecentFile(filename);

		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Build pk3/zip archive from the directory [path]
// -----------------------------------------------------------------------------
bool archiveoperations::buildArchive(string_view path)
{
	// Create temporary archive
	Archive zip(ArchiveFormat::Zip);

	// Create dialog
	filedialog::FDInfo info;
	if (filedialog::saveFile(info, "Build Archive", zip.fileExtensionString(), maineditor::windowWx()))
	{
		ui::showSplash("Building " + info.filenames[0], true);
		ui::setSplashProgress(0.0f);

		// prevent for "archive in archive" when saving in the current directory
		if (wxFileExists(info.filenames[0]))
			wxRemoveFile(info.filenames[0]);

		// Log
		ui::setSplashProgressMessage("Importing files...");
		ui::setSplashProgress(-1.0f);

		// Import all files into new archive
		zip.importDir(path, archive_dir_ignore_hidden);
		ui::setSplashProgress(1.0f);
		ui::setSplashMessage("Saving archive...");
		ui::setSplashProgressMessage("");

		// Save the archive
		if (!zip.save(info.filenames[0]))
		{
			ui::hideSplash();

			// If there was an error pop up a message box
			wxMessageBox(wxString::Format("Error:\n%s", global::error), "Error", wxICON_ERROR);
			return false;
		}
	}

	ui::hideSplash();

	return true;
}

// -----------------------------------------------------------------------------
// Removes any patches and associated entries from [archive] that are not used
// in any texture definitions
// -----------------------------------------------------------------------------
bool archiveoperations::removeUnusedPatches(Archive* archive)
{
	if (!archive)
		return false;

	// Find PNAMES entry
	ArchiveSearchOptions opt;
	opt.match_type = EntryType::fromId("pnames");
	auto pnames    = archive->findLast(opt);

	// Find TEXTUREx entries
	opt.match_type  = EntryType::fromId("texturex");
	auto tx_entries = archive->findAll(opt);

	// Can't do anything without PNAMES/TEXTUREx
	if (!pnames || tx_entries.empty())
		return false;

	// Open patch table
	PatchTable ptable;
	ptable.loadPNAMES(pnames, archive);

	// Open texturex entries to update patch usage
	vector<TextureXList*> tx_lists;
	for (auto& entry : tx_entries)
	{
		auto texturex = new TextureXList();
		texturex->readTEXTUREXData(entry, ptable);
		for (unsigned t = 0; t < texturex->size(); t++)
			ptable.updatePatchUsage(texturex->texture(t));
		tx_lists.push_back(texturex);
	}

	// Go through patch table
	unsigned              removed = 0;
	vector<ArchiveEntry*> to_remove;
	for (unsigned a = 0; a < ptable.nPatches(); a++)
	{
		auto& p = ptable.patch(a);

		// Check if used in any texture
		if (p.used_in.empty())
		{
			// Unused

			// If its entry is in the archive, flag it to be removed
			auto entry = app::resources().getPatchEntry(p.name, "patches", archive);
			if (entry && entry->parent() == archive)
				to_remove.push_back(entry);

			// Update texturex list patch indices
			for (auto& tx_list : tx_lists)
				tx_list->removePatch(p.name);

			// Remove the patch from the patch table
			log::info(wxString::Format("Removed patch %s", p.name));
			removed++;
			ptable.removePatch(a--);
		}
	}

	// Remove unused patch entries
	for (auto& a : to_remove)
	{
		log::info(wxString::Format("Removed entry %s", a->name()));
		archive->removeEntry(a);
	}

	// Write PNAMES changes
	ptable.writePNAMES(pnames);

	// Write TEXTUREx changes
	for (unsigned a = 0; a < tx_lists.size(); a++)
		tx_lists[a]->writeTEXTUREXData(tx_entries[a], ptable);

	// Cleanup
	for (auto& tx_list : tx_lists)
		delete tx_list;

	// Notify user
	wxMessageBox(
		wxString::Format("Removed %d patches and %lu entries. See console log for details.", removed, to_remove.size()),
		"Removed Unused Patches",
		wxOK | wxICON_INFORMATION);

	return true;
}

// -----------------------------------------------------------------------------
// Checks [archive] for multiple entries of the same name, and displays a list
// of duplicate entry names if any are found
// -----------------------------------------------------------------------------
bool archiveoperations::checkDuplicateEntryNames(Archive* archive)
{
	StrIntMap map_namecounts;
	PathMap   map_entries;

	// Get list of all entries in archive
	vector<ArchiveEntry*> entries;
	archive->putEntryTreeAsList(entries);

	// Go through list
	for (auto& entry : entries)
	{
		// Skip directory entries
		if (entry->type() == EntryType::folderType())
			continue;

		// Increment count for entry name
		map_namecounts[entry->path(true)] += 1;

		// Enqueue entries
		map_entries[string{ entry->nameNoExt() }].push_back(entry);
	}

	// Generate string of duplicate entry names
	wxString dups;
	// Treeless archives such as WADs can just include a simple list of duplicated names and how often they appear
	if (archive->isTreeless())
	{
		auto i = map_namecounts.begin();
		while (i != map_namecounts.end())
		{
			if (i->second > 1)
			{
				wxString name = i->first;
				name.Remove(0, 1);
				dups += wxString::Format("%s appears %d times\n", name, i->second);
			}
			++i;
		}
		// Hierarchized archives, however, need to compare only the name (not the whole path) and to display the full
		// path of each entry with a duplicated name, so that they might be found more easily than by having the user
		// recurse through the entire directory tree -- such a task is something a program should do instead.
	}
	else
	{
		auto i = map_entries.begin();
		while (i != map_entries.end())
		{
			if (i->second.size() > 1)
			{
				wxString name;
				dups += wxString::Format("\n%i entries are named %s\t", i->second.size(), i->first);
				auto j = i->second.begin();
				while (j != i->second.end())
				{
					name = (*j)->path(true);
					name.Remove(0, 1);
					dups += wxString::Format("\t%s", name);
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

// -----------------------------------------------------------------------------
// Compare the archive's entries with those sharing the same name and namespace
// in the base resource archive, deleting duplicates
// -----------------------------------------------------------------------------
void archiveoperations::removeEntriesUnchangedFromIWAD(Archive* archive)
{
	// Do nothing if there is no base resource archive,
	// or if the archive *is* the base resource archive.
	auto bra = app::archiveManager().baseResourceArchive();
	if (bra == nullptr || bra == archive || archive == nullptr)
		return;

	// Get list of all entries in archive
	vector<ArchiveEntry*> entries;
	archive->putEntryTreeAsList(entries);

	// Init search options
	ArchiveSearchOptions search;
	ArchiveEntry*        other = nullptr;
	wxString             dups  = "";
	size_t               count = 0;

	// Go through list
	for (auto& entry : entries)
	{
		// Skip directory entries
		if (entry->type() == EntryType::folderType())
			continue;

		// Skip markers
		if (entry->type() == EntryType::mapMarkerType() || entry->size() == 0)
			continue;

		// Now, let's look for a counterpart in the IWAD
		search.match_namespace = archive->detectNamespace(entry);
		search.match_name      = entry->name();
		other                  = bra->findLast(search);

		// If there is one, and it is identical, remove it
		if (other != nullptr && (other->data().crc() == entry->data().crc()))
		{
			++count;
			dups += wxString::Format("%s\n", search.match_name);
			archive->removeEntry(entry);
			entry = nullptr;
		}
	}


	// If no duplicates exist, do nothing
	if (count == 0)
	{
		wxMessageBox("No duplicated entries exist");
		return;
	}

	wxString message = wxString::Format(
		"The following %ld entr%s duplicated from the base resource archive and deleted:",
		count,
		(count > 1) ? "ies were" : "y was");

	// Display list of deleted duplicate entries
	ExtMessageDialog msg(theMainWindow, (count > 1) ? "Deleted Entries" : "Deleted Entry");
	msg.setExt(dups);
	msg.setMessage(message);
	msg.ShowModal();
}

// -----------------------------------------------------------------------------
// Checks entries of the same name from the base resource archive. Also checks
// texture definitions. This helps know what your archive overrides.
// -----------------------------------------------------------------------------
bool archiveoperations::checkOverriddenEntriesInIWAD(Archive* archive)
{
	// Do nothing if there is no base resource archive,
	// or if the archive *is* the base resource archive.
	auto bra = app::archiveManager().baseResourceArchive();
	if (bra == nullptr || bra == archive || archive == nullptr)
		return false;

	// Get list of all entries in archive
	vector<ArchiveEntry*> entries;
	archive->putEntryTreeAsList(entries);

	// Init search options
	ArchiveSearchOptions search;
	ArchiveEntry*        other     = nullptr;
	wxString             overrides = "";
	size_t               count     = 0;

	// Go through list
	for (auto& entry : entries)
	{
		// Skip directory entries
		if (entry->type() == EntryType::folderType())
			continue;

		// Skip markers
		if (entry->type() == EntryType::mapMarkerType() || entry->size() == 0)
			continue;

		// Now, let's look for a counterpart in the IWAD
		search.match_namespace = archive->detectNamespace(entry);
		search.match_name      = entry->name();
		other                  = bra->findLast(search);

		// If there is one list it
		if (other != nullptr)
		{
			++count;
			overrides += wxString::Format("%s: %s\n", search.match_namespace, search.match_name);
		}
	}

	// If no overrides exist, do nothing
	if (count == 0)
	{
		wxMessageBox("No overridden entries exist");
	}

	wxString message = wxString::Format(
		"The following %ld entr%s overridden from the base resource archive:",
		count,
		(count > 1) ? "ies were" : "y was");

	// Display list of duplicate entries
	ExtMessageDialog msg(theMainWindow, (count > 1) ? "Overridden Entries" : "Deleted Entry");
	msg.setExt(overrides);
	msg.setMessage(message);
	msg.ShowModal();




	// Find all texture entries
	std::unordered_map<ArchiveEntry*, TextureXList>                          braTextureEntries;
	std::unordered_multimap<string, std::pair<ArchiveEntry*, ArchiveEntry*>> duplicate_texture_entries;
	std::set<string>                                                         found_duplicate_textures;

	ArchiveSearchOptions pnamesopt;
	pnamesopt.match_type = EntryType::fromId("pnames");

	ArchiveSearchOptions texturexopt;
	texturexopt.match_type = EntryType::fromId("texturex");

	ArchiveSearchOptions zdtexturesopt;
	zdtexturesopt.match_type = EntryType::fromId("zdtextures");


	auto bra_pnames = bra->findLast(pnamesopt);

	// Load BRA patch table and BRA textures
	PatchTable bra_ptable;
	if (bra_pnames)
	{
		bra_ptable.loadPNAMES(bra_pnames);

		// Load all BRA Texturex entries
		for (ArchiveEntry* texturexentry : bra->findAll(texturexopt))
		{
			braTextureEntries[texturexentry].readTEXTUREXData(texturexentry, bra_ptable);
		}
	}

	// Load BRA Zdoom Textures
	for (ArchiveEntry* texturesentry : bra->findAll(zdtexturesopt))
	{
		braTextureEntries[texturesentry].readTEXTURESData(texturesentry);
	}

	// If we ended up not loading textures from base resource archive
	if (braTextureEntries.empty())
	{
		log::error("Base resource archive has no texture entries to compare against");
		return true;
	}

	// Find patch table in archive
	auto pnames = archive->findLast(pnamesopt);

	// Load patch table if we have it
	PatchTable ptable;
	if (pnames)
		ptable.loadPNAMES(pnames);

	auto processTextureList = [&found_duplicate_textures, &duplicate_texture_entries, &braTextureEntries](
								  ArchiveEntry* textureEntry, TextureXList& textureList)
	{
		for (unsigned a = 0; a < textureList.textures().size(); a++)
		{
			CTexture* this_texture = textureList.texture(a);

			for (auto const& iter : braTextureEntries)
			{
				int other_texture_index = iter.second.textureIndex(this_texture->name());

				if (other_texture_index >= 0)
				{
					// Other texture with this name found
					log::info(wxString::Format("Found Overridden Texture: %s.", this_texture->name()));
					found_duplicate_textures.insert(this_texture->name());
					duplicate_texture_entries.emplace(this_texture->name(), std::make_pair(textureEntry, iter.first));
				}
			}
		}
	};

	// Load textures
	for (ArchiveEntry* textureEntry : archive->findAll(texturexopt))
	{
		TextureXList textureList;
		textureList.readTEXTUREXData(textureEntry, ptable);

		processTextureList(textureEntry, textureList);
	}

	for (ArchiveEntry* textureEntry : archive->findAll(zdtexturesopt))
	{
		TextureXList textureList;
		textureList.readTEXTURESData(textureEntry);

		processTextureList(textureEntry, textureList);
	}

	if (!found_duplicate_textures.empty())
	{
		wxString dups = "";

		for (const string& duplicate_entry : found_duplicate_textures)
		{
			dups += wxString::Format("\n%s", duplicate_entry);

			auto duplicated_entries_range = duplicate_texture_entries.equal_range(duplicate_entry);

			for (auto entry_iter = duplicated_entries_range.first; entry_iter != duplicated_entries_range.second;
				 ++entry_iter)
			{
				ArchiveEntry* duplicated_entry = entry_iter->second.first;
				ArchiveEntry* bra_entry        = entry_iter->second.second;
				dups += wxString::Format(
					"\n\tThis Archive Asset Path: %s%s", duplicated_entry->path(), duplicated_entry->name());
				dups += wxString::Format("\n\tIwad Asset Path: %s%s", bra_entry->path(), bra_entry->name());
			}
		}

		// Display list of duplicate entry names
		ExtMessageDialog msg_dialog(theMainWindow, "Overridden Texture Entries");
		msg_dialog.setExt(dups);
		msg_dialog.setMessage("The following textures are overridden:");
		msg_dialog.ShowModal();
	}
	else
	{
		wxMessageBox(
			"Didn't find any textures overridden from the iwad",
			"Overridden Texture Entries",
			wxOK | wxCENTER | wxICON_INFORMATION);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Checks entries of the same name from the base resource archive. Also checks
// texture definitions. This helps know what your archive overrides.
// This is a ZDoom version that has additional behavior for checking across
// flats, patches, and other assets.
// -----------------------------------------------------------------------------
bool archiveoperations::checkZDoomOverriddenEntriesInIWAD(Archive* archive)
{
	// Do nothing if there is no base resource archive,
	// or if the archive *is* the base resource archive.
	auto bra = app::archiveManager().baseResourceArchive();
	if (bra == nullptr || bra == archive || archive == nullptr)
		return false;

	std::unordered_multimap<string, ArchiveEntry*> archiveTexEntries;
	std::unordered_multimap<string, ArchiveEntry*> overiddenBraTexEntries;
	std::set<string>                               overiddenBraTexNames;

	// Load BRA pnames
	ArchiveSearchOptions pnames_opt;
	pnames_opt.match_type = EntryType::fromId("pnames");

	auto braPnames = bra->findLast(pnames_opt);

	auto process_entries = [&archiveTexEntries](const vector<ArchiveEntry*>& archive_entries)
	{
		for (auto& archive_entry : archive_entries)
		{
			// Skip markers
			if (archive_entry->size() == 0)
				continue;

			string entry_name{ archive_entry->upperNameNoExt() };

			archiveTexEntries.emplace(entry_name, archive_entry);
		}
	};

	auto process_patch_table = [&archiveTexEntries](ArchiveEntry* pnames_entry, const PatchTable& patch_table)
	{
		for (int patchIndex = 0; patchIndex < patch_table.nPatches(); ++patchIndex)
		{
			string patch_name = string(patch_table.patchName(patchIndex));
			patch_name        = strutil::upperIP(patch_name);

			archiveTexEntries.emplace(patch_name, pnames_entry);
		}
	};

	auto process_texture_list = [&archiveTexEntries](ArchiveEntry* texture_archive_entry, TextureXList& texture_list)
	{
		for (unsigned texture_index = 0; texture_index < texture_list.size(); texture_index++)
		{
			// Skip markers
			if (texture_archive_entry->size() == 0)
				continue;

			auto texture = texture_list.texture(texture_index);

			string texture_name = string(texture->name());
			texture_name        = strutil::upperIP(texture_name);

			archiveTexEntries.emplace(texture_name, texture_archive_entry);
		}
	};

	// Find all textures
	{
		ArchiveSearchOptions search_opt;
		search_opt.match_namespace = "textures";
		process_entries(archive->findAll(search_opt));
	}

	// Find all flats
	{
		ArchiveSearchOptions search_opt;
		search_opt.match_namespace = "flats";
		process_entries(archive->findAll(search_opt));
	}

	auto pnames = archive->findLast(pnames_opt);

	// Load patch table
	if (pnames)
	{
		PatchTable ptable;
		ptable.loadPNAMES(pnames);

		// Don't process the patch table if we loaded it from the iWad
		if (pnames != braPnames)
		{
			process_patch_table(pnames, ptable);
		}

		// Load all Texturex entries
		ArchiveSearchOptions texturexopt;
		texturexopt.match_type = EntryType::fromId("texturex");

		for (ArchiveEntry* texturexentry : archive->findAll(texturexopt))
		{
			TextureXList texture_list;
			texture_list.readTEXTUREXData(texturexentry, ptable, true);

			process_texture_list(texturexentry, texture_list);
		}
	}

	// Load all zdtextures entries
	{
		ArchiveSearchOptions zdtexturesopt;
		zdtexturesopt.match_type = EntryType::fromId("zdtextures");

		for (ArchiveEntry* texturesentry : archive->findAll(zdtexturesopt))
		{
			TextureXList texture_list;
			texture_list.readTEXTURESData(texturesentry);

			process_texture_list(texturesentry, texture_list);
		}
	}

	auto process_bra_entries = [&archiveTexEntries, &overiddenBraTexEntries, &overiddenBraTexNames, &braPnames](
								   const vector<ArchiveEntry*>& archive_entries)
	{
		for (auto& archive_entry : archive_entries)
		{
			// Skip markers
			if (archive_entry->size() == 0)
				continue;

			string entry_name{ archive_entry->upperNameNoExt() };

			auto iter = archiveTexEntries.find(entry_name);

			// If the pnames ptr is the same, we loaded pnames from the bra, so don't mark it as overridden
			if (iter != archiveTexEntries.end() && iter->second != braPnames)
			{
				overiddenBraTexEntries.emplace(entry_name, archive_entry);
				overiddenBraTexNames.insert(entry_name);
			}
		}
	};

	auto process_bra_patch_table = [&archiveTexEntries, &overiddenBraTexEntries, &overiddenBraTexNames](
									   ArchiveEntry* pnames_entry, const PatchTable& patch_table)
	{
		for (int patchIndex = 0; patchIndex < patch_table.nPatches(); ++patchIndex)
		{
			string patch_name = string(patch_table.patchName(patchIndex));
			patch_name        = strutil::upperIP(patch_name);

			archiveTexEntries.emplace(patch_name, pnames_entry);

			auto iter = archiveTexEntries.find(patch_name);

			// If the pnames ptr is the same, we loaded pnames from the bra, so don't mark it as overridden
			if (iter != archiveTexEntries.end() && iter->second != pnames_entry)
			{
				overiddenBraTexEntries.emplace(patch_name, pnames_entry);
				overiddenBraTexNames.insert(patch_name);
			}
		}
	};

	auto process_bra_texture_list = [&archiveTexEntries, &overiddenBraTexEntries, &overiddenBraTexNames, &braPnames](
										ArchiveEntry* texture_archive_entry, TextureXList& texture_list)
	{
		for (unsigned texture_index = 0; texture_index < texture_list.size(); texture_index++)
		{
			// Skip markers
			if (texture_archive_entry->size() == 0)
				continue;

			auto texture = texture_list.texture(texture_index);

			string texture_name = string(texture->name());
			texture_name        = strutil::upperIP(texture_name);

			auto iter = archiveTexEntries.find(texture_name);

			// If the duplicate is the bra pnames, don't mark it as overridden
			if (iter != archiveTexEntries.end() && iter->second != braPnames)
			{
				overiddenBraTexEntries.emplace(texture_name, texture_archive_entry);
				overiddenBraTexNames.insert(texture_name);
			}
		}
	};

	// Find all textures
	{
		ArchiveSearchOptions search_opt;
		search_opt.match_namespace = "textures";
		process_bra_entries(bra->findAll(search_opt));
	}

	// Find all flats
	{
		ArchiveSearchOptions search_opt;
		search_opt.match_namespace = "flats";
		process_bra_entries(bra->findAll(search_opt));
	}

	// Load patch table
	if (braPnames)
	{
		PatchTable ptable;
		ptable.loadPNAMES(braPnames);

		process_bra_patch_table(braPnames, ptable);

		// Load all Texturex entries
		ArchiveSearchOptions texturexopt;
		texturexopt.match_type = EntryType::fromId("texturex");

		for (ArchiveEntry* texturexentry : bra->findAll(texturexopt))
		{
			TextureXList texture_list;
			texture_list.readTEXTUREXData(texturexentry, ptable, true);

			process_bra_texture_list(texturexentry, texture_list);
		}
	}

	// Load all zdtextures entries
	{
		ArchiveSearchOptions zdtexturesopt;
		zdtexturesopt.match_type = EntryType::fromId("zdtextures");

		for (ArchiveEntry* texturesentry : bra->findAll(zdtexturesopt))
		{
			TextureXList texture_list;
			texture_list.readTEXTURESData(texturesentry);

			process_bra_texture_list(texturesentry, texture_list);
		}
	}

	if (overiddenBraTexEntries.empty())
	{
		wxMessageBox("No overridden textures exist");
		return false;
	}

	wxString dups = "";

	for (const string& overriddenTexName : overiddenBraTexNames)
	{
		dups += wxString::Format("\n%s", overriddenTexName);

		{
			auto entries_range = archiveTexEntries.equal_range(overriddenTexName);

			for (auto entry_iter = entries_range.first; entry_iter != entries_range.second; ++entry_iter)
			{
				ArchiveEntry* entry = entry_iter->second;

				// skip BRA pnames, we don't want to print that as if it's our archive's asset
				if (entry == braPnames)
				{
					continue;
				}

				dups += wxString::Format("\n\tThis Archive Asset Path: %s%s", entry->path(), entry->name());
			}
		}

		{
			auto entries_range = overiddenBraTexEntries.equal_range(overriddenTexName);

			for (auto entry_iter = entries_range.first; entry_iter != entries_range.second; ++entry_iter)
			{
				ArchiveEntry* entry = entry_iter->second;
				dups += wxString::Format("\n\tIwad Asset Path: %s%s", entry->path(), entry->name());
			}
		}
	}

	// Display list of duplicate entry names
	ExtMessageDialog msg(theMainWindow, "iWad Overridden Entries");
	msg.setExt(dups);
	msg.setMessage("The following entry data is overridden from the iWad:");
	msg.ShowModal();

	return true;
}

// -----------------------------------------------------------------------------
// Checks [archive] for multiple entries with the same data, and displays a list
// of the duplicate entries' names if any are found
// -----------------------------------------------------------------------------
bool archiveoperations::checkDuplicateEntryContent(const Archive* archive)
{
	CRCMap map_entries;

	// Get list of all entries in archive
	vector<ArchiveEntry*> entries;
	archive->putEntryTreeAsList(entries);
	wxString dups = "";

	// Go through list
	for (auto& entry : entries)
	{
		// Skip directory entries
		if (entry->type() == EntryType::folderType())
			continue;

		// Skip markers
		if (entry->type() == EntryType::mapMarkerType() || entry->size() == 0)
			continue;

		// Enqueue entries
		map_entries[entry->data().crc()].push_back(entry);
	}

	// Now iterate through the dupes to list the name of the duplicated entries
	auto i = map_entries.begin();
	while (i != map_entries.end())
	{
		if (i->second.size() > 1)
		{
			wxString name = i->second[0]->path(true);
			name.Remove(0, 1);
			dups += wxString::Format("\n%s\t(%8x) duplicated by", name, i->first);
			auto j = i->second.begin() + 1;
			while (j != i->second.end())
			{
				name = (*j)->path(true);
				name.Remove(0, 1);
				dups += wxString::Format("\t%s", name);
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
int      n_tex_anim       = 13;
wxString tex_anim_start[] = {
	"BLODGR1",  "SLADRIP1", "BLODRIP1", "FIREWALA", "GSTFONT1", "FIRELAV3", "FIREMAG1",
	"FIREBLU1", "ROCKRED1", "BFALL1",   "SFALL1",   "WFALL1",   "DBRAIN1",
};
wxString tex_anim_end[] = {
	"BLODGR4",  "SLADRIP3", "BLODRIP4", "FIREWALL", "GSTFONT3", "FIRELAVA", "FIREMAG3",
	"FIREBLU2", "ROCKRED3", "BFALL4",   "SFALL4",   "WFALL4",   "DBRAIN4",
};

int      n_flat_anim       = 9;
wxString flat_anim_start[] = {
	"NUKAGE1", "FWATER1", "SWATER1", "LAVA1", "BLOOD1", "RROCK05", "SLIME01", "SLIME05", "SLIME09",
};
wxString flat_anim_end[] = {
	"NUKAGE3", "FWATER4", "SWATER4", "LAVA4", "BLOOD3", "RROCK08", "SLIME04", "SLIME08", "SLIME12",
};

struct TexUsed
{
	bool used;
	TexUsed() : used(false) {}
};
WX_DECLARE_STRING_HASH_MAP(TexUsed, TexUsedMap);
void archiveoperations::removeUnusedTextures(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// --- Build list of used textures ---
	TexUsedMap used_textures;
	int        total_maps = 0;

	// Get all SIDEDEFS entries
	ArchiveSearchOptions opt;
	opt.match_type = EntryType::fromId("map_sidedefs");
	auto sidedefs  = archive->findAll(opt);
	total_maps += sidedefs.size();

	// Go through and add used textures to list
	DoomMapFormat::SideDef sdef;
	wxString               tex_lower, tex_middle, tex_upper;
	for (auto& sidedef : sidedefs)
	{
		int nsides = sidedef->size() / 30;
		sidedef->seek(0, SEEK_SET);
		for (int s = 0; s < nsides; s++)
		{
			// Read side data
			sidedef->read(&sdef, 30);

			// Get textures
			tex_lower  = wxString::FromAscii(sdef.tex_lower, 8);
			tex_middle = wxString::FromAscii(sdef.tex_middle, 8);
			tex_upper  = wxString::FromAscii(sdef.tex_upper, 8);

			// Add to used textures list
			used_textures[tex_lower].used  = true;
			used_textures[tex_middle].used = true;
			used_textures[tex_upper].used  = true;
		}
	}

	// Get all TEXTMAP entries
	opt.match_name = "TEXTMAP";
	opt.match_type = EntryType::fromId("udmf_textmap");
	auto udmfmaps  = archive->findAll(opt);
	total_maps += udmfmaps.size();

	// Go through and add used textures to list
	Tokenizer tz;
	tz.setSpecialCharacters("{};=");
	for (auto& udmfmap : udmfmaps)
	{
		// Open in tokenizer
		tz.openMem(udmfmap->data(), "UDMF TEXTMAP");

		// Go through text tokens
		wxString token = tz.getToken();
		while (!token.IsEmpty())
		{
			// Check for sidedef definition
			if (token == "sidedef")
			{
				tz.getToken(); // Skip {

				token = tz.getToken();
				while (token != "}")
				{
					// Check for texture property
					if (token == "texturetop" || token == "texturemiddle" || token == "texturebottom")
					{
						tz.getToken(); // Skip =
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
	opt.match_name  = "";
	opt.match_type  = EntryType::fromId("texturex");
	auto tx_entries = archive->findAll(opt);

	// Go through texture lists
	PatchTable    ptable; // Dummy patch table, patch info not needed here
	wxArrayString unused_tex;
	for (auto& tx_entrie : tx_entries)
	{
		TextureXList txlist;
		txlist.readTEXTUREXData(tx_entrie, ptable);

		// Go through textures
		bool anim = false;
		for (unsigned t = 1; t < txlist.size(); t++)
		{
			wxString texname = txlist.texture(t)->name();

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
					anim    = false;
					thisend = true;
					break;
				}
			}

			// Mark if unused and not part of an animation
			if (!used_textures[texname].used && !anim && !thisend)
				unused_tex.Add(txlist.texture(t)->name());
		}
	}

	// Pop up a dialog with a checkbox list of unused textures
	wxMultiChoiceDialog dialog(
		theMainWindow,
		"The following textures are not used in any map,\nselect which textures to delete",
		"Delete Unused Textures",
		unused_tex);

	// Get base resource textures (if any)
	auto                  base_resource = app::archiveManager().baseResourceArchive();
	vector<ArchiveEntry*> base_tx_entries;
	if (base_resource)
		base_tx_entries = base_resource->findAll(opt);
	PatchTable   pt_temp;
	TextureXList tx;
	for (auto& texturex : base_tx_entries)
		tx.readTEXTUREXData(texturex, pt_temp, true);
	vector<wxString> base_resource_textures;
	for (unsigned a = 0; a < tx.size(); a++)
		base_resource_textures.emplace_back(tx.texture(a)->name());

	// Determine which textures to check initially
	wxArrayInt selection;
	for (unsigned a = 0; a < unused_tex.size(); a++)
	{
		bool swtex = false;

		// Check for switch texture
		if (unused_tex[a].StartsWith("SW1"))
		{
			// Get counterpart switch name
			wxString swname = unused_tex[a];
			swname.Replace("SW1", "SW2", false);

			// Check if its counterpart is used
			if (used_textures[swname].used)
				swtex = true;
		}
		else if (unused_tex[a].StartsWith("SW2"))
		{
			// Get counterpart switch name
			wxString swname = unused_tex[a];
			swname.Replace("SW2", "SW1", false);

			// Check if its counterpart is used
			if (used_textures[swname].used)
				swtex = true;
		}

		// Check for base resource texture
		bool br_tex = false;
		for (auto& texture : base_resource_textures)
		{
			if (texture.CmpNoCase(unused_tex[a]) == 0)
			{
				log::info(3, "Texture " + texture + " is in base resource");
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
		for (auto& entry : tx_entries)
		{
			TextureXList txlist;
			txlist.readTEXTUREXData(entry, ptable);

			// Go through selected textures to delete
			for (int i : selection)
			{
				// Get texture index
				int index = txlist.textureIndex(wxutil::strToView(unused_tex[i]));

				// Delete it from the list (if found)
				if (index >= 0)
				{
					txlist.removeTexture(index);
					n_removed++;
				}
			}

			// Write texture list data back to entry
			txlist.writeTEXTUREXData(entry, ptable);
		}
	}

	wxMessageBox(wxString::Format("Removed %d unused textures", n_removed));
}

void archiveoperations::removeUnusedFlats(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// --- Build list of used flats ---
	TexUsedMap used_textures;
	int        total_maps = 0;

	// Get all SECTORS entries
	ArchiveSearchOptions opt;
	opt.match_type = EntryType::fromId("map_sectors");
	auto sectors   = archive->findAll(opt);
	total_maps += sectors.size();

	// Go through and add used flats to list
	DoomMapFormat::Sector sec;
	wxString              tex_floor, tex_ceil;
	for (auto& sector : sectors)
	{
		int nsec = sector->size() / 26;
		sector->seek(0, SEEK_SET);
		for (int s = 0; s < nsec; s++)
		{
			// Read sector data
			sector->read(&sec, 26);

			// Get textures
			tex_floor = wxString::FromAscii(sec.f_tex, 8);
			tex_ceil  = wxString::FromAscii(sec.c_tex, 8);

			// Add to used textures list
			used_textures[tex_floor].used = true;
			used_textures[tex_ceil].used  = true;
		}
	}

	// Get all TEXTMAP entries
	opt.match_name = "TEXTMAP";
	opt.match_type = EntryType::fromId("udmf_textmap");
	auto udmfmaps  = archive->findAll(opt);
	total_maps += udmfmaps.size();

	// Go through and add used flats to list
	Tokenizer tz;
	tz.setSpecialCharacters("{};=");
	for (auto& udmfmap : udmfmaps)
	{
		// Open in tokenizer
		tz.openMem(udmfmap->data(), "UDMF TEXTMAP");

		// Go through text tokens
		wxString token = tz.getToken();
		while (!token.IsEmpty())
		{
			// Check for sector definition
			if (token == "sector")
			{
				tz.getToken(); // Skip {

				token = tz.getToken();
				while (token != "}")
				{
					// Check for texture property
					if (token == "texturefloor" || token == "textureceiling")
					{
						tz.getToken(); // Skip =
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
	opt.match_name      = "";
	opt.match_namespace = "flats";
	opt.match_type      = nullptr;
	auto flats          = archive->findAll(opt);

	// Create list of all unused flats
	wxArrayString unused_tex;
	bool          anim = false;
	for (auto& flat : flats)
	{
		// Skip markers
		if (flat->size() == 0)
			continue;

		// Check for animation start
		string flatname{ flat->nameNoExt() };
		for (int b = 0; b < n_flat_anim; b++)
		{
			if (flatname == flat_anim_start[b])
			{
				anim = true;
				log::info(wxString::Format("%s anim start", flatname));
				break;
			}
		}

		// Check for animation end
		bool thisend = false;
		for (int b = 0; b < n_flat_anim; b++)
		{
			if (flatname == flat_anim_end[b])
			{
				anim    = false;
				thisend = true;
				log::info(wxString::Format("%s anim end", flatname));
				break;
			}
		}

		// Add if not animated
		if (!used_textures[flatname].used && !anim && !thisend)
			unused_tex.Add(flatname);
	}

	// Pop up a dialog with a checkbox list of unused textures
	wxMultiChoiceDialog dialog(
		theMainWindow,
		"The following textures are not used in any map,\nselect which textures to delete",
		"Delete Unused Textures",
		unused_tex);

	// Select all flats initially
	wxArrayInt selection;
	for (unsigned a = 0; a < unused_tex.size(); a++)
		selection.push_back(a);
	dialog.SetSelections(selection);

	int n_removed = 0;
	if (dialog.ShowModal() == wxID_OK)
	{
		// Go through selected flats
		selection           = dialog.GetSelections();
		opt.match_namespace = "flats";
		for (int i : selection)
		{
			opt.match_name      = unused_tex[i];
			ArchiveEntry* entry = archive->findFirst(opt);
			archive->removeEntry(entry);
			n_removed++;
		}
	}

	wxMessageBox(wxString::Format("Removed %d unused flats", n_removed));
}

void archiveoperations::removeUnusedZDoomTextures(Archive* archive)
{
	// Check archive was given
	if (!archive)
		return;

	// Remove entry is super slow if the archive is open in a tab, so warn the user we are closing the tab
	// It can take over 30 seconds to remove 50 entries! I did a little debugging and found that processing the
	// removedEntry signal is what takes long, but having the archive open in a minimal unmanaged state seems to help

	int dialog_answer = wxMessageBox(
		"This operation is extremely slow if the archive has many entries and is open in SLADE with a tab. This tool "
		"will close the archive and reopen it in the background to process it, and save changes when done. You should "
		"make sure to save any changes now if you have any. Also, keep in mind this tool won't find any textures you "
		"reference in scripts. There is currently limited support for animated and switch textures so the tool will "
		"deselect all such textures found in ANIMDEFS by default and you can manually choose to delete them later. The "
		"ANIMDEFS parser itself is not quite reliable yet either and may not handle particularly complex syntax.",
		"Clean Zdoom Texture Entries.",
		wxOK | wxCANCEL | wxICON_WARNING);

	if (dialog_answer != wxOK)
	{
		return;
	}

	app::archiveManager().closeArchive(archive);

	// must keep this smart pointer around or the archive gets dealloced immediately
	// from the heap and we get huge memory issues while referencing a dangling pointer
	auto ptr_archive = archive->format() == ArchiveFormat::Dir
						   ? app::archiveManager().openDirArchive(archive->filename(), false, true)
						   : app::archiveManager().openArchive(archive->filename(), false, true);
	archive          = ptr_archive.get();

	// --- Build list of used textures ---
	TexUsedMap used_textures;
	int        total_maps = 0;

	auto process_maps_in_archive_func = [&total_maps, &used_textures](Archive* archive)
	{
		// Get all SIDEDEFS entries
		ArchiveSearchOptions side_def_opt;
		side_def_opt.match_type = EntryType::fromId("map_sidedefs");
		auto sidedefs           = archive->findAll(side_def_opt);
		total_maps += sidedefs.size();

		// Go through and add used textures to list
		DoomMapFormat::SideDef sdef;
		wxString               tex_lower, tex_middle, tex_upper;
		for (auto& sidedef : sidedefs)
		{
			int nsides = sidedef->size() / 30;
			sidedef->seek(0, SEEK_SET);
			for (int s = 0; s < nsides; s++)
			{
				// Read side data
				sidedef->read(&sdef, 30);

				// Get textures
				tex_lower  = wxString::FromAscii(sdef.tex_lower, 8);
				tex_middle = wxString::FromAscii(sdef.tex_middle, 8);
				tex_upper  = wxString::FromAscii(sdef.tex_upper, 8);

				// Add to used textures list
				used_textures[tex_lower].used  = true;
				used_textures[tex_middle].used = true;
				used_textures[tex_upper].used  = true;
			}
		}

		// Get all SECTORS entries
		ArchiveSearchOptions sectors_opt;
		sectors_opt.match_type = EntryType::fromId("map_sectors");
		auto sectors           = archive->findAll(sectors_opt);
		total_maps += sectors.size();

		// Go through and add used flats to list
		DoomMapFormat::Sector sec;
		wxString              tex_floor, tex_ceil;
		for (auto& sector : sectors)
		{
			int nsec = sector->size() / 26;
			sector->seek(0, SEEK_SET);
			for (int s = 0; s < nsec; s++)
			{
				// Read sector data
				sector->read(&sec, 26);

				// Get textures
				tex_floor = wxString::FromAscii(sec.f_tex, 8);
				tex_ceil  = wxString::FromAscii(sec.c_tex, 8);

				// Add to used textures list
				used_textures[tex_floor].used = true;
				used_textures[tex_ceil].used  = true;
			}
		}

		// Get all TEXTMAP entries
		ArchiveSearchOptions text_map_opt;
		text_map_opt.match_name = "TEXTMAP";
		text_map_opt.match_type = EntryType::fromId("udmf_textmap");
		auto udmfmaps           = archive->findAll(text_map_opt);
		total_maps += udmfmaps.size();

		// Go through and add used textures and flats to list
		Tokenizer tz;
		tz.setSpecialCharacters("{};=");
		for (auto& udmfmap : udmfmaps)
		{
			// Open in tokenizer
			tz.openMem(udmfmap->data(), "UDMF TEXTMAP");

			// Go through text tokens
			wxString token = tz.getToken();
			while (!token.IsEmpty())
			{
				// Check for sidedef definition
				if (token == "sidedef")
				{
					tz.getToken(); // Skip {

					token = tz.getToken();
					while (token != "}")
					{
						// Check for texture property
						if (token == "texturetop" || token == "texturemiddle" || token == "texturebottom")
						{
							tz.getToken(); // Skip =
							used_textures[tz.getToken()].used = true;
						}

						token = tz.getToken();
					}
				}
				// Check for sector definition
				else if (token == "sector")
				{
					tz.getToken(); // Skip {

					token = tz.getToken();
					while (token != "}")
					{
						// Check for texture property
						if (token == "texturefloor" || token == "textureceiling")
						{
							tz.getToken(); // Skip =
							used_textures[tz.getToken()].used = true;
						}

						token = tz.getToken();
					}
				}

				// Next token
				token = tz.getToken();
			}
		}
	};

	process_maps_in_archive_func(archive);

	// Get all wad entries and their maps
	ArchiveSearchOptions wad_opt;
	wad_opt.match_type     = EntryType::fromId("wad");
	wad_opt.search_subdirs = true;
	auto wads              = archive->findAll(wad_opt);

	for (auto wad_entry : wads)
	{
		auto wad_archive = app::archiveManager().openArchive(wad_entry, false, false);
		process_maps_in_archive_func(wad_archive.get());
	}

	// Check if any maps were found
	if (total_maps == 0)
	{
		wxMessageBox(wxString::Format("Didn't find any maps, so doing no cleanup."));
		return;
	}

	// Load all animdefs
	ArchiveSearchOptions anim_defs_opt;
	anim_defs_opt.match_type = EntryType::fromId("animdefs");
	auto animdefs            = archive->findAll(anim_defs_opt);

	TexUsedMap exclude_tex;

	// Extremely limited animdef parser to just find all PIC entries and parse all RANGE entries
	for (auto& animdef : animdefs)
	{
		log::info(wxString::Format("Found animdef %s.", animdef->name()));

		Tokenizer tz;
		tz.setSpecialCharacters("");

		// Open in tokenizer
		tz.openMem(animdef->data(), "ZDOOM ANIMDEF");

		auto get_tex_name_and_range_num =
			[](const wxString& tex_full_name, wxString& tex_name, long& range_number, int& number_digit_chars)
		{
			// If the full thing is a number
			if (tex_full_name.ToLong(&range_number))
			{
				number_digit_chars = tex_full_name.length();
				return true;
			}

			int tex_name_end_pos = tex_full_name.size() - 1;

			for (; tex_name_end_pos >= 0; tex_name_end_pos--)
			{
				wxChar ch = tex_full_name[tex_name_end_pos];

				if (!wxIsdigit(ch))
				{
					break;
				}
			}

			if (tex_name_end_pos == tex_full_name.length() - 1)
			{
				return false;
			}

			tex_name.assign(tex_full_name.SubString(0, tex_name_end_pos));
			number_digit_chars = tex_full_name.size() - tex_name_end_pos - 1;
			return tex_full_name.Mid(tex_name_end_pos + 1).ToLong(&range_number);
		};

		auto get_animated_tex_name = [](const wxString& tex_name_prefix, long tex_name_num, int number_digit_chars)
		{
			wxString animated_tex_name       = tex_name_prefix;
			wxString animatex_tex_num_format = wxString::Format("%%0%dld", number_digit_chars);
			wxString animated_tex_num        = wxString::Format(animatex_tex_num_format, tex_name_num);

			animated_tex_name.append(animated_tex_num);

			return animated_tex_name;
		};

		// Go through text tokens
		wxString token = tz.getToken();
		wxString curr_full_tex_name;
		wxString curr_tex_name;
		long     curr_tex_num                = 0;
		int      curr_tex_number_digit_chars = 0;

		while (!token.IsEmpty())
		{
			if (token.CmpNoCase("texture") == 0 || token.CmpNoCase("flat") == 0)
			{
				curr_full_tex_name = tz.getToken();
				get_tex_name_and_range_num(
					curr_full_tex_name, curr_tex_name, curr_tex_num, curr_tex_number_digit_chars);

				exclude_tex[curr_full_tex_name].used = true;

				log::info(wxString::Format("Found texture/flat animated texture definition %s.", curr_full_tex_name));
			}
			else if (token.CmpNoCase("range") == 0)
			{
				wxString last_tex_name;
				long     last_tex_num;
				int      last_tex_number_digit_chars;

				token = tz.getToken();

				if (get_tex_name_and_range_num(token, last_tex_name, last_tex_num, last_tex_number_digit_chars))
				{
					exclude_tex[token].used = true;

					// Get the range in between
					for (int tex_range = curr_tex_num + 1; tex_range < last_tex_num; ++tex_range)
					{
						wxString animated_tex_name = get_animated_tex_name(
							last_tex_name, tex_range, last_tex_number_digit_chars);
						exclude_tex[animated_tex_name].used = true;

						log::info(wxString::Format("Found range animated texture definition %s.", animated_tex_name));
					}

					log::info(wxString::Format("Found range animated texture definition %s.", token));
				}
			}
			else if (token.CmpNoCase("pic") == 0)
			{
				wxString tex_name;
				long     tex_num;
				int      tex_number_digit_chars;

				token = tz.getToken();

				if (get_tex_name_and_range_num(token, tex_name, tex_num, tex_number_digit_chars))
				{
					// If the name part is empty, we just have a number
					if (tex_name.empty())
					{
						wxString animated_tex_name = get_animated_tex_name(
							curr_full_tex_name, tex_num, curr_tex_number_digit_chars);
						exclude_tex[animated_tex_name].used = true;

						log::info(wxString::Format("Found pic animated texture definition %s.", animated_tex_name));
					}
					else
					{
						exclude_tex[token].used = true;

						log::info(wxString::Format("Found pic animated texture definition %s.", token));
					}
				}
				else
				{
					exclude_tex[token].used = true;

					log::info(wxString::Format("Found pic animated texture definition %s.", token));
				}
			}
			else if (token.CmpNoCase("cameratexture") == 0)
			{
				token                   = tz.getToken();
				exclude_tex[token].used = true;

				log::info(wxString::Format("Found cameratexture animated texture definition %s.", token));
			}
			else if (token.CmpNoCase("switch") == 0)
			{
				token                   = tz.getToken();
				exclude_tex[token].used = true;

				log::info(wxString::Format("Found switch animated texture definition %s.", token));
			}
			else if (token.CmpNoCase("animateddoor") == 0)
			{
				token                   = tz.getToken();
				exclude_tex[token].used = true;

				log::info(wxString::Format("Found animated door animated texture definition %s.", token));
			}

			// Next token
			token = tz.getToken();
		}
	}

	// Find all textures
	ArchiveSearchOptions tex_opt;
	tex_opt.match_namespace = "textures";
	auto textures           = archive->findAll(tex_opt);

	// Create list of all unused textures
	wxArrayString         unused_tex;
	vector<ArchiveEntry*> unused_entries;
	for (auto& texture : textures)
	{
		// Skip markers
		if (texture->size() == 0)
			continue;

		string texture_name{ texture->nameNoExt() };

		// TODO: When animdefs parser is more reliable, exclude animated textures here
		if (!used_textures[texture_name].used)
		{
			unused_tex.Add(texture_name);
			unused_entries.push_back(texture);
		}
	}

	// Pop up a dialog with a checkbox list of unused flats
	wxMultiChoiceDialog textures_dialog(
		theMainWindow,
		"The following textures are not used in any map,\nselect which textures to delete. Textures found in Animdefs "
		"are unselected by default.",
		"Delete Unused Textures",
		unused_tex);

	// Select all textures initially
	wxArrayInt selection;
	for (unsigned a = 0; a < unused_tex.size(); a++)
	{
		if (!exclude_tex[unused_tex[a]].used)
			selection.push_back(a);
	}
	textures_dialog.SetSelections(selection);

	int n_removed = 0;
	if (textures_dialog.ShowModal() == wxID_OK)
	{
		// Go through selected flats
		selection = textures_dialog.GetSelections();
		for (int i : selection)
		{
			archive->removeEntry(unused_entries[i]);
			n_removed++;
		}
	}

	wxMessageBox(wxString::Format("Removed %d unused textures", n_removed));

	// Find all flats
	ArchiveSearchOptions flat_opt;
	flat_opt.match_namespace = "flats";
	auto flats               = archive->findAll(flat_opt);

	// Create list of all unused flats
	unused_tex.clear();
	unused_entries.clear();
	for (auto& flat : flats)
	{
		// Skip markers
		if (flat->size() == 0)
			continue;

		string flatname{ flat->nameNoExt() };

		// TODO: When animdefs parser is more reliable, exclude animated textures here
		if (!used_textures[flatname].used)
		{
			unused_tex.Add(flatname);
			unused_entries.push_back(flat);
		}
	}

	// Pop up a dialog with a checkbox list of unused flats
	wxMultiChoiceDialog flats_dialog(
		theMainWindow,
		"The following flats are not used in any map,\nselect which flats to delete. Textures found in Animdefs are "
		"unselected by default.",
		"Delete Unused Flats",
		unused_tex);

	// Select all flats initially
	selection.clear();
	for (unsigned a = 0; a < unused_tex.size(); a++)
	{
		if (!exclude_tex[unused_tex[a]].used)
			selection.push_back(a);
	}
	flats_dialog.SetSelections(selection);

	n_removed = 0;
	if (flats_dialog.ShowModal() == wxID_OK)
	{
		// Go through selected flats
		selection = flats_dialog.GetSelections();
		for (int i : selection)
		{
			archive->removeEntry(unused_entries[i]);
			n_removed++;
		}
	}

	wxMessageBox(wxString::Format("Removed %d unused flats", n_removed));

	auto process_texture_list =
		[archive, &used_textures, &exclude_tex, &unused_tex, &selection, &n_removed](
			ArchiveEntry* texture_archive_entry, TextureXList& texture_list, const PatchTable* ptable)
	{
		for (unsigned texture_index = 0; texture_index < texture_list.size(); texture_index++)
		{
			auto texture = texture_list.texture(texture_index);

			// Skip the first null texture
			if (texture_index == 0
				&& (texture->name() == "AASHITTY" || texture->name() == "AASTINKY" || texture->name() == "BADPATCH"
					|| texture->name() == "ABADONE"))
			{
				continue;
			}

			// TODO: When animdefs parser is more reliable, exclude animated textures here
			if (!used_textures[texture->name()].used)
				unused_tex.Add(texture->name());
		}

		// Pop up a dialog with a checkbox list of unused flats
		wxMultiChoiceDialog textures_dialog(
			theMainWindow,
			wxString::Format(
				"The following textures in entry %s are not used in any map,\nselect which textures to delete. "
				"Textures found in Animdefs are unselected by default.",
				texture_archive_entry->name()),
			"Delete Unused Textures",
			unused_tex);

		// Select all textures initially
		selection.clear();
		for (unsigned a = 0; a < unused_tex.size(); a++)
		{
			if (!exclude_tex[unused_tex[a]].used)
				selection.push_back(a);
		}
		textures_dialog.SetSelections(selection);

		n_removed = 0;
		if (textures_dialog.ShowModal() == wxID_OK)
		{
			// Go through selected textures
			selection = textures_dialog.GetSelections();
			for (int i : selection)
			{
				texture_list.removeTexture(texture_list.textureIndex(string(unused_tex[i].c_str())));
				n_removed++;
			}
		}

		wxMessageBox(wxString::Format("Removed %d unused textures", n_removed));

		if (texture_list.size())
		{
			if (ptable)
			{
				texture_list.writeTEXTUREXData(texture_archive_entry, *ptable);
			}
			else
			{
				texture_list.writeTEXTURESData(texture_archive_entry);
			}
		}
		else
		{
			// If we emptied out the entry, just delete it
			archive->removeEntry(texture_archive_entry);
		}
	};

	ArchiveSearchOptions pnames_opt;
	pnames_opt.match_type = EntryType::fromId("pnames");
	auto pnames           = archive->findLast(pnames_opt);

	// Load patch table
	PatchTable ptable;
	if (pnames)
	{
		ptable.loadPNAMES(pnames);

		// Load all Texturex entries
		ArchiveSearchOptions texturexopt;
		texturexopt.match_type = EntryType::fromId("texturex");

		for (ArchiveEntry* texturexentry : archive->findAll(texturexopt))
		{
			TextureXList texture_list;
			texture_list.readTEXTUREXData(texturexentry, ptable, true);

			process_texture_list(texturexentry, texture_list, &ptable);
		}
	}

	// Load all zdtextures entries
	ArchiveSearchOptions zdtexturesopt;
	zdtexturesopt.match_type = EntryType::fromId("zdtextures");

	for (ArchiveEntry* texturesentry : archive->findAll(zdtexturesopt))
	{
		TextureXList texture_list;
		texture_list.readTEXTURESData(texturesentry);

		process_texture_list(texturesentry, texture_list, nullptr);
	}

	archive->save();

	wxMessageBox(
		wxString::Format("Archive %s has been saved to disk. You can reopen it in SLADE now.", archive->filename()));
	app::archiveManager().closeArchive(archive);
}

bool archiveoperations::checkDuplicateZDoomTextures(Archive* archive)
{
	std::unordered_multimap<string, ArchiveEntry*> found_entries;
	std::set<string>                               found_duplicates;

	auto process_entries = [&found_entries, &found_duplicates](const vector<ArchiveEntry*>& archive_entries)
	{
		for (auto& archive_entry : archive_entries)
		{
			// Skip markers
			if (archive_entry->size() == 0)
				continue;

			string entry_name{ archive_entry->upperNameNoExt() };

			if (found_entries.find(entry_name) != found_entries.end())
			{
				found_duplicates.insert(entry_name);
			}

			found_entries.emplace(entry_name, archive_entry);
		}
	};

	auto process_patch_table =
		[&found_entries, &found_duplicates](ArchiveEntry* pnames_entry, const PatchTable& patch_table)
	{
		for (int patchIndex = 0; patchIndex < patch_table.nPatches(); ++patchIndex)
		{
			string patch_name = string(patch_table.patchName(patchIndex));
			patch_name        = strutil::upperIP(patch_name);

			if (found_entries.find(patch_name) != found_entries.end())
			{
				found_duplicates.insert(patch_name);
			}

			found_entries.emplace(patch_name, pnames_entry);
		}
	};

	auto process_texture_list =
		[&found_entries, &found_duplicates](ArchiveEntry* texture_archive_entry, TextureXList& texture_list)
	{
		for (unsigned texture_index = 0; texture_index < texture_list.size(); texture_index++)
		{
			// Skip markers
			if (texture_archive_entry->size() == 0)
				continue;

			auto texture = texture_list.texture(texture_index);

			string texture_name = string(texture->name());
			texture_name        = strutil::upperIP(texture_name);

			if (found_entries.find(texture_name) != found_entries.end())
			{
				found_duplicates.insert(texture_name);
			}

			found_entries.emplace(texture_name, texture_archive_entry);
		}
	};

	// Find all textures
	{
		ArchiveSearchOptions search_opt;
		search_opt.match_namespace = "textures";
		process_entries(archive->findAll(search_opt));
	}

	// Find all flats
	{
		ArchiveSearchOptions search_opt;
		search_opt.match_namespace = "flats";
		process_entries(archive->findAll(search_opt));
	}

	ArchiveSearchOptions pnames_opt;
	pnames_opt.match_type = EntryType::fromId("pnames");
	auto pnames           = archive->findLast(pnames_opt);

	// Load patch table
	if (pnames)
	{
		PatchTable ptable;
		ptable.loadPNAMES(pnames);

		process_patch_table(pnames, ptable);

		// Load all Texturex entries
		ArchiveSearchOptions texturexopt;
		texturexopt.match_type = EntryType::fromId("texturex");

		for (ArchiveEntry* texturexentry : archive->findAll(texturexopt))
		{
			TextureXList texture_list;
			texture_list.readTEXTUREXData(texturexentry, ptable, true);

			process_texture_list(texturexentry, texture_list);
		}
	}

	// Load all zdtextures entries
	{
		ArchiveSearchOptions zdtexturesopt;
		zdtexturesopt.match_type = EntryType::fromId("zdtextures");

		for (ArchiveEntry* texturesentry : archive->findAll(zdtexturesopt))
		{
			TextureXList texture_list;
			texture_list.readTEXTURESData(texturesentry);

			process_texture_list(texturesentry, texture_list);
		}
	}

	if (found_duplicates.empty())
	{
		wxMessageBox("No duplicated textures exist");
		return false;
	}

	wxString dups = "";

	for (const string& duplicate_entry : found_duplicates)
	{
		dups += wxString::Format("\n%s", duplicate_entry);

		auto duplicated_entries_range = found_entries.equal_range(duplicate_entry);

		for (auto entry_iter = duplicated_entries_range.first; entry_iter != duplicated_entries_range.second;
			 ++entry_iter)
		{
			ArchiveEntry* duplicated_entry = entry_iter->second;
			dups += wxString::Format("\n\t%s%s", duplicated_entry->path(), duplicated_entry->name());
		}
	}

	// Display list of duplicate entry names
	ExtMessageDialog msg(theMainWindow, "Duplicate Entries");
	msg.setExt(dups);
	msg.setMessage("The following entry data are duplicated:");
	msg.ShowModal();

	return true;
}

bool archiveoperations::checkDuplicateZDoomPatches(Archive* archive)
{
	std::unordered_multimap<string, ArchiveEntry*> found_entries;
	std::set<string>                               found_duplicates;

	ArchiveSearchOptions pnames_opt;
	pnames_opt.match_type = EntryType::fromId("pnames");
	auto pnames           = archive->findLast(pnames_opt);

	// Load patch table
	if (pnames)
	{
		PatchTable ptable;
		ptable.loadPNAMES(pnames);

		for (const PatchTable::Patch& patch_entry : ptable.patches())
		{
			string entry_name = string(patch_entry.name);
			entry_name        = strutil::upperIP(entry_name);

			if (found_entries.find(entry_name) != found_entries.end())
			{
				found_duplicates.insert(entry_name);
			}

			found_entries.emplace(entry_name, pnames);
		}
	}

	// Find all patches
	{
		ArchiveSearchOptions search_opt;
		search_opt.match_namespace = "patches";

		for (auto& archive_entry : archive->findAll(search_opt))
		{
			// Skip markers
			if (archive_entry->size() == 0)
				continue;

			string entry_name{ archive_entry->upperNameNoExt() };

			if (found_entries.find(entry_name) != found_entries.end())
			{
				found_duplicates.insert(entry_name);
			}

			found_entries.emplace(entry_name, archive_entry);
		}
	}

	if (found_duplicates.empty())
	{
		wxMessageBox("No duplicated patches exist");
		return false;
	}

	wxString dups = "";

	for (const string& duplicate_entry : found_duplicates)
	{
		dups += wxString::Format("\n%s", duplicate_entry);

		auto duplicated_entries_range = found_entries.equal_range(duplicate_entry);

		for (auto entry_iter = duplicated_entries_range.first; entry_iter != duplicated_entries_range.second;
			 ++entry_iter)
		{
			ArchiveEntry* duplicated_entry = entry_iter->second;
			dups += wxString::Format("\n\t%s%s", duplicated_entry->path(), duplicated_entry->name());
		}
	}

	// Display list of duplicate entry names
	ExtMessageDialog msg(theMainWindow, "Duplicate Entries");
	msg.setExt(dups);
	msg.setMessage("The following entry data are duplicated:");
	msg.ShowModal();

	return true;
}

CONSOLE_COMMAND(test_cleantex, 0, false)
{
	auto current = maineditor::currentArchive();
	if (current)
		archiveoperations::removeUnusedTextures(current);
}

CONSOLE_COMMAND(test_cleanflats, 0, false)
{
	auto current = maineditor::currentArchive();
	if (current)
		archiveoperations::removeUnusedFlats(current);
}

CONSOLE_COMMAND(test_cleanzdoomtex, 0, false)
{
	auto current = maineditor::currentArchive();
	if (current)
		archiveoperations::removeUnusedZDoomTextures(current);
}

void importEntryDataKeepType(ArchiveEntry* entry, const void* data, unsigned size)
{
	auto type = entry->type();
	entry->importMem(data, size);
	entry->setType(type, type->reliability());
}
size_t replaceThingsDoom(ArchiveEntry* entry, int oldtype, int newtype)
{
	if (entry == nullptr)
		return 0;

	size_t size      = entry->size();
	size_t numthings = size / sizeof(DoomMapFormat::Thing);
	size_t changed   = 0;

	auto things = new DoomMapFormat::Thing[numthings];
	memcpy(things, entry->rawData(), size);

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
		importEntryDataKeepType(entry, things, size);
	delete[] things;

	return changed;
}
size_t replaceThingsDoom64(ArchiveEntry* entry, int oldtype, int newtype)
{
	if (entry == nullptr)
		return 0;

	size_t size      = entry->size();
	size_t numthings = size / sizeof(Doom64MapFormat::Thing);
	size_t changed   = 0;

	auto things = new Doom64MapFormat::Thing[numthings];
	memcpy(things, entry->rawData(), size);

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
		importEntryDataKeepType(entry, things, size);
	delete[] things;

	return changed;
}
size_t replaceThingsHexen(ArchiveEntry* entry, int oldtype, int newtype)
{
	if (entry == nullptr)
		return 0;

	size_t size      = entry->size();
	size_t numthings = size / sizeof(HexenMapFormat::Thing);
	size_t changed   = 0;

	auto things = new HexenMapFormat::Thing[numthings];
	memcpy(things, entry->rawData(), size);

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
		importEntryDataKeepType(entry, things, size);
	delete[] things;

	return changed;
}
size_t replaceThingsUDMF(const ArchiveEntry* entry, int oldtype, int newtype)
{
	if (entry == nullptr)
		return 0;

	size_t changed = 0;
	// TODO: parse and replace code
	// Import the changes if needed
	if (changed > 0)
	{
		// entry->importMemChunk(mc);
	}
	return changed;
}
size_t archiveoperations::replaceThings(Archive* archive, int oldtype, int newtype)
{
	size_t changed = 0;
	// Check archive was given
	if (!archive)
		return changed;

	// Get all maps
	auto     maps   = archive->detectMaps();
	wxString report = "";

	for (auto& map : maps)
	{
		auto m_head = map.head.lock();
		if (!m_head)
			continue;

		size_t achanged = 0;
		// Is it an embedded wad?
		if (map.archive)
		{
			// Attempt to open entry as wad archive
			auto temp_archive = std::make_shared<Archive>(ArchiveFormat::Wad);
			if (temp_archive->open(m_head->data()))
			{
				achanged = archiveoperations::replaceThings(temp_archive.get(), oldtype, newtype);
				MemChunk mc;
				if (!(temp_archive->write(mc)))
				{
					achanged = 0;
				}
				else
				{
					temp_archive->close();
					if (!(m_head->importMemChunk(mc)))
					{
						achanged = 0;
					}
				}
			}
		}
		else
		{
			// Find the map entry to modify
			auto          entries = map.entries(*archive);
			ArchiveEntry* things  = nullptr;
			if (map.format == MapFormat::Doom || map.format == MapFormat::Doom64 || map.format == MapFormat::Hexen)
			{
				for (auto mapentry : entries)
				{
					if (mapentry->type() == EntryType::fromId("map_things"))
					{
						things = mapentry;
						break;
					}
				}
			}
			else if (map.format == MapFormat::UDMF)
			{
				for (auto mapentry : entries)
				{
					if (mapentry->type() == EntryType::fromId("udmf_textmap"))
					{
						things = mapentry;
						break;
					}
				}
			}

			// Did we get a map entry?
			if (things)
			{
				switch (map.format)
				{
				case MapFormat::Doom:   achanged = replaceThingsDoom(things, oldtype, newtype); break;
				case MapFormat::Hexen:  achanged = replaceThingsHexen(things, oldtype, newtype); break;
				case MapFormat::Doom64: achanged = replaceThingsDoom64(things, oldtype, newtype); break;
				case MapFormat::UDMF:   achanged = replaceThingsUDMF(things, oldtype, newtype); break;
				default:                log::warning("Unknown map format for " + m_head->name()); break;
				}
			}
		}
		report += wxString::Format("%s:\t%i things changed\n", m_head->name(), achanged);
		changed += achanged;
	}
	log::info(1, report);
	return changed;
}

CONSOLE_COMMAND(replacethings, 2, true)
{
	auto current = maineditor::currentArchive();
	int  oldtype, newtype;

	if (current && strutil::toInt(args[0], oldtype) && strutil::toInt(args[1], newtype))
	{
		archiveoperations::replaceThings(current, oldtype, newtype);
	}
}

CONSOLE_COMMAND(convertmapchex1to3, 0, false)
{
	Archive* current    = maineditor::currentArchive();
	long     rep[23][2] = {
        //  #	Chex 1 actor			==>	Chex 3 actor			(unwanted replacement)
        { 25, 78 },   //  0	ChexTallFlower2			==> PropFlower1				(PropGlobeStand)
        { 28, 79 },   //  1	ChexTallFlower			==>	PropFlower2				(PropPhone)
        { 30, 74 },   //  2	ChexCavernStalagmite	==>	PropStalagmite			(PropPineTree)
        { 31, 50 },   //  3	ChexSubmergedPlant		==>	PropHydroponicPlant		(PropGreyRock)
        { 32, 73 },   //  4	ChexCavernColumn		==>	PropPillar				(PropBarrel)
        { 34, 80 },   //  5	ChexChemicalFlask		==>	PropBeaker				(PropCandlestick)
        { 35, 36 },   //  6	ChexGasTank				==>	PropOxygenTank			(PropCandelabra)
        { 43, 9061 }, //  7	ChexOrangeTree			==>	TreeOrange				(PropTorchTree)
        { 45, 70 },   //  8	ChexCivilian1			==>	PropCaptive1			(PropGreenTorch)
        { 47, 9060 }, //  9	ChexAppleTree			==>	TreeApple				(PropStalagtite)
        { 54, 9058 }, // 10	ChexBananaTree			==>	TreeBanana				(PropSpaceship -- must go before its own
                      // replacement)
        { 48,
			  54 },       // 11	ChexSpaceship			==>	PropSpaceship			(PropTechPillar -- must go after banana tree
                      // replacement)
        { 55, 42 },   // 12	ChexLightColumn			==>	LabCoil					(PropShortBlueTorch)
        { 56, 26 },   // 13	ChexCivilian2			==>	PropCaptive2			(PropShortGreenTorch)
        { 57, 52 },   // 14	ChexCivilian3			==>	PropCaptive3			(PropShortRedTorch)
        { 3002, 58 }, // 15	F.CycloptisCommonus		==>	F.CycloptisCommonusV3	(FlemoidusStridicus)
        { 3003, 69 }, // 16	Flembrane				==>	FlembraneV3				(FlemoidusMaximus)
        { 33,
			  53 },     // 17	ChexMineCart			==> PropBazoikCart			(none, but the sprite is modified otherwise)
        { 27, 81 }, // 18	"HeadOnAStick"			==> PropSmallBrush
        { 53, 75 }, // 19	"Meat5"					==> PropStalagtite2
        { 49, 63 }, // 20	Redundant bats
        { 51, 59 }, // 21	Redundant hanging plant #1
        { 50, 61 }, // 22	Redundant hanging plant #2
	};
	for (auto& i : rep)
	{
		archiveoperations::replaceThings(current, i[0], i[1]);
	}
}

CONSOLE_COMMAND(convertmapchex2to3, 0, false)
{
	auto current    = maineditor::currentArchive();
	long rep[20][2] = {
		{ 3001, 9057 }, //  0	Quadrumpus
		{ 3002, 9050 }, //  1	Larva
		{ 27, 81 },     //  2	"HeadOnAStick"		==> PropSmallBrush
		{ 70, 49 },     //  3	"BurningBarrel"		==> PropStool
		{ 36, 9055 },   //  4	Chex Warrior
		{ 52, 9054 },   //  5	Tutanhkamen
		{ 53, 9053 },   //  6	Ramses
		{ 30, 9052 },   //  7	Thinker
		{ 31, 9051 },   //  8	David
		{ 54, 76 },     //  9	Triceratops
		{ 32, 23 },     // 10	Chef -- replaced by a dead lost soul in Chex 3
		{ 33, 9056 },   // 11	Big spoon
		{ 34, 35 },     // 12	Street light
		{ 62, 9053 },   // 13	Ramses again
		{ 56, 49 },     // 14	Barstool again
		{ 57, 77 },     // 15	T-rex
		{ 49, 63 },     // 16	Redundant bats
		{ 51, 59 },     // 17	Redundant hanging plant #1
		{ 50, 61 },     // 18	Redundant hanging plant #2
	};
	for (int i = 0; i < 19; ++i)
	{
		archiveoperations::replaceThings(current, rep[i][0], rep[i][1]);
	}
}

size_t replaceSpecialsDoom(ArchiveEntry* entry, int oldtype, int newtype, bool tag, int oldtag, int newtag)
{
	if (entry == nullptr)
		return 0;

	size_t size     = entry->size();
	size_t numlines = size / sizeof(DoomMapFormat::LineDef);
	size_t changed  = 0;

	auto lines = new DoomMapFormat::LineDef[numlines];
	memcpy(lines, entry->rawData(), size);

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
		importEntryDataKeepType(entry, lines, size);
	delete[] lines;

	return changed;
}
size_t replaceSpecialsDoom64(ArchiveEntry* entry, int oldtype, int newtype, bool tag, int oldtag, int newtag)
{
	return 0;
}
size_t replaceSpecialsHexen(
	ArchiveEntry* l_entry,
	ArchiveEntry* t_entry,
	int           oldtype,
	int           newtype,
	bool          arg0,
	bool          arg1,
	bool          arg2,
	bool          arg3,
	bool          arg4,
	int           oldarg0,
	int           oldarg1,
	int           oldarg2,
	int           oldarg3,
	int           oldarg4,
	int           newarg0,
	int           newarg1,
	int           newarg2,
	int           newarg3,
	int           newarg4)
{
	if (l_entry == nullptr && t_entry == nullptr)
		return 0;

	size_t size    = 0;
	size_t changed = 0;

	if (l_entry)
	{
		size            = l_entry->size();
		size_t numlines = size / sizeof(HexenMapFormat::LineDef);

		auto lines = new HexenMapFormat::LineDef[numlines];
		memcpy(lines, l_entry->rawData(), size);
		size_t lchanged = 0;

		// Perform replacement
		for (size_t l = 0; l < numlines; ++l)
		{
			if (lines[l].type == oldtype)
			{
				if ((!arg0 || lines[l].args[0] == oldarg0) && (!arg1 || lines[l].args[1] == oldarg1)
					&& (!arg2 || lines[l].args[2] == oldarg2) && (!arg3 || lines[l].args[3] == oldarg3)
					&& (!arg4 || lines[l].args[4] == oldarg4))
				{
					lines[l].type = newtype;
					if (arg0)
						lines[l].args[0] = newarg0;
					if (arg1)
						lines[l].args[1] = newarg1;
					if (arg2)
						lines[l].args[2] = newarg2;
					if (arg3)
						lines[l].args[3] = newarg3;
					if (arg4)
						lines[l].args[4] = newarg4;
					++lchanged;
				}
			}
		}
		// Import the changes if needed
		if (lchanged > 0)
		{
			importEntryDataKeepType(l_entry, lines, size);
			changed += lchanged;
		}
		delete[] lines;
	}

	if (t_entry)
	{
		size             = t_entry->size();
		size_t numthings = size / sizeof(HexenMapFormat::Thing);

		auto things = new HexenMapFormat::Thing[numthings];
		memcpy(things, t_entry->rawData(), size);
		size_t tchanged = 0;

		// Perform replacement
		for (size_t t = 0; t < numthings; ++t)
		{
			if (things[t].type == oldtype)
			{
				if ((!arg0 || things[t].args[0] == oldarg0) && (!arg1 || things[t].args[1] == oldarg1)
					&& (!arg2 || things[t].args[2] == oldarg2) && (!arg3 || things[t].args[3] == oldarg3)
					&& (!arg4 || things[t].args[4] == oldarg4))
				{
					things[t].type = newtype;
					if (arg0)
						things[t].args[0] = newarg0;
					if (arg1)
						things[t].args[1] = newarg1;
					if (arg2)
						things[t].args[2] = newarg2;
					if (arg3)
						things[t].args[3] = newarg3;
					if (arg4)
						things[t].args[4] = newarg4;
					++tchanged;
				}
			}
		}
		// Import the changes if needed
		if (tchanged > 0)
		{
			importEntryDataKeepType(t_entry, things, size);
			changed += tchanged;
		}
		delete[] things;
	}

	return changed;
}
size_t replaceSpecialsUDMF(
	const ArchiveEntry* entry,
	int                 oldtype,
	int                 newtype,
	bool                arg0,
	bool                arg1,
	bool                arg2,
	bool                arg3,
	bool                arg4,
	int                 oldarg0,
	int                 oldarg1,
	int                 oldarg2,
	int                 oldarg3,
	int                 oldarg4,
	int                 newarg0,
	int                 newarg1,
	int                 newarg2,
	int                 newarg3,
	int                 newarg4)
{
	if (entry == nullptr)
		return 0;

	size_t changed = 0;
	// TODO: parse and replace code
	// Import the changes if needed
	if (changed > 0)
	{
		// entry->importMemChunk(mc);
	}
	return changed;
}
size_t archiveoperations::replaceSpecials(
	Archive* archive,
	int      oldtype,
	int      newtype,
	bool     lines,
	bool     things,
	bool     arg0,
	int      oldarg0,
	int      newarg0,
	bool     arg1,
	int      oldarg1,
	int      newarg1,
	bool     arg2,
	int      oldarg2,
	int      newarg2,
	bool     arg3,
	int      oldarg3,
	int      newarg3,
	bool     arg4,
	int      oldarg4,
	int      newarg4)
{
	size_t changed = 0;
	// Check archive was given
	if (!archive)
		return changed;

	// Get all maps
	auto     maps   = archive->detectMaps();
	wxString report = "";

	for (auto& map : maps)
	{
		auto m_head = map.head.lock();
		if (!m_head)
			continue;

		size_t achanged = 0;
		// Is it an embedded wad?
		if (map.archive)
		{
			// Attempt to open entry as wad archive
			auto temp_archive = std::make_unique<Archive>(ArchiveFormat::Wad);
			if (temp_archive->open(m_head.get()))
			{
				achanged = archiveoperations::replaceSpecials(
					temp_archive.get(),
					oldtype,
					newtype,
					lines,
					things,
					arg0,
					oldarg0,
					newarg0,
					arg1,
					oldarg1,
					newarg1,
					arg2,
					oldarg2,
					newarg2,
					arg3,
					oldarg3,
					newarg3,
					arg4,
					oldarg4,
					newarg4);
				MemChunk mc;
				if (!(temp_archive->write(mc)))
				{
					achanged = 0;
				}
				else
				{
					temp_archive->close();
					if (!(m_head->importMemChunk(mc)))
					{
						achanged = 0;
					}
				}
			}
		}
		else
		{
			// Find the map entry to modify
			ArchiveEntry* t_entry = nullptr;
			ArchiveEntry* l_entry = nullptr;
			auto          entries = map.entries(*archive);
			if (map.format == MapFormat::Doom || map.format == MapFormat::Doom64 || map.format == MapFormat::Hexen)
			{
				for (auto mapentry : entries)
				{
					if (things && mapentry->type() == EntryType::fromId("map_things"))
					{
						t_entry = mapentry;
						if (l_entry || !lines)
							break;
					}
					if (lines && mapentry->type() == EntryType::fromId("map_linedefs"))
					{
						l_entry = mapentry;
						if (t_entry || !things)
							break;
					}
				}
			}
			else if (map.format == MapFormat::UDMF)
			{
				for (auto mapentry : entries)
				{
					if (mapentry->type() == EntryType::fromId("udmf_textmap"))
					{
						l_entry = t_entry = mapentry;
						break;
					}
				}
			}

			// Did we get a map entry?
			if (l_entry || t_entry)
			{
				switch (map.format)
				{
				case MapFormat::Doom:
					if (arg1 || arg2 || arg3 || arg4) // Do nothing if Hexen specials are being modified
						break;
					achanged = replaceSpecialsDoom(l_entry, oldtype, newtype, arg0, oldarg0, newarg0);
					break;
				case MapFormat::Hexen:
					if (oldtype > 255 || newtype > 255) // Do nothing if Doom specials are being modified
						break;
					achanged = replaceSpecialsHexen(
						l_entry,
						t_entry,
						oldtype,
						newtype,
						arg0,
						arg1,
						arg2,
						arg3,
						arg4,
						oldarg0,
						oldarg1,
						oldarg2,
						oldarg3,
						oldarg4,
						newarg0,
						newarg1,
						newarg2,
						newarg3,
						newarg4);
					break;
				case MapFormat::Doom64:
					if (arg1 || arg2 || arg3 || arg4) // Do nothing if Hexen specials are being modified
						break;
					achanged = replaceSpecialsDoom64(l_entry, oldtype, newtype, arg0, oldarg0, newarg0);
					break;
				case MapFormat::UDMF:
					achanged = replaceSpecialsUDMF(
						l_entry,
						oldtype,
						newtype,
						arg0,
						arg1,
						arg2,
						arg3,
						arg4,
						oldarg0,
						oldarg1,
						oldarg2,
						oldarg3,
						oldarg4,
						newarg0,
						newarg1,
						newarg2,
						newarg3,
						newarg4);
					break;
				default: log::warning("Unknown map format for " + m_head->name()); break;
				}
			}
		}
		report += wxString::Format("%s:\t%i specials changed\n", m_head->name(), achanged);
		changed += achanged;
	}
	log::info(1, report);
	return changed;
}

CONSOLE_COMMAND(replacespecials, 2, true)
{
	Archive* current = maineditor::currentArchive();
	int      oldtype, newtype;
	bool     arg0 = false, arg1 = false, arg2 = false, arg3 = false, arg4 = false;
	int      oldarg0, oldarg1, oldarg2, oldarg3, oldarg4;
	int      newarg0, newarg1, newarg2, newarg3, newarg4;
	size_t   fullarg = args.size();
	size_t   oldtail = (fullarg / 2) - 1;
	size_t   newtail = fullarg - 1;
	bool     run     = false;

	if (fullarg > 2 && (fullarg % 2 == 0))
	{
		switch (fullarg)
		{
		case 12: arg4 = strutil::toInt(args[oldtail--], oldarg4) && strutil::toInt(args[newtail--], newarg4);
		case 10: arg3 = strutil::toInt(args[oldtail--], oldarg3) && strutil::toInt(args[newtail--], newarg3);
		case 8:  arg2 = strutil::toInt(args[oldtail--], oldarg2) && strutil::toInt(args[newtail--], newarg2);
		case 6:  arg1 = strutil::toInt(args[oldtail--], oldarg1) && strutil::toInt(args[newtail--], newarg1);
		case 4:  arg0 = strutil::toInt(args[oldtail--], oldarg0) && strutil::toInt(args[newtail--], newarg0);
		case 2:  run = strutil::toInt(args[oldtail--], oldtype) && strutil::toInt(args[newtail--], newtype); break;
		default: log::warning(wxString::Format("Invalid number of arguments: %d", fullarg));
		}
	}

	if (current && run)
	{
		archiveoperations::replaceSpecials(
			current,
			oldtype,
			newtype,
			true,
			true,
			arg0,
			oldarg0,
			newarg0,
			arg1,
			oldarg1,
			newarg1,
			arg2,
			oldarg2,
			newarg2,
			arg3,
			oldarg3,
			newarg3,
			arg4,
			oldarg4,
			newarg4);
	}
}

bool replaceTextureString(char* str, wxString oldtex, wxString newtex)
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
				if (newtex[i] == '*')
					break;
				// Keep just this character as-is?
				if (newtex[i] == '?')
					continue;
				// Else, copy the character
				str[i] = newtex[i];
			}
			else
				str[i] = 0;
		}
	}
	return go;
}
size_t replaceFlatsDoomHexen(
	ArchiveEntry*   entry,
	const wxString& oldtex,
	const wxString& newtex,
	bool            floor,
	bool            ceiling)
{
	if (entry == nullptr)
		return 0;

	size_t size       = entry->size();
	size_t numsectors = size / sizeof(DoomMapFormat::Sector);
	bool   fchanged, cchanged;
	size_t changed = 0;

	auto sectors = new DoomMapFormat::Sector[numsectors];
	memcpy(sectors, entry->rawData(), size);

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
		importEntryDataKeepType(entry, sectors, size);
	delete[] sectors;

	return changed;
}
size_t replaceWallsDoomHexen(
	ArchiveEntry*   entry,
	const wxString& oldtex,
	const wxString& newtex,
	bool            lower,
	bool            middle,
	bool            upper)
{
	if (entry == nullptr)
		return 0;

	size_t size     = entry->size();
	size_t numsides = size / sizeof(DoomMapFormat::SideDef);
	bool   lchanged, mchanged, uchanged;
	size_t changed = 0;

	DoomMapFormat::SideDef* sides = new DoomMapFormat::SideDef[numsides];
	memcpy(sides, entry->rawData(), size);
	char compare[9];
	compare[8] = 0;

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
		importEntryDataKeepType(entry, sides, size);
	delete[] sides;

	return changed;
}
size_t replaceFlatsDoom64(ArchiveEntry* entry, const wxString& oldtex, const wxString& newtex, bool floor, bool ceiling)
{
	if (entry == nullptr)
		return 0;

	size_t size       = entry->size();
	size_t numsectors = size / sizeof(Doom64MapFormat::Sector);
	bool   fchanged, cchanged;
	size_t changed = 0;

	uint16_t oldhash = app::resources().getTextureHash(oldtex.ToStdString());
	uint16_t newhash = app::resources().getTextureHash(newtex.ToStdString());

	auto sectors = new Doom64MapFormat::Sector[numsectors];
	memcpy(sectors, entry->rawData(), size);

	// Perform replacement
	for (size_t s = 0; s < numsectors; ++s)
	{
		fchanged = cchanged = false;
		if (floor && oldhash == sectors[s].f_tex)
		{
			sectors[s].f_tex = newhash;
			fchanged         = true;
		}
		if (ceiling && oldhash == sectors[s].c_tex)
		{
			sectors[s].c_tex = newhash;
			cchanged         = true;
		}
		if (fchanged || cchanged)
			++changed;
	}
	// Import the changes if needed
	if (changed > 0)
		importEntryDataKeepType(entry, sectors, size);
	delete[] sectors;

	return changed;
}
size_t replaceWallsDoom64(
	ArchiveEntry*   entry,
	const wxString& oldtex,
	const wxString& newtex,
	bool            lower,
	bool            middle,
	bool            upper)
{
	if (entry == nullptr)
		return 0;

	size_t size     = entry->size();
	size_t numsides = size / sizeof(Doom64MapFormat::SideDef);
	bool   lchanged, mchanged, uchanged;
	size_t changed = 0;

	uint16_t oldhash = app::resources().getTextureHash(oldtex.ToStdString());
	uint16_t newhash = app::resources().getTextureHash(newtex.ToStdString());

	auto sides = new Doom64MapFormat::SideDef[numsides];
	memcpy(sides, entry->rawData(), size);

	// Perform replacement
	for (size_t s = 0; s < numsides; ++s)
	{
		lchanged = mchanged = uchanged = false;
		if (lower && oldhash == sides[s].tex_lower)
		{
			sides[s].tex_lower = newhash;
			lchanged           = true;
		}
		if (middle && oldhash == sides[s].tex_middle)
		{
			sides[s].tex_middle = newhash;
			mchanged            = true;
		}
		if (upper && oldhash == sides[s].tex_upper)
		{
			sides[s].tex_upper = newhash;
			uchanged           = true;
		}
		if (lchanged || mchanged || uchanged)
			++changed;
	}
	// Import the changes if needed
	if (changed > 0)
		importEntryDataKeepType(entry, sides, size);
	delete[] sides;

	return changed;
}
size_t replaceTexturesUDMF(
	const ArchiveEntry* entry,
	const wxString&     oldtex,
	const wxString&     newtex,
	bool                floor,
	bool                ceiling,
	bool                lower,
	bool                middle,
	bool                upper)
{
	if (entry == nullptr)
		return 0;

	size_t changed = 0;
	// TODO: parse and replace code
	// Import the changes if needed
	if (changed > 0)
	{
		// entry->importMemChunk(mc);
	}
	return changed;
}
size_t archiveoperations::replaceTextures(
	Archive*        archive,
	const wxString& oldtex,
	const wxString& newtex,
	bool            floor,
	bool            ceiling,
	bool            lower,
	bool            middle,
	bool            upper)
{
	size_t changed = 0;
	// Check archive was given
	if (!archive)
		return changed;

	// Get all maps
	auto     maps   = archive->detectMaps();
	wxString report = "";

	for (auto& map : maps)
	{
		auto m_head = map.head.lock();
		if (!m_head)
			continue;

		size_t achanged = 0;
		// Is it an embedded wad?
		if (map.archive)
		{
			// Attempt to open entry as wad archive
			auto temp_archive = std::make_unique<Archive>(ArchiveFormat::Wad);
			if (temp_archive->open(m_head.get()))
			{
				achanged = archiveoperations::replaceTextures(
					temp_archive.get(), oldtex, newtex, floor, ceiling, lower, middle, upper);
				MemChunk mc;
				if (!(temp_archive->write(mc)))
				{
					achanged = 0;
				}
				else
				{
					temp_archive->close();
					if (!(m_head->importMemChunk(mc)))
					{
						achanged = 0;
					}
				}
			}
		}
		else
		{
			// Find the map entry to modify
			ArchiveEntry* sectors = nullptr;
			ArchiveEntry* sides   = nullptr;
			auto          entries = map.entries(*archive);
			if (map.format == MapFormat::Doom || map.format == MapFormat::Doom64 || map.format == MapFormat::Hexen)
			{
				for (auto mapentry : entries)
				{
					if ((floor || ceiling) && (mapentry->type() == EntryType::fromId("map_sectors")))
					{
						sectors = mapentry;
						if (sides || !(lower || middle || upper))
							break;
					}
					if ((lower || middle || upper) && (mapentry->type() == EntryType::fromId("map_sidedefs")))
					{
						sides = mapentry;
						if (sectors || !(floor || ceiling))
							break;
					}
				}
			}
			else if (map.format == MapFormat::UDMF)
			{
				for (auto mapentry : entries)
				{
					if (mapentry->type() == EntryType::fromId("udmf_textmap"))
					{
						sectors = sides = mapentry;
						break;
					}
				}
			}

			// Did we get a map entry?
			if (sectors || sides)
			{
				switch (map.format)
				{
				case MapFormat::Doom:
				case MapFormat::Hexen:
					achanged = 0;
					if (sectors)
						achanged += replaceFlatsDoomHexen(sectors, oldtex, newtex, floor, ceiling);
					if (sides)
						achanged += replaceWallsDoomHexen(sides, oldtex, newtex, lower, middle, upper);
					break;
				case MapFormat::Doom64:
					achanged = 0;
					if (sectors)
						achanged += replaceFlatsDoom64(sectors, oldtex, newtex, floor, ceiling);
					if (sides)
						achanged += replaceWallsDoom64(sides, oldtex, newtex, lower, middle, upper);
					break;
				case MapFormat::UDMF:
					achanged = replaceTexturesUDMF(sectors, oldtex, newtex, floor, ceiling, lower, middle, upper);
					break;
				default: log::warning("Unknown map format for " + m_head->name()); break;
				}
			}
		}
		report += wxString::Format("%s:\t%i elements changed\n", m_head->name(), achanged);
		changed += achanged;
	}
	log::info(1, report);
	return changed;
}

CONSOLE_COMMAND(replacetextures, 2, true)
{
	auto current = maineditor::currentArchive();

	if (current)
	{
		archiveoperations::replaceTextures(current, args[0], args[1], true, true, true, true, true);
	}
}
