
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
string get_ui_state = "SELECT value FROM ui_state WHERE name = ?";
string put_ui_state = "INSERT OR REPLACE INTO ui_state (name, value) VALUES (?,?)";
} // namespace


// -----------------------------------------------------------------------------
//
// UI Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::ui
{
template<typename T> void saveState(string_view name, T value)
{
	auto ps = database::context().preparedStatement("put_ui_state", put_ui_state, true);
	ps.bind(1, name);
	ps.bind(2, value);
	ps.exec();
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
									  { ENTRYLIST_SIZE_WIDTH, 80 },
									  { ENTRYLIST_TYPE_VISIBLE, true },
									  { ENTRYLIST_TYPE_WIDTH, 150 },
									  { ENTRYLIST_NAME_WIDTH_LIST, 150 },
									  { ENTRYLIST_NAME_WIDTH_TREE, 180 },
									  { ARCHIVEPANEL_SPLIT_POS_LIST, 300 },
									  { ARCHIVEPANEL_SPLIT_POS_TREE, 300 },
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

	auto ps = database::context().preparedStatement(
		"init_ui_state", "INSERT OR IGNORE INTO ui_state VALUES (?,?)", true);

	for (const auto& prop : props)
	{
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

		ps.exec();
		ps.reset();
	}
}

// -----------------------------------------------------------------------------
// Returns true if saved state [name] exists in the database
// -----------------------------------------------------------------------------
bool ui::hasSavedState(const char* name)
{
	auto ps = database::context().preparedStatement("ui_has_saved_state", "SELECT * FROM ui_state WHERE name = ?");
	ps.bind(1, name);
	return ps.executeStep();
}

// -----------------------------------------------------------------------------
// Returns boolean UI state value [name]
// -----------------------------------------------------------------------------
bool ui::getStateBool(string_view name)
{
	auto val = false;

	auto ps = database::context().preparedStatement("get_ui_state", get_ui_state);
	ps.bind(1, name);
	if (ps.executeStep())
		val = ps.getColumn(0).getInt() > 0;

	return val;
}

// -----------------------------------------------------------------------------
// Returns int UI state value [name]
// -----------------------------------------------------------------------------
int ui::getStateInt(string_view name)
{
	auto val = 0;

	auto ps = database::context().preparedStatement("get_ui_state", get_ui_state);
	ps.bind(1, name);
	if (ps.executeStep())
		val = ps.getColumn(0).getInt();

	return val;
}

// -----------------------------------------------------------------------------
// Returns float UI state value [name]
// -----------------------------------------------------------------------------
double ui::getStateFloat(string_view name)
{
	auto val = 0.;

	auto ps = database::context().preparedStatement("get_ui_state", get_ui_state);
	ps.bind(1, name);
	if (ps.executeStep())
		val = ps.getColumn(0).getDouble();

	return val;
}

// -----------------------------------------------------------------------------
// Returns string UI state value [name]
// -----------------------------------------------------------------------------
string ui::getStateString(string_view name)
{
	string val;

	auto ps = database::context().preparedStatement("get_ui_state", get_ui_state);
	ps.bind(1, name);
	if (ps.executeStep())
		val = ps.getColumn(0).getString();

	return val;
}

// -----------------------------------------------------------------------------
// Sets UI boolean state [name] to [value] in the database
// -----------------------------------------------------------------------------
void ui::saveStateBool(string_view name, bool value)
{
	return saveState<bool>(name, value);
}

// -----------------------------------------------------------------------------
// Sets UI int state [name] to [value] in the database
// -----------------------------------------------------------------------------
void ui::saveStateInt(string_view name, int value)
{
	return saveState<int>(name, value);
}

// -----------------------------------------------------------------------------
// Sets UI float state [name] to [value] in the database
// -----------------------------------------------------------------------------
void ui::saveStateFloat(string_view name, double value)
{
	return saveState<double>(name, value);
}

// -----------------------------------------------------------------------------
// Sets UI string state [name] to [value] in the database
// -----------------------------------------------------------------------------
void ui::saveStateString(string_view name, string_view value)
{
	return saveState<string_view>(name, value);
}

// -----------------------------------------------------------------------------
// Toggles UI boolean state [name]
// -----------------------------------------------------------------------------
void ui::toggleStateBool(string_view name)
{
	auto ps = database::context().preparedStatement(
		"toggle_ui_state_bool", "UPDATE ui_state SET value = CASE value WHEN 0 THEN 1 ELSE 0 END WHERE name = ?", true);
	{
		ps.bind(1, name);
		ps.exec();
	}
}
