
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Library.cpp
// Description: Functions dealing with the program's archive 'library', which is
//              essentially a bunch of info about archives that have been opened
//              in SLADE.
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
#include "Library.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormat.h"
#include "Archive/ArchiveFormatHandler.h"
#include "ArchiveEntry.h"
#include "ArchiveFile.h"
#include "ArchiveMap.h"
#include "ArchiveUIConfig.h"
#include "Database/Context.h"
#include "Database/Database.h"
#include "Database/Transaction.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include <SQLiteCpp/Database.h>

using namespace slade;
using namespace library;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::library
{
Signals     lib_signals;
std::atomic lib_scan_running{ false }; // Whether a library scan is currently running, set to false to request stop

string insert_archive_bookmark = "INSERT OR REPLACE INTO archive_bookmark VALUES (?,?)";

vector<string> recent_files;
} // namespace slade::library


// -----------------------------------------------------------------------------
//
// library Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Initializes the library
// -----------------------------------------------------------------------------
void library::init()
{
	// Remove pre-3.3.0 recent files that exist in the library
	// (and have been opened)
	vector<string> rf_to_remove;
	for (const auto& recent_file : recent_files)
	{
		// Find archive_file row id from path
		auto archive_id = archiveFileId(recent_file);
		if (archive_id < 0)
			continue;

		// Load row details
		ArchiveFileRow archive_file{ db::global(), archive_id };

		// If the last_opened time exists, we can remove it from the pre-3.3.0
		// recent files list
		if (archive_file.last_opened > 0)
			rf_to_remove.push_back(recent_file);
	}
	for (const auto& path : rf_to_remove)
		VECTOR_REMOVE(recent_files, path);
}

// -----------------------------------------------------------------------------
// Reads all info from the library about the given [archive].
// Returns the library id of the archive
// -----------------------------------------------------------------------------
int64_t library::readArchiveInfo(const Archive& archive)
{
	try
	{
		// Find archive_file row for [archive] ---------------------------------

		// Create row from file path to use for comparison
		auto archive_file = ArchiveFileRow{ archive.filename(), archive.formatId() };

		// Check for parent archive
		if (auto* parent = archive.parentArchive())
		{
			auto* entry            = archive.parentEntry();
			archive_file.parent_id = parent->libraryId();
			archive_file.path      = parent->filename() + "/" + entry->name();
		}

		// Find existing archive_file row id for the archive's filename
		auto archive_id = archiveFileId(archive_file.path, archive_file.parent_id);
		if (archive_id < 0)
		{
			// Not found - look for match

			// Can't match folder archives by data (yet)
			// TODO: Figure out a good way to store size/hash for folder archive
			if (archive_file.format_id == "folder")
				return -1;

			// Find archive_file row with matching data
			// If no data match found this archive doesn't exist in the library
			auto match_id = findArchiveFileIdFromData(archive_file.size, archive_file.hash, archive_file.parent_id);
			if (match_id < 0)
				return -1;

			// Check if the matched file exists on disk (or in the parent archive)
			// - If it exists the archive has likely been copied so copy its
			//   data in the library
			// - If it doesn't exist the archive has likely been moved so just
			//   use and update the existing (matched) row in the library
			auto match_row = ArchiveFileRow{ db::global(), match_id };
			if (archive_file.parent_id < 0 && fileutil::fileExists(match_row.path))
				archive_id = copyArchiveFile(archive_file.path, match_id);
			else if (archive_file.parent_id >= 0 && archive.parentArchive()->entryAtPath(archive_file.path))
				archive_id = copyArchiveFile(archive_file.path, match_id);
			else
			{
				archive_id = match_id;

				// Update existing row with new file details
				auto existing_row          = ArchiveFileRow{ db::global(), archive_id };
				existing_row.path          = archive_file.path;
				existing_row.last_modified = archive_file.last_modified;
				existing_row.format_id     = archive_file.format_id;
				existing_row.update();
			}
		}

		// Read archive_entry rows for archive ---------------------------------

		vector<ArchiveEntry*> all_entries;
		archive.putEntryTreeAsList(all_entries);
		readEntryInfo(archive_id, all_entries);


		// Finish up
		archive.setLibraryId(archive_id);
		return archive_id;
	}
	catch (SQLite::Exception& ex)
	{
		log::error("Error reading archive info from the library: {}", ex.what());
		return -1;
	}
}

