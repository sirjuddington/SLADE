
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    VWadArchive.cpp
// Description: VWadArchive, archive class to handle vwad format archives
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
#include "VWadArchive.h"
#include "App.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "UI/WxUtils.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#include "WadArchive.h"
#include <vwadprng.h>
#include <vwadvfs.h>
#include <vwadwrite.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, vwad_allow_duplicate_names, false, CVar::Save)
CVAR(String, vwad_private_key, "", CVar::Flag::Save)
CVAR(String, vwad_author_name, "", CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)
EXTERN_CVAR(Int, max_entry_size_mb)


// -----------------------------------------------------------------------------
//
// VWadArchive Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// VWadArchive class constructor
// -----------------------------------------------------------------------------
VWadArchive::VWadArchive() : Archive("vwad")
{
	if (vwad_allow_duplicate_names)
		rootDir()->allowDuplicateNames(true);
}

// -----------------------------------------------------------------------------
// VWadArchive class destructor
// -----------------------------------------------------------------------------
VWadArchive::~VWadArchive()
{
	if (fileutil::fileExists(temp_file_))
		fileutil::removeFile(temp_file_);
}

static int vwad_ioseek(vwad_iostream *strm, int pos)
{
    assert(pos >= 0);
    FILE *fl = (FILE *)strm->udata;
    assert(fl != NULL);
    if (fseek(fl, pos, SEEK_SET) != 0)
        return -1;
    return 0;
}

static int vwad_ioread(vwad_iostream *strm, void *buf, int bufsize)
{
    assert(bufsize > 0);
    FILE *fl = (FILE *)strm->udata;
    assert(fl != NULL);
    if (fread(buf, bufsize, 1, fl) != 1)
        return -1;
    return 0;
}

