
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveMap.cpp
// Description: ArchiveMapRow struct and related functions
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
#include "ArchiveMap.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/MapDesc.h"
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
string update_archive_map = "UPDATE archive_map SET name = ?, format = ? WHERE archive_id = ? AND header_entry_id = ?";
string insert_archive_map = "INSERT INTO archive_map (archive_id, header_entry_id, name, format) VALUES (?,?,?,?)";
} // namespace slade::library


// -----------------------------------------------------------------------------
//
// ArchiveMapRow Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArchiveMapRow constructor
// Reads existing data from the database. If row [archive_id]+[header_entry_id]
// doesn't exist in the database, the row id will be set to -1
// -----------------------------------------------------------------------------
ArchiveMapRow::ArchiveMapRow(database::Context& db, int64_t archive_id, int64_t header_entry_id)
{
	if (auto sql = db.cacheQuery(
			"get_archive_map", "SELECT * FROM archive_map WHERE archive_id = ? AND header_entry_id = ?"))
	{
		sql->bind(1, archive_id);
		sql->bind(2, header_entry_id);

		if (sql->executeStep())
		{
			this->archive_id      = archive_id;
			this->header_entry_id = header_entry_id;
			name                  = sql->getColumn(2).getString();
			auto format_int       = sql->getColumn(3).getInt();

			if (format_int <= static_cast<int>(MapFormat::Unknown))
				format = static_cast<MapFormat>(format_int);
		}

		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// ArchiveMapRow constructor
// Initializes the row with info from [map_desc]
// -----------------------------------------------------------------------------
ArchiveMapRow::ArchiveMapRow(int64_t archive_id, const MapDesc& map_desc) : archive_id{ archive_id }
{
	auto* head_entry = map_desc.head.lock().get();
	if (!head_entry)
	{
		this->archive_id = -1;
		return;
	}

	header_entry_id = head_entry->libraryId();
	name            = map_desc.name;
	format          = map_desc.format;
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, the inserted row id is returned, otherwise returns -1.
// (note the returned row id isn't the 'id' column since that is not a primary
//  key, it returns the sqlite rowid of the row)
// -----------------------------------------------------------------------------
int64_t ArchiveMapRow::insert()
{
	int64_t row_id = -1;

	if (auto sql = db::cacheQuery("insert_archive_map", insert_archive_map, true))
	{
		sql->bind(1, archive_id);
		sql->bind(2, header_entry_id);
		sql->bind(3, name);
		sql->bind(4, static_cast<int>(format));

		if (sql->exec() > 0)
			row_id = db::connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	return row_id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
bool ArchiveMapRow::update() const
{
	// Ignore invalid id
	if (archive_id < 0 || header_entry_id < 0)
	{
		log::warning("Trying to update archive_map row with no archive+entry id");
		return false;
	}

	auto rows = 0;

	if (auto sql = db::cacheQuery("update_archive_map", update_archive_map, true))
	{
		sql->bind(1, name);
		sql->bind(2, static_cast<int>(format));
		sql->bind(3, archive_id);
		sql->bind(4, header_entry_id);

		rows = sql->exec();

		sql->reset();
	}

	return rows > 0;
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, archive_id and header_entry_id will be set to -1
// -----------------------------------------------------------------------------
bool ArchiveMapRow::remove()
{
	// Ignore invalid id
	if (archive_id < 0 || header_entry_id < 0)
	{
		log::warning("Trying to remove archive_map row with no archive+entry id");
		return false;
	}

	auto rows = 0;

	if (auto sql = db::cacheQuery(
			"delete_archive_map", "DELETE FROM archive_map WHERE archive_id = ? AND header_entry_id = ?"))
	{
		sql->bind(1, archive_id);
		sql->bind(2, header_entry_id);
		rows = sql->exec();
		sql->reset();
	}

	if (rows > 0)
	{
		archive_id      = -1;
		header_entry_id = -1;
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
// Rebuilds all archive_map rows for [archive_id] from [archive]
// -----------------------------------------------------------------------------
void library::updateArchiveMaps(int64_t archive_id, const Archive& archive)
{
	auto maps = archive.detectMaps();

	// Delete existing map rows
	db::exec(fmt::format("DELETE FROM archive_map WHERE archive_id = {}", archive_id));

	// Add detected maps to database
	for (const auto& map : maps)
	{
		ArchiveMapRow row{ archive_id, map };
		row.insert();
	}
}