// -----------------------------------------------------------------------------
// Sets the [last_opened] time for [archive_id] in the library
// -----------------------------------------------------------------------------
void library::setArchiveLastOpenedTime(int64_t archive_id, time_t last_opened)
{
	try
	{
		if (auto sql = db::cacheQuery(
				"lib_set_archive_last_opened", "UPDATE archive_file SET last_opened = ? WHERE id = ?", true))
		{
			sql->bind(1, last_opened);
			sql->bind(2, archive_id);
			sql->exec();
			sql->reset();
		}

		lib_signals.archive_file_updated(archive_id);
	}
	catch (SQLite::Exception& ex)
	{
		log::error("Error setting archive last opened time in library: {}", ex.what());
	}
}

// -----------------------------------------------------------------------------
// Writes all info for [archive] into the library.
// Returns the library id of the archive
// -----------------------------------------------------------------------------
int64_t library::writeArchiveInfo(const Archive& archive)
{
	try
	{
		// Create row from archive path + format
		auto archive_file = ArchiveFileRow{ archive.filename(), archive.formatId() };

		// Check for parent archive
		if (auto* parent = archive.parentArchive())
		{
			auto* entry            = archive.parentEntry();
			archive_file.parent_id = parent->libraryId();
			archive_file.path      = parent->filename() + "/" + entry->name();
			archive_file.size      = entry->size();
			archive_file.hash      = entry->hash();
		}

		// Get id of row in database if it exists
		archive_file.id      = archiveFileId(archive_file.path, archive_file.parent_id);
		auto new_archive_row = archive_file.id < 0;

		// Keep last opened time if the row exists
		if (!new_archive_row)
			archive_file.last_opened = archiveFileLastOpened(archive_file.id);

		// Write row to database
		saveArchiveFile(archive_file);

		// Create archive_ui_config row if needed
		if (new_archive_row)
		{
			ArchiveUIConfigRow ui_config{ archive_file.id, archive.formatInfo().supports_dirs };
			ui_config.insert();
		}

		// Write entries to database
		vector<ArchiveEntry*> all_entries;
		archive.putEntryTreeAsList(all_entries);
		rebuildEntries(archive_file.id, all_entries);

		// Write maps to database
		updateArchiveMaps(archive_file.id, archive);

		// Remove from pre-3.3.0 recent files list if it's there
		if (auto rf = find(recent_files.begin(), recent_files.end(), archive_file.path); rf != recent_files.end())
			recent_files.erase(rf);

		// Finish up
		archive.setLibraryId(archive_file.id);
		return archive_file.id;
	}
	catch (SQLite::Exception& ex)
	{
		log::error("Error writing archive info to the library: {}", ex.what());
		return -1;
	}
}

// -----------------------------------------------------------------------------
// (Re)Writes all info for [archive]'s entries into the library
// -----------------------------------------------------------------------------
void library::writeArchiveEntryInfo(const Archive& archive)
{
	// If it doesn't exist in the library need to add it
	if (archive.libraryId() < 0)
	{
		writeArchiveInfo(archive);
		return;
	}

	try
	{
		// Write entries to database
		vector<ArchiveEntry*> all_entries;
		archive.putEntryTreeAsList(all_entries);
		rebuildEntries(archive.libraryId(), all_entries);
	}
	catch (SQLite::Exception& ex)
	{
		log::error("Error writing archive entry info to the library: {}", ex.what());
	}
}

// -----------------------------------------------------------------------------
// (Re)Writes all info for [archive]'s maps into the library
// -----------------------------------------------------------------------------
void library::writeArchiveMapInfo(const Archive& archive)
{
	// If it doesn't exist in the library need to add it
	if (archive.libraryId() < 0)
	{
		writeArchiveInfo(archive);
		return;
	}

	try
	{
		// Write maps to database
		updateArchiveMaps(archive.libraryId(), archive);
	}
	catch (SQLite::Exception& ex)
	{
		log::error("Error writing archive map info to the library: {}", ex.what());
	}
}

// -----------------------------------------------------------------------------
// Removes all archives in the library that no longer exist on disk
// -----------------------------------------------------------------------------
void library::removeMissingArchives()
{
	try
	{
		vector<int64_t> to_remove;

		if (auto sql = db::cacheQuery("lib_all_archive_paths", "SELECT id, path FROM archive_file WHERE parent_id < 0"))
		{
			while (sql->executeStep())
			{
				if (!fileutil::fileExists(sql->getColumn(1).getString()))
					to_remove.push_back(sql->getColumn(0).getInt64());
			}

			sql->reset();
		}

		for (auto id : to_remove)
		{
			log::info("Removing archive {} from library (no longer exists)", id);
			removeArchiveFile(id);
		}
	}
	catch (SQLite::Exception& ex)
	{
		log::error("Error removing missing archives from the library: {}", ex.what());
	}
}

