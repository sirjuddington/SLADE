
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    State.cpp
// Description: Functions handling database storage/retrieval of UI state info
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
#include "State.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Database/Context.h"
#include "Database/Statement.h"
#include "Utility/Named.h"
#include "Utility/Property.h"
#include <SQLiteCpp/Column.h>

using namespace slade;
using namespace ui;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
string get_ui_state    = "SELECT value FROM ui_state WHERE name = ? AND archive_id IS ?";
string insert_ui_state = "INSERT INTO ui_state (name, value, archive_id) VALUES (?,?,?)";
string update_ui_state = "UPDATE ui_state SET value = ? WHERE name = ? AND archive_id IS ?";
} // namespace


// -----------------------------------------------------------------------------
//
// UI Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
bool hasSavedState(string_view name, optional<i64> archive_id)
{
	auto ps = database::context().preparedStatement(
		"ui_has_saved_state", "SELECT archive_id FROM ui_state WHERE name = ? AND archive_id IS ?");
	ps.bind(1, name);
	ps.bind(2, archive_id);
	return ps.executeStep();
}

template<typename T> void saveState(string_view name, T value, optional<i64> archive_id)
{
	if (hasSavedState(name, archive_id))
	{
		auto ps = database::context().preparedStatement("update_ui_state", update_ui_state, true);
		ps.bind(1, value);
		ps.bind(2, name);
		ps.bind(3, archive_id);
		ps.exec();
	}
	else
	{
		auto ps = database::context().preparedStatement("insert_ui_state", insert_ui_state, true);
		ps.bind(1, name);
		ps.bind(2, value);
		ps.bind(3, archive_id);
		ps.exec();
	}
}

inline optional<i64> archiveDbId(const Archive* archive)
{
	if (archive)
		return app::archiveManager().archiveDbId(*archive);
	return {};
}
} // namespace slade::ui


// -----------------------------------------------------------------------------
// Initializes UI state values to defaults in the database
// (if they don't already exist there)
// -----------------------------------------------------------------------------
void ui::initStateProps()
{
	// Set default values
	vector<Named<Property>> props = { { ENTRYLIST_INDEX_VISIBLE, false },
									  { ENTRYLIST_INDEX_WIDTH, 50 },
									  { ENTRYLIST_SIZE_VISIBLE, true },
									  { ENTRYLIST_SIZE_WIDTH, 70 },
									  { ENTRYLIST_TYPE_VISIBLE, true },
									  { ENTRYLIST_TYPE_WIDTH, 180 },
									  { ENTRYLIST_NAME_WIDTH_LIST, 110 },
									  { ENTRYLIST_NAME_WIDTH_TREE, 190 },
									  { ENTRYLIST_VIEW_TYPE, 1 },
									  { ARCHIVEPANEL_SPLIT_POS_LIST, 370 },
									  { ARCHIVEPANEL_SPLIT_POS_TREE, 450 },
									  { ARCHIVE_LAST_CREATED_FORMAT, "wad" },
									  { COLOURISEDIALOG_LAST_COLOUR, "RGB(255, 0, 0)" },
									  { TINTDIALOG_LAST_COLOUR, "RGB(255, 0, 0)" },
									  { TINTDIALOG_LAST_AMOUNT, 50 },
									  { ZOOM_GFXCANVAS, 100 },
									  { ZOOM_CTEXTURECANVAS, 100 },
									  { BROWSERWINDOW_MAXIMIZED, false },
									  { MAINWINDOW_MAXIMIZED, true },
									  { MAPEDITORWINDOW_MAXIMIZED, true },
									  { SCRIPTMANAGERWINDOW_MAXIMIZED, false },
									  { SETUP_WIZARD_RUN, false } };

	auto ps = database::context().preparedStatement("init_ui_state", "INSERT INTO ui_state VALUES (?,?,?)", true);

	for (const auto& prop : props)
	{
		if (hasSavedState(prop.name, std::nullopt))
			continue;

		ps.bind(1, prop.name);
		switch (prop.value.index())
		{
		case 0:  ps.bind(2, std::get<bool>(prop.value)); break;
		case 1:  ps.bind(2, std::get<int>(prop.value)); break;
		case 2:  ps.bind(2, std::get<unsigned>(prop.value)); break;
		case 3:  ps.bind(2, std::get<double>(prop.value)); break;
		case 4:  ps.bind(2, std::get<string>(prop.value)); break;
		default: continue;
		}
		ps.bind(3);

		ps.exec();
		ps.reset();
	}
}

// -----------------------------------------------------------------------------
// Returns true if saved state [name] exists in the database for [archive].
// If no archive is given the global saved state is checked
// -----------------------------------------------------------------------------
bool ui::hasSavedState(string_view name, const Archive* archive)
{
	return hasSavedState(name, archiveDbId(archive));
}

// -----------------------------------------------------------------------------
// Returns boolean UI state value [name] for [archive].
// If no archive is given or the value is not set for the archive, the global
// value is returned
// -----------------------------------------------------------------------------
bool ui::getStateBool(string_view name, const Archive* archive)
{
	auto val = false;

	auto ps = database::context().preparedStatement("get_ui_state", get_ui_state);
	ps.bind(1, name);
	ps.bind(2, archiveDbId(archive));
	if (ps.executeStep())
		val = ps.getColumn(0).getInt() > 0;
	else if (archive)
	{
		// No value for the archive, get global value
		ps.reset();
		ps.bind(2);
		if (ps.executeStep())
			val = ps.getColumn(0).getInt() > 0;
	}

	return val;
}

