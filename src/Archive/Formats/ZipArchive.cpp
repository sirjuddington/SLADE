
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
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
#include "WadArchive.h"
#include <fstream>


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// Structs
//
// -----------------------------------------------------------------------------
namespace
{
// Struct representing a zip file header
struct ZipFileHeader
{
	uint32_t sig;
	uint16_t version;
	uint16_t flag;
	uint16_t compression;
	uint16_t mod_time;
	uint16_t mod_date;
	uint32_t crc;
	uint32_t size_comp;
	uint32_t size_orig;
	uint16_t len_fn;
	uint16_t len_extra;
};
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Helper function to check a file exists
// -----------------------------------------------------------------------------
bool fileExists(const wxString& filename)
{
	std::ifstream file(CHR(filename));
	return file.good();
}
} // namespace


// -----------------------------------------------------------------------------
//
// ZipArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ZipArchive class destructor
// -----------------------------------------------------------------------------
ZipArchive::~ZipArchive()
{
	if (fileExists(temp_file_))
		wxRemoveFile(temp_file_);
}

// -----------------------------------------------------------------------------
// Reads zip data from a file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ZipArchive::open(const wxString& filename)
{
	// Check the file exists
	if (!fileExists(filename))
	{
		Global::error = "File does not exist";
		return false;
	}

	// Copy the zip to a temp file (for use when saving)
	generateTempFileName(filename);
	wxCopyFile(filename, temp_file_);

	// Open the file
	wxFFileInputStream in(filename);
	if (!in.IsOk())
	{
		Global::error = "Unable to open file";
		return false;
	}

	// Create zip stream
	wxZipInputStream zip(in);
	if (!zip.IsOk())
	{
		Global::error = "Invalid zip file";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Go through all zip entries
	int  entry_index = 0;
	auto zip_entry   = zip.GetNextEntry();
	UI::setSplashProgressMessage("Reading zip data");
	while (zip_entry)
	{
		UI::setSplashProgress(-1.0f);
		if (zip_entry->GetMethod() != wxZIP_METHOD_DEFLATE && zip_entry->GetMethod() != wxZIP_METHOD_STORE)
		{
			Global::error = "Unsupported zip compression method";
			setMuted(false);
			return false;
		}

		if (!zip_entry->IsDir())
		{
			// Get the entry name as a wxFileName (so we can break it up)
			wxFileName fn(zip_entry->GetName(wxPATH_UNIX), wxPATH_UNIX);

			// Create entry
			auto new_entry = std::make_shared<ArchiveEntry>(
				Misc::fileNameToLumpName(fn.GetFullName()), zip_entry->GetSize());

			// Setup entry info
			new_entry->setLoaded(false);
			new_entry->exProp("ZipIndex") = entry_index;

			// Add entry and directory to directory tree
			auto ndir = createDir(fn.GetPath(true, wxPATH_UNIX));
			ndir->addEntry(new_entry);

			// Read the data, if possible
			auto ze_size = zip_entry->GetSize();
			if (ze_size < 250 * 1024 * 1024)
			{
				if (ze_size > 0)
				{
					vector<uint8_t> data(ze_size);
					zip.Read(data.data(), ze_size); // Note: this is where exceedingly large files cause an exception.
					new_entry->importMem(data.data(), ze_size);
				}
				new_entry->setLoaded(true);

				// Determine its type
				EntryType::detectEntryType(new_entry.get());

				// Unload data if needed
				if (!archive_load_data)
					new_entry->unloadData();
			}
			else
			{
				Global::error = wxString::Format(
					"Entry too large: %s is %u mb", zip_entry->GetName(wxPATH_UNIX), ze_size / (1 << 20));
				setMuted(false);
				return false;
			}
		}
		else
		{
			// Zip entry is a directory, add it to the directory tree
			wxFileName fn(zip_entry->GetName(wxPATH_UNIX), wxPATH_UNIX);
			createDir(fn.GetPath(true, wxPATH_UNIX));
		}

		// Go to next entry in the zip file
		delete zip_entry;
		zip_entry = zip.GetNextEntry();
		entry_index++;
	}
	UI::updateSplash();

	// Set all entries/directories to unmodified
	vector<ArchiveEntry*> entry_list;
	putEntryTreeAsList(entry_list);
	for (auto& entry : entry_list)
		entry->setState(ArchiveEntry::State::Unmodified);

	// Enable announcements
	setMuted(false);

	// Setup variables
	this->filename_ = filename;
	setModified(false);
	on_disk_ = true;

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads zip format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ZipArchive::open(MemChunk& mc)
{
	// Write the MemChunk to a temp file
	wxString tempfile = App::path("slade-temp-open.zip", App::Dir::Temp);
	mc.exportFile(tempfile);

	// Load the file
	bool success = open(tempfile);

	// Clean up
	wxRemoveFile(tempfile);

	return success;
}

// -----------------------------------------------------------------------------
// Writes the zip archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool ZipArchive::write(MemChunk& mc, bool update)
{
	bool success = false;

	// Write to a temporary file
	wxString tempfile = App::path("slade-temp-write.zip", App::Dir::Temp);
	if (write(tempfile, true))
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
bool ZipArchive::write(const wxString& filename, bool update)
{
	// Open the file
	wxFFileOutputStream out(filename);
	if (!out.IsOk())
	{
		Global::error = "Unable to open file for saving. Make sure it isn't in use by another program.";
		return false;
	}

	// Open as zip for writing
	wxZipOutputStream zip(out, 9);
	if (!zip.IsOk())
	{
		Global::error = "Unable to create zip for saving";
		return false;
	}

	// Open old zip for copying, from the temp file that was copied on opening.
	// This is used to copy any entries that have been previously saved/compressed
	// and are unmodified, to greatly speed up zip file saving by not having to
	// recompress unchanged entries
	std::unique_ptr<wxFFileInputStream> in;
	std::unique_ptr<wxZipInputStream>   inzip;
	vector<wxZipEntry*>                 c_entries;
	if (fileExists(temp_file_))
	{
		in    = std::make_unique<wxFFileInputStream>(temp_file_);
		inzip = std::make_unique<wxZipInputStream>(*in);

		if (inzip->IsOk())
		{
			// Get a list of all entries in the old zip
			c_entries.resize(inzip->GetTotalEntries());
			for (int a = 0; a < c_entries.size(); a++)
				c_entries[a] = inzip->GetNextEntry();
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
	for (size_t a = 0; a < entries.size(); a++)
	{
		if (entries[a]->type() == EntryType::folderType())
		{
			// If the current entry is a folder, just write a directory entry and continue
			zip.PutNextDirEntry(entries[a]->path(true));
			if (update)
				entries[a]->setState(ArchiveEntry::State::Unmodified);
			continue;
		}

		// Get entry zip index
		int index = -1;
		if (entries[a]->exProps().propertyExists("ZipIndex"))
			index = entries[a]->exProp("ZipIndex");

		auto saname = Misc::lumpNameToFileName(entries[a]->name());
		if (!inzip || entries[a]->state() != ArchiveEntry::State::Unmodified || index < 0
			|| index >= inzip->GetTotalEntries())
		{
			// If the current entry has been changed, or doesn't exist in the old zip,
			// (re)compress its data and write it to the zip
			auto zipentry = new wxZipEntry(entries[a]->path() + saname);
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
		if (update)
		{
			entries[a]->setState(ArchiveEntry::State::Unmodified);
			entries[a]->exProp("ZipIndex") = (int)a;
		}
	}

	// Clean up
	zip.Close();
	out.Close();

	// Update the temp file
	if (temp_file_.IsEmpty())
		generateTempFileName(filename);
	wxCopyFile(filename, temp_file_);

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the saved copy of the archive if any.
// Returns false if the entry is invalid, doesn't belong to the archive or
// doesn't exist in the saved copy, true otherwise.
// -----------------------------------------------------------------------------
bool ZipArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check that the entry belongs to this archive
	if (entry->parent() != this)
	{
		Log::error(wxString::Format(
			"ZipArchive::loadEntryData: Entry %s attempting to load data from wrong parent!", entry->name()));
		return false;
	}

	// Do nothing if the entry's size is zero,
	// or if it has already been loaded
	if (entry->size() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Check that the entry has a zip index
	int zip_index;
	if (entry->exProps().propertyExists("ZipIndex"))
		zip_index = entry->exProp("ZipIndex");
	else
	{
		Log::error(wxString::Format("ZipArchive::loadEntryData: Entry %s has no zip entry index!", entry->name()));
		return false;
	}

	// Open the file
	wxFFileInputStream in(filename_);
	if (!in.IsOk())
	{
		Log::error(wxString::Format("ZipArchive::loadEntryData: Unable to open zip file \"%s\"!", filename_));
		return false;
	}

	// Create zip stream
	wxZipInputStream zip(in);
	if (!zip.IsOk())
	{
		Log::error(wxString::Format("ZipArchive::loadEntryData: Invalid zip file \"%s\"!", filename_));
		return false;
	}

	// Lock entry state
	entry->lockState();

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
		Log::error(wxString::Format("Error: ZipEntry for entry \"%s\" does not exist in zip", entry->name()));
		return false;
	}

	// Read the data
	vector<uint8_t> data(zentry->GetSize());
	zip.Read(data.data(), zentry->GetSize());
	entry->importMem(data.data(), zentry->GetSize());

	// Set the entry to loaded
	entry->setLoaded();
	entry->unlockState();

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
ArchiveEntry* ZipArchive::addEntry(ArchiveEntry* entry, const wxString& add_namespace, bool copy)
{
	// Check namespace
	if (add_namespace.IsEmpty() || add_namespace == "global")
		return Archive::addEntry(entry, 0xFFFFFFFF, nullptr, copy);

	// Get/Create namespace dir
	auto dir = createDir(add_namespace.Lower());

	// Add the entry to the dir
	return Archive::addEntry(entry, 0xFFFFFFFF, dir, copy);
}

// -----------------------------------------------------------------------------
// Returns the mapdesc_t information about the map at [entry], if [entry] is
// actually a valid map (ie. a wad archive in the maps folder)
// -----------------------------------------------------------------------------
Archive::MapDesc ZipArchive::mapDesc(ArchiveEntry* entry)
{
	MapDesc map;

	// Check entry
	if (!checkEntry(entry))
		return map;

	// Check entry type
	if (entry->type()->formatId() != "archive_wad")
		return map;

	// Check entry directory
	if (entry->parentDir()->parent() != rootDir() || entry->parentDir()->name() != "maps")
		return map;

	// Setup map info
	map.archive = true;
	map.head    = entry;
	map.end     = entry;
	map.name    = entry->name(true).Upper();

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
	auto mapdir = dir("maps");
	if (!mapdir)
		return ret;

	// Go through entries in map dir
	for (unsigned a = 0; a < mapdir->numEntries(); a++)
	{
		auto entry = mapdir->entryAt(a);

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
		md.name    = entry->name(true).Upper();
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
	auto dir           = rootDir();
	options.match_name = options.match_name.Lower();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.IsEmpty())
	{
		dir = this->dir(options.match_namespace);

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
	auto dir           = rootDir();
	options.match_name = options.match_name.Lower();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.IsEmpty())
	{
		dir = this->dir(options.match_namespace);

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
	auto dir           = rootDir();
	options.match_name = options.match_name.Lower();
	vector<ArchiveEntry*> ret;

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.IsEmpty())
	{
		dir = this->dir(options.match_namespace);

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
void ZipArchive::generateTempFileName(const wxString& filename)
{
	wxFileName tfn(filename);
	temp_file_ = App::path(CHR(tfn.GetFullName()), App::Dir::Temp);
	if (wxFileExists(temp_file_))
	{
		// Make sure we don't overwrite an existing temp file
		// (in case there are multiple zips open with the same name)
		int n = 1;
		while (true)
		{
			temp_file_ = App::path(fmt::format("{}.{}", CHR(tfn.GetFullName()), n), App::Dir::Temp);
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
	if (mc.size() < sizeof(ZipFileHeader))
		return false;

	// Read first file header
	ZipFileHeader header;
	mc.seek(0, SEEK_SET);
	mc.read(&header, sizeof(ZipFileHeader));

	// Check header signature
	if (header.sig != 0x04034b50)
		return false;

	// The zip format is horrendous, so this will do for checking
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid zip archive
// -----------------------------------------------------------------------------
bool ZipArchive::isZipArchive(const wxString& filename)
{
	// Open the file for reading
	wxFile file(filename);

	// Check it opened
	if (!file.IsOpened())
		return false;

	// Read first file header
	ZipFileHeader header;
	file.Read(&header, sizeof(ZipFileHeader));

	// Check header signature
	if (header.sig != 0x04034b50)
		return false;

	// The zip format is horrendous, so this will do for checking
	return true;
}
