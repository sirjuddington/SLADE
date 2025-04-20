
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
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
#include "Archive/ArchiveManager.h"
#include "Archive/EntryType/EntryDataFormat.h"
#include "Archive/Formats/WadArchive.h"
#include "BinaryControlLump.h"
#include "General/Clipboard.h"
#include "General/Console.h"
#include "General/Misc.h"
#include "GfxOffsetsClipboardItem.h"
#include "Graphics/Graphics.h"
#include "Graphics/SImage/SIFormat.h"
#include "MainEditor/MainEditor.h"
#include "SLADEWxApp.h"
#include "UI/Dialogs/ExtMessageDialog.h"
#include "UI/Dialogs/Preferences/PreferencesDialog.h"
#include "UI/TextureXEditor/TextureXEditor.h"
#include "Utility/FileMonitor.h"
#include "Utility/FileUtils.h"
#include "Utility/Memory.h"
#include "Utility/SFileDialog.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, path_acc, "", CVar::Flag::Save);
CVAR(String, path_acc_libs, "", CVar::Flag::Save);
CVAR(String, path_java, "", CVar::Flag::Save);
CVAR(String, path_decohack, "", CVar::Flag::Save);
CVAR(String, path_pngout, "", CVar::Flag::Save);
CVAR(String, path_pngcrush, "", CVar::Flag::Save);
CVAR(String, path_deflopt, "", CVar::Flag::Save);
CVAR(String, path_db2, "", CVar::Flag::Save)
CVAR(Bool, acc_always_show_output, false, CVar::Flag::Save);
CVAR(Bool, decohack_always_show_output, false, CVar::Flag::Save);


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
			auto new_name = wxGetTextFromUser(
								wxS("Enter new entry name:"), wxS("Rename"), wxString::FromUTF8(entry->name()))
								.utf8_string();

			// Rename entry (if needed)
			if (!new_name.empty() && entry->name() != new_name)
			{
				if (!archive->renameEntry(entry, new_name))
					wxMessageBox(
						WX_FMT("Unable to rename entry {}: {}", entry->name(), global::error),
						wxS("Rename Entry"),
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
							wxS("Enter new entry name: (* = unchanged, ^ = alphabet letter, ^^ = lower case\n% = "
								"alphabet repeat number, & = entry number, %% or && = n-1)"),
							wxS("Rename"),
							wxString::FromUTF8(filter))
							.utf8_string();

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
							wxString::FromUTF8(
								fmt::format("Unable to rename entry {}: {}", entries[a]->name(), global::error)),
							wxS("Rename Entry"),
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
							wxS("Enter new directory name:"),
							WX_FMT("Rename Directory {}", old_name),
							wxString::FromUTF8(old_name))
							.utf8_string();

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
	auto          name = misc::lumpNameToFileName(entry->name());
	strutil::Path fn(name);

	// Add appropriate extension if needed
	if (!fn.hasExtension())
		fn.setExtension(entry->type()->extension());

	// Run save file dialog
	filedialog::FDInfo info;
	if (filedialog::saveFile(
			info, "Export Entry \"" + entry->name() + "\"", "Any File (*.*)|*", maineditor::windowWx(), fn.fullPath()))
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
			info, "Export Multiple Entries (Filename is ignored)", "Any File (*.*)|*", maineditor::windowWx()))
	{
		// Go through the selected entries
		for (auto& entry : entries)
		{
			// Setup entry filename
			strutil::Path fn(entry->name());
			fn.setPath(info.path);

			// Add file extension if it doesn't exist
			if (!fn.hasExtension())
			{
				fn.setExtension(entry->type()->extension());

				// ...unless a file already exists with said extension
				if (fileutil::fileExists(fn.fullPath()))
					fn.setExtension("");
			}

			// Do export
			entry->exportFile(fn.fullPath());
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

	if (path.empty())
	{
		// Check for DB2 location registry key
		wxRegKey key(wxRegKey::HKLM, wxS("SOFTWARE\\CodeImp\\Doom Builder"));
		key.QueryValue(wxS("Location"), path);

		// Browse for executable if DB2 isn't installed
		if (path.IsEmpty())
		{
			wxMessageBox(
				wxS("Could not find the installation directory of Doom Builder 2, please browse for the DB2 "
					"executable"),
				wxS("Doom Builder 2 Not Found"));

			filedialog::FDInfo info;
			if (filedialog::openFile(
					info, "Browse for DB2 Executable", filedialog::executableExtensionString(), nullptr, "Builder.exe"))
			{
				path_db2 = info.filenames[0];
				path     = wxString::FromUTF8(info.filenames[0]);
			}
			else
				return false;
		}
		else
		{
			// Add default executable name
			path += wxS("\\Builder.exe");
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
	auto cmd = fmt::format("{} \"{}\" -map {}", path.utf8_string(), filename, entry->name());

	// Add base resource archive to command line
	auto base = app::archiveManager().baseResourceArchive();
	if (base)
	{
		if (base->formatId() == "wad")
			cmd += fmt::format(" -resource wad \"{}\"", base->filename());
		else if (base->formatId() == "zip")
			cmd += fmt::format(" -resource pk3 \"{}\"", base->filename());
	}

	// Add resource archives to command line
	for (int a = 0; a < app::archiveManager().numArchives(); ++a)
	{
		auto archive = app::archiveManager().getArchive(a);

		// Check archive type (only wad and zip supported by db2)
		if (archive->formatId() == "wad")
			cmd += fmt::format(" -resource wad \"{}\"", archive->filename());
		else if (archive->formatId() == "zip")
			cmd += fmt::format(" -resource pk3 \"{}\"", archive->filename());
	}

	// Run DB2
	FileMonitor* fm = new DB2MapFileMonitor(filename, entry->parent(), string{ entry->nameNoExt() });
	wxExecute(wxString::FromUTF8(cmd), wxEXEC_ASYNC, fm->process());

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
	Archive::SearchOptions opt;
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
			wxMessageBox(wxS("Cannot perform this action on an IWAD"), wxS("Error"), wxICON_ERROR);
		else
			wxMessageBox(
				wxS("Cannot perform this action because one or more texture related entries is locked. Please close "
					"the archive's texture editor if it is open."),
				wxS("Error"),
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
	Archive::SearchOptions opt;
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
			wxMessageBox(wxS("Cannot perform this action on an IWAD"), wxS("Error"), wxICON_ERROR);
		else
			wxMessageBox(
				wxS("Cannot perform this action because one or more texture related entries is locked. Please close "
					"the archive's texture editor if it is open."),
				wxS("Error"),
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
			log::error("Entry {} is not a valid image", entry->name());
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
	Archive::SearchOptions opt;
	opt.match_type = EntryType::fromId("pnames");
	auto pnames    = parent->findLast(opt);

	// Check it exists
	if (!pnames)
	{
		wxMessageBox(
			wxS("Unable to convert - could not find PNAMES entry"), wxS("Convert to TEXTURES"), wxICON_ERROR | wxOK);
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
	Archive::SearchOptions opt;
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
		wxS("Don't run this on TEXTURE entries unless your wad/archive is intended for newer more advanced source "
			"ports like GZDoom. The newer source ports can still properly access iwad textures if you don't include "
			"their entries in a pwad. However, older engines may rely on all of the iwad TEXTUREs being redefined in a "
			"pwad to work correctly. You should have nothing to worry about for ZDoom Format TEXTURES files."),
		wxS("Remove duplicate texture entries."),
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

	Archive::SearchOptions opt;
	opt.match_type  = EntryType::fromId("pnames");
	auto bra_pnames = bra->findLast(opt);

	// Load patch table
	PatchTable bra_ptable;
	if (bra_pnames)
	{
		bra_ptable.loadPNAMES(bra_pnames);

		// Load all Texturex entries
		Archive::SearchOptions texturexopt;
		texturexopt.match_type = EntryType::fromId("texturex");

		for (ArchiveEntry* texturexentry : bra->findAll(texturexopt))
		{
			bra_tx_list.readTEXTUREXData(texturexentry, bra_ptable, true);
		}
	}

	// Load all zdtextures entries
	Archive::SearchOptions zdtexturesopt;
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
				log::info("Cleaning duplicate entries from TEXTUREx entry {}.", entry->name());
			}
			else
			{
				log::error("Skipping cleaning TEXTUREx entry {} since this archive has no patch table.", entry->name());
				// Skip cleaning this texturex entry if there is no patch table for us to load it with
				continue;
			}
		}
		else if (entry->type()->id() == "zdtextures")
		{
			tx.readTEXTURESData(entry);
			log::info("Cleaning duplicate entries from ZDoom TEXTURES entry {}.", entry->name());
		}

		if (tx.removeDupesFoundIn(bra_tx_list))
		{
			log::info("Cleaned entries from: {}.", entry->name());

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
				log::info("{} no longer has any entries so deleting it.", entry->name());
			}

			ret = true;
		}
		else
		{
			log::info("Found no entries to clean from: {}.", entry->name());
		}
	}

	if (ret)
	{
		wxMessageBox(
			wxS("Found duplicate texture entries to remove. Check the console for output info. The PATCH table was "
				"left untouched. You can either delete it or clean it using the Remove Unused Patches tool."),
			wxS("Remove duplicate texture entries."),
			wxOK | wxCENTER | wxICON_INFORMATION);
	}
	else
	{
		wxMessageBox(
			wxS("Didn't find any duplicate texture entries to remove. Check the console for output info."),
			wxS("Remove duplicate texture entries."),
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
			wxS("This will remove all textures that are made out of a basic single patch from this textures entry. It "
				"will also rename all of the patches to the texture name and move them from the patches to the "
				"textures folder."),
			wxS("Clean single patch texture entries."),
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
			wxS("This currently only works with archives that support directories"),
			wxS("Clean single patch texture entries."),
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
			log::info("Cleaned entries from: {}.", entry->name());

			if (tx.size())
			{
				tx.writeTEXTURESData(entry);
			}
			else
			{
				// If we emptied out the entry, just delete it
				parent->removeEntry(entry);
				log::info("{} no longer has any entries so deleting it.", entry->name());
			}

			ret = true;
		}
	}

	if (ret)
	{
		wxMessageBox(
			wxS("Found texture entries to clean. Check the console for output info."),
			wxS("Clean single patch texture entries."),
			wxOK | wxCENTER | wxICON_INFORMATION);
	}
	else
	{
		wxMessageBox(
			wxS("Didn't find any texture entries to clean. Check the console for output info."),
			wxS("Clean single patch texture entries."),
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
		wxMessageBox(wxS("Error: Entry does not appear to be text"), wxS("Error"), wxOK | wxCENTRE | wxICON_ERROR);
		return false;
	}

	// Check if the ACC path is set up
	if (path_acc.value.empty() || !fileutil::fileExists(path_acc))
	{
		wxMessageBox(
			wxS("Error: ACC path not defined, please configure in SLADE preferences"),
			wxS("Error"),
			wxOK | wxCENTRE | wxICON_ERROR);
		PreferencesDialog::openPreferences(parent, "ACS");
		return false;
	}

	// Setup some path strings
	auto srcfile       = app::path(fmt::format("{}.acs", entry->nameNoExt()), app::Dir::Temp);
	auto ofile         = app::path(fmt::format("{}.o", entry->nameNoExt()), app::Dir::Temp);
	auto include_paths = strutil::splitV(path_acc_libs, ';');

	// Setup command options
	string opt;
	if (hexen)
		opt += " -h";
	if (!include_paths.empty())
	{
		for (const auto& include_path : include_paths)
			opt += fmt::format(" -i \"{}\"", include_path);
	}

	// Find/export any resource libraries
	Archive::SearchOptions sopt;
	sopt.match_type        = EntryType::fromId("acs");
	sopt.search_subdirs    = true;
	auto           entries = app::archiveManager().findAllResourceEntries(sopt);
	vector<string> lib_paths;
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
		lib_paths.push_back(path);
		log::info(2, "Exporting ACS library {}", res_entry->name());
	}

	// Export script to file
	entry->exportFile(srcfile);

	// Execute acc
	string        command = "\"" + path_acc.value + "\"" + " " + opt + " \"" + srcfile + "\" \"" + ofile + "\"";
	wxArrayString output;
	wxArrayString errout;
	wxGetApp().SetTopWindow(parent);
	wxExecute(wxString::FromUTF8(command), output, errout, wxEXEC_SYNC);
	wxGetApp().SetTopWindow(maineditor::windowWx());

	// Log output
	log::console("ACS compiler output:");
	string output_log;
	if (!output.IsEmpty())
	{
		const char* title1 = "=== Log: ===\n";
		log::console(title1);
		output_log += title1;
		for (const auto& line : output)
		{
			log::console(line.utf8_string());
			output_log += line.utf8_string();
		}
	}

	if (!errout.IsEmpty())
	{
		const char* title2 = "\n=== Error log: ===\n";
		log::console(title2);
		output_log += title2;
		for (const auto& line : errout)
		{
			log::console(line.utf8_string());
			output_log += line.utf8_string() + "\n";
		}
	}

	// Delete source file
	fileutil::removeFile(srcfile);

	// Delete library files
	for (const auto& lib_path : lib_paths)
		fileutil::removeFile(lib_path);

	// Check it compiled successfully
	bool success = fileutil::fileExists(ofile);
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
				Archive::SearchOptions opt;
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
		fileutil::removeFile(ofile);
	}

	if (!success || acc_always_show_output)
	{
		string errors;
		auto   path = app::path("acs.err", app::Dir::Temp);
		if (fileutil::fileExists(path))
		{
			// Read acs.err to string
			SFile file(path);
			file.read(errors, file.size());
		}
		else
			errors = output_log;

		if (!errors.empty() || !success)
		{
			ExtMessageDialog dlg(nullptr, success ? "ACC Output" : "Error Compiling");
			dlg.setMessage(
				success ? "The following errors were encountered while compiling, please fix them and recompile:"
						: "Compiler output shown below: ");
			dlg.setExt(errors);
			dlg.ShowModal();
		}

		return success;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Attempts to compile [entry] as DECOHack.
// -----------------------------------------------------------------------------
bool entryoperations::compileDECOHack(ArchiveEntry* entry, ArchiveEntry* target, wxFrame* parent)
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
		wxMessageBox(wxS("Error: Entry does not appear to be text"), wxS("Error"), wxOK | wxCENTRE | wxICON_ERROR);
		return false;
	}

	// Check if the DoomTools path is set up
	if (path_decohack.empty() || !fileutil::fileExists(path_decohack))
	{
		wxMessageBox(
			wxS("Error: DoomTools path not defined, please configure in SLADE preferences"),
			wxS("Error"),
			wxOK | wxCENTRE | wxICON_ERROR);
		PreferencesDialog::openPreferences(parent, "DECOHack");
		return false;
	}

	// Check if the Java path is set up
	if (path_java.empty() || !fileutil::fileExists(path_java))
	{
		wxMessageBox(
			wxS("Error: Java path not defined, please configure in SLADE preferences"),
			wxS("Error"),
			wxOK | wxCENTRE | wxICON_ERROR);
		PreferencesDialog::openPreferences(parent, "DECOHack");
		return false;
	}

	// Setup some path strings
	auto srcfile = app::path(fmt::format("{}.dh", entry->nameNoExt()), app::Dir::Temp);
	auto dehfile = app::path(fmt::format("{}.deh", entry->nameNoExt()), app::Dir::Temp);

	// Find/export any resource libraries
	Archive::SearchOptions sopt;
	sopt.match_type        = EntryType::fromId("decohack");
	sopt.search_subdirs    = true;
	auto           entries = app::archiveManager().findAllResourceEntries(sopt);
	vector<string> lib_paths;
	for (auto& res_entry : entries)
	{
		// Ignore entries from other archives
		if (archive && (archive->filename(true) != res_entry->parent()->filename(true)))
			continue;

		auto path = app::path(fmt::format("{}.dh", res_entry->nameNoExt()), app::Dir::Temp);
		res_entry->exportFile(path);
		lib_paths.push_back(path);
		log::info(2, "Exporting DECOHack file {}", res_entry->name());
	}

	// Export script to file
	entry->exportFile(srcfile);

	// Execute DECOHack
	auto command = fmt::format(
		R"("{}" -cp "{}" -Xms64M -Xmx1G net.mtrop.doom.tools.DecoHackMain "{}" -o "{}")",
		path_java.value,
		path_decohack.value,
		srcfile,
		dehfile);
	wxArrayString output;
	wxArrayString errout;
	wxGetApp().SetTopWindow(parent);
	wxExecute(wxString::FromUTF8(command), output, errout, wxEXEC_SYNC);
	wxGetApp().SetTopWindow(maineditor::windowWx());

	// Log output
	log::console("DECOHack compiler output:");
	string output_log;
	if (!output.IsEmpty())
	{
		const char* title1 = "=== Log: ===\n";
		log::console(title1);
		output_log += title1;
		for (const auto& line : output)
		{
			log::console(line.utf8_string());
			output_log += line.utf8_string();
		}
	}

	if (!errout.IsEmpty())
	{
		const char* title2 = "\n=== Error log: ===\n";
		log::console(title2);
		output_log += title2;
		for (const auto& line : errout)
		{
			log::console(line.utf8_string());
			output_log += line.utf8_string() + "\n";
		}
	}

	// Delete source file
	fileutil::removeFile(srcfile);

	// Delete library files
	for (const auto& lib_path : lib_paths)
		fileutil::removeFile(lib_path);

	// Check it compiled successfully
	bool success = fileutil::fileExists(dehfile);
	if (success)
	{
		// If no target entry was given, find one
		if (!target)
		{
			// Get entry before DECOHACK
			auto prev = archive->entryAt(archive->entryIndex(entry) - 1);

			// Create a new entry there
			prev = archive->addNewEntry("DEHACKED", archive->entryIndex(entry)).get();

			// Import compiled dehacked
			prev->importFile(dehfile);
		}
		else
			target->importFile(dehfile);

		// Delete compiled script file
		fileutil::removeFile(dehfile);
	}

	if (!success || decohack_always_show_output)
	{
		auto errors = output_log;

		if (!errors.empty() || !success)
		{
			ExtMessageDialog dlg(nullptr, success ? "DECOHack Output" : "Error Compiling");
			dlg.setMessage(
				success ? "The following errors were encountered while compiling, please fix them and recompile:"
						: "Compiler output shown below: ");
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
bool entryoperations::exportAsPNG(ArchiveEntry* entry, const string& filename)
{
	// Check entry was given
	if (!entry)
		return false;

	// Create image from entry
	SImage image;
	if (!misc::loadImageFromEntry(&image, entry))
	{
		log::error("Error converting {}: {}", entry->name(), global::error);
		return false;
	}

	// Write png data
	MemChunk png;
	auto     fmt_png = SIFormat::getFormat("png");
	if (!fmt_png->saveImage(image, png, maineditor::currentPalette(entry)))
	{
		log::error("Error converting {}", entry->name());
		return false;
	}

	// Export file
	return png.exportFile(filename);
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
		wxMessageBox(wxS("Error: Entry does not appear to be PNG"), wxS("Error"), wxOK | wxCENTRE | wxICON_ERROR);
		return false;
	}

	// Check if the PNG tools path are set up, at least one of them should be
	string pngpathc = path_pngcrush;
	string pngpatho = path_pngout;
	string pngpathd = path_deflopt;
	if ((pngpathc.empty() || !fileutil::fileExists(pngpathc)) && (pngpatho.empty() || !fileutil::fileExists(pngpatho))
		&& (pngpathd.empty() || !fileutil::fileExists(pngpathd)))
	{
		log::error(1, "PNG tool paths not defined or invalid, no optimization done.");
		return false;
	}

	// Save special chunks
	bool          alphchunk     = gfx::pngGetalPh(entry->data());
	auto          grabchunk     = gfx::getImageOffsets(entry->data());
	string        errormessages = "";
	wxArrayString output;
	wxArrayString errors;
	size_t        oldsize   = entry->size();
	size_t        crushsize = 0, outsize = 0, deflsize = 0;
	bool          crushed = false, outed = false;

	// Run PNGCrush
	if (!pngpathc.empty() && fileutil::fileExists(pngpathc))
	{
		strutil::Path fn(pngpathc);
		fn.setExtension("opt");
		string pngfile = fn.fullPath();
		fn.setExtension("png");
		string optfile = fn.fullPath();
		entry->exportFile(pngfile);

		string command = path_pngcrush.value + " -brute \"" + pngfile + "\" \"" + optfile + "\"";
		output.Empty();
		errors.Empty();
		wxExecute(wxString::FromUTF8(command), output, errors, wxEXEC_SYNC);

		if (fileutil::fileExists(optfile))
		{
			if (optfile.size() < oldsize)
			{
				entry->importFile(optfile);
				fileutil::removeFile(optfile);
				fileutil::removeFile(pngfile);
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
			string crushlog;
			if (errors.GetCount())
			{
				crushlog += "PNGCrush error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					crushlog += errors[i].utf8_string() + "\n";
				errormessages += crushlog;
			}
			if (output.GetCount())
			{
				crushlog += "PNGCrush output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					crushlog += output[i].utf8_string() + "\n";
			}
			log::info(1, crushlog);
		}
	}

	// Run PNGOut
	if (!pngpatho.empty() && fileutil::fileExists(pngpatho))
	{
		strutil::Path fn(pngpatho);
		fn.setExtension("opt");
		string pngfile = fn.fullPath();
		fn.setExtension("png");
		string optfile = fn.fullPath();
		entry->exportFile(pngfile);

		string command = path_pngout.value + " /y \"" + pngfile + "\" \"" + optfile + "\"";
		output.Empty();
		errors.Empty();
		wxExecute(wxString::FromUTF8(command), output, errors, wxEXEC_SYNC);

		if (fileutil::fileExists(optfile))
		{
			if (optfile.size() < oldsize)
			{
				entry->importFile(optfile);
				fileutil::removeFile(optfile);
				fileutil::removeFile(pngfile);
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
			string pngoutlog;
			if (errors.GetCount())
			{
				pngoutlog += "PNGOut error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					pngoutlog += errors[i].utf8_string() + "\n";
				errormessages += pngoutlog;
			}
			if (output.GetCount())
			{
				pngoutlog += "PNGOut output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					pngoutlog += output[i].utf8_string() + "\n";
			}
			log::info(1, pngoutlog);
		}
	}

	// Run deflopt
	if (!pngpathd.empty() && fileutil::fileExists(pngpathd))
	{
		strutil::Path fn(pngpathd);
		fn.setExtension("png");
		string pngfile = fn.fullPath();
		entry->exportFile(pngfile);

		string command = path_deflopt.value + " /sf \"" + pngfile + "\"";
		output.Empty();
		errors.Empty();
		wxExecute(wxString::FromUTF8(command), output, errors, wxEXEC_SYNC);

		entry->importFile(pngfile);
		fileutil::removeFile(pngfile);
		deflsize = entry->size();

		// send app output to console if wanted
		if (false)
		{
			string defloptlog;
			if (errors.GetCount())
			{
				defloptlog += "DeflOpt error messages:\n";
				for (size_t i = 0; i < errors.GetCount(); ++i)
					defloptlog += errors[i].utf8_string() + "\n";
				errormessages += defloptlog;
			}
			if (output.GetCount())
			{
				defloptlog += "DeflOpt output messages:\n";
				for (size_t i = 0; i < output.GetCount(); ++i)
					defloptlog += output[i].utf8_string() + "\n";
			}
			log::info(1, defloptlog);
		}
	}
	output.Clear();
	errors.Clear();

	// Rewrite special chunks
	if (alphchunk)
		gfx::pngSetalPh(entry->data(), true);
	if (grabchunk)
		gfx::setImageOffsets(entry->data(), grabchunk->x, grabchunk->y);

	log::info(
		"PNG {} size {} =PNGCrush=> {} =PNGout=> {} =DeflOpt=> {} =+grAb/alPh=> {}",
		entry->name(),
		oldsize,
		crushsize,
		outsize,
		deflsize,
		entry->size());


	if (!crushed && !outed && !errormessages.empty())
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
// Copies the offsets from the given entry to the clipboard
// -----------------------------------------------------------------------------
bool entryoperations::copyGfxOffsets(ArchiveEntry& entry)
{
	// Check entry type
	if (!gfx::supportsOffsets(entry))
	{
		log::error(R"(Entry "{}" is of type "{}" which does not support offsets)", entry.name(), entry.type()->name());
		return false;
	}

	// Get offsets
	auto offsets = gfx::getImageOffsets(entry.data());
	if (!offsets)
	{
		log::error("Entry \"{}\" has no offsets to copy", entry.name());
		return false;
	}

	// Add/update offsets on clipboard
	GfxOffsetsClipboardItem* offset_item = nullptr;
	for (auto i = 0; i < app::clipboard().size(); ++i)
	{
		auto item = app::clipboard().item(i);
		if (item->type() == ClipboardItem::Type::GfxOffsets)
		{
			offset_item = dynamic_cast<GfxOffsetsClipboardItem*>(item);
			break;
		}
	}
	if (offset_item)
		offset_item->setOffsets(*offsets);
	else
		app::clipboard().add(std::make_unique<GfxOffsetsClipboardItem>(*offsets));

	return true;
}

// -----------------------------------------------------------------------------
// Pastes the offsets from the clipboard to the given entries
// -----------------------------------------------------------------------------
bool entryoperations::pasteGfxOffsets(const vector<ArchiveEntry*>& entries)
{
	// Check for copied offsets
	auto offset_item = app::clipboard().firstItem(ClipboardItem::Type::GfxOffsets);
	if (!offset_item)
		return false;

	auto offsets = dynamic_cast<GfxOffsetsClipboardItem*>(offset_item)->offsets();

	for (auto entry : entries)
	{
		// Check entry is gfx that supports offsets
		if (!gfx::supportsOffsets(*entry))
		{
			log::warning(
				R"(Entry "{}" is of type "{}" which does not support offsets)", entry->name(), entry->type()->name());
			continue;
		}

		// Set offsets in gfx entry
		MemChunk data(entry->rawData(), entry->size());
		gfx::setImageOffsets(data, offsets.x, offsets.y);
		entry->importMemChunk(data);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Converts ANIMATED data in [entry] to ANIMDEFS format, written to [animdata]
// -----------------------------------------------------------------------------
bool entryoperations::convertAnimated(ArchiveEntry* entry, MemChunk* animdata, bool animdefs)
{
	auto                 cursor = entry->rawData(true);
	auto                 eodata = cursor + entry->size();
	const AnimatedEntry* animation;
	string               conversion;
	int                  lasttype = -1;

	while (cursor < eodata && *cursor != animtype::STOP)
	{
		// reads an entry
		if (cursor + sizeof(AnimatedEntry) > eodata)
		{
			log::error(1, "ANIMATED entry is corrupt");
			return false;
		}
		animation = (AnimatedEntry*)cursor;
		cursor += sizeof(AnimatedEntry);

		// Create animation string
		if (animdefs)
		{
			conversion = fmt::format(
				"{}\tOptional\t{:<8}\tRange\t{:<8}\tTics {}{}",
				animation->type ? "Texture" : "Flat",
				animation->first,
				animation->last,
				animation->speed,
				animation->type == animtype::DECALS ? " AllowDecals\n" : "\n");
		}
		else
		{
			if ((animation->type > 1 ? 1 : animation->type) != lasttype)
			{
				conversion = fmt::format(
					"#animated {}, spd is number of frames between changes\n"
					"[{}]\n#spd    last        first\n",
					animation->type ? "textures" : "flats",
					animation->type ? "TEXTURES" : "FLATS");
				lasttype = animation->type;
				if (lasttype > 1)
					lasttype = 1;
				animdata->reSize(animdata->size() + conversion.length(), true);
				animdata->write(conversion.data(), conversion.length());
			}
			conversion = fmt::format("{:<8d}{:<12}{:<12}\n", animation->speed, animation->last, animation->first);
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
bool entryoperations::convertSwitches(ArchiveEntry* entry, MemChunk* animdata, bool animdefs)
{
	auto                 cursor = entry->rawData(true);
	auto                 eodata = cursor + entry->size();
	const SwitchesEntry* switches;
	string               conversion;

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
		switches = (SwitchesEntry*)cursor;
		cursor += sizeof(SwitchesEntry);

		// Create animation string
		if (animdefs)
		{
			conversion = fmt::format(
				"Switch\tDoom {}\t\t{:<8}\tOn Pic\t{:<8}\tTics 0\n", switches->type, switches->off, switches->on);
		}
		else
		{
			conversion = fmt::format("{:<8}{:<12}{:<12}\n", switches->type, switches->off, switches->on);
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
bool entryoperations::convertSwanTbls(ArchiveEntry* entry, MemChunk* animdata, bool switches)
{
	Tokenizer tz(Tokenizer::Hash);
	tz.openMem(entry->data(), entry->name());

	string token;
	char   buffer[23];
	while ((token = tz.getToken()).length())
	{
		// Animated flats or textures
		if (!switches && (token == "[FLATS]" || token == "[TEXTURES]"))
		{
			bool texture = token == "[TEXTURES]";
			do
			{
				int    speed = tz.getInteger();
				string last  = tz.getToken();
				string first = tz.getToken();
				if (last.length() > 8)
				{
					log::error("String {} is too long for an animated {} name!", last, texture ? "texture" : "flat");
					return false;
				}
				if (first.length() > 8)
				{
					log::error("String {} is too long for an animated {} name!", first, texture ? "texture" : "flat");
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
				buffer[19] = (uint8_t)(speed % 256);
				buffer[20] = (uint8_t)(speed >> 8) % 256;
				buffer[21] = (uint8_t)(speed >> 16) % 256;
				buffer[22] = (uint8_t)(speed >> 24) % 256;

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
				int    type = tz.getInteger();
				string off  = tz.getToken();
				string on   = tz.getToken();
				if (off.length() > 8)
				{
					log::error("String {} is too long for a switch name!", off);
					return false;
				}
				if (on.length() > 8)
				{
					log::error("String {} is too long for a switch name!", on);
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
				buffer[18] = (uint8_t)(type % 256);
				buffer[19] = (uint8_t)(type >> 8) % 256;

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
			log::error("Entry {} cannot be repaired.", entry->name());
			return;
		}
		uint32_t chsz = memory::readB32(data.data(), pointer);
		if (pointer + 12 + chsz > entry->size())
		{
			log::error("Entry {} cannot be repaired.", entry->name());
			return;
		}
		uint32_t crc = misc::crc(data.data() + pointer + 4, 4 + chsz);
		if (crc != memory::readB32(data.data(), pointer + 8 + chsz))
		{
			log::error(
				"Chunk {}{}{}{} has bad CRC",
				data[pointer + 4],
				data[pointer + 5],
				data[pointer + 6],
				data[pointer + 7]);
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