// -----------------------------------------------------------------------------
// Returns the filenames of the [count] most recently opened archives
// -----------------------------------------------------------------------------
vector<string> library::recentFiles(unsigned count)
{
	vector<string> paths;

	try
	{
		// Get or create cached query to select base resource paths
		if (auto sql = db::cacheQuery(
				"lib_recent_files",
				"SELECT path FROM archive_file "
				"WHERE last_opened > 0 AND parent_id < 0 "
				"ORDER BY last_opened DESC LIMIT ?"))
		{
			sql->bind(1, count);

			// Execute query and add results to list
			while (sql->executeStep())
				paths.push_back(sql->getColumn(0).getString());

			sql->reset();
		}
	}
	catch (SQLite::Exception& ex)
	{
		log::error("Error getting recent files from the library: {}", ex.what());
	}

	// Append pre-3.3.0 recent files list to make up count if required
	if (!recent_files.empty() && paths.size() < count)
	{
		for (const auto& recent_file : recent_files)
		{
			if (paths.size() == count)
				break;

			paths.push_back(recent_file);
		}
	}

	return paths;
}

// -----------------------------------------------------------------------------
// Finds and scans all archives in [path] (recursively), adding or updating them
// in the library. Files with extensions in [ignore_ext] will be ignored.
//
// This is safe to run in a background thread, and only one scan can be running
// at any time
// -----------------------------------------------------------------------------
void library::scanArchivesInDir(string_view path, const vector<string>& ignore_ext, bool rebuild)
{
	if (lib_scan_running)
	{
		// Abort if a scan is already running (eg. in another thread)
		log::warning("Library scan already running, can only have one running at once");
		return;
	}

	auto files = fileutil::allFilesInDir(path, true);

	lib_scan_running = true;

	for (auto& filename : files)
	{
		// Sanitize path
		strutil::replaceIP(filename, "\\", "/");

		// Check extension
		auto ext = strutil::Path::extensionOf(filename);
		if (VECTOR_EXISTS(ignore_ext, ext))
		{
			log::debug("File {} has ignored extension, skipping", filename);
			continue;
		}
		if (!archive::isKnownExtension(ext))
		{
			log::debug("File {} has unknown archive extension, skipping", filename);
			continue;
		}

		// Check if the file exists in the library
		auto lib_id = archiveFileId(filename);
		if (lib_id >= 0 && !rebuild)
		{
			// Check if the file on disk hasn't been modified since it was last updated in the library
			auto lib_file_modified = archiveFileLastModified(lib_id);
			if (lib_file_modified == fileutil::fileModifiedTime(filename))
			{
				log::info(
					"Library Scan: File {} is already in library and has not been modified since last scanned",
					filename);
				continue;
			}
		}

		// Check if file is a known archive format
		auto format = archive::detectArchiveFormat(filename);
		if (format != ArchiveFormat::Unknown)
		{
			auto archive = std::make_unique<Archive>(format);

			log::info("Library Scan: Scanning file \"{}\" (detected as {})", filename, archive->formatInfo().name);

			if (!archive->open(filename, true))
			{
				log::info("Library Scan: Failed to open archive file {}: {}", filename, global::error);
				continue;
			}

			auto id = readArchiveInfo(*archive);
			if (id < 0 || rebuild)
			{
				if (!rebuild)
					log::info("Library Scan: Archive file doesn't exist in library, adding");

				writeArchiveInfo(*archive);
			}
			else
				log::info("Library Scan: Archive already exists in library");
		}
		else
			log::debug("File {} is not a known/valid archive format, skipping", filename);

		// Check if stop scan was requested
		if (!lib_scan_running)
		{
			log::info("Library Scan: Stop scan requested, ending scan");
			return;
		}
	}

	lib_scan_running = false;
}

// -----------------------------------------------------------------------------
// Stops the currently running library scan (if any)
// -----------------------------------------------------------------------------
void library::stopArchiveDirScan()
{
	if (lib_scan_running)
		lib_scan_running = false;
}

// -----------------------------------------------------------------------------
// Returns true if a library scan is currently running
// -----------------------------------------------------------------------------
bool library::archiveDirScanRunning()
{
	return lib_scan_running;
}

// -----------------------------------------------------------------------------
// Returns all bookmarked entry ids for [archive_id]
// -----------------------------------------------------------------------------
vector<int64_t> library::bookmarkedEntries(int64_t archive_id)
{
	vector<int64_t> entry_ids;

	if (auto sql = db::cacheQuery(
			"archive_all_bookmarks", "SELECT entry_id FROM archive_bookmark WHERE archive_id = ?"))
	{
		sql->bind(1, archive_id);
		while (sql->executeStep())
			entry_ids.push_back(sql->getColumn(0).getInt64());
		sql->reset();
	}

	return entry_ids;
}

