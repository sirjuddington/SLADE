
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveFile.cpp
// Description: ArchiveFileRow struct and related functions
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
#include "ArchiveEntry.h"
#include "Database/Context.h"
#include "Database/Database.h"
#include "Library.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
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
// SQL query strings
string update_archive_file =
	"UPDATE archive_file "
	"SET path = ?, size = ?, hash = ?, format_id = ?, last_opened = ?, last_modified = ?, parent_id = ? "
	"WHERE id = ?";
string insert_archive_file =
	"REPLACE INTO archive_file (path, size, hash, format_id, last_opened, last_modified, parent_id) "
	"VALUES (?,?,?,?,?,?,?)";
} // namespace slade::library


// -----------------------------------------------------------------------------
//
// ArchiveFileRow Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArchiveFileRow constructor
// Gets info from the file at [file_path] if it exists
// -----------------------------------------------------------------------------
ArchiveFileRow::ArchiveFileRow(string_view file_path, string_view format_id) : path{ file_path }, format_id{ format_id }
{
	// Sanitize path
	strutil::replaceIP(path, "\\", "/");

	// Get file info
	if (fileutil::fileExists(file_path))
	{
		SFile file(file_path);
		size          = file.size();
		hash          = file.calculateHash();
		last_modified = fileutil::fileModifiedTime(file_path);
	}
}

