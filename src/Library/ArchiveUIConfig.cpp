
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveUIConfig.cpp
// Description: ArchiveUIConfigRow struct and related functions
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
#include "ArchiveUIConfig.h"
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
string update_archive_ui_config =
	"UPDATE archive_ui_config "
	"SET elist_index_visible = ?, elist_index_width = ?, elist_name_width = ?, elist_size_visible = ?, "
	"    elist_size_width = ?, elist_type_visible = ?, elist_type_width = ?, elist_sort_column = ?, "
	"    elist_sort_descending = ?, splitter_position = ? "
	"WHERE archive_id = ?";
string insert_archive_ui_config =
	"INSERT INTO archive_ui_config (archive_id, elist_index_visible, elist_index_width, elist_name_width, "
	"                               elist_size_visible, elist_size_width, elist_type_visible, elist_type_width, "
	"                               elist_sort_column, elist_sort_descending, splitter_position) "
	"VALUES (?,?,?,?,?,?,?,?,?,?,?)";
} // namespace slade::library


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, elist_colsize_name_tree)
EXTERN_CVAR(Int, elist_colsize_name_list)
EXTERN_CVAR(Int, elist_colsize_size)
EXTERN_CVAR(Int, elist_colsize_type)
EXTERN_CVAR(Int, elist_colsize_index)
EXTERN_CVAR(Bool, elist_colsize_show)
EXTERN_CVAR(Bool, elist_coltype_show)
EXTERN_CVAR(Bool, elist_colindex_show)
EXTERN_CVAR(Int, ap_splitter_position_tree)
EXTERN_CVAR(Int, ap_splitter_position_list)


