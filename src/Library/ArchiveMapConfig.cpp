
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveMapConfig.cpp
// Description: ArchiveMapConfigRow struct and related functions
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
#include "ArchiveMapConfig.h"
#include "Database/Context.h"
#include "Database/Database.h"
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
string insert_archive_map_config = "INSERT INTO archive_map_config VALUES (?,?,?)";
string update_archive_map_config =
	"UPDATE archive_map_config "
	"SET game = ?, port = ? "
	"WHERE archive_id = ?";
} // namespace slade::library


// -----------------------------------------------------------------------------
//
// ArchiveMapConfigRow Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArchiveMapConfigRow constructor
// Reads existing data from the database. If a row with [archive_id] doesn't
// exist in the database, the row archive_id will be set to -1
// -----------------------------------------------------------------------------
ArchiveMapConfigRow::ArchiveMapConfigRow(database::Context& db, int64_t archive_id)
{
	if (auto sql = db.cacheQuery("get_archive_map_config", "SELECT * FROM archive_map_config WHERE archive_id = ?"))
	{
		sql->bind(1, archive_id);

		if (sql->executeStep())
		{
			this->archive_id = archive_id;
			game             = sql->getColumn(1).getString();
			port             = sql->getColumn(2).getString();
		}

		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, the inserted row id is returned, otherwise returns -1
// -----------------------------------------------------------------------------
int64_t ArchiveMapConfigRow::insert() const
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to insert archive_map_config row with no archive_id");
		return false;
	}

	int64_t row_id = -1;

	if (auto sql = db::cacheQuery("insert_archive_map_config", insert_archive_map_config, true))
	{
		sql->bind(1, archive_id);
		sql->bind(2, game);
		sql->bind(3, port);

		if (sql->exec() > 0)
			row_id = db::connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	return row_id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
bool ArchiveMapConfigRow::update() const
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to update archive_map_config row with no archive_id");
		return false;
	}

	int rows = 0;

	if (auto sql = db::cacheQuery("update_archive_map_config", update_archive_map_config, true))
	{
		sql->bind(1, game);
		sql->bind(2, port);
		sql->bind(3, archive_id);

		rows = sql->exec();

		sql->reset();
	}

	return rows > 0;
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, archive_id will be set to -1
// -----------------------------------------------------------------------------
bool ArchiveMapConfigRow::remove()
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to delete archive_map_config row with no archive_id");
		return false;
	}

	auto rows = 0;
	if (auto sql = db::cacheQuery("delete_archive_map_config", "DELETE FROM archive_map_config WHERE archive_id = ?"))
	{
		sql->bind(1, archive_id);
		rows = sql->exec();
		sql->reset();
	}

	if (rows > 0)
	{
		archive_id = -1;
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
// Returns the archive_map_config row for [archive_id].
// If it doesn't exist in the database, the row's archive_id will be -1
// -----------------------------------------------------------------------------
ArchiveMapConfigRow library::getArchiveMapConfig(int64_t archive_id)
{
	return { db::global(), archive_id };
}

// -----------------------------------------------------------------------------
// Saves [row] to the database, either inserts or updates if the row for
// archive_id already exists
// -----------------------------------------------------------------------------
bool library::saveArchiveMapConfig(const ArchiveMapConfigRow& row)
{
	if (row.archive_id < 0)
		return false;

	// Update/Insert
	return db::rowIdExists("archive_map_config", row.archive_id, "archive_id") ? row.update() : row.insert() >= 0;
}
