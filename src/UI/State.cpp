
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Database/Database.h"
#include "Database/DbUtils.h"
#include "Database/Statement.h"
#include "General/UI.h"
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

// -----------------------------------------------------------------------------
// Initializes UI state values to defaults in the database
// (if they don't already exist there)
// -----------------------------------------------------------------------------
void ui::initStateProps()
{
	// Set default values
	vector<Named<Property>> props = { { "EntryListIndexVisible", false },
									  { "EntryListIndexWidth", scalePx(50) },
									  { "EntryListSizeVisible", true },
									  { "EntryListSizeWidth", scalePx(80) },
									  { "EntryListTypeVisible", true },
									  { "EntryListTypeWidth", scalePx(150) },
									  { "EntryListNameWidthList", scalePx(150) },
									  { "EntryListNameWidthTree", scalePx(180) },
									  { "ArchivePanelSplitPosList", scalePx(300) },
									  { "ArchivePanelSplitPosTree", scalePx(300) },
									  { "ArchiveLastCreatedFormat", "wad" },
									  { "ColouriseDialogLastColour", "RGB(255, 0, 0)" },
									  { "TintDialogLastColour", "RGB(255, 0, 0)" },
									  { "TintDialogLastAmount", 50 },
									  { "ZoomGfxCanvas", 100 },
									  { "ZoomCTextureCanvas", 100 },
									  { "LibraryPanelFilenameWidth", scalePx(250) },
									  { "LibraryPanelPathVisible", true },
									  { "LibraryPanelPathWidth", scalePx(250) },
									  { "LibraryPanelSizeVisible", true },
									  { "LibraryPanelSizeWidth", scalePx(100) },
									  { "LibraryPanelTypeVisible", true },
									  { "LibraryPanelTypeWidth", scalePx(150) },
									  { "LibraryPanelLastOpenedVisible", true },
									  { "LibraryPanelLastOpenedWidth", scalePx(200) },
									  { "LibraryPanelFileModifiedVisible", true },
									  { "LibraryPanelFileModifiedWidth", scalePx(200) },
									  { "LibraryPanelEntryCountVisible", true },
									  { "LibraryPanelEntryCountWidth", scalePx(80) },
									  { "LibraryPanelMapCountVisible", true },
									  { "LibraryPanelMapCountWidth", scalePx(80) },
									  { "BrowserWindowMaximized", false },
									  { "MainWindowMaximized", true },
									  { "MapEditorWindowMaximized", true },
									  { "ScriptManagerWindowMaximized", false },
									  { "SetupWizardRun", false } };

	if (auto sql = db::cacheQuery("init_ui_state", "INSERT OR IGNORE INTO ui_state VALUES (?,?)", true))
	{
		for (const auto& prop : props)
		{
			sql->bind(1, prop.name);
			switch (prop.value.index())
			{
			case 0:  sql->bind(2, std::get<bool>(prop.value)); break;
			case 1:  sql->bind(2, std::get<int>(prop.value)); break;
			case 2:  sql->bind(2, std::get<unsigned>(prop.value)); break;
			case 3:  sql->bind(2, std::get<double>(prop.value)); break;
			case 4:  sql->bind(2, std::get<string>(prop.value)); break;
			default: sql->clearBindings(); continue;
			}

			sql->exec();
			sql->reset();
		}
	}
}

// -----------------------------------------------------------------------------
// Returns true if saved state [name] exists in the database
// -----------------------------------------------------------------------------
bool ui::hasSavedState(string_view name)
{
	return db::rowExists<string_view>(*db::connectionRO(), "ui_state", "name", name);
}

// -----------------------------------------------------------------------------
// Returns boolean UI state value [name]
// -----------------------------------------------------------------------------
bool ui::getStateBool(string_view name)
{
	auto val = false;

	if (auto sql = db::cacheQuery("get_ui_state", get_ui_state))
	{
		sql->bind(1, name);
		if (sql->executeStep())
			val = sql->getColumn(0).getInt() > 0;
		sql->reset();
	}

	return val;
}

// -----------------------------------------------------------------------------
// Returns int UI state value [name]
// -----------------------------------------------------------------------------
int ui::getStateInt(string_view name)
{
	auto val = 0;

	if (auto sql = db::cacheQuery("get_ui_state", get_ui_state))
	{
		sql->bind(1, name);
		if (sql->executeStep())
			val = sql->getColumn(0).getInt();
		sql->reset();
	}

	return val;
}

// -----------------------------------------------------------------------------
// Returns float UI state value [name]
// -----------------------------------------------------------------------------
double ui::getStateFloat(string_view name)
{
	auto val = 0.;

	if (auto sql = db::cacheQuery("get_ui_state", get_ui_state))
	{
		sql->bind(1, name);
		if (sql->executeStep())
			val = sql->getColumn(0).getDouble();
		sql->reset();
	}

	return val;
}

// -----------------------------------------------------------------------------
// Returns string UI state value [name]
// -----------------------------------------------------------------------------
string ui::getStateString(string_view name)
{
	string val;

	if (auto sql = db::cacheQuery("get_ui_state", get_ui_state))
	{
		sql->bind(1, name);
		if (sql->executeStep())
			val = sql->getColumn(0).getString();
		sql->reset();
	}

	return val;
}

// -----------------------------------------------------------------------------
// Sets UI boolean state [name] to [value] in the database
// -----------------------------------------------------------------------------
void ui::saveStateBool(string_view name, bool value)
{
	if (auto sql = db::cacheQuery("put_ui_state", put_ui_state, true))
	{
		sql->bind(1, name);
		sql->bind(2, value);
		sql->exec();
		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Sets UI int state [name] to [value] in the database
// -----------------------------------------------------------------------------
void ui::saveStateInt(string_view name, int value)
{
	if (auto sql = db::cacheQuery("put_ui_state", put_ui_state, true))
	{
		sql->bind(1, name);
		sql->bind(2, value);
		sql->exec();
		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Sets UI float state [name] to [value] in the database
// -----------------------------------------------------------------------------
void ui::saveStateFloat(string_view name, double value)
{
	if (auto sql = db::cacheQuery("put_ui_state", put_ui_state, true))
	{
		sql->bind(1, name);
		sql->bind(2, value);
		sql->exec();
		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Sets UI string state [name] to [value] in the database
// -----------------------------------------------------------------------------
void ui::saveStateString(string_view name, string_view value)
{
	if (auto sql = db::cacheQuery("put_ui_state", put_ui_state, true))
	{
		sql->bind(1, name);
		sql->bind(2, value);
		sql->exec();
		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Toggles UI boolean state [name]
// -----------------------------------------------------------------------------
void ui::toggleStateBool(string_view name)
{
	if (auto sql = db::cacheQuery(
			"toggle_ui_state_bool",
			"UPDATE ui_state SET value = CASE value WHEN 0 THEN 1 ELSE 0 END WHERE name = ?",
			true))
	{
		sql->bind(1, name);
		sql->exec();
		sql->reset();
	}
}
