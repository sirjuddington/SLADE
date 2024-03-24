
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    EntryOperations.cpp
// Description: Functions that perform specific operations on entries
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
#include "EntryOperations.h"
#include "App.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryDataFormat.h"
#include "Archive/EntryType/EntryType.h"
#include "Archive/Formats/WadArchive.h"
#include "Archive/MapDesc.h"
#include "BinaryControlLump.h"
#include "General/Console.h"
#include "General/Misc.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/CTexture/PatchTable.h"
#include "Graphics/CTexture/TextureXList.h"
#include "Graphics/Graphics.h"
#include "Graphics/SImage/SIFormat.h"
#include "MainEditor/MainEditor.h"
#include "SLADEWxApp.h"
#include "UI/Dialogs/ExtMessageDialog.h"
#include "UI/Dialogs/Preferences/PreferencesDialog.h"
#include "UI/TextureXEditor/TextureXEditor.h"
#include "Utility/FileMonitor.h"
#include "Utility/Memory.h"
#include "Utility/SFileDialog.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, path_acc, "", CVar::Flag::Save);
CVAR(String, path_acc_libs, "", CVar::Flag::Save);
CVAR(String, path_pngout, "", CVar::Flag::Save);
CVAR(String, path_pngcrush, "", CVar::Flag::Save);
CVAR(String, path_deflopt, "", CVar::Flag::Save);
CVAR(String, path_db2, "", CVar::Flag::Save)
CVAR(Bool, acc_always_show_output, false, CVar::Flag::Save);