// -----------------------------------------------------------------------------
// Adds a bookmarked entry to the library
// -----------------------------------------------------------------------------
void library::addBookmark(int64_t archive_id, int64_t entry_id)
{
	if (archive_id < 0 || entry_id < 0)
		return;

	if (auto sql = db::cacheQuery("insert_archive_bookmark", insert_archive_bookmark, true))
	{
		sql->bind(1, archive_id);
		sql->bind(2, entry_id);
		sql->exec();
		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Removes a bookmarked entry from the library
// -----------------------------------------------------------------------------
void library::removeBookmark(int64_t archive_id, int64_t entry_id)
{
	if (archive_id < 0 || entry_id < 0)
		return;

	if (auto sql = db::cacheQuery(
			"delete_archive_bookmark", "DELETE FROM archive_bookmark WHERE archive_id = ? AND entry_id = ?", true))
	{
		sql->bind(1, archive_id);
		sql->bind(2, entry_id);
		sql->exec();
		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Removes all bookmarked entries for [archive_id] in the library
// -----------------------------------------------------------------------------
void library::removeArchiveBookmarks(int64_t archive_id)
{
	db::connectionRW()->exec(fmt::format("DELETE FROM archive_bookmark WHERE archive_id = {}", archive_id));
}

// -----------------------------------------------------------------------------
// Writes multiple bookmarked entries to the library
// -----------------------------------------------------------------------------
void library::writeArchiveBookmarks(int64_t archive_id, const vector<int64_t>& entry_ids)
{
	auto connection = db::connectionRW();

	// Delete existing bookmarks in library first
	removeArchiveBookmarks(archive_id);

	// Insert bookmark rows
	if (auto sql = db::cacheQuery("insert_archive_bookmark", insert_archive_bookmark, true))
	{
		db::Transaction transaction{ connection, false };
		transaction.beginIfNoActiveTransaction();

		for (const auto& entry_id : entry_ids)
		{
			sql->bind(1, archive_id);
			sql->bind(2, entry_id);
			sql->exec();
			sql->reset();
		}

		transaction.commit();
	}
}

// -----------------------------------------------------------------------------
// Attempts to find the EntryType id of the given [entry] by finding an entry
// in the library with the exact same name and data
// -----------------------------------------------------------------------------
string library::findEntryTypeId(const ArchiveEntry& entry)
{
	if (entry.size() == 0)
		return "marker";

	string format_id;

	if (auto sql = db::cacheQuery(
			"find_entry_type_id", "SELECT type_id FROM archive_entry WHERE name = ? AND hash = ?"))
	{
		sql->bind(1, entry.name());
		sql->bind(2, entry.hash());

		if (sql->executeStep())
			format_id = sql->getColumn(0).getString();

		sql->reset();
	}

	return format_id;
}

// -----------------------------------------------------------------------------
// Reads the pre-3.3.0 recent_files section in slade3.cfg
// -----------------------------------------------------------------------------
void library::readPre330RecentFiles(Tokenizer& tz)
{
	while (!tz.checkOrEnd("}"))
	{
		// Read recent file path
		auto path = wxString::FromUTF8(tz.current().text.c_str()).ToStdString();
		tz.adv();

		// Check the path is valid
		if (!(fileutil::fileExists(path) || fileutil::dirExists(path)))
			continue;

		recent_files.insert(recent_files.begin(), path);
	}

	tz.adv(); // Skip ending }
}

// -----------------------------------------------------------------------------
// Returns the library signals struct
// -----------------------------------------------------------------------------
Signals& library::signals()
{
	return lib_signals;
}



#include "General/Console.h"

CONSOLE_COMMAND(lib_cleanup, 0, true)
{
	log::console("Removing missing archives...");
	library::removeMissingArchives();

	log::console("Library cleanup complete");
}

CONSOLE_COMMAND(lib_scan, 1, true)
{
	if (args[0] == "stop")
	{
		// Stop scan requested
		lib_scan_running = false;
		log::console("Library scan stop requested, will stop after the current archive is finished scanning");
		return;
	}

	vector<string> ignore_ext; //{ "zip" };

	// Start scan in background thread
	std::thread scan_thread(
		[ignore_ext, args]
		{
			// Create+Register database connection context for thread
			db::Context ctx{ db::programDatabasePath() };
			db::registerThreadContext(ctx);

			bool rebuild = false;
			if (args.size() >= 2 && args[1] == "rebuild")
				rebuild = true;

			library::scanArchivesInDir(args[0], ignore_ext, rebuild);

			db::deregisterThreadContexts();
		});
	scan_thread.detach();
}