// -----------------------------------------------------------------------------
// Returns int UI state value [name] for [archive].
// If no archive is given or the value is not set for the archive, the global
// value is returned
// -----------------------------------------------------------------------------
int ui::getStateInt(string_view name, const Archive* archive)
{
	auto val = 0;

	auto ps = database::context().preparedStatement("get_ui_state", get_ui_state);
	ps.bind(1, name);
	ps.bind(2, archiveDbId(archive));
	if (ps.executeStep())
		val = ps.getColumn(0).getInt();
	else if (archive)
	{
		// No value for the archive, get global value
		ps.reset();
		ps.bind(2);
		if (ps.executeStep())
			val = ps.getColumn(0).getInt();
	}

	return val;
}

// -----------------------------------------------------------------------------
// Returns float UI state value [name] for [archive].
// If no archive is given or the value is not set for the archive, the global
// value is returned
// -----------------------------------------------------------------------------
double ui::getStateFloat(string_view name, const Archive* archive)
{
	auto val = 0.;

	auto ps = database::context().preparedStatement("get_ui_state", get_ui_state);
	ps.bind(1, name);
	ps.bind(2, archiveDbId(archive));
	if (ps.executeStep())
		val = ps.getColumn(0).getDouble();
	else if (archive)
	{
		// No value for the archive, get global value
		ps.reset();
		ps.bind(2);
		if (ps.executeStep())
			val = ps.getColumn(0).getDouble();
	}

	return val;
}

// -----------------------------------------------------------------------------
// Returns string UI state value [name] for [archive].
// If no archive is given or the value is not set for the archive, the global
// value is returned
// -----------------------------------------------------------------------------
string ui::getStateString(string_view name, const Archive* archive)
{
	string val;

	auto ps = database::context().preparedStatement("get_ui_state", get_ui_state);
	ps.bind(1, name);
	ps.bind(2, archiveDbId(archive));
	if (ps.executeStep())
		val = ps.getColumn(0).getString();
	else if (archive)
	{
		// No value for the archive, get global value
		ps.reset();
		ps.bind(2);
		if (ps.executeStep())
			val = ps.getColumn(0).getString();
	}

	return val;
}

// -----------------------------------------------------------------------------
// Sets UI boolean state [name] for [archive] to [value] in the database.
// If no archive is given, the global value is set.
// If an archive is given and [save_global] is true, the value will also be
// saved as the global value
// -----------------------------------------------------------------------------
void ui::saveStateBool(string_view name, bool value, const Archive* archive, bool save_global)
{
	auto db_id = archiveDbId(archive);
	if (db_id.has_value())
		saveState<bool>(name, value, db_id);
	if (!db_id.has_value() || save_global)
		saveState<bool>(name, value, std::nullopt);
}

// -----------------------------------------------------------------------------
// Sets UI int state [name] for [archive] to [value] in the database.
// If no archive is given, the global value is set.
// If an archive is given and [save_global] is true, the value will also be
// saved as the global value
// -----------------------------------------------------------------------------
void ui::saveStateInt(string_view name, int value, const Archive* archive, bool save_global)
{
	auto db_id = archiveDbId(archive);
	if (db_id.has_value())
		saveState<int>(name, value, db_id);
	if (!db_id.has_value() || save_global)
		saveState<int>(name, value, std::nullopt);
}

// -----------------------------------------------------------------------------
// Sets UI float state [name] for [archive] to [value] in the database.
// If no archive is given, the global value is set.
// If an archive is given and [save_global] is true, the value will also be
// saved as the global value
// -----------------------------------------------------------------------------
void ui::saveStateFloat(string_view name, double value, const Archive* archive, bool save_global)
{
	auto db_id = archiveDbId(archive);
	if (db_id.has_value())
		saveState<double>(name, value, db_id);
	if (!db_id.has_value() || save_global)
		saveState<double>(name, value, std::nullopt);
}

// -----------------------------------------------------------------------------
// Sets UI string state [name] for [archive] to [value] in the database.
// If no archive is given, the global value is set.
// If an archive is given and [save_global] is true, the value will also be
// saved as the global value
// -----------------------------------------------------------------------------
void ui::saveStateString(string_view name, string_view value, const Archive* archive, bool save_global)
{
	auto db_id = archiveDbId(archive);
	if (db_id.has_value())
		saveState<string_view>(name, value, db_id);
	if (!db_id.has_value() || save_global)
		saveState<string_view>(name, value, std::nullopt);
}

// -----------------------------------------------------------------------------
// Toggles UI boolean state [name] for [archive].
// If no archive is given, the global value is toggled
// -----------------------------------------------------------------------------
void ui::toggleStateBool(string_view name, const Archive* archive)
{
	auto ps = database::context().preparedStatement(
		"toggle_ui_state_bool",
		"UPDATE ui_state SET value = CASE value WHEN 0 THEN 1 ELSE 0 END WHERE name = ? AND archive_id IS ?",
		true);
	{
		ps.bind(1, name);
		ps.bind(2, archiveDbId(archive));
		ps.exec();
	}
}
