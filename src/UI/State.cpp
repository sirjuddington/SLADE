
#include "Main.h"
#include "State.h"
#include "General/Database.h"
#include "General/UI.h"
#include "Utility/Named.h"
#include "Utility/Property.h"

using namespace slade;
using namespace ui;


namespace
{
string get_ui_state = "SELECT value FROM ui_state WHERE name = ?";
string put_ui_state = "INSERT OR REPLACE INTO ui_state (name, value) VALUES (?,?)";
} // namespace


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

bool ui::hasSavedState(string_view name)
{
	return db::rowExists<string_view>(*db::connectionRO(), "ui_state", "name", name);
}

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