// -----------------------------------------------------------------------------
// Reads vwad data from a file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool VWadArchive::open(string_view filename)
{
	// Check the file exists
	if (!fileutil::fileExists(filename))
	{
		global::error = "File does not exist";
		return false;
	}

	// Copy the vwad to a temp file (for use when saving)
	generateTempFileName(filename);
	fileutil::copyFile(filename, temp_file_);

	// Open the file
	FILE *in = fopen(wxutil::strFromView(filename).c_str(), "rb");
	if (!in)
	{
		global::error = "Unable to open file";
		return false;
	}

	// Create vwad stream
	vwad_iostream *vwad_stream = (vwad_iostream *)calloc(1, sizeof(vwad_iostream));
	vwad_stream->udata = in;
	vwad_stream->seek = vwad_ioseek;
	vwad_stream->read = vwad_ioread;

	// Create vwad handle using stream
	vwad_handle *vwad_hndl = vwad_open_archive(vwad_stream, VWAD_OPEN_DEFAULT, NULL);

	if (!vwad_hndl)
	{
		global::error = "Invalid vwad file";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	const ArchiveModSignalBlocker sig_blocker{ *this };

	// Check vwad properties (if applicable)
	author_ = vwad_get_archive_author(vwad_hndl);
	title_  = vwad_get_archive_title(vwad_hndl);
	int comment_size = vwad_get_archive_comment_size(vwad_hndl);
	if (comment_size > 0)
	{
		comment_.resize(comment_size + 1);
		vwad_get_archive_comment(vwad_hndl, comment_.data(), comment_size + 1);
	}
	signed_ = (vwad_is_authenticated(vwad_hndl) && vwad_has_pubkey(vwad_hndl));
	if (signed_)
	{
		vwad_public_key rawpubkey;
		vwad_get_pubkey(vwad_hndl, rawpubkey);
		vwad_z85_key z85_key;
		vwad_z85_encode_key(rawpubkey, z85_key);
		pubkey_ = std::string(z85_key);
	}

	// Go through all vwad entries
	vwad_fidx entry_index = 0;
	vwad_fidx total = vwad_get_archive_file_count(vwad_hndl);
	string vwad_entry_filename; // used for libvwad path normalization
	vwad_entry_filename.resize(256); // libvwad normalization max char limit
	ui::setSplashProgressMessage("Reading vwad data");
	for (; entry_index < total; entry_index++)
	{
		ui::setSplashProgress(-1.0f);

		const char *entry_name = vwad_get_file_name(vwad_hndl, entry_index);

		if (!entry_name)
			continue;

		memset(vwad_entry_filename.data(), 0, 256);

		if (vwad_normalize_file_name(entry_name, vwad_entry_filename.data()) < 0)
			continue;

		// Since libvwad normalization retains the trailing slash for directories, check to see if
		// the last character is a slash and if not consider it a file
		if (vwad_entry_filename.back() != '/')
		{
			// Get the entry name as a Path (so we can break it up)
			strutil::Path fn(vwad_entry_filename);

			// Create entry
			auto new_entry = std::make_shared<ArchiveEntry>(
				misc::fileNameToLumpName(fn.fileName()), vwad_get_file_size(vwad_hndl, entry_index));

			// Setup entry info
			new_entry->setLoaded(false);
			new_entry->exProp("VWadIndex") = entry_index;

			// Add entry and directory to directory tree
			auto ndir = createDir(fn.path(true));
			ndir->addEntry(new_entry, true);

			if (const auto ve_size = vwad_get_file_size(vwad_hndl, entry_index); ve_size < max_entry_size_mb * 1024 * 1024)
			{
				if (ve_size > 0)
				{
					vwad_fd vwad_entry_fd = vwad_open_fidx(vwad_hndl, entry_index);
					if (vwad_entry_fd < 0)
					{
						global::error = fmt::format("Error getting vWad file descriptor for: {}", fn.fullPath());
						return false;
					}
					vector<uint8_t> data(ve_size);
					if (vwad_read(vwad_hndl, vwad_entry_fd, data.data(), ve_size) < 0)
					{
						global::error = fmt::format("Error importing vWad entry: {}", fn.fullPath());
						vwad_fclose(vwad_hndl, vwad_entry_fd);
						return false;
					}
					new_entry->importMem(data.data(), static_cast<uint32_t>(ve_size));
					vwad_fclose(vwad_hndl, vwad_entry_fd);
				}
				new_entry->setLoaded(true);

				// Determine its type
				EntryType::detectEntryType(*new_entry);

				// Unload data if needed
				if (!archive_load_data)
					new_entry->unloadData();
			}
			else
			{
				global::error = fmt::format("Entry too large: {} is {} mb", fn.fullPath(), ve_size / (1 << 20));
				return false;
			}
		}
		else
		{
			// VWad entry is a directory, add it to the directory tree
			strutil::Path fn(vwad_entry_filename);
			createDir(fn.path(true));
		}
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
// Reads vwad format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool VWadArchive::open(MemChunk& mc)
{
	// Write the MemChunk to a temp file
	const auto tempfile = app::path("slade-temp-open.vwad", app::Dir::Temp);
	mc.exportFile(tempfile);

	// Load the file
	const bool success = open(tempfile);

	// Clean up
	fileutil::removeFile(tempfile);

	return success;
}

// -----------------------------------------------------------------------------
// Writes the vwad archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool VWadArchive::write(MemChunk& mc, bool update)
{
	bool success = false;

	// Write to a temporary file
	const auto tempfile = app::path("slade-temp-write.vwad", app::Dir::Temp);
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
// Writes the vwad archive to a file
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool VWadArchive::write(string_view filename, bool update)
{
	// If no entries at all, do not attempt to make a vWAD
	if (numEntries() == 0)
	{
		global::error = "Cannot write empty vWADs!";
		return false;
	}

	// Check for entries with duplicate names (not allowed for vwads)
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

	// Open old vwad for copying, from the temp file that was copied on opening.
	// This is used to copy any entries that have been previously saved/compressed
	// and are unmodified, to greatly speed up vwad file saving by not having to
	// recompress unchanged entries
	FILE *invwad = NULL;
	vwad_iostream *invwad_stream = NULL;
	vwad_handle *invwad_handle = NULL;
	if (fileutil::fileExists(temp_file_))
	{
		invwad = fopen(temp_file_.c_str(), "rb");
		if (invwad)
		{
			invwad_stream = (vwad_iostream *)calloc(1, sizeof(vwad_iostream));
			invwad_stream->udata = invwad;
			invwad_stream->seek = vwad_ioseek;
			invwad_stream->read = vwad_ioread;
			invwad_handle = vwad_open_archive(invwad_stream, VWAD_OPEN_DEFAULT, NULL);
			if (!invwad_handle)
			{
				free(invwad_stream);
				invwad_stream = NULL;
				fclose(invwad);
				invwad = NULL;
			}
		}
	}

	// Open the file
	FILE *out = fopen(wxutil::strFromView(filename).c_str(), "wb+");
	if (!out)
	{
		global::error = "Unable to open file for saving. Make sure it isn't in use by another program.";
		return false;
	}

	// Open as vwad for writing
	vwadwr_iostream *vwad = vwadwr_new_file_stream(out);
	if (!vwad)
	{
		global::error = "Unable to create vwad for saving";
		fclose(out);
		return false;
	}

	vwadwr_secret_key privkey;
	vwadwr_public_key pubkey;
	vwadwr_uint archive_flag;

	if (!vwad_private_key.empty())
	{
		if (vwadwr_z85_decode_key(vwad_private_key.value.c_str(), privkey) < 0)
		{
			global::error = "Unable to decode vwad_private_key (bad key?)";
			vwadwr_close_file_stream(vwad);
			return false;
		}
		if (!vwadwr_is_good_privkey(privkey))
		{
			global::error = "vwad_private_key is not sufficiently strong, generate a new one";
			vwadwr_close_file_stream(vwad);
			return false;
		}
		archive_flag = VWADWR_NEW_DEFAULT;
	}
	else //randomly generate signing key
	{
		do 
		{
			prng_randombytes(privkey, sizeof(vwadwr_secret_key));
		} while (!vwadwr_is_good_privkey(privkey));
		archive_flag = VWADWR_NEW_DONT_SIGN;
	}

	int vwad_error = 0;

	vwadwr_archive *vwad_archive = vwadwr_new_archive(NULL, vwad, !vwad_author_name.empty() ? vwad_author_name.value.c_str() : 
		(!author_.empty() ? author_.c_str() : NULL), !title_.empty() ? title_.c_str() : NULL, !comment_.empty() ? comment_.c_str() : NULL, 
		archive_flag, privkey, pubkey, &vwad_error);

	if (!vwad_archive)
	{
		global::error = "Unable to create vwad for saving";
		vwadwr_close_file_stream(vwad);
		fclose(out);
		return false;
	}

	// Get a linear list of all entries in the archive
	vector<ArchiveEntry*> entries;
	putEntryTreeAsList(entries);

	// Go through all entries
	auto n_entries = entries.size();
	ui::setSplashProgressMessage("Writing vwad entries");
	ui::setSplashProgress(0.0f);
	ui::updateSplash();
	for (size_t a = 0; a < n_entries; a++)
	{
		ui::setSplashProgress(static_cast<float>(a) / static_cast<float>(n_entries));

		// Can't write "just" a directory
		if (entries[a]->type() == EntryType::folderType())
		{
			if (update)
				entries[a]->setState(ArchiveEntry::State::Unmodified);
			continue;
		}
		// Can't write nameless entires
		if (entries[a]->name().empty())
		{
			if (update)
				entries[a]->setState(ArchiveEntry::State::Unmodified);
			log::error("Attempted to write vWAD entry with an empty name.");
			continue;
		}

		// Get entry vwad index
		int index = -1;
		if (entries[a]->exProps().contains("VWadIndex"))
			index = entries[a]->exProp<int>("VWadIndex");

		auto saname = entries[a]->path() + entries[a]->name();

		if (!invwad || entries[a]->state() != ArchiveEntry::State::Unmodified || index < 0
			|| index >= vwad_get_archive_file_count(invwad_handle))
		{
			// If the current entry has been changed, or doesn't exist in the old vwad,
			// (re)compress its data and write it to the vwad
			vwadwr_fhandle vwad_hndl = vwadwr_create_file(vwad_archive, VWADWR_COMP_MEDIUM, saname.c_str(), 
				NULL, std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
			if (vwad_hndl < 0)
			{
				global::error = fmt::format("Unable to write {} to vwad", saname);
				if (invwad_handle)
					vwad_close_archive(&invwad_handle);
				vwadwr_finish_archive(&vwad_archive);
				vwadwr_close_file_stream(vwad);
				return false;
			}
			else
			{
				vwadwr_write(vwad_archive, vwad_hndl, entries[a]->rawData(), entries[a]->size());
				vwadwr_close_file(vwad_archive, vwad_hndl);
			}
		}
		else
		{
			vwadwr_fhandle vwad_hndl = vwadwr_create_raw_file(vwad_archive,
														vwad_get_file_name(invwad_handle, index),
														vwad_get_file_group_name(invwad_handle, index),
														vwad_get_fcrc32(invwad_handle, index),
														vwad_get_ftime(invwad_handle, index));
			if (vwad_hndl < 0) 
			{
				global::error = fmt::format("Unable to copy {} to vwad", saname);
				vwad_close_archive(&invwad_handle);
				vwadwr_finish_archive(&vwad_archive);
				vwadwr_close_file_stream(vwad);
				return false;
			}

			const int entry_chunk_count = vwad_get_file_chunk_count(invwad_handle, index);
			if (entry_chunk_count < 0) 
			{
				global::error = fmt::format("Unable to copy {} to vwad", saname);
				vwad_close_archive(&invwad_handle);
				vwadwr_close_file(vwad_archive, vwad_hndl);
				vwadwr_finish_archive(&vwad_archive);
				vwadwr_close_file_stream(vwad);
				return false;
			}

			if (entry_chunk_count > 0)
			{
				char *buf = (char *)malloc(VWAD_MAX_RAW_CHUNK_SIZE);
				int pksz, upksz;
				vwad_bool packed;
				vwad_result res;

				for (int chunk = 0; chunk < entry_chunk_count; chunk++)
				{
					memset(buf, 0, VWAD_MAX_RAW_CHUNK_SIZE);
					res = vwad_get_raw_file_chunk_info(invwad_handle, index, chunk, &pksz, &upksz, &packed);
					if (res != VWAD_OK) 
					{
						global::error = fmt::format("Unable to copy {} to vwad", saname);
						vwad_close_archive(&invwad_handle);
						vwadwr_close_file(vwad_archive, vwad_hndl);
						vwadwr_finish_archive(&vwad_archive);
						vwadwr_close_file_stream(vwad);
						free(buf);
						return false;
					}
					res = vwad_read_raw_file_chunk(invwad_handle, index, chunk, buf);
					if (res != VWAD_OK) 
					{
						global::error = fmt::format("Unable to copy {} to vwad", saname);
						vwad_close_archive(&invwad_handle);
						vwadwr_close_file(vwad_archive, vwad_hndl);
						vwadwr_finish_archive(&vwad_archive);
						vwadwr_close_file_stream(vwad);
						free(buf);
						return false;
					}
					res = vwadwr_write_raw_chunk(vwad_archive, vwad_hndl, buf, pksz, upksz, packed);
					if (res != VWADWR_OK) 
					{
						global::error = fmt::format("Unable to copy {} to vwad", saname);
						vwad_close_archive(&invwad_handle);
						vwadwr_close_file(vwad_archive, vwad_hndl);
						vwadwr_finish_archive(&vwad_archive);
						vwadwr_close_file_stream(vwad);
						free(buf);
						return false;
					}
				}
				free(buf);
			}

			if (vwadwr_close_file(vwad_archive, vwad_hndl) != VWADWR_OK) 
			{
				global::error = fmt::format("Unable to copy {} to vwad", saname);
				vwad_close_archive(&invwad_handle);
				vwadwr_finish_archive(&vwad_archive);
				vwadwr_close_file_stream(vwad);
				return false;
			}
		}

		// Update entry info
		if (update)
		{
			entries[a]->setState(ArchiveEntry::State::Unmodified);
			entries[a]->exProp("VWadIndex") = static_cast<int>(a);
		}
	}

	// Clean up
	if (invwad_handle)
		vwad_close_archive(&invwad_handle);
	if (vwadwr_finish_archive(&vwad_archive) < 0)
	{
		vwadwr_close_file_stream(vwad);
		return false;
	}
	else
		vwadwr_close_file_stream(vwad);

	// Update the temp file
	if (temp_file_.empty())
		generateTempFileName(filename);
	fileutil::copyFile(filename, temp_file_);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the saved copy of the archive if any.
// Returns false if the entry is invalid, doesn't belong to the archive or
// doesn't exist in the saved copy, true otherwise.
// -----------------------------------------------------------------------------
bool VWadArchive::loadEntryData(ArchiveEntry* entry)
{
	// Check that the entry belongs to this archive
	if (entry->parent() != this)
	{
		log::error("VWadArchive::loadEntryData: Entry {} attempting to load data from wrong parent!", entry->name());
		return false;
	}

	// Do nothing if the entry's size is zero,
	// or if it has already been loaded
	if (entry->size() == 0 || entry->isLoaded())
	{
		entry->setLoaded();
		return true;
	}

	// Check that the entry has a vwad index
	int vwad_index;
	if (entry->exProps().contains("VWadIndex"))
		vwad_index = entry->exProp<int>("VWadIndex");
	else
	{
		log::error("VWadArchive::loadEntryData: Entry {} has no vwad entry index!", entry->name());
		return false;
	}

	// Open the file
	FILE *in = fopen(filename_.c_str(), "rb");
	if (!in)
	{
		log::error("VWadArchive::loadEntryData: Unable to open vwad file \"{}\"!", filename_);
		return false;
	}

	// Create vwad stream
	vwad_iostream *vwad_stream = (vwad_iostream *)calloc(1, sizeof(vwad_iostream));
	vwad_stream->udata = in;
	vwad_stream->seek = vwad_ioseek;
	vwad_stream->read = vwad_ioread;

	// Create vwad handle using stream
	vwad_handle *vwad_hndl = vwad_open_archive(vwad_stream, VWAD_OPEN_DEFAULT, NULL);

	if (!vwad_hndl)
	{
		log::error("VWadArchive::loadEntryData: Invalid vwad file \"{}\"!", filename_);
		return false;
	}

	// Lock entry state
	entry->lockState();

	// Abort if entry doesn't exist in vwad (some kind of error)
	vwad_fd ventry = vwad_open_fidx(vwad_hndl, vwad_index);
	if (ventry < 0)
	{
		log::error("Error: VWadEntry for entry \"{}\" does not exist in vwad", entry->name());
		return false;
	}

	// Read the data
	int ventry_size = vwad_get_file_size(vwad_hndl, vwad_index);
	vector<uint8_t> data(ventry_size);
	if (vwad_read(vwad_hndl, ventry, data.data(), ventry_size) < 0)
	{
		log::error("Error: VWadEntry for entry \"{}\" encountered a read error", entry->name());
		vwad_fclose(vwad_hndl, vwad_index);
		vwad_close_archive(&vwad_hndl);
		return false;
	}
	entry->importMem(data.data(), static_cast<uint32_t>(ventry_size));

	// Set the entry to loaded
	entry->setLoaded();
	entry->unlockState();

	// Clean up
	vwad_fclose(vwad_hndl, vwad_index);
	vwad_close_archive(&vwad_hndl);

	return true;
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace].
// If [copy] is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
//
// In a vwad archive, a namespace is simply a first-level directory, ie
// <root>/<namespace>
// -----------------------------------------------------------------------------
shared_ptr<ArchiveEntry> VWadArchive::addEntry(shared_ptr<ArchiveEntry> entry, string_view add_namespace)
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
Archive::MapDesc VWadArchive::mapDesc(ArchiveEntry* maphead)
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
vector<Archive::MapDesc> VWadArchive::detectMaps()
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
ArchiveEntry* VWadArchive::findFirst(SearchOptions& options)
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
ArchiveEntry* VWadArchive::findLast(SearchOptions& options)
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
vector<ArchiveEntry*> VWadArchive::findAll(SearchOptions& options)
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
void VWadArchive::generateTempFileName(string_view filename)
{
	const strutil::Path tfn(filename);
	temp_file_ = app::path(tfn.fileName(), app::Dir::Temp);
	if (wxFileExists(temp_file_))
	{
		// Make sure we don't overwrite an existing temp file
		// (in case there are multiple vwads open with the same name)
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

string VWadArchive::getAuthor()
{
	string ret = author_.empty() ? "" : author_;
	return ret;
}

string VWadArchive::getTitle()
{
	string ret = title_.empty() ? "" : title_;
	return ret;
}

string VWadArchive::getComment()
{
	string ret = comment_.empty() ? "" : comment_;
	return ret;
}

bool   VWadArchive::isSigned()
{
	return signed_;
}

string VWadArchive::getPublicKey()
{
	string ret = pubkey_.empty() ? "" : pubkey_;
	return ret;
}

// -----------------------------------------------------------------------------
//
// VWadArchive Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid vwad archive
// -----------------------------------------------------------------------------
bool VWadArchive::isVWadArchive(MemChunk& mc)
{
	// Check size
	if (mc.size() < 22)
		return false;

	// Read first 4 bytes
	uint32_t sig;
	mc.seek(0, SEEK_SET);
	mc.read(&sig, sizeof(uint32_t));

	// Check for signature
	if (sig == 0x44415756) // File header
		return true;

	return false;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid vwad archive
// -----------------------------------------------------------------------------
bool VWadArchive::isVWadArchive(const string& filename)
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
	if (sig == 0x44415756) // File header
		return true;

	return false;
}

// -----------------------------------------------------------------------------
// Generates an ASCII-encoded private key for vWAD signing
// -----------------------------------------------------------------------------
string vwad::generatePrivateKey()
{
	vwadwr_secret_key privkey;
	do 
	{
		prng_randombytes(privkey, sizeof(vwadwr_secret_key));
	} while (!vwadwr_is_good_privkey(privkey));
	vwadwr_z85_key z85_key;
	vwadwr_z85_encode_key(privkey, z85_key);
	string encoded_key(z85_key);
	return encoded_key;
}

// -----------------------------------------------------------------------------
// Derives an ASCII-encoded public key from a provided ASCII-encoded private key
// -----------------------------------------------------------------------------
string vwad::derivePublicKey(string_view privkey)
{
	vwadwr_secret_key decoded_key;
	vwadwr_z85_decode_key(wxutil::strFromView(privkey).c_str(), decoded_key);
	vwadwr_public_key pubkey;
	vwadwr_z85_get_pubkey(pubkey, decoded_key);
	vwadwr_z85_key z85_key;
	vwadwr_z85_encode_key(pubkey, z85_key);
	string encoded_key(z85_key);
	return encoded_key;
}