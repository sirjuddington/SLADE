
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveRunConfig.cpp
// Description: Struct and functions for working with the archive_run_config
//              table
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
#include "Database/Context.h"
#include "Database/Statement.h"
#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Database.h>

using namespace slade;
using namespace database;


// -----------------------------------------------------------------------------
//
// ArchiveRunConfig Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads in the current archive_run_config row from [ps]
// -----------------------------------------------------------------------------
void ArchiveRunConfig::read(Statement& ps)
{
	if (!ps.statement().hasRow())
		return;

	archive_id    = ps.getColumn(0).getInt();
	executable_id = ps.getColumn(1).getString();
	run_config    = ps.getColumn(2).getInt();
	run_extra     = ps.getColumn(3).getString();
	iwad_path     = ps.getColumn(4).getString();
}

// -----------------------------------------------------------------------------
// Writes this archive_run_config row to the database.
// If it doesn't already exist it will be inserted, otherwise it will be updated
// -----------------------------------------------------------------------------
void ArchiveRunConfig::write()
{
	if (!context().rowIdExists("archive_run_config", archive_id, "archive_id"))
		insert();
	else
		update();
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, the inserted row id is returned, otherwise returns -1
// -----------------------------------------------------------------------------
i64 ArchiveRunConfig::insert()
{
	i64 row_id = -1;

	if (archive_id < 0)
	{
		log::warning("Trying to insert archive_run_config row with no archive_id");
		return row_id;
	}

	auto ps = context().preparedStatement(
		"insert_archive_run_config",
		"INSERT INTO archive_run_config (archive_id, executable_id, run_config, run_extra, iwad_path) "
		"VALUES (?,?,?,?,?)",
		true);

	ps.bind(1, archive_id);
	ps.bind(2, executable_id);
	ps.bind(3, run_config);
	ps.bind(4, run_extra);
	ps.bind(5, iwad_path);

	if (ps.exec() > 0)
		row_id = context().connectionRW()->getLastInsertRowid();
	else
		log::warning("Failed to insert archive_run_config row for archive_id {}", archive_id);

	return row_id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
void ArchiveRunConfig::update() const
{
	if (archive_id < 0)
	{
		log::warning("Trying to update archive_run_config row with no archive_id");
		return;
	}

	auto ps = context().preparedStatement(
		"update_archive_run_config",
		"UPDATE archive_run_config "
		"SET executable_id = ?, run_config = ?, run_extra = ?, iwad_path = ? "
		"WHERE archive_id = ?",
		true);

	ps.bind(1, executable_id);
	ps.bind(2, run_config);
	ps.bind(3, run_extra);
	ps.bind(4, iwad_path);
	ps.bind(5, archive_id);

	if (ps.exec() <= 0)
		log::warning("Failed to update archive_run_config row for archive_id {}", archive_id);
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, archive_id will be set to -1
// -----------------------------------------------------------------------------
void ArchiveRunConfig::remove()
{
	if (archive_id < 0)
	{
		log::warning("Trying to remove archive_run_config row with no archive_id");
		return;
	}

	auto ps = context().preparedStatement(
		"remove_archive_run_config", "DELETE FROM archive_run_config WHERE archive_id = ?", true);

	ps.bind(1, archive_id);

	if (ps.exec() > 0)
		archive_id = -1;
	else
		log::warning("Failed to remove archive_run_config row for archive_id {}", archive_id);
}

// -----------------------------------------------------------------------------
// Returns the archive_run_config row for [archive_id].
// If it doesn't exist in the database, the row's archive_id will be -1
// -----------------------------------------------------------------------------
ArchiveRunConfig database::getArchiveRunConfig(i64 archive_id)
{
	ArchiveRunConfig archive_run_config;

	if (archive_id < 0)
	{
		log::warning("Trying to get archive_run_config row with invalid id");
		return archive_run_config;
	}

	auto ps = context().preparedStatement(
		"get_archive_run_config", "SELECT * FROM archive_run_config WHERE archive_id = ?");

	ps.bind(1, archive_id);

	if (ps.executeStep())
		archive_run_config.read(ps);

	return archive_run_config;
}
