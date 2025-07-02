
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveUIConfig.cpp
// Description: Struct and functions for working with the archive_ui_config
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
#include "ArchiveUIConfig.h"
#include "Database/Context.h"
#include "Database/Statement.h"
#include "UI/State.h"
#include <SQLiteCpp/Database.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// ArchiveUIConfig Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Reads in the current archive_ui_config row from [ps]
// -----------------------------------------------------------------------------
void database::ArchiveUIConfig::read(Statement& ps)
{
	if (!ps.statement().hasRow())
		return;

	archive_id            = ps.getColumn(0).getInt();
	elist_index_visible   = ps.getColumn(1).getInt() > 0;
	elist_index_width     = ps.getColumn(2).getInt();
	elist_name_width      = ps.getColumn(3).getInt();
	elist_size_visible    = ps.getColumn(4).getInt() > 0;
	elist_size_width      = ps.getColumn(5).getInt();
	elist_type_visible    = ps.getColumn(6).getInt() > 0;
	elist_type_width      = ps.getColumn(7).getInt();
	elist_sort_column     = ps.getColumn(8).getString();
	elist_sort_descending = ps.getColumn(9).getInt() > 0;
	splitter_position     = ps.getColumn(10).getInt();
}

// -----------------------------------------------------------------------------
// Writes this archive_ui_config row to the database.
// If it doesn't already exist it will be inserted, otherwise it will be updated
// -----------------------------------------------------------------------------
void database::ArchiveUIConfig::write()
{
	if (!context().rowIdExists("archive_ui_config", archive_id, "archive_id"))
		insert();
	else
		update();
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, the inserted row id is returned, otherwise returns -1
// -----------------------------------------------------------------------------
i64 database::ArchiveUIConfig::insert()
{
	i64 row_id = -1;

	if (archive_id < 0)
	{
		log::warning("Trying to insert archive_ui_config row with no archive_id");
		return row_id;
	}

	auto ps = context().preparedStatement(
		"insert_archive_ui_config",
		"INSERT INTO archive_ui_config (archive_id, elist_index_visible, elist_index_width, elist_name_width, "
		"                               elist_size_visible, elist_size_width, elist_type_visible, elist_type_width, "
		"                               elist_sort_column, elist_sort_descending, splitter_position) "
		"VALUES (?,?,?,?,?,?,?,?,?,?,?)",
		true);

	ps.bind(1, archive_id);
	ps.bind(2, elist_index_visible);
	ps.bind(3, elist_index_width);
	ps.bind(4, elist_name_width);
	ps.bind(5, elist_size_visible);
	ps.bind(6, elist_size_width);
	ps.bind(7, elist_type_visible);
	ps.bind(8, elist_type_width);
	ps.bind(9, elist_sort_column);
	ps.bind(10, elist_sort_descending);
	ps.bind(11, splitter_position);

	if (ps.exec() > 0)
		row_id = context().connectionRW()->getLastInsertRowid();

	return row_id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
void database::ArchiveUIConfig::update() const
{
	if (archive_id < 0)
	{
		log::warning("Trying to update archive_ui_config row with no archive_id");
		return;
	}

	auto ps = context().preparedStatement(
		"update_archive_ui_config",
		"UPDATE archive_ui_config "
		"SET elist_index_visible = ?, elist_index_width = ?, elist_name_width = ?, elist_size_visible = ?, "
		"    elist_size_width = ?, elist_type_visible = ?, elist_type_width = ?, elist_sort_column = ?, "
		"    elist_sort_descending = ?, splitter_position = ? "
		"WHERE archive_id = ?",
		true);

	ps.bind(1, elist_index_visible);
	ps.bind(2, elist_index_width);
	ps.bind(3, elist_name_width);
	ps.bind(4, elist_size_visible);
	ps.bind(5, elist_size_width);
	ps.bind(6, elist_type_visible);
	ps.bind(7, elist_type_width);
	ps.bind(8, elist_sort_column);
	ps.bind(9, elist_sort_descending);
	ps.bind(10, splitter_position);
	ps.bind(11, archive_id);

	if (ps.exec() <= 0)
		log::warning(
			"Failed to update archive_ui_config row with archive_id {} (most likely the id does not exist)",
			archive_id);
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, archive_id will be set to -1
// -----------------------------------------------------------------------------
void database::ArchiveUIConfig::remove()
{
	if (archive_id < 0)
	{
		log::warning("Trying to remove archive_ui_config row with no id");
		return;
	}

	auto ps = context().preparedStatement(
		"delete_archive_ui_config", "DELETE FROM archive_ui_config WHERE archive_id = ?");

	ps.bind(1, archive_id);

	if (ps.exec() <= 0)
		log::warning(
			"Failed to delete archive_ui_config row with archive_id {} (most likely the id does not exist)",
			archive_id);
	else
		archive_id = -1;
}

void database::ArchiveUIConfig::setDefaults(bool tree_view)
{
	elist_index_visible = ui::getStateBool(ui::ENTRYLIST_INDEX_VISIBLE);
	elist_index_width   = ui::getStateInt(ui::ENTRYLIST_INDEX_WIDTH);
	elist_name_width    = ui::getStateInt(tree_view ? ui::ENTRYLIST_NAME_WIDTH_TREE : ui::ENTRYLIST_NAME_WIDTH_LIST);
	elist_size_visible  = ui::getStateBool(ui::ENTRYLIST_SIZE_VISIBLE);
	elist_size_width    = ui::getStateInt(ui::ENTRYLIST_SIZE_WIDTH);
	elist_type_visible  = ui::getStateBool(ui::ENTRYLIST_TYPE_VISIBLE);
	elist_type_width    = ui::getStateInt(ui::ENTRYLIST_TYPE_WIDTH);
	splitter_position = ui::getStateInt(tree_view ? ui::ARCHIVEPANEL_SPLIT_POS_TREE : ui::ARCHIVEPANEL_SPLIT_POS_LIST);
}


// -----------------------------------------------------------------------------
//
// Database Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the archive_ui_config row for [archive_id].
// If it doesn't exist in the database, the row's archive_id will be -1
// -----------------------------------------------------------------------------
database::ArchiveUIConfig database::getArchiveUIConfig(i64 archive_id)
{
	ArchiveUIConfig archive_ui_config;

	if (archive_id < 0)
	{
		log::warning("Trying to get archive_ui_config row with invalid id");
		return archive_ui_config;
	}

	auto ps = context().preparedStatement(
		"get_archive_ui_config", "SELECT * FROM archive_ui_config WHERE archive_id = ?");

	ps.bind(1, archive_id);

	if (ps.executeStep())
		archive_ui_config.read(ps);

	return archive_ui_config;
}

int database::archiveUIConfigSplitterPos(i64 archive_id)
{
	int splitter_pos = -1;

	auto ps = context().preparedStatement(
		"archive_ui_config_splitter_pos", "SELECT splitter_position FROM archive_ui_config WHERE archive_id = ?");

	ps.bind(1, archive_id);

	if (ps.executeStep())
		splitter_pos = ps.getColumn(0).getInt();

	return splitter_pos;
}

void database::saveArchiveUIConfigSplitterPos(i64 archive_id, int splitter_pos)
{
	auto ps = context().preparedStatement(
		"update_archive_ui_config_splitter_position",
		"UPDATE archive_ui_config SET splitter_position = ? WHERE archive_id = ?",
		true);

	ps.bind(1, splitter_pos);
	ps.bind(2, archive_id);

	ps.exec();
}