// -----------------------------------------------------------------------------
// ArchiveFileRow constructor
// Reads existing data from the database. If row [id] doesn't exist in the
// database, the row id will be set to -1
// -----------------------------------------------------------------------------
ArchiveFileRow::ArchiveFileRow(database::Context& db, int64_t id) : id{ id }
{
	// Load from database
	if (auto sql = db.cacheQuery("get_archive_file", "SELECT * FROM archive_file WHERE id = ?"))
	{
		sql->clearBindings();
		sql->bind(1, id);

		if (sql->executeStep())
		{
			path          = sql->getColumn(1).getString();
			size          = sql->getColumn(2).getUInt();
			hash          = sql->getColumn(3).getString();
			format_id     = sql->getColumn(4).getString();
			last_opened   = sql->getColumn(5).getInt64();
			last_modified = sql->getColumn(6).getInt64();
			parent_id     = sql->getColumn(7).getInt64();
		}
		else
		{
			log::warning("archive_file row with id {} does not exist in the database", id);
			this->id = -1;
		}

		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// ArchiveFileRow constructor
// Reads data from result columns in the given SQLite statement [sql]
// -----------------------------------------------------------------------------
ArchiveFileRow::ArchiveFileRow(const SQLite::Statement* sql)
{
	if (!sql)
		return;

	id            = sql->getColumn(0).getInt64();
	path          = sql->getColumn(1).getString();
	size          = sql->getColumn(2).getUInt();
	hash          = sql->getColumn(3).getString();
	format_id     = sql->getColumn(4).getString();
	last_opened   = sql->getColumn(5).getInt64();
	last_modified = sql->getColumn(6).getInt64();
	parent_id     = sql->getColumn(7).getInt64();
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, id will be updated and returned, otherwise returns -1
// -----------------------------------------------------------------------------
int64_t ArchiveFileRow::insert()
{
	if (id >= 0)
	{
		log::warning("Trying to insert archive_file row id {} that already exists", id);
		return id;
	}

	if (auto sql = db::cacheQuery("insert_archive_file", insert_archive_file, true))
	{
		sql->clearBindings();

		sql->bind(1, path);
		sql->bind(2, size);
		sql->bind(3, hash);
		sql->bind(4, format_id);
		sql->bind(5, last_opened);
		sql->bind(6, last_modified);
		sql->bind(7, parent_id);

		if (sql->exec() > 0)
			id = db::connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	if (id >= 0)
		library::signals().archive_file_inserted(id);

	return id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
bool ArchiveFileRow::update() const
{
	// Ignore invalid id
	if (id < 0)
	{
		log::warning("Trying to update archive_file row with no id");
		return false;
	}

	auto rows = 0;
	if (auto sql = db::cacheQuery("update_archive_file", update_archive_file, true))
	{
		sql->clearBindings();

		sql->bind(1, path);
		sql->bind(2, size);
		sql->bind(3, hash);
		sql->bind(4, format_id);
		sql->bind(5, last_opened);
		sql->bind(6, last_modified);
		sql->bind(7, parent_id);
		sql->bind(8, id);

		rows = sql->exec();
		sql->reset();
	}

	if (rows > 0)
	{
		library::signals().archive_file_updated(id);
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, id will be set to -1
// -----------------------------------------------------------------------------
bool ArchiveFileRow::remove()
{
	// Ignore invalid id
	if (id < 0)
	{
		log::warning("Trying to remove archive_file row with no id");
		return false;
	}

	auto rows = 0;

	if (auto sql = db::cacheQuery("delete_archive_file", "DELETE FROM archive_file WHERE id = ?"))
	{
		sql->bind(1, id);
		rows = sql->exec();
		sql->reset();
	}

	if (rows > 0)
	{
		library::signals().archive_file_deleted(id);
		id = -1;
		return true;
	}

	return false;
}


// -----------------------------------------------------------------------------
//
// library Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Returns the archive_file row id for [filename] (in [parent_id] if given),
// or -1 if it does not exist in the database
// -----------------------------------------------------------------------------
int64_t library::archiveFileId(string_view filename, int64_t parent_id)
{
	int64_t archive_id = -1;

	if (auto sql = db::cacheQuery("lib_get_archive_id", "SELECT id FROM archive_file WHERE path = ? AND parent_id = ?"))
	{
		sql->bind(1, filename);
		sql->bind(2, parent_id);

		if (sql->executeStep())
			archive_id = sql->getColumn(0);

		sql->reset();
	}

	return archive_id;
}

// -----------------------------------------------------------------------------
// Returns the first archive_file row id found that has a matching [size] and
// [hash], or -1 if none found
// -----------------------------------------------------------------------------
int64_t library::findArchiveFileIdFromData(unsigned size, string_view hash, int64_t parent_id)
{
	int64_t archive_id = -1;

	if (auto sql = db::cacheQuery(
			"lib_find_archive_id_data", "SELECT id FROM archive_file WHERE size = ? AND hash = ? AND parent_id = ?"))
	{
		sql->bind(1, size);
		sql->bind(2, hash);
		sql->bind(3, parent_id);

		if (sql->executeStep())
			archive_id = sql->getColumn(0);

		sql->reset();
	}

	return archive_id;
}

// -----------------------------------------------------------------------------
// Saves [row] to the database, either inserts or updates depending on the id
// -----------------------------------------------------------------------------
bool library::saveArchiveFile(ArchiveFileRow& row)
{
	if (row.id < 0)
		return row.insert() >= 0;
	else
		return row.update();
}

// -----------------------------------------------------------------------------
// Creates a new archive_file row in the database for [file_path], copying data
// from an existing row [copy_from_id] including any related data eg.
// archive_entry rows.
// Returns the id of the created row or -1 if copy_from_id was invalid
// -----------------------------------------------------------------------------
int64_t library::copyArchiveFile(string_view file_path, int64_t copy_from_id)
{
	// Get row to copy
	ArchiveFileRow archive_file{ db::global(), copy_from_id };
	if (archive_file.id < 0)
		return -1;

	// Set path
	archive_file.id   = -1;
	archive_file.path = file_path;

	// Reset last opened time
	archive_file.last_opened = 0;

	// Add new archive_file row
	auto archive_id = archive_file.insert();

	// Copy entries
	copyArchiveEntries(copy_from_id, archive_id);

	return archive_id;
}

// -----------------------------------------------------------------------------
// Removes the archive_file row [id] from the database including all related
// data eg. archive_entry etc.
// -----------------------------------------------------------------------------
void library::removeArchiveFile(int64_t id)
{
	// Delete row from archive_file
	// (all related data will also be removed via cascading foreign keys)
	if (db::connectionRW()->exec(fmt::format("DELETE FROM archive_file WHERE id = {}", id)) > 0)
		library::signals().archive_file_deleted(id);
}

// -----------------------------------------------------------------------------
// Returns the time archive [id] in the library was last opened (as time_t)
// -----------------------------------------------------------------------------
time_t library::archiveFileLastOpened(int64_t id)
{
	time_t last_opened = 0;

	if (auto sql = db::cacheQuery("get_archive_file_last_opened", "SELECT last_opened FROM archive_file WHERE id = ?"))
	{
		sql->bind(1, id);

		if (sql->executeStep())
			last_opened = sql->getColumn(0).getInt64();

		sql->reset();
	}

	return last_opened;
}

// -----------------------------------------------------------------------------
// Returns the time archive [id] in the library was last modified on disk
// (as time_t)
// -----------------------------------------------------------------------------
time_t library::archiveFileLastModified(int64_t id)
{
	time_t last_modified = 0;

	if (auto sql = db::cacheQuery(
			"get_archive_file_last_modified", "SELECT last_modified FROM archive_file WHERE id = ?"))
	{
		sql->bind(1, id);

		if (sql->executeStep())
			last_modified = sql->getColumn(0).getInt64();

		sql->reset();
	}

	return last_modified;
}

// -----------------------------------------------------------------------------
// Returns models for all rows in the archive_file table
// -----------------------------------------------------------------------------
vector<ArchiveFileRow> library::allArchiveFileRows()
{
	vector<ArchiveFileRow> rows;

	if (auto sql = db::cacheQuery("all_archive_file_rows", "SELECT * FROM archive_file"))
	{
		while (sql->executeStep())
			rows.emplace_back(sql);
	}

	return rows;
}