// -----------------------------------------------------------------------------
//
// EntryOperations Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Opens a dialog to rename one or more [entries].
// If multiple entries are given, a mass-rename is performed
// -----------------------------------------------------------------------------
bool entryoperations::rename(const vector<ArchiveEntry*>& entries, Archive* archive, bool each)
{
	// Define alphabet
	static const string alphabet       = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	static const string alphabet_lower = "abcdefghijklmnopqrstuvwxyz";

	// Check any are selected
	if (each || entries.size() == 1)
	{
		// If only one entry is selected, or "rename each" mode is desired, just do basic rename
		for (auto* entry : entries)
		{
			// Prompt for a new name
			wxString new_name = wxGetTextFromUser("Enter new entry name:", "Rename", entry->name());

			// Rename entry (if needed)
			if (!new_name.IsEmpty() && entry->name() != new_name)
			{
				if (!archive->renameEntry(entry, new_name.ToStdString()))
					wxMessageBox(
						wxString::Format("Unable to rename entry %s: %s", entry->name(), global::error),
						"Rename Entry",
						wxICON_EXCLAMATION | wxOK);
			}
		}
	}
	else if (entries.size() > 1)
	{
		// Get a list of entry names
		vector<string> names;
		for (auto& entry : entries)
			names.emplace_back(entry->nameNoExt());

		// Get filter string
		auto filter = misc::massRenameFilter(names);

		// Prompt for a new name
		auto new_name = wxGetTextFromUser(
							"Enter new entry name: (* = unchanged, ^ = alphabet letter, ^^ = lower case\n% = alphabet "
							"repeat number, "
							"& = entry number, %% or && = n-1)",
							"Rename",
							filter)
							.ToStdString();

		// Apply mass rename to list of names
		if (!new_name.empty())
		{
			misc::doMassRename(names, new_name);

			// Go through the list
			for (size_t a = 0; a < entries.size(); a++)
			{
				auto entry = entries[a];

				// If the entry is a folder then skip it
				if (entry->type() == EntryType::folderType())
					continue;

				// Get current name as wxFileName for processing
				strutil::Path fn(entry->name());

				// Rename the entry (if needed)
				if (fn.fileName(false) != names[a])
				{
					auto filename = names[a];
					int  num      = a / alphabet.size();
					int  cn       = a - (num * alphabet.size());
					strutil::replaceIP(filename, "^^", { alphabet_lower.data() + cn, 1 });
					strutil::replaceIP(filename, "^", { alphabet.data() + cn, 1 });
					strutil::replaceIP(filename, "%%", fmt::format("{}", num));
					strutil::replaceIP(filename, "%", fmt::format("{}", num + 1));
					strutil::replaceIP(filename, "&&", fmt::format("{}", a));
					strutil::replaceIP(filename, "&", fmt::format("{}", a + 1));
					fn.setFileName(filename); // Change name

					// Rename in archive
					if (!archive->renameEntry(entry, fn.fileName(), true))
						wxMessageBox(
							wxString::Format("Unable to rename entry %s: %s", entries[a]->name(), global::error),
							"Rename Entry",
							wxICON_EXCLAMATION | wxOK);
				}
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Opens a dialog to rename one or more [dirs]
// -----------------------------------------------------------------------------
bool entryoperations::renameDir(const vector<ArchiveDir*>& dirs, Archive* archive)
{
	// Go through the list
	for (auto* dir : dirs)
	{
		// Get the current directory's name
		auto old_name = dir->name();

		// Prompt for a new name
		auto new_name = wxGetTextFromUser(
							"Enter new directory name:", wxString::Format("Rename Directory %s", old_name), old_name)
							.ToStdString();

		// Do nothing if no name was entered
		if (new_name.empty())
			continue;

		// Discard any given path (for now)
		new_name = strutil::Path::fileNameOf(new_name);

		// Rename the directory if the new entered name is different from the original
		if (new_name != old_name)
			archive->renameDir(dir, new_name);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Opens a save file dialog to export an [entry] to a file
// -----------------------------------------------------------------------------
bool entryoperations::exportEntry(ArchiveEntry* entry)
{
	wxString   name = misc::lumpNameToFileName(entry->name());
	wxFileName fn(name);

	// Add appropriate extension if needed
	if (fn.GetExt().Len() == 0)
		fn.SetExt(entry->type()->extension());

	// Run save file dialog
	filedialog::FDInfo info;
	if (filedialog::saveFile(
			info,
			"Export Entry \"" + entry->name() + "\"",
			"Any File (*.*)|*.*",
			maineditor::windowWx(),
			fn.GetFullName().ToStdString()))
		entry->exportFile(info.filenames[0]); // Export entry if ok was clicked

	return true;
}

// -----------------------------------------------------------------------------
// Opens a directory selection dialog to export multiple [entries] and [dirs] to
// -----------------------------------------------------------------------------
bool entryoperations::exportEntries(const vector<ArchiveEntry*>& entries, const vector<ArchiveDir*>& dirs)
{
	// Run save files dialog
	filedialog::FDInfo info;
	if (filedialog::saveFiles(
			info, "Export Multiple Entries (Filename is ignored)", "Any File (*.*)|*.*", maineditor::windowWx()))
	{
		// Go through the selected entries
		for (auto& entry : entries)
		{
			// Setup entry filename
			wxFileName fn(entry->name());
			fn.SetPath(info.path);

			// Add file extension if it doesn't exist
			if (!fn.HasExt())
			{
				fn.SetExt(entry->type()->extension());

				// ...unless a file already exists with said extension
				if (wxFileExists(fn.GetFullPath()))
					fn.SetEmptyExt();
			}

			// Do export
			entry->exportFile(fn.GetFullPath().ToStdString());
		}

		// Go through selected dirs
		for (auto& dir : dirs)
			dir->exportTo(string{ info.path + "/" + dir->name() });
	}

	return true;
}

// -----------------------------------------------------------------------------
// Opens the map at [entry] with Doom Builder 2, including all open resource
// archives.
// Sets up a FileMonitor to update the map in the archive if any changes are
// made to it in DB2
// -----------------------------------------------------------------------------
bool entryoperations::openMapDB2(ArchiveEntry* entry)
{
#ifdef __WXMSW__ // Windows only
	wxString path = path_db2;

	if (path.IsEmpty())
	{
		// Check for DB2 location registry key
		wxRegKey key(wxRegKey::HKLM, "SOFTWARE\\CodeImp\\Doom Builder");
		key.QueryValue("Location", path);

		// Browse for executable if DB2 isn't installed
		if (path.IsEmpty())
		{
			wxMessageBox(
				"Could not find the installation directory of Doom Builder 2, please browse for the DB2 executable",
				"Doom Builder 2 Not Found");

			filedialog::FDInfo info;
			if (filedialog::openFile(
					info, "Browse for DB2 Executable", filedialog::executableExtensionString(), nullptr, "Builder.exe"))
			{
				path_db2 = info.filenames[0];
				path     = info.filenames[0];
			}
			else
				return false;
		}
		else
		{
			// Add default executable name
			path += "\\Builder.exe";
		}
	}

	// Get map info for entry
	auto map = entry->parent()->mapDesc(entry);

	// Check valid map
	if (!map.archive && map.format == MapFormat::Unknown)
		return false;

	// Export the map to a temp .wad file
	auto filename = app::path(
		fmt::format("{}-{}.wad", entry->parent()->filename(false), entry->nameNoExt()), app::Dir::Temp);
	std::replace(filename.begin(), filename.end(), '/', '-');
	if (map.archive)
	{
		entry->exportFile(filename);
		entry->lock();
	}
	else
	{
		// Write map entries to temporary wad archive
		if (auto m_head = map.head.lock())
		{
			auto       m_end = map.end.lock();
			WadArchive archive;

			// Add map entries to archive
			auto parent = entry->parent();
			auto e      = m_head;
			auto index  = parent->entryIndex(m_head.get());
			while (true)
			{
				archive.addEntry(std::make_shared<ArchiveEntry>(*e), "");
				e->lock();
				if (e == m_end)
					break;
				e = parent->entryAt(++index)->getShared();
			}

			// Write archive to file
			archive.save(filename);
		}
	}

	// Generate Doom Builder command line
	wxString cmd = wxString::Format("%s \"%s\" -map %s", path, filename, entry->name());

	// Add base resource archive to command line
	auto base = app::archiveManager().baseResourceArchive();
	if (base)
	{
		if (base->formatId() == "wad")
			cmd += wxString::Format(" -resource wad \"%s\"", base->filename());
		else if (base->formatId() == "zip")
			cmd += wxString::Format(" -resource pk3 \"%s\"", base->filename());
	}

	// Add resource archives to command line
	for (int a = 0; a < app::archiveManager().numArchives(); ++a)
	{
		auto archive = app::archiveManager().getArchive(a);

		// Check archive type (only wad and zip supported by db2)
		if (archive->formatId() == "wad")
			cmd += wxString::Format(" -resource wad \"%s\"", archive->filename());
		else if (archive->formatId() == "zip")
			cmd += wxString::Format(" -resource pk3 \"%s\"", archive->filename());
	}

	// Run DB2
	FileMonitor* fm = new DB2MapFileMonitor(filename, entry->parent(), string{ entry->nameNoExt() });
	wxExecute(cmd, wxEXEC_ASYNC, fm->process());

	return true;
#else
	return false;
#endif //__WXMSW__
}

// -----------------------------------------------------------------------------
// Adds all [entries] to their parent archive's patch table, if it exists.
// If not, the user is prompted to create or import texturex entries
// -----------------------------------------------------------------------------
bool entryoperations::addToPatchTable(const vector<ArchiveEntry*>& entries)
{
	// Check any entries were given
	if (entries.empty())
		return true;

	// Get parent archive
	auto parent = entries[0]->parent();
	if (parent == nullptr)
		return true;

	// Find patch table in parent archive
	ArchiveSearchOptions opt;
	opt.match_type = EntryType::fromId("pnames");
	auto pnames    = parent->findLast(opt);

	// Check it exists
	if (!pnames)
	{
		// Create texture entries
		if (!TextureXEditor::setupTextureEntries(parent))
			return false;

		pnames = parent->findLast(opt);

		// If the archive already has ZDoom TEXTURES, it might still
		// not have a PNAMES lump; so create an empty one.
		if (!pnames)
		{
			auto new_pnames = std::make_shared<ArchiveEntry>("PNAMES.lmp", 4);
			pnames          = new_pnames.get();
			uint32_t nada   = 0;
			pnames->write(&nada, 4);
			pnames->seek(0, SEEK_SET);
			parent->addEntry(new_pnames);
		}
	}

	// Check it isn't locked (texturex editor open or iwad)
	if (pnames->isLocked())
	{
		if (parent->isReadOnly())
			wxMessageBox("Cannot perform this action on an IWAD", "Error", wxICON_ERROR);
		else
			wxMessageBox(
				"Cannot perform this action because one or more texture related entries is locked. Please close the "
				"archive's texture editor if it is open.",
				"Error",
				wxICON_ERROR);

		return false;
	}

	// Load to patch table
	PatchTable ptable;
	ptable.loadPNAMES(pnames);

	// Add entry names to patch table
	for (auto& entry : entries)
	{
		// Check entry type
		if (!(entry->type()->extraProps().contains("image")))
		{
			log::error("Entry {} is not a valid image", entry->name());
			continue;
		}

		// Check entry name
		if (entry->nameNoExt().size() > 8)
		{
			log::error(
				"Entry {} has too long a name to add to the patch table (name must be 8 characters max)",
				entry->name());
			continue;
		}

		ptable.addPatch(entry->nameNoExt());
	}

	// Write patch table data back to pnames entry
	return ptable.writePNAMES(pnames);
}

// -----------------------------------------------------------------------------
// Same as addToPatchTable, but also creates a single-patch texture from each
// added patch
// -----------------------------------------------------------------------------
bool entryoperations::createTexture(const vector<ArchiveEntry*>& entries)
{
	// Check any entries were given
	if (entries.empty())
		return true;

	// Get parent archive
	auto parent = entries[0]->parent();

	// Create texture entries if needed
	if (!TextureXEditor::setupTextureEntries(parent))
		return false;

	// Find texturex entry to add to
	ArchiveSearchOptions opt;
	opt.match_type = EntryType::fromId("texturex");
	auto texturex  = parent->findFirst(opt);

	// Check it exists
	bool zdtextures = false;
	if (!texturex)
	{
		opt.match_type = EntryType::fromId("zdtextures");
		texturex       = parent->findFirst(opt);

		if (!texturex)
			return false;
		else
			zdtextures = true;
	}

	// Find patch table in parent archive
	ArchiveEntry* pnames = nullptr;
	if (!zdtextures)
	{
		opt.match_type = EntryType::fromId("pnames");
		pnames         = parent->findLast(opt);

		// Check it exists
		if (!pnames)
			return false;
	}

	// Check entries aren't locked (texture editor open or iwad)
	if ((pnames && pnames->isLocked()) || texturex->isLocked())
	{
		if (parent->isReadOnly())
			wxMessageBox("Cannot perform this action on an IWAD", "Error", wxICON_ERROR);
		else
			wxMessageBox(
				"Cannot perform this action because one or more texture related entries is locked. Please close the "
				"archive's texture editor if it is open.",
				"Error",
				wxICON_ERROR);

		return false;
	}

	TextureXList tx;
	PatchTable   ptable;
	if (zdtextures)
	{
		// Load TEXTURES
		tx.readTEXTURESData(texturex);
	}
	else
	{
		// Load patch table
		ptable.loadPNAMES(pnames);

		// Load TEXTUREx
		tx.readTEXTUREXData(texturex, ptable);
	}

	// Create textures from entries
	SImage image;
	for (auto& entry : entries)
	{
		// Check entry type
		if (!(entry->type()->extraProps().contains("image")))
		{
			log::error(wxString::Format("Entry %s is not a valid image", entry->name()));
			continue;
		}

		// Check entry name
		string name{ entry->nameNoExt() };
		if (name.size() > 8)
		{
			log::error(
				"Entry {} has too long a name to add to the patch table (name must be 8 characters max)",
				entry->name());
			continue;
		}

		// Add to patch table
		if (!zdtextures)
			ptable.addPatch(name);

		// Load patch to temp image
		misc::loadImageFromEntry(&image, entry);

		// Create texture
		auto ntex = std::make_unique<CTexture>(zdtextures);
		ntex->setName(name);
		ntex->addPatch(name, 0, 0);
		ntex->setWidth(image.width());
		ntex->setHeight(image.height());

		// Setup texture scale
		if (tx.format() == TextureXList::Format::Textures)
			ntex->setScale({ 1., 1. });
		else
			ntex->setScale({ 0., 0. });

		// Add to texture list
		tx.addTexture(std::move(ntex));
	}

	if (zdtextures)
	{
		// Write texture data back to textures entry
		tx.writeTEXTURESData(texturex);
	}
	else
	{
		// Write patch table data back to pnames entry
		ptable.writePNAMES(pnames);

		// Write texture data back to texturex entry
		tx.writeTEXTUREXData(texturex, ptable);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Converts multiple TEXTURE1/2 entries to a single ZDoom text-based TEXTURES
// entry
// -----------------------------------------------------------------------------
bool entryoperations::convertTextures(const vector<ArchiveEntry*>& entries)
{
	// Check any entries were given
	if (entries.empty())
		return false;

	// Get parent archive of entries
	auto parent = entries[0]->parent();

	// Can't do anything if entry isn't in an archive
	if (!parent)
		return false;

	// Find patch table in parent archive
	ArchiveSearchOptions opt;
	opt.match_type = EntryType::fromId("pnames");
	auto pnames    = parent->findLast(opt);

	// Check it exists
	if (!pnames)
	{
		wxMessageBox("Unable to convert - could not find PNAMES entry", "Convert to TEXTURES", wxICON_ERROR | wxOK);
		return false;
	}

	// Load patch table
	PatchTable ptable;
	ptable.loadPNAMES(pnames);

	// Read all texture entries to a single list
	TextureXList tx;
	for (auto& entry : entries)
		tx.readTEXTUREXData(entry, ptable, true);

	// Convert to extended (TEXTURES) format
	tx.convertToTEXTURES();

	// Create new TEXTURES entry and write to it
	auto textures = parent->addNewEntry("TEXTURES", parent->entryIndex(entries[0]));
	if (textures)
	{
		bool ok = tx.writeTEXTURESData(textures.get());
		EntryType::detectEntryType(*textures);
		textures->setExtensionByType();
		return ok;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Detect errors in a TEXTUREx entry
// -----------------------------------------------------------------------------
bool entryoperations::findTextureErrors(const vector<ArchiveEntry*>& entries)
{
	// Check any entries were given
	if (entries.empty())
		return false;

	// Get parent archive of entries
	auto parent = entries[0]->parent();

	// Can't do anything if entry isn't in an archive
	if (!parent)
		return false;

	// Find patch table in parent archive
	ArchiveSearchOptions opt;
	opt.match_type = EntryType::fromId("pnames");
	auto pnames    = parent->findLast(opt);

	// Check it exists
	if (!pnames)
		return false;

	// Load patch table
	PatchTable ptable;
	ptable.loadPNAMES(pnames);

	// Read all texture entries to a single list
	TextureXList tx;
	for (auto& entry : entries)
		tx.readTEXTUREXData(entry, ptable, true);

	// Detect errors
	tx.findErrors();

	return true;
}

// -----------------------------------------------------------------------------
// Clean texture entries that are duplicates of entries in the iwad
// -----------------------------------------------------------------------------
bool entryoperations::cleanTextureIwadDupes(const vector<ArchiveEntry*>& entries)
{
	// Check any entries were given
	if (entries.empty())
		return false;

	int dialog_answer = wxMessageBox(
		"Don't run this on TEXTURE entries unless your wad/archive is intended for newer more advanced source ports "
		"like GZDoom. The newer source ports can still properly access iwad textures if you don't include their "
		"entries in a pwad. However, older engines may rely on all of the iwad TEXTUREs being redefined in a pwad to "
		"work correctly. You should have nothing to worry about for ZDoom Format TEXTURES files.",
		"Remove duplicate texture entries.",
		wxOK | wxCANCEL | wxICON_WARNING);

	if (dialog_answer != wxOK)
	{
		return false;
	}

	// Get parent archive of entries
	auto parent = entries[0]->parent();

	// Can't do anything if entry isn't in an archive
	if (!parent)
		return false;

	// Do nothing if there is no base resource archive,
	// or if the archive *is* the base resource archive.
	auto bra = app::archiveManager().baseResourceArchive();
	if (bra == nullptr || bra == parent)
		return false;

	// Now load base resource archive textures into a single list
	TextureXList bra_tx_list;

	ArchiveSearchOptions opt;
	opt.match_type  = EntryType::fromId("pnames");
	auto bra_pnames = bra->findLast(opt);

	// Load patch table
	PatchTable bra_ptable;
	if (bra_pnames)
	{
		bra_ptable.loadPNAMES(bra_pnames);

		// Load all Texturex entries
		ArchiveSearchOptions texturexopt;
		texturexopt.match_type = EntryType::fromId("texturex");

		for (ArchiveEntry* texturexentry : bra->findAll(texturexopt))
		{
			bra_tx_list.readTEXTUREXData(texturexentry, bra_ptable, true);
		}
	}

	// Load all zdtextures entries
	ArchiveSearchOptions zdtexturesopt;
	zdtexturesopt.match_type = EntryType::fromId("zdtextures");

	for (ArchiveEntry* texturesentry : bra->findAll(zdtexturesopt))
	{
		bra_tx_list.readTEXTURESData(texturesentry);
	}

	// If we ended up not loading textures from base resource archive
	if (!bra_tx_list.size())
	{
		log::error("Base resource archive has no texture entries to compare against");
		return false;
	}

	// Find patch table in parent archive
	auto pnames = parent->findLast(opt);

	// Load patch table if we have it
	PatchTable ptable;
	if (pnames)
		ptable.loadPNAMES(pnames);

	bool ret = false;

	// For each selected entry, perform the clean operation and save it out
	for (auto& entry : entries)
	{
		TextureXList tx;

		bool is_texturex = false;

		// If it's a texturex entry
		if (entry->type()->id() == "texturex")
		{
			if (pnames)
			{
				tx.readTEXTUREXData(entry, ptable, true);
				is_texturex = true;
				log::info(wxString::Format("Cleaning duplicate entries from TEXTUREx entry %s.", entry->name()));
			}
			else
			{
				log::error(wxString::Format(
					"Skipping cleaning TEXTUREx entry %s since this archive has no patch table.", entry->name()));
				// Skip cleaning this texturex entry if there is no patch table for us to load it with
				continue;
			}
		}
		else if (entry->type()->id() == "zdtextures")
		{
			tx.readTEXTURESData(entry);
			log::info(wxString::Format("Cleaning duplicate entries from ZDoom TEXTURES entry %s.", entry->name()));
		}

		if (tx.removeDupesFoundIn(bra_tx_list))
		{
			log::info(wxString::Format("Cleaned entries from: %s.", entry->name()));

			if (tx.size())
			{
				if (is_texturex)
				{
					tx.writeTEXTUREXData(entry, ptable);
				}
				else
				{
					tx.writeTEXTURESData(entry);
				}
			}
			else
			{
				// If we emptied out the entry, just delete it
				parent->removeEntry(entry);
				log::info(wxString::Format("%s no longer has any entries so deleting it.", entry->name()));
			}

			ret = true;
		}
		else
		{
			log::info(wxString::Format("Found no entries to clean from: %s.", entry->name()));
		}
	}

	if (ret)
	{
		wxMessageBox(
			"Found duplicate texture entries to remove. Check the console for output info. The PATCH table was left "
			"untouched. You can either delete it or clean it using the Remove Unused Patches tool.",
			"Remove duplicate texture entries.",
			wxOK | wxCENTER | wxICON_INFORMATION);
	}
	else
	{
		wxMessageBox(
			"Didn't find any duplicate texture entries to remove. Check the console for output info.",
			"Remove duplicate texture entries.",
			wxOK | wxCENTER | wxICON_INFORMATION);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Clean ZDTEXTURES entries that are just a single patch
// -----------------------------------------------------------------------------
bool entryoperations::cleanZdTextureSinglePatch(const vector<ArchiveEntry*>& entries)
{
	// Check any entries were given
	if (entries.empty())
		return false;

	// Get parent archive of entries
	auto parent = entries[0]->parent();

	// Can't do anything if entry isn't in an archive
	if (!parent)
		return false;

	if (parent->formatDesc().supports_dirs)
	{
		int dialog_answer = wxMessageBox(
			"This will remove all textures that are made out of a basic single patch from this textures entry. It will "
			"also rename all of the patches to the texture name and move them from the patches to the textures folder.",
			"Clean single patch texture entries.",
			wxOK | wxCANCEL | wxICON_WARNING);

		if (dialog_answer != wxOK)
		{
			return false;
		}
	}
	else
	{
		// Warn that patch to texture conversion only works archives that support folders
		wxMessageBox(
			"This currently only works with archives that support directories",
			"Clean single patch texture entries.",
			wxOK | wxICON_WARNING);
		return false;
	}

	bool ret = false;

	for (auto& entry : entries)
	{
		TextureXList tx;
		tx.readTEXTURESData(entry);
		if (tx.cleanTEXTURESsinglePatch(parent))
		{
			log::info(wxString::Format("Cleaned entries from: %s.", entry->name()));

			if (tx.size())
			{
				tx.writeTEXTURESData(entry);
			}
			else
			{
				// If we emptied out the entry, just delete it
				parent->removeEntry(entry);
				log::info(wxString::Format("%s no longer has any entries so deleting it.", entry->name()));
			}

			ret = true;
		}
	}

	if (ret)
	{
		wxMessageBox(
			"Found texture entries to clean. Check the console for output info.",
			"Clean single patch texture entries.",
			wxOK | wxCENTER | wxICON_INFORMATION);
	}
	else
	{
		wxMessageBox(
			"Didn't find any texture entries to clean. Check the console for output info.",
			"Clean single patch texture entries.",
			wxOK | wxCENTER | wxICON_INFORMATION);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Attempts to compile [entry] as an ACS script.
// If the entry is named SCRIPTS, the compiled data is imported to the BEHAVIOR
// entry previous to it, otherwise it is imported to a same-name compiled
// library entry in the acs namespace
// -----------------------------------------------------------------------------
bool entryoperations::compileACS(ArchiveEntry* entry, bool hexen, ArchiveEntry* target, wxFrame* parent)
{
	// Check entry was given
	if (!entry)
		return false;

	// Check entry has a parent (this is useless otherwise)
	auto* archive = entry->parent();
	if (!target && !archive)
		return false;

	// Check entry is text
	if (!EntryDataFormat::format("text")->isThisFormat(entry->data()))
	{
		wxMessageBox("Error: Entry does not appear to be text", "Error", wxOK | wxCENTRE | wxICON_ERROR);
		return false;
	}

	// Check if the ACC path is set up
	wxString accpath = path_acc;
	if (accpath.IsEmpty() || !wxFileExists(accpath))
	{
		wxMessageBox(
			"Error: ACC path not defined, please configure in SLADE preferences",
			"Error",
			wxOK | wxCENTRE | wxICON_ERROR);
		PreferencesDialog::openPreferences(parent, "ACS");
		return false;
	}

	// Setup some path strings
	auto srcfile       = app::path(fmt::format("{}.acs", entry->nameNoExt()), app::Dir::Temp);
	auto ofile         = app::path(fmt::format("{}.o", entry->nameNoExt()), app::Dir::Temp);
	auto include_paths = wxSplit(path_acc_libs, ';');

	// Setup command options
	wxString opt;
	if (hexen)
		opt += " -h";
	if (!include_paths.IsEmpty())
	{
		for (const auto& include_path : include_paths)
			opt += wxString::Format(" -i \"%s\"", include_path);
	}

	// Find/export any resource libraries
	ArchiveSearchOptions sopt;
	sopt.match_type       = EntryType::fromId("acs");
	sopt.search_subdirs   = true;
	auto          entries = app::archiveManager().findAllResourceEntries(sopt);
	wxArrayString lib_paths;
	for (auto& res_entry : entries)
	{
		// Ignore SCRIPTS
		if (res_entry->upperNameNoExt() == "SCRIPTS")
			continue;

		// Ignore entries from other archives
		if (archive && (archive->filename(true) != res_entry->parent()->filename(true)))
			continue;

		auto path = app::path(fmt::format("{}.acs", res_entry->nameNoExt()), app::Dir::Temp);
		res_entry->exportFile(path);
		lib_paths.Add(path);
		log::info(2, "Exporting ACS library {}", res_entry->name());
	}

	// Export script to file
	entry->exportFile(srcfile);

	// Execute acc
	wxString      command = "\"" + path_acc + "\"" + " " + opt + " \"" + srcfile + "\" \"" + ofile + "\"";
	wxArrayString output;
	wxArrayString errout;
	wxGetApp().SetTopWindow(parent);
	wxExecute(command, output, errout, wxEXEC_SYNC);
	wxGetApp().SetTopWindow(maineditor::windowWx());

	// Log output
	log::console("ACS compiler output:");
	wxString output_log;
	if (!output.IsEmpty())
	{
		const char* title1 = "=== Log: ===\n";
		log::console(title1);
		output_log += title1;
		for (const auto& line : output)
		{
			log::console(line);
			output_log += line;
		}
	}

	if (!errout.IsEmpty())
	{
		const char* title2 = "\n=== Error log: ===\n";
		log::console(title2);
		output_log += title2;
		for (const auto& line : errout)
		{
			log::console(line);
			output_log += line + "\n";
		}
	}

	// Delete source file
	wxRemoveFile(srcfile);

	// Delete library files
	for (const auto& lib_path : lib_paths)
		wxRemoveFile(lib_path);

	// Check it compiled successfully
	bool success = wxFileExists(ofile);
	if (success)
	{
		// If no target entry was given, find one
		if (!target)
		{
			// Check if the script is a map script (BEHAVIOR)
			if (entry->upperName() == "SCRIPTS")
			{
				// Get entry before SCRIPTS
				auto prev = archive->entryAt(archive->entryIndex(entry) - 1);

				// Create a new entry there if it isn't BEHAVIOR
				if (!prev || prev->upperName() != "BEHAVIOR")
					prev = archive->addNewEntry("BEHAVIOR", archive->entryIndex(entry)).get();

				// Import compiled script
				prev->importFile(ofile);
			}
			else
			{
				// Otherwise, treat it as a library

				// See if the compiled library already exists as an entry
				ArchiveSearchOptions opt;
				opt.match_namespace = "acs";
				opt.match_name      = entry->nameNoExt();
				if (archive->formatDesc().names_extensions)
				{
					opt.match_name += ".o";
					opt.ignore_ext = false;
				}
				auto lib = archive->findLast(opt);

				// If it doesn't exist, create it
				if (!lib)
				{
					auto new_lib = std::make_shared<ArchiveEntry>(fmt::format("{}.o", entry->nameNoExt()));
					lib          = archive->addEntry(new_lib, "acs").get();
				}

				// Import compiled script
				lib->importFile(ofile);
			}
		}
		else
			target->importFile(ofile);

		// Delete compiled script file
		wxRemoveFile(ofile);
	}

	if (!success || acc_always_show_output)
	{
		wxString errors;
		if (wxFileExists(app::path("acs.err", app::Dir::Temp)))
		{
			// Read acs.err to string
			wxFile       file(app::path("acs.err", app::Dir::Temp));
			vector<char> buf(file.Length());
			file.Read(buf.data(), file.Length());
			errors = wxString::From8BitData(buf.data(), file.Length());
		}
		else
			errors = output_log;

		if (!errors.empty() || !success)
		{
			ExtMessageDialog dlg(nullptr, success ? "ACC Output" : "Error Compiling");
			dlg.setMessage(
				success ? "The following errors were encountered while compiling, please fix them and recompile:" :
						  "Compiler output shown below: ");
			dlg.setExt(errors);
			dlg.ShowModal();
		}

		return success;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Converts [entry] to a PNG image (if possible) and saves the PNG data to a
// file [filename]. Does not alter the entry data itself
// -----------------------------------------------------------------------------
bool entryoperations::exportAsPNG(ArchiveEntry* entry, const wxString& filename)
{
	// Check entry was given
	if (!entry)
		return false;

	// Create image from entry
	SImage image;
	if (!misc::loadImageFromEntry(&image, entry))
	{
		log::error(wxString::Format("Error converting %s: %s", entry->name(), global::error));
		return false;
	}

	// Write png data
	MemChunk png;
	auto     fmt_png = SIFormat::getFormat("png");
	if (!fmt_png->saveImage(image, png, maineditor::currentPalette(entry)))
	{
		log::error(wxString::Format("Error converting %s", entry->name()));
		return false;
	}

	// Export file
	return png.exportFile(filename.ToStdString());
}

// -----------------------------------------------------------------------------
// Attempts to optimize [entry] using external PNG optimizers.
// -----------------------------------------------------------------------------
bool entryoperations::optimizePNG(ArchiveEntry* entry)
{
	// Check entry was given
	if (!entry)
		return false;

	// Check entry has a parent (this is useless otherwise)
	if (!entry->parent())
		return false;

	// Check entry is PNG
	if (!EntryDataFormat::format("img_png")->isThisFormat(entry->data()))
	{
		wxMessageBox("Error: Entry does not appear to be PNG", "Error", wxOK | wxCENTRE | wxICON_ERROR);
		return false;
	}

	// Check if the PNG tools path are set up, at least one of them should be
	wxString pngpathc = path_pngcrush;
	wxString pngpatho = path_pngout;
	wxString pngpathd = path_deflopt;
	if ((pngpathc.IsEmpty() || !wxFileExists(pngpathc)) && (pngpatho.IsEmpty() || !wxFileExists(pngpatho))
		&& (pngpathd.IsEmpty() || !wxFileExists(pngpathd)))
	{
		log::error(1, "PNG tool paths not defined or invalid, no optimization done.");
		return false;
	}

	// Save special chunks
	bool          alphchunk     = gfx::pngGetalPh(entry->data());
	auto          grabchunk     = gfx::getImageOffsets(entry->data());
	wxString      errormessages = "";
	wxArrayString output;
	wxArrayString errors;
	size_t        oldsize   = entry->size();
	size_t        crushsize = 0, outsize = 0, deflsize = 0;
	bool          crushed = false, outed = false;

	// Run PNGCrush
	if (!pngpathc.IsEmpty() && wxFileExists(pngpathc))
	{
		wxFileName fn(pngpathc);
		fn.SetExt("opt");
		wxString pngfile = fn.GetFullPath();
		fn.SetExt("png");
		wxString optfile = fn.GetFullPath();
		entry->exportFile(pngfile.ToStdString());

		wxString command = path_pngcrush + " -brute \"" + pngfile + "\" \"" + optfile + "\"";
		output.Empty();
		errors.Empty();
		wxExecute(command, output, errors, wxEXEC_SYNC);

		if (wxFileExists(optfile))
		{
			if (optfile.size() < oldsize)
			{
				entry->importFile(optfile.ToStdString());
				wxRemoveFile(optfile);
				wxRemoveFile(pngfile);
			}
			else
				errormessages += "PNGCrush failed to reduce file size further.\n";
			crushed = true;
		}
		else
			errormessages += "PNGCrush failed to create optimized file.\n";
		crushsize = entry->size();

		// send app output to console if wanted
		if (false)
		{
			wxString crushlog = "";
			if (errors.GetCount())
			{
				crushlog += "PNGCrush error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					crushlog += errors[i] + "\n";
				errormessages += crushlog;
			}
			if (output.GetCount())
			{
				crushlog += "PNGCrush output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					crushlog += output[i] + "\n";
			}
			log::info(1, crushlog);
		}
	}

	// Run PNGOut
	if (!pngpatho.IsEmpty() && wxFileExists(pngpatho))
	{
		wxFileName fn(pngpatho);
		fn.SetExt("opt");
		wxString pngfile = fn.GetFullPath();
		fn.SetExt("png");
		wxString optfile = fn.GetFullPath();
		entry->exportFile(pngfile.ToStdString());

		wxString command = path_pngout + " /y \"" + pngfile + "\" \"" + optfile + "\"";
		output.Empty();
		errors.Empty();
		wxExecute(command, output, errors, wxEXEC_SYNC);

		if (wxFileExists(optfile))
		{
			if (optfile.size() < oldsize)
			{
				entry->importFile(optfile.ToStdString());
				wxRemoveFile(optfile);
				wxRemoveFile(pngfile);
			}
			else
				errormessages += "PNGout failed to reduce file size further.\n";
			outed = true;
		}
		else if (!crushed)
			// Don't treat it as an error if PNGout couldn't create a smaller file than PNGCrush
			errormessages += "PNGout failed to create optimized file.\n";
		outsize = entry->size();

		// send app output to console if wanted
		if (false)
		{
			wxString pngoutlog = "";
			if (errors.GetCount())
			{
				pngoutlog += "PNGOut error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					pngoutlog += errors[i] + "\n";
				errormessages += pngoutlog;
			}
			if (output.GetCount())
			{
				pngoutlog += "PNGOut output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					pngoutlog += output[i] + "\n";
			}
			log::info(1, pngoutlog);
		}
	}

	// Run deflopt
	if (!pngpathd.IsEmpty() && wxFileExists(pngpathd))
	{
		wxFileName fn(pngpathd);
		fn.SetExt("png");
		wxString pngfile = fn.GetFullPath();
		entry->exportFile(pngfile.ToStdString());

		wxString command = path_deflopt + " /sf \"" + pngfile + "\"";
		output.Empty();
		errors.Empty();
		wxExecute(command, output, errors, wxEXEC_SYNC);

		entry->importFile(pngfile.ToStdString());
		wxRemoveFile(pngfile);
		deflsize = entry->size();

		// send app output to console if wanted
		if (false)
		{
			wxString defloptlog = "";
			if (errors.GetCount())
			{
				defloptlog += "DeflOpt error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					defloptlog += errors[i] + "\n";
				errormessages += defloptlog;
			}
			if (output.GetCount())
			{
				defloptlog += "DeflOpt output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					defloptlog += output[i] + "\n";
			}
			log::info(1, defloptlog);
		}
	}
	output.Clear();
	errors.Clear();

	// Rewrite special chunks
	if (alphchunk || grabchunk)
	{
		MemChunk data(entry->data());

		if (alphchunk)
			gfx::pngSetalPh(data, true);
		if (grabchunk)
			gfx::setImageOffsets(data, grabchunk->x, grabchunk->y);

		entry->importMemChunk(data);
	}


	log::info(
		"PNG {} size {} =PNGCrush=> {} =PNGout=> {} =DeflOpt=> {} =+grAb/alPh=> {}",
		entry->name(),
		oldsize,
		crushsize,
		outsize,
		deflsize,
		entry->size());


	if (!crushed && !outed && !errormessages.IsEmpty())
	{
		ExtMessageDialog dlg(nullptr, "Optimizing Report");
		dlg.setMessage("The following issues were encountered while optimizing:");
		dlg.setExt(errormessages);
		dlg.ShowModal();

		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Converts ANIMATED data in [entry] to ANIMDEFS format, written to [animdata]
// -----------------------------------------------------------------------------
bool entryoperations::convertAnimated(const ArchiveEntry* entry, MemChunk* animdata, bool animdefs)
{
	auto                 cursor = entry->rawData();
	auto                 eodata = cursor + entry->size();
	const AnimatedEntry* animation;
	wxString             conversion;
	int                  lasttype = -1;

	while (cursor < eodata && *cursor != animtype::STOP)
	{
		// reads an entry
		if (cursor + sizeof(AnimatedEntry) > eodata)
		{
			log::error(1, "ANIMATED entry is corrupt");
			return false;
		}
		animation = reinterpret_cast<const AnimatedEntry*>(cursor);
		cursor += sizeof(AnimatedEntry);

		// Create animation string
		if (animdefs)
		{
			conversion = wxString::Format(
				"%s\tOptional\t%-8s\tRange\t%-8s\tTics %i%s",
				(animation->type ? "Texture" : "Flat"),
				animation->first,
				animation->last,
				animation->speed,
				(animation->type == animtype::DECALS ? " AllowDecals\n" : "\n"));
		}
		else
		{
			if ((animation->type > 1 ? 1 : animation->type) != lasttype)
			{
				conversion = wxString::Format(
					"#animated %s, spd is number of frames between changes\n"
					"[%s]\n#spd    last        first\n",
					animation->type ? "textures" : "flats",
					animation->type ? "TEXTURES" : "FLATS");
				lasttype = animation->type;
				if (lasttype > 1)
					lasttype = 1;
				animdata->reSize(animdata->size() + conversion.length(), true);
				animdata->write(conversion.data(), conversion.length());
			}
			conversion = wxString::Format("%-8d%-12s%-12s\n", animation->speed, animation->last, animation->first);
		}

		// Write string to animdata
		animdata->reSize(animdata->size() + conversion.length(), true);
		animdata->write(conversion.data(), conversion.length());
	}
	return true;
}

// -----------------------------------------------------------------------------
// Converts SWITCHES data in [entry] to ANIMDEFS format, written to [animdata]
// -----------------------------------------------------------------------------
bool entryoperations::convertSwitches(const ArchiveEntry* entry, MemChunk* animdata, bool animdefs)
{
	auto                 cursor = entry->rawData();
	auto                 eodata = cursor + entry->size();
	const SwitchesEntry* switches;
	wxString             conversion;

	if (!animdefs)
	{
		conversion =
			"#switches usable with each IWAD, 1=SW, 2=registered DOOM, 3=DOOM2\n"
			"[SWITCHES]\n#epi    texture1        texture2\n";
		animdata->reSize(animdata->size() + conversion.length(), true);
		animdata->write(conversion.data(), conversion.length());
	}

	while (cursor < eodata && *cursor != switchtype::STOP)
	{
		// reads an entry
		if (cursor + sizeof(SwitchesEntry) > eodata)
		{
			log::error(1, "SWITCHES entry is corrupt");
			return false;
		}
		switches = reinterpret_cast<const SwitchesEntry*>(cursor);
		cursor += sizeof(SwitchesEntry);

		// Create animation string
		if (animdefs)
		{
			conversion = wxString::Format(
				"Switch\tDoom %d\t\t%-8s\tOn Pic\t%-8s\tTics 0\n", switches->type, switches->off, switches->on);
		}
		else
		{
			conversion = wxString::Format("%-8d%-12s%-12s\n", switches->type, switches->off, switches->on);
		}

		// Write string to animdata
		animdata->reSize(animdata->size() + conversion.length(), true);
		animdata->write(conversion.data(), conversion.length());
	}
	return true;
}

// -----------------------------------------------------------------------------
// Converts SWANTBLS data in [entry] to binary format, written to [animdata]
// -----------------------------------------------------------------------------
bool entryoperations::convertSwanTbls(const ArchiveEntry* entry, MemChunk* animdata, bool switches)
{
	Tokenizer tz(Tokenizer::Hash);
	tz.openMem(entry->data(), entry->name());

	wxString token;
	char     buffer[23];
	while ((token = tz.getToken()).length())
	{
		// Animated flats or textures
		if (!switches && (token == "[FLATS]" || token == "[TEXTURES]"))
		{
			bool texture = token == "[TEXTURES]";
			do
			{
				int      speed = tz.getInteger();
				wxString last  = tz.getToken();
				wxString first = tz.getToken();
				if (last.length() > 8)
				{
					log::error(wxString::Format(
						"String %s is too long for an animated %s name!", last, (texture ? "texture" : "flat")));
					return false;
				}
				if (first.length() > 8)
				{
					log::error(wxString::Format(
						"String %s is too long for an animated %s name!", first, (texture ? "texture" : "flat")));
					return false;
				}

				// reset buffer
				int limit;
				memset(buffer, 0, 23);

				// Write animation type
				buffer[0] = texture;

				// Write last texture name
				limit = std::min<int>(8, last.length());
				for (int a = 0; a < limit; ++a)
					buffer[1 + a] = last[a];

				// Write first texture name
				limit = std::min<int>(8, first.length());
				for (int a = 0; a < limit; ++a)
					buffer[10 + a] = first[a];

				// Write animation duration
				buffer[19] = static_cast<uint8_t>(speed % 256);
				buffer[20] = static_cast<uint8_t>(speed >> 8) % 256;
				buffer[21] = static_cast<uint8_t>(speed >> 16) % 256;
				buffer[22] = static_cast<uint8_t>(speed >> 24) % 256;

				// Save buffer to MemChunk
				if (!animdata->reSize(animdata->size() + 23, true))
					return false;
				if (!animdata->write(buffer, 23))
					return false;

				// Look for possible end of loop
				token = tz.peekToken();
			} while (token.length() && token[0] != '[');
		}

		// Switches
		else if (switches && token == "[SWITCHES]")
		{
			do
			{
				int      type = tz.getInteger();
				wxString off  = tz.getToken();
				wxString on   = tz.getToken();
				if (off.length() > 8)
				{
					log::error(wxString::Format("String %s is too long for a switch name!", off));
					return false;
				}
				if (on.length() > 8)
				{
					log::error(wxString::Format("String %s is too long for a switch name!", on));
					return false;
				}

				// reset buffer
				int limit;
				memset(buffer, 0, 20);

				// Write off texture name
				limit = std::min<int>(8, off.length());
				for (int a = 0; a < limit; ++a)
					buffer[0 + a] = off[a];

				// Write first texture name
				limit = std::min<int>(8, on.length());
				for (int a = 0; a < limit; ++a)
					buffer[9 + a] = on[a];

				// Write switch type
				buffer[18] = static_cast<uint8_t>(type % 256);
				buffer[19] = static_cast<uint8_t>(type >> 8) % 256;

				// Save buffer to MemChunk
				if (!animdata->reSize(animdata->size() + 20, true))
					return false;
				if (!animdata->write(buffer, 20))
					return false;

				// Look for possible end of loop
				token = tz.peekToken();
			} while (token.length() && token[0] != '[');
		}
	}
	return true;
	// Note that we do not terminate the list here!
}


void fixpngsrc(ArchiveEntry* entry)
{
	if (!entry)
		return;
	auto            source = entry->rawData();
	vector<uint8_t> data(entry->size());
	memcpy(data.data(), source, entry->size());

	// Last check that it's a PNG
	uint32_t header1 = memory::readB32(data.data(), 0);
	uint32_t header2 = memory::readB32(data.data(), 4);
	if (header1 != 0x89504E47 || header2 != 0x0D0A1A0A)
		return;

	// Loop through each chunk and recompute CRC
	uint32_t pointer      = 8;
	bool     neededchange = false;
	while (pointer < entry->size())
	{
		if (pointer + 12 > entry->size())
		{
			log::error(wxString::Format("Entry %s cannot be repaired.", entry->name()));
			return;
		}
		uint32_t chsz = memory::readB32(data.data(), pointer);
		if (pointer + 12 + chsz > entry->size())
		{
			log::error(wxString::Format("Entry %s cannot be repaired.", entry->name()));
			return;
		}
		uint32_t crc = misc::crc(data.data() + pointer + 4, 4 + chsz);
		if (crc != memory::readB32(data.data(), pointer + 8 + chsz))
		{
			log::error(wxString::Format(
				"Chunk %c%c%c%c has bad CRC",
				data[pointer + 4],
				data[pointer + 5],
				data[pointer + 6],
				data[pointer + 7]));
			neededchange              = true;
			data[pointer + 8 + chsz]  = crc >> 24;
			data[pointer + 9 + chsz]  = (crc & 0x00ffffff) >> 16;
			data[pointer + 10 + chsz] = (crc & 0x0000ffff) >> 8;
			data[pointer + 11 + chsz] = (crc & 0x000000ff);
		}
		pointer += (chsz + 12);
	}
	// Import new data with fixed CRC
	if (neededchange)
	{
		entry->importMem(data.data(), entry->size());
	}
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

CONSOLE_COMMAND(fixpngcrc, 0, true)
{
	auto selection = maineditor::currentEntrySelection();
	if (selection.empty())
	{
		log::info(1, "No entry selected");
		return;
	}
	for (auto& entry : selection)
	{
		if (entry->type()->formatId() == "img_png")
			fixpngsrc(entry);
	}
}
