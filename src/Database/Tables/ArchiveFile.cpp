
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveFile.cpp
// Description: Struct and functions for working with the archive_file table
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
#include "ArchiveFile.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
#include "Database/Context.h"
#include "Database/Database.h"
#include "Database/Statement.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#include <SQLiteCpp/Database.h>

using namespace slade;
using namespace database;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
// SQL query strings
string update_archive_file =
	"UPDATE archive_file "
	"SET path = ?, size = ?, hash = ?, format_id = ?, last_opened = ?, last_modified = ?, parent_id = ? "
	"WHERE id = ?";
string insert_archive_file =
	"REPLACE INTO archive_file (path, size, hash, format_id, last_opened, last_modified, parent_id) "
	"VALUES (?,?,?,?,?,?,?)";
string delete_archive_file = "DELETE FROM archive_file WHERE id = ?";
string get_archive_file    = "SELECT * FROM archive_file WHERE id = ?";
} // namespace

CVAR(Int, max_recent_files, 25, CVar::Flag::Save)


// -----------------------------------------------------------------------------
//
// ArchiveFile Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ArchiveFile constructor
// Reads existing data from the database. If row [id] doesn't exist in the
// database, the row id will be set to -1
// -----------------------------------------------------------------------------
ArchiveFile::ArchiveFile(Context& db, i64 id) : id{ id }
{
	auto ps = db.preparedStatement("get_archive_file", get_archive_file);

	ps.bind(1, id);

	if (ps.executeStep())
	{
		path          = ps.getColumn(1).getString();
		size          = ps.getColumn(2).getUInt();
		hash          = ps.getColumn(3).getString();
		format_id     = ps.getColumn(4).getString();
		last_opened   = ps.getColumn(5).getInt64();
		last_modified = ps.getColumn(6).getInt64();
		parent_id     = ps.getColumn(7).getInt64();
	}
	else
	{
		log::warning("archive_file row with id {} does not exist in the database", id);
		this->id = -1;
	}
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, id will be updated and returned, otherwise returns -1
// -----------------------------------------------------------------------------
i64 ArchiveFile::insert(Context& db)
{
	if (id >= 0)
	{
		log::warning("Trying to insert archive_file row id {} that already exists", id);
		return id;
	}

	auto ps = db.preparedStatement("insert_archive_file", insert_archive_file, true);

	ps.bind(1, path);
	ps.bind(2, size);
	ps.bind(3, hash);
	ps.bind(4, format_id);
	ps.bindDateTime(5, last_opened);
	ps.bindDateTime(6, last_modified);
	ps.bind(7, parent_id);

	if (ps.exec() > 0)
		id = db.connectionRW()->getLastInsertRowid();

	return id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
void ArchiveFile::update(Context& db) const
{
	// Ignore invalid id
	if (id < 0)
	{
		log::warning("Trying to update archive_file row with no id");
		return;
	}

	auto ps = db.preparedStatement("update_archive_file", update_archive_file, true);

	ps.bind(1, path);
	ps.bind(2, size);
	ps.bind(3, hash);
	ps.bind(4, format_id);
	ps.bindDateTime(5, last_opened);
	ps.bindDateTime(6, last_modified);
	ps.bind(7, parent_id);
	ps.bind(8, id);

	if (ps.exec() <= 0)
		log::warning("Failed to update archive_file row with id {} (most likely the id does not exist)", id);
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, id will be set to -1
// -----------------------------------------------------------------------------
void ArchiveFile::remove(Context& db)
{
	// Ignore invalid id
	if (id < 0)
	{
		log::warning("Trying to remove archive_file row with no id");
		return;
	}

	auto ps = db.preparedStatement("delete_archive_file", delete_archive_file);

	ps.bind(1, id);

	if (ps.exec() <= 0)
		log::warning("Failed to delete archive_file row with id {} (most likely the id does not exist)", id);
	else
		id = -1;
}


// -----------------------------------------------------------------------------
//
// Database Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the archive_file row id for [path] (in [parent_id] if given),
// or -1 if it does not exist in the database
// -----------------------------------------------------------------------------
i64 database::archiveFileId(Context& db, const string& path, i64 parent_id)
{
	i64 archive_id = -1;

	auto ps = db.preparedStatement("get_archive_id", "SELECT id FROM archive_file WHERE path = ? AND parent_id = ?");

	ps.bind(1, path);
	ps.bind(2, parent_id);

	if (ps.executeStep())
		archive_id = ps.getColumn(0);

	return archive_id;
}

// -----------------------------------------------------------------------------
// Returns the archive_file row id for [archive], or -1 if it does not exist in
// the database
// -----------------------------------------------------------------------------
i64 database::archiveFileId(Context& db, const Archive& archive)
{
	auto path      = strutil::replace(archive.filename(), "\\", "/");
	auto parent_id = -1;

	if (auto* parent = archive.parentArchive())
	{
		path      = parent->filename() + "/" + archive.parentEntry()->name();
		parent_id = app::archiveManager().archiveDbId(*parent);
	}

	return archiveFileId(db, path, parent_id);
}

// -----------------------------------------------------------------------------
// Returns the last opened time for the archive_file row with [id].
// Returns 0 if it does not exist in the database or has never been opened
// -----------------------------------------------------------------------------
time_t database::archiveFileLastOpened(Context& db, i64 id)
{
	time_t last_opened = 0;

	auto ps = db.preparedStatement("get_archive_file_last_opened", "SELECT last_opened FROM archive_file WHERE id = ?");
	ps.bind(1, id);

	if (ps.executeStep())
		last_opened = ps.getColumn(0).getInt64();

	return last_opened;
}

// -----------------------------------------------------------------------------
// Sets the [last_opened] time for the archive_file row with [archive_id]
// -----------------------------------------------------------------------------
void database::setArchiveFileLastOpened(Context& db, int64_t archive_id, time_t last_opened)
{
	auto ps = db.preparedStatement(
		"set_archive_file_last_opened", "UPDATE archive_file SET last_opened = ? WHERE id = ?", true);
	ps.bindDateTime(1, last_opened);
	ps.bind(2, archive_id);

	if (ps.exec() == 0)
		log::error("Failed to set last opened time for archive with id {}", archive_id);
	else
		signals().archive_file_updated();
}

// -----------------------------------------------------------------------------
// Writes [archive] info to the archive_file table in the database.
// Returns the archive_file row id for the archive, or -1 if an error occurred.
// -----------------------------------------------------------------------------
i64 database::writeArchiveFile(Context& db, const Archive& archive)
{
	ArchiveFile archive_file;

	archive_file.id        = app::archiveManager().archiveDbId(archive);
	archive_file.path      = strutil::replace(archive.filename(), "\\", "/");
	archive_file.format_id = archive.formatId();

	// Keep existing last opened time
	if (archive_file.id >= 0)
		archive_file.last_opened = archiveFileLastOpened(db, archive_file.id);

	if (auto* parent = archive.parentArchive())
	{
		// Embedded archive
		auto entry             = archive.parentEntry();
		archive_file.parent_id = app::archiveManager().archiveDbId(*parent);
		archive_file.path      = parent->filename() + "/" + entry->name();
		archive_file.size      = entry->size();
		archive_file.hash      = entry->data().hash();
	}
	else
	{
		// Archive file/dir on disk
		archive_file.parent_id = -1;
		if (fileutil::fileExists(archive.filename()))
		{
			SFile file{ archive.filename() };
			archive_file.size          = file.size();
			archive_file.hash          = file.calculateHash();
			archive_file.last_modified = fileutil::fileModifiedTime(archive.filename());
		}
	}

	// Write to database
	if (archive_file.id < 0)
		archive_file.insert(db);
	else
		archive_file.update(db);

	signals().archive_file_updated();

	return archive_file.id;
}

// -----------------------------------------------------------------------------
// Returns a list of the most recently opened archives, up to [count] max, or
// max_recent_files cvar if [count] is 0
// -----------------------------------------------------------------------------
vector<string> database::recentFiles(Context& db, unsigned count)
{
	vector<string> paths;

	if (count == 0)
		count = max_recent_files;

	auto ps = db.preparedStatement(
		"recent_files",
		"SELECT path FROM archive_file "
		"WHERE last_opened > 0 AND parent_id < 0 "
		"ORDER BY last_opened DESC LIMIT ?");
	ps.bind(1, count);

	while (ps.executeStep())
		paths.push_back(ps.getColumn(0).getString());

	return paths;
}
