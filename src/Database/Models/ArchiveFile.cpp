
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveFile.cpp
// Description: Database model for a row in the archive_file table
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
#include "Database/Context.h"
#include "Database/Statement.h"
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
