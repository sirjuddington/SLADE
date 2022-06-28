
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveRunConfig.cpp
// Description: ArchiveRunConfigRow struct and related functions
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
#include "ArchiveRunConfig.h"
#include "General/Database.h"

using namespace slade;
using namespace library;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::library
{
string insert_archive_run_config = "INSERT INTO archive_run_config VALUES (?,?,?,?)";
string update_archive_run_config =
	"UPDATE archive_run_config "
	"SET executable_id = ?, run_config = ?, run_extra = ? "
	"WHERE archive_id = ?";
} // namespace slade::library


// -----------------------------------------------------------------------------
//
// ArchiveRunConfigRow Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArchiveRunConfigRow constructor
// Reads existing data from the database. If a row with [archive_id] doesn't
// exist in the database, the row archive_id will be set to -1
// -----------------------------------------------------------------------------
ArchiveRunConfigRow::ArchiveRunConfigRow(database::Context& db, int64_t archive_id)
{
	if (auto sql = db.cacheQuery("get_archive_run_config", "SELECT * FROM archive_run_config WHERE archive_id = ?"))
	{
		sql->bind(1, archive_id);

		if (sql->executeStep())
		{
			this->archive_id = archive_id;
			executable_id    = sql->getColumn(1).getString();
			run_config       = sql->getColumn(2).getInt();
			run_extra        = sql->getColumn(3).getString();
		}

		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, the inserted row id is returned, otherwise returns -1
// -----------------------------------------------------------------------------
int64_t ArchiveRunConfigRow::insert() const
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to insert archive_run_config row with no archive_id");
		return false;
	}

	int64_t row_id = -1;

	if (auto sql = db::cacheQuery("insert_archive_run_config", insert_archive_run_config, true))
	{
		sql->bind(1, archive_id);
		sql->bind(2, executable_id);
		sql->bind(3, run_config);
		sql->bind(4, run_extra);

		if (sql->exec() > 0)
			row_id = db::connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	return row_id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
bool ArchiveRunConfigRow::update() const
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to update archive_run_config row with no archive_id");
		return false;
	}

	int rows = 0;

	if (auto sql = db::cacheQuery("update_archive_run_config", update_archive_run_config, true))
	{
		sql->bind(1, executable_id);
		sql->bind(2, run_config);
		sql->bind(3, run_extra);
		sql->bind(4, archive_id);

		rows = sql->exec();

		sql->reset();
	}

	return rows > 0;
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, archive_id will be set to -1
// -----------------------------------------------------------------------------
bool ArchiveRunConfigRow::remove()
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to delete archive_run_config row with no archive_id");
		return false;
	}

	auto rows = 0;
	if (auto sql = db::cacheQuery("delete_archive_run_config", "DELETE FROM archive_run_config WHERE archive_id = ?"))
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
// Returns the archive_run_config row for [archive_id].
// If it doesn't exist in the database, the row's archive_id will be -1
// -----------------------------------------------------------------------------
ArchiveRunConfigRow library::getArchiveRunConfig(int64_t archive_id)
{
	return { db::global(), archive_id };
}

// -----------------------------------------------------------------------------
// Saves [row] to the database, either inserts or updates if the row for
// archive_id already exists
// -----------------------------------------------------------------------------
bool library::saveArchiveRunConfig(const ArchiveRunConfigRow& row)
{
	if (row.archive_id < 0)
		return false;

	// Update/Insert
	return db::rowIdExists("archive_run_config", row.archive_id, "archive_id") ? row.update() : row.insert() >= 0;
}
