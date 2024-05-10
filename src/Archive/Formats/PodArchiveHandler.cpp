
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PodArchiveHandler.cpp
// Description: ArchiveFormatHandler for Terminal Velocity / Fury3 POD archives
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
#include "PodArchiveHandler.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "General/Console.h"
#include "General/UI.h"
#include "MainEditor/MainEditor.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PodArchiveHandler Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// PodArchiveHandler class constructor
// -----------------------------------------------------------------------------
PodArchiveHandler::PodArchiveHandler() : ArchiveFormatHandler(ArchiveFormat::Pod)
{
	// Blank id
	memset(id_, 0, 80);
}

// -----------------------------------------------------------------------------
// Sets the description/id of the pod archive
// -----------------------------------------------------------------------------
void PodArchiveHandler::setId(string_view id)
{
	memset(id_, 0, 80);
	memcpy(id_, id.data(), id.size());
}

// -----------------------------------------------------------------------------
// Reads pod format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool PodArchiveHandler::open(Archive& archive, const MemChunk& mc, bool detect_types)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read no. of files
	mc.seek(0, 0);
	uint32_t num_files;
	mc.read(&num_files, 4);

	// Read id
	mc.read(id_, 80);

	// Read directory
	vector<FileEntry> files(num_files);
	mc.read(files.data(), num_files * sizeof(FileEntry));

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	ArchiveModSignalBlocker sig_blocker{ archive };

	// Create entries
	ui::setSplashProgressMessage("Reading pod archive data");
	for (unsigned a = 0; a < num_files; a++)
	{
		// Create entry
		auto new_entry = std::make_shared<ArchiveEntry>(strutil::Path::fileNameOf(files[a].name), files[a].size);
		new_entry->setOffsetOnDisk(files[a].offset);
		new_entry->setSizeOnDisk();

		// Add entry and directory to directory tree
		auto ndir = createDir(archive, strutil::Path::pathOf(files[a].name, false));
		ndir->addEntry(new_entry);

		// Read data
		new_entry->importMemChunk(mc, files[a].offset, files[a].size);

		new_entry->setState(EntryState::Unmodified);

		log::info(5, "File size: {}, offset: {}, name: {}", files[a].size, files[a].offset, files[a].name);
	}

	// Detect entry types
	if (detect_types)
		archive.detectAllEntryTypes();

	// Setup variables
	sig_blocker.unblock();
	archive.setModified(false);

	ui::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the pod archive to a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool PodArchiveHandler::write(Archive& archive, MemChunk& mc)
{
	// Get all entries
	vector<ArchiveEntry*> entries;
	archive.putEntryTreeAsList(entries);

	// Process entries
	int      ndirs     = 0;
	uint32_t data_size = 0;
	for (auto& entry : entries)
	{
		if (entry->isFolderType())
			ndirs++;
		else
			data_size += entry->size();
	}

	// Init MemChunk
	mc.clear();
	mc.reSize(4 + 80 + (entries.size() * 40) + data_size, false);
	log::info(5, "MC size {}", mc.size());

	// Write no. entries
	uint32_t n_entries = entries.size() - ndirs;
	log::info(5, "n_entries {}", n_entries);
	mc.write(&n_entries, 4);

	// Write id
	log::info(5, "id {}", id_);
	mc.write(id_, 80);

	// Write directory
	FileEntry fe;
	fe.offset = 4 + 80 + (n_entries * 40);
	for (auto& entry : entries)
	{
		if (entry->isFolderType())
			continue;

		// Name
		memset(fe.name, 0, 32);
		auto path = entry->path(true);
		std::replace(path.begin(), path.end(), '/', '\\');
		path = strutil::afterFirst(path, '\\');
		// log::info(2, path);
		memcpy(fe.name, path.data(), path.size());

		// Size
		fe.size = entry->size();

		// Write directory entry
		mc.write(fe.name, 32);
		mc.write(&fe.size, 4);
		mc.write(&fe.offset, 4);
		log::info(
			5, "entry {}: old={} new={} size={}", fe.name, entry->exProp<int>("Offset"), fe.offset, entry->size());

		// Update entry
		entry->setOffsetOnDisk(fe.offset);
		entry->setSizeOnDisk();

		// Next offset
		fe.offset += fe.size;
	}

	// Write entry data
	for (auto& entry : entries)
		if (!entry->isFolderType())
			mc.write(entry->rawData(), entry->size());

	return true;
}


// -----------------------------------------------------------------------------
//
// PodArchiveHandler Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Checks if the given data is a valid pod archive
// -----------------------------------------------------------------------------
bool PodArchiveHandler::isThisFormat(const MemChunk& mc)
{
	// Check size for header
	if (mc.size() < 84)
		return false;

	// Read no. of files
	mc.seek(0, 0);
	uint32_t num_files;
	mc.read(&num_files, 4);
	if (num_files == 0)
		return false; // 0 files, unlikely to be a valid archive

	// Read id
	char id[80];
	mc.read(id, 80);

	// Check size for directory
	auto dir_end = 84 + (num_files * 40);
	if (mc.size() < dir_end)
		return false;

	// Read directory and check offsets
	FileEntry entry;
	for (unsigned a = 0; a < num_files; a++)
	{
		mc.read(&entry, 40);
		auto end = entry.offset + entry.size;
		if (end > mc.size() || end < dir_end)
			return false;
	}
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid pod archive
// -----------------------------------------------------------------------------
bool PodArchiveHandler::isThisFormat(const string& filename)
{
	wxFile file;
	if (!file.Open(filename))
		return false;

	file.SeekEnd(0);
	uint32_t file_size = file.Tell();

	// Check size for header
	if (file_size < 84)
	{
		file.Close();
		return false;
	}

	// Read no. of files
	file.Seek(0);
	uint32_t num_files;
	file.Read(&num_files, 4);
	if (num_files == 0)
		return false; // 0 files, unlikely to be a valid archive

	// Read id
	char id[80];
	file.Read(id, 80);

	// Check size for directory
	auto dir_end = 84 + (num_files * 40);
	if (file_size < dir_end)
	{
		file.Close();
		return false;
	}

	// Read directory and check offsets
	FileEntry entry;
	for (unsigned a = 0; a < num_files; a++)
	{
		file.Read(&entry, 40);
		auto end = entry.offset + entry.size;
		if (end > file_size || end < dir_end)
			return false;
	}
	return true;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------

CONSOLE_COMMAND(pod_get_id, 0, 1)
{
	auto archive = maineditor::currentArchive();
	if (archive && archive->format() == ArchiveFormat::Pod)
		log::console(string{ dynamic_cast<PodArchiveHandler*>(&archive->formatHandler())->getId() });
	else
		log::console("Current tab is not a POD archive");
}

CONSOLE_COMMAND(pod_set_id, 1, true)
{
	auto archive = maineditor::currentArchive();
	if (archive && archive->format() == ArchiveFormat::Pod)
		dynamic_cast<PodArchiveHandler*>(&archive->formatHandler())->setId(strutil::truncate(args[0], 80));
	else
		log::console("Current tab is not a POD archive");
}