// -----------------------------------------------------------------------------
//
// ArchiveUIConfigRow Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArchiveFileRow constructor
// Reads existing data from the database. If a row with [archive_id] doesn't
// exist in the database, the row archive_id will be set to -1
// -----------------------------------------------------------------------------
ArchiveUIConfigRow::ArchiveUIConfigRow(db::Context& db, int64_t archive_id) : archive_id{ archive_id }
{
	if (auto sql = db.cacheQuery("get_archive_ui_config", "SELECT * FROM archive_ui_config WHERE archive_id = ?"))
	{
		sql->clearBindings();
		sql->bind(1, archive_id);

		if (sql->executeStep())
		{
			elist_index_visible   = sql->getColumn(1).getInt() > 0;
			elist_index_width     = sql->getColumn(2).getInt();
			elist_name_width      = sql->getColumn(3).getInt();
			elist_size_visible    = sql->getColumn(4).getInt() > 0;
			elist_size_width      = sql->getColumn(5).getInt();
			elist_type_visible    = sql->getColumn(6).getInt() > 0;
			elist_type_width      = sql->getColumn(7).getInt();
			elist_sort_column     = sql->getColumn(8).getString();
			elist_sort_descending = sql->getColumn(9).getInt() > 0;
			splitter_position     = sql->getColumn(10).getInt();
		}
		else
		{
			log::warning("archive_ui_config row with archive_id {} does not exist in the database", archive_id);
			this->archive_id = -1;
		}

		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// ArchiveFileRow constructor
// Initializes the row for [archive_id] with 'default' values taken from cvars
// depending on [tree_view]
// -----------------------------------------------------------------------------
ArchiveUIConfigRow::ArchiveUIConfigRow(int64_t archive_id, bool tree_view) : archive_id{ archive_id }
{
	elist_index_visible = elist_colindex_show;
	elist_index_width   = elist_colsize_index;
	elist_name_width    = tree_view ? elist_colsize_name_tree : elist_colsize_name_list;
	elist_size_visible  = elist_colsize_show;
	elist_size_width    = elist_colsize_size;
	elist_type_visible  = elist_coltype_show;
	elist_type_width    = elist_colsize_type;
	splitter_position   = tree_view ? ap_splitter_position_tree : ap_splitter_position_list;

	log::debug("Created default entry list config for archive {}", archive_id);
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, the inserted row id is returned, otherwise returns -1
// -----------------------------------------------------------------------------
int64_t ArchiveUIConfigRow::insert() const
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to insert archive_ui_config row with no archive_id");
		return false;
	}

	int64_t row_id = -1;

	if (auto sql = db::cacheQuery("insert_archive_ui_config", insert_archive_ui_config, true))
	{
		sql->clearBindings();

		sql->bind(1, archive_id);
		sql->bind(2, elist_index_visible);
		sql->bind(3, elist_index_width);
		sql->bind(4, elist_name_width);
		sql->bind(5, elist_size_visible);
		sql->bind(6, elist_size_width);
		sql->bind(7, elist_type_visible);
		sql->bind(8, elist_type_width);
		sql->bind(9, elist_sort_column);
		sql->bind(10, elist_sort_descending);
		sql->bind(11, splitter_position);

		if (sql->exec() > 0)
			row_id = db::connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	return row_id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
bool ArchiveUIConfigRow::update() const
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to update archive_ui_config row with no archive_id");
		return false;
	}

	auto rows = 0;
	if (auto sql = db::cacheQuery("update_archive_ui_config", update_archive_ui_config, true))
	{
		sql->clearBindings();

		sql->bind(1, elist_index_visible);
		sql->bind(2, elist_index_width);
		sql->bind(3, elist_name_width);
		sql->bind(4, elist_size_visible);
		sql->bind(5, elist_size_width);
		sql->bind(6, elist_type_visible);
		sql->bind(7, elist_type_width);
		sql->bind(8, elist_sort_column);
		sql->bind(9, elist_sort_descending);
		sql->bind(10, splitter_position);
		sql->bind(11, archive_id);

		rows = sql->exec();
		sql->reset();
	}

	return rows > 0;
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, archive_id will be set to -1
// -----------------------------------------------------------------------------
bool ArchiveUIConfigRow::remove()
{
	// Ignore invalid id
	if (archive_id < 0)
	{
		log::warning("Trying to delete archive_ui_config row with no archive_id");
		return false;
	}

	auto rows = 0;
	if (auto sql = db::cacheQuery("delete_archive_ui_config", "DELETE FROM archive_ui_config WHERE archive_id = ?"))
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
// Returns the archive_ui_config row for [archive_id].
// If it doesn't exist in the database, the row's archive_id will be -1
// -----------------------------------------------------------------------------
ArchiveUIConfigRow library::getArchiveUIConfig(int64_t archive_id)
{
	return { db::global(), archive_id };
}

// -----------------------------------------------------------------------------
// Saves [row] to the database, either inserts or updates if the row for
// archive_id already exists
// -----------------------------------------------------------------------------
bool library::saveArchiveUIConfig(const ArchiveUIConfigRow& row)
{
	if (row.archive_id < 0)
		return false;

	log::debug("Saving entry list config for archive {}", row.archive_id);

	// Update/Insert
	return db::rowIdExists("archive_ui_config", row.archive_id, "archive_id") ? row.update() : row.insert() >= 0;
}

// -----------------------------------------------------------------------------
// Returns the splitter position for [archive_id], or -1 if no config exists
// -----------------------------------------------------------------------------
int library::archiveUIConfigSplitterPos(int64_t archive_id)
{
	int splitter_pos = -1;

	if (auto sql = db::cacheQuery(
			"archive_ui_config_splitter_pos", "SELECT splitter_position FROM archive_ui_config WHERE archive_id = ?"))
	{
		sql->clearBindings();
		sql->bind(1, archive_id);

		if (sql->executeStep())
			splitter_pos = sql->getColumn(0).getInt();

		sql->reset();
	}

	return splitter_pos;
}

// -----------------------------------------------------------------------------
// Saves the splitter position for [archive_id]
// -----------------------------------------------------------------------------
bool library::saveArchiveUIConfigSplitterPos(int64_t archive_id, int splitter_pos)
{
	bool updated = false;

	if (auto sql = db::cacheQuery(
			"update_archive_ui_config_splitter_position",
			"UPDATE archive_ui_config SET splitter_position = ? WHERE archive_id = ?",
			true))
	{
		sql->clearBindings();
		sql->bind(1, splitter_pos);
		sql->bind(2, archive_id);

		updated = sql->exec() > 0;

		sql->reset();
	}

	return updated;
}
