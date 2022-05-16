
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ZipArchive.cpp
// Description: ZipArchive, archive class to handle zip format archives
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
#include "ZipArchive.h"
#include "App.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#include "WadArchive.h"
#include <fstream>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, zip_allow_duplicate_names, false, CVar::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, max_entry_size_mb)


// -----------------------------------------------------------------------------
//
// ZipArchive Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ZipArchive class constructor
// -----------------------------------------------------------------------------
ZipArchive::ZipArchive() : Archive("zip")
{
	if (zip_allow_duplicate_names)
		rootDir()->allowDuplicateNames(true);
}

// -----------------------------------------------------------------------------
// ZipArchive class destructor
// -----------------------------------------------------------------------------
ZipArchive::~ZipArchive()
{
	if (fileutil::fileExists(temp_file_))
		fileutil::removeFile(temp_file_);
}

// -----------------------------------------------------------------------------
// Reads zip data from a file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ZipArchive::open(string_view filename)
{
	// Check the file exists
	if (!fileutil::fileExists(filename))
	{
		global::error = "File does not exist";
		return false;
	}

	// Copy the zip to a temp file (for use when saving)
	generateTempFileName(filename);
	fileutil::copyFile(filename, temp_file_);

	// Open the file
	wxFFileInputStream in(wxutil::strFromView(filename));
	if (!in.IsOk())
	{
		global::error = "Unable to open file";
		return false;
	}

	// Create zip stream
	wxZipInputStream zip(in);
	if (!zip.IsOk())
	{
		global::error = "Invalid zip file";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	const ArchiveModSignalBlocker sig_blocker{ *this };

	// Go through all zip entries
	int  entry_index = 0;
	auto zip_entry   = zip.GetNextEntry();
	ui::setSplashProgressMessage("Reading zip data");
	while (zip_entry)
	{
		ui::setSplashProgress(-1.0f);
		if (zip_entry->GetMethod() != wxZIP_METHOD_DEFLATE && zip_entry->GetMethod() != wxZIP_METHOD_STORE)
		{
			global::error = "Unsupported zip compression method";
			return false;
		}

		if (!zip_entry->IsDir())
		{
			// Get the entry name as a Path (so we can break it up)
			strutil::Path fn(wxutil::strToView(zip_entry->GetName(wxPATH_UNIX)));

			// Create entry
			auto new_entry = std::make_shared<ArchiveEntry>(
				misc::fileNameToLumpName(fn.fileName()), zip_entry->GetSize());

			// Setup entry info
			new_entry->exProp("ZipIndex") = entry_index;

			// Add entry and directory to directory tree
			auto ndir = createDir(fn.path(true));
			ndir->addEntry(new_entry, true);

			if (const auto ze_size = zip_entry->GetSize(); ze_size < max_entry_size_mb * 1024 * 1024)
			{
				if (ze_size > 0)
				{
					vector<uint8_t> data(ze_size);
					zip.Read(data.data(), ze_size); // Note: this is where exceedingly large files cause an exception.
					new_entry->importMem(data.data(), static_cast<uint32_t>(ze_size));
				}

				// Determine its type
				EntryType::detectEntryType(*new_entry);
			}
			else
			{
				global::error = fmt::format("Entry too large: {} is {} mb", fn.fullPath(), ze_size / (1 << 20));
				return false;
			}
		}
		else
		{
			// Zip entry is a directory, add it to the directory tree
			strutil::Path fn(wxutil::strToView(zip_entry->GetName(wxPATH_UNIX)));
			createDir(fn.path(true));
		}

		// Go to next entry in the zip file
		delete zip_entry;
		zip_entry = zip.GetNextEntry();
		entry_index++;
	}
	ui::updateSplash();

	// Set all entries/directories to unmodified
	vector<ArchiveEntry*> entry_list;
	putEntryTreeAsList(entry_list);
	for (auto& entry : entry_list)
		entry->setState(ArchiveEntry::State::Unmodified);

	// Enable announcements
	sig_blocker.unblock();

	// Setup variables
	filename_      = filename;
	file_modified_ = fileutil::fileModifiedTime(filename);
	setModified(false);
	on_disk_ = true;

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads zip format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ZipArchive::open(MemChunk& mc)
{
	// Write the MemChunk to a temp file
	const auto tempfile = app::path("slade-temp-open.zip", app::Dir::Temp);
	mc.exportFile(tempfile);

	// Load the file
	const bool success = open(tempfile);

	// Clean up
	fileutil::removeFile(tempfile);

	return success;
}

// -----------------------------------------------------------------------------
// Writes the zip archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ZipArchive::write(MemChunk& mc)
{
	bool success = false;

	// Write to a temporary file
	const auto tempfile = app::path("slade-temp-write.zip", app::Dir::Temp);
	if (write(tempfile))
	{
		// Load file into MemChunk
		success = mc.importFile(tempfile);
	}

	// Clean up
	wxRemoveFile(tempfile);

	return success;
}

// -----------------------------------------------------------------------------
// Writes the zip archive to a file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ZipArchive::write(string_view filename)
{
	// Check for entries with duplicate names (not allowed for zips)
	auto all_dirs = rootDir()->allDirectories();
	all_dirs.insert(all_dirs.begin(), rootDir());
	for (const auto& dir : all_dirs)
	{
		if (auto* dup_entry = dir->findDuplicateEntryName())
		{
			global::error = fmt::format("Multiple entries named {} found in {}", dup_entry->name(), dup_entry->path());
			return false;
		}
	}

	// Open the file
	wxFFileOutputStream out(wxutil::strFromView(filename));
	if (!out.IsOk())
	{
		global::error = "Unable to open file for saving. Make sure it isn't in use by another program.";
		return false;
	}

	// Open as zip for writing
	wxZipOutputStream zip(out, 9);
	if (!zip.IsOk())
	{
		global::error = "Unable to create zip for saving";
		return false;
	}

	// Open old zip for copying, from the temp file that was copied on opening.
	// This is used to copy any entries that have been previously saved/compressed
	// and are unmodified, to greatly speed up zip file saving by not having to
	// recompress unchanged entries
	unique_ptr<wxZipInputStream>   inzip;
	unique_ptr<wxFFileInputStream> in;
	vector<wxZipEntry*>            c_entries;
	if (fileutil::fileExists(temp_file_))
	{
		in    = std::make_unique<wxFFileInputStream>(temp_file_);
		inzip = std::make_unique<wxZipInputStream>(*in);

		if (inzip->IsOk())
		{
			// Get a list of all entries in the old zip
			c_entries.resize(inzip->GetTotalEntries(), nullptr);
			for (unsigned a = 0; a < c_entries.size(); a++)
			{
				c_entries[a] = inzip->GetNextEntry();

				// Stop if reading the zip failed
				if (!c_entries[a])
					break;
			}
			inzip->Reset();
		}
		else
		{
			in    = nullptr;
			inzip = nullptr;
		}
	}

	// Get a linear list of all entries in the archive
	vector<ArchiveEntry*> entries;
	putEntryTreeAsList(entries);

	// Go through all entries
	auto n_entries = entries.size();
	ui::setSplashProgressMessage("Writing zip entries");
	ui::setSplashProgress(0.0f);
	ui::updateSplash();
	for (size_t a = 0; a < n_entries; a++)
	{
		ui::setSplashProgress(a, n_entries);

		if (entries[a]->type() == EntryType::folderType())
		{
			// If the current entry is a folder, just write a directory entry and continue
			zip.PutNextDirEntry(entries[a]->path(true));
			entries[a]->setState(ArchiveEntry::State::Unmodified);
			continue;
		}

		// Get entry zip index
		int index = -1;
		if (entries[a]->exProps().contains("ZipIndex"))
			index = entries[a]->exProp<int>("ZipIndex");

		auto saname = misc::lumpNameToFileName(entries[a]->name());
		if (!inzip || entries[a]->state() != ArchiveEntry::State::Unmodified || index < 0
			|| index >= inzip->GetTotalEntries() || !c_entries[index])
		{
			// If the current entry has been changed, or doesn't exist in the old zip,
			// (re)compress its data and write it to the zip
			const auto zipentry = new wxZipEntry(entries[a]->path() + saname);
			zip.PutNextEntry(zipentry);
			zip.Write(entries[a]->rawData(), entries[a]->size());
		}
		else
		{
			// If the entry is unmodified and exists in the old zip, just copy it over
			c_entries[index]->SetName(entries[a]->path() + saname);
			zip.CopyEntry(c_entries[index], *inzip);
			inzip->Reset();
		}

		// Update entry info
		entries[a]->setState(ArchiveEntry::State::Unmodified);
		entries[a]->exProp("ZipIndex") = static_cast<int>(a);
	}

	// Clean up
	zip.Close();
	out.Close();

	// Update the temp file
	if (temp_file_.empty())
		generateTempFileName(filename);
	fileutil::copyFile(filename, temp_file_);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Loads an [entry]'s data from the saved copy of the archive if any into [out].
// Returns false if the entry is invalid, doesn't belong to the archive or
// doesn't exist in the saved copy, true otherwise.
// -----------------------------------------------------------------------------
bool ZipArchive::loadEntryData(const ArchiveEntry* entry, MemChunk& out)
{
	// Check that the entry belongs to this archive
	if (entry->parent() != this)
	{
		log::error("ZipArchive::loadEntryData: Entry {} attempting to load data from wrong parent!", entry->name());
		return false;
	}

	// Check that the entry has a zip index
	int zip_index;
	if (entry->exProps().contains("ZipIndex"))
		zip_index = entry->exProps().get<int>("ZipIndex");
	else
	{
		log::error("ZipArchive::loadEntryData: Entry {} has no zip entry index!", entry->name());
		return false;
	}

	// Open the file
	wxFFileInputStream in(filename_);
	if (!in.IsOk())
	{
		log::error("ZipArchive::loadEntryData: Unable to open zip file \"{}\"!", filename_);
		return false;
	}

	// Create zip stream
	wxZipInputStream zip(in);
	if (!zip.IsOk())
	{
		log::error("ZipArchive::loadEntryData: Invalid zip file \"{}\"!", filename_);
		return false;
	}

	// Skip to correct entry in zip
	auto zentry = zip.GetNextEntry();
	for (long a = 0; a < zip_index; a++)
	{
		delete zentry;
		zentry = zip.GetNextEntry();
	}

	// Abort if entry doesn't exist in zip (some kind of error)
	if (!zentry)
	{
		log::error("Error: ZipEntry for entry \"{}\" does not exist in zip", entry->name());
		return false;
	}

	// Read the data
	vector<uint8_t> data(zentry->GetSize());
	zip.Read(data.data(), zentry->GetSize());
	out.importMem(data.data(), static_cast<uint32_t>(zentry->GetSize()));

	// Clean up
	delete zentry;

	return true;
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace].
// If [copy] is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
//
// In a zip archive, a namespace is simply a first-level directory, ie
// <root>/<namespace>
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> ZipArchive::addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace)
{
	// Check namespace
	if (add_namespace.empty() || add_namespace == "global")
		return Archive::addEntry(entry, 0xFFFFFFFF, nullptr);

	// Get/Create namespace dir
	const auto dir = createDir(strutil::lower(add_namespace));

	// Add the entry to the dir
	return Archive::addEntry(entry, 0xFFFFFFFF, dir.get());
}

// -----------------------------------------------------------------------------
// Returns the mapdesc_t information about the map at [entry], if [entry] is
// actually a valid map (ie. a wad archive in the maps folder)
// -----------------------------------------------------------------------------
Archive::MapDesc ZipArchive::mapDesc(ArchiveEntry* maphead)
{
	MapDesc map;

	// Check entry
	if (!checkEntry(maphead))
		return map;

	// Check entry type
	if (maphead->type()->formatId() != "archive_wad")
		return map;

	// Check entry directory
	if (maphead->parentDir()->parent() != rootDir() || maphead->parentDir()->name() != "maps")
		return map;

	// Setup map info
	map.archive = true;
	map.head    = maphead->getShared();
	map.end     = maphead->getShared();
	map.name    = maphead->upperNameNoExt();

	return map;
}

// -----------------------------------------------------------------------------
// Detects all the maps in the archive and returns a vector of information about
// them.
// -----------------------------------------------------------------------------
vector<Archive::MapDesc> ZipArchive::detectMaps()
{
	vector<MapDesc> ret;

	// Get the maps directory
	const auto mapdir = dirAtPath("maps");
	if (!mapdir)
		return ret;

	// Go through entries in map dir
	for (unsigned a = 0; a < mapdir->numEntries(); a++)
	{
		auto entry = mapdir->sharedEntryAt(a);

		// Maps can only be wad archives
		if (entry->type()->formatId() != "archive_wad")
			continue;

		// Detect map format (probably kinda slow but whatever, no better way to do it really)
		auto       format = MapFormat::Unknown;
		WadArchive tempwad;
		tempwad.open(entry->data());
		auto emaps = tempwad.detectMaps();
		if (!emaps.empty())
			format = emaps[0].format;

		// Add map description
		MapDesc md;
		md.head    = entry;
		md.end     = entry;
		md.archive = true;
		md.name    = entry->upperNameNoExt();
		md.format  = format;
		ret.push_back(md);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Returns the first entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* ZipArchive::findFirst(SearchOptions& options)
{
	// Init search variables
	auto dir = rootDir().get();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = dirAtPath(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return nullptr;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	auto opt            = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findFirst(opt);
}

// -----------------------------------------------------------------------------
// Returns the last entry matching the search criteria in [options], or null if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* ZipArchive::findLast(SearchOptions& options)
{
	// Init search variables
	auto dir = rootDir().get();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = dirAtPath(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return nullptr;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	auto opt            = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findLast(opt);
}

// -----------------------------------------------------------------------------
// Returns all entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> ZipArchive::findAll(SearchOptions& options)
{
	// Init search variables
	auto                  dir = rootDir().get();
	vector<ArchiveEntry*> ret;

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = dirAtPath(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return ret;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	auto opt            = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findAll(opt);
}

// -----------------------------------------------------------------------------
// Generates the temp file path to use, from [filename].
// The temp file will be in the configured temp folder
// -----------------------------------------------------------------------------
void ZipArchive::generateTempFileName(string_view filename)
{
	const strutil::Path tfn(filename);
	temp_file_ = app::path(tfn.fileName(), app::Dir::Temp);
	if (wxFileExists(temp_file_))
	{
		// Make sure we don't overwrite an existing temp file
		// (in case there are multiple zips open with the same name)
		int n = 1;
		while (true)
		{
			temp_file_ = app::path(fmt::format("{}.{}", tfn.fileName(), n), app::Dir::Temp);
			if (!wxFileExists(temp_file_))
				break;

			n++;
		}
	}
}


// -----------------------------------------------------------------------------
//
// ZipArchive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid zip archive
// -----------------------------------------------------------------------------
bool ZipArchive::isZipArchive(MemChunk& mc)
{
	// Check size
	if (mc.size() < 22)
		return false;

	// Read first 4 bytes
	uint32_t sig;
	mc.seek(0, SEEK_SET);
	mc.read(&sig, sizeof(uint32_t));

	// Check for signature
	if (sig != 0x04034b50 && // File header
		sig != 0x06054b50)   // End of central directory
		return false;

	// The zip format is horrendous, so this will do for checking
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid zip archive
// -----------------------------------------------------------------------------
bool ZipArchive::isZipArchive(const string& filename)
{
	// Open the file for reading
	wxFile file(filename);

	// Check it opened
	if (!file.IsOpened())
		return false;

	// Read first 4 bytes
	uint32_t sig;
	file.Read(&sig, sizeof(uint32_t));

	// Check for signature
	if (sig != 0x04034b50 && // File header
		sig != 0x06054b50)   // End of central directory
		return false;

	// The zip format is horrendous, so this will do for checking
	return true;
}
