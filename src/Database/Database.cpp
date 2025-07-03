
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Database.cpp
// Description: Functions for working with the SLADE program database
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
#include "Database.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveFormatHandler.h"
#include "Context.h"
#include "General/Console.h"
#include "Tables/ArchiveFile.h"
#include "UI/State.h"
#include "UI/UI.h"
#include "Utility/DateTime.h"
#include "Utility/FileUtils.h"
#include "Utility/StringPair.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include <SQLiteCpp/Database.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::database
{
int db_version = 1;
} // namespace slade::database


// -----------------------------------------------------------------------------
//
// Database Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::database
{
// -----------------------------------------------------------------------------
// Migrates the window layout from a pre-3.3.0 .layout file to the database
// -----------------------------------------------------------------------------
void migrateWindowLayout(string_view filename, const char* window_id)
{
	// Open layout file
	Tokenizer tz;
	if (!tz.openFile(app::path(filename, app::Dir::User)))
		return;

	// Parse layout
	vector<StringPair> layouts;
	while (true)
	{
		// Read component+layout pair
		auto component = tz.getToken();
		auto layout    = tz.getToken();
		layouts.emplace_back(component, layout);

		// Check if we're done
		if (tz.peekToken().empty())
			break;
	}

	ui::setWindowLayout(window_id, layouts);
}

// -----------------------------------------------------------------------------
// Creates any missing tables/views in the SLADE database [db]
// -----------------------------------------------------------------------------
void createMissingTables(Context& db)
{
	// Get slade.pk3 dir with table definition scripts
	auto tables_dir = app::programResource()->dirAtPath("database/tables");
	if (!tables_dir)
		throw std::runtime_error("Unable to initialize SLADE database: no table definitions in slade.pk3");

	for (const auto& entry : tables_dir->entries())
	{
		// Check table exists
		string table_name{ strutil::Path::fileNameOf(entry->name(), false) };
		if (db.tableExists(table_name))
			continue;

		// Doesn't exist, create table
		string sql{ reinterpret_cast<const char*>(entry->data().data()), entry->data().size() };
		try
		{
			db.exec(sql);
			log::info("Created database table {}", table_name);
		}
		catch (const std::exception& ex)
		{
			throw std::runtime_error(fmt::format("Failed to create database table {}: {}", table_name, ex.what()));
		}
	}

	// Get slade.pk3 dir with view definition scripts
	if (auto views_dir = app::programResource()->dirAtPath("database/views"))
	{
		for (const auto& entry : views_dir->entries())
		{
			// Check view exists
			string view_name{ strutil::Path::fileNameOf(entry->name(), false) };
			if (db.viewExists(view_name))
				continue;

			// Doesn't exist, create view
			string sql{ reinterpret_cast<const char*>(entry->data().data()), entry->data().size() };
			try
			{
				db.exec(sql);
				log::info("Created database view {}", view_name);
			}
			catch (const std::exception& ex)
			{
				throw std::runtime_error(fmt::format("Failed to create database view {}: {}", view_name, ex.what()));
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Creates and initializes a new program database file at [file_path]
// -----------------------------------------------------------------------------
void createDatabase(const string& file_path)
{
	auto db = Context(file_path, true);

	// Create tables
	createMissingTables(db);

	// Init db_info table
	try
	{
		SQLite::Statement sql{ *db.connectionRW(), "INSERT INTO db_info (version) VALUES (?)" };
		sql.bind(1, db_version);
		sql.exec();
	}
	catch (const std::exception& ex)
	{
		throw std::runtime_error(fmt::format("Failed to initialize database: {}", ex.what()));
	}
}

// -----------------------------------------------------------------------------
// Updates the program database tables
// -----------------------------------------------------------------------------
void updateDatabase(int prev_version)
{
	log::info("Updating database from v{} to v{}...", prev_version, db_version);

	// Create missing tables
	auto& db = context();
	createMissingTables(db);

	// Update db_info.version
	db.exec(fmt::format("UPDATE db_info SET version = {}", db_version));

	// Done
	log::info("Database updated to v{} successfully", db_version);
}
} // namespace slade::database

// -----------------------------------------------------------------------------
// Returns true if the program database file exists
// -----------------------------------------------------------------------------
bool database::fileExists()
{
	return fileutil::fileExists(app::path("slade.sqlite", app::Dir::User));
}

// -----------------------------------------------------------------------------
// Returns the path to the program database file
// -----------------------------------------------------------------------------
string database::programDatabasePath()
{
	return app::path("slade.sqlite", app::Dir::User);
}

// -----------------------------------------------------------------------------
// Initialises the program database, creating it if it doesn't exist and opening
// the 'global' connection context.
// Returns false if the database couldn't be created or the global context
// failed to open, true otherwise
// -----------------------------------------------------------------------------
bool database::init()
{
	try
	{
		auto db_path = programDatabasePath();

		// Create database if needed
		bool created = false;
		if (!fileutil::fileExists(db_path))
		{
			createDatabase(db_path);
			created = true;
		}

		// Open global connections to database (for main thread usage only)
		auto& db = context();
		db.open(db_path);

		// Migrate old config stuff to database
		if (created)
			migrateConfigs();

		// Update the database if needed
		auto existing_version = db.connectionRO()->execAndGet("SELECT version FROM db_info").getInt();
		if (existing_version < db_version)
			updateDatabase(existing_version);
	}
	catch (const std::exception& ex)
	{
		log::error("Failed to initialize database: {}", ex.what());
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Closes the global connection context to the database
// -----------------------------------------------------------------------------
void database::close()
{
	context().vacuum(); // Shrink size on disk
	context().close();
}

// -----------------------------------------------------------------------------
// Migrates various configurations from text/cfg files to the SLADE program
// database
// -----------------------------------------------------------------------------
void database::migrateConfigs()
{
#define MIGRATE_CVAR_BOOL(cvar, state) else if (tz.check(#cvar)) ui::saveStateBool(ui::state, tz.peek().asBool())
#define MIGRATE_CVAR_INT(cvar, state) else if (tz.check(#cvar)) ui::saveStateInt(ui::state, tz.peek().asInt())
#define MIGRATE_CVAR_INT_DIP(cvar, state) \
	else if (tz.check(#cvar)) ui::saveStateInt(ui::state, wxWindow::ToDIP(tz.peek().asInt(), nullptr))
#define MIGRATE_CVAR_STRING(cvar, state) else if (tz.check(#cvar)) ui::saveStateString(ui::state, tz.peek().text)

	// Migrate window layouts from .layout files
	// (except script manager which has a different format)
	migrateWindowLayout("mainwindow.layout", "main");
	migrateWindowLayout("mapwindow.layout", "map");

	// Migrate various things from pre-3.3.0 config
	Tokenizer tz;
	if (!tz.openFile(app::path("slade3.cfg", app::Dir::User)))
		return;
	while (!tz.atEnd())
	{
		// Migrate old CVars to UI state table
		if (tz.advIf("cvars", 2))
		{
			while (!tz.checkOrEnd("}"))
			{
				// Last archive format
				if (tz.check("archive_last_created_format"))
					ui::saveStateString(ui::ARCHIVE_LAST_CREATED_FORMAT, tz.peek().text);

				// Window maximized flags
				MIGRATE_CVAR_BOOL(browser_maximised, BROWSERWINDOW_MAXIMIZED);
				MIGRATE_CVAR_BOOL(mw_maximized, MAINWINDOW_MAXIMIZED);
				MIGRATE_CVAR_BOOL(mew_maximized, MAPEDITORWINDOW_MAXIMIZED);
				MIGRATE_CVAR_BOOL(sm_maximized, SCRIPTMANAGERWINDOW_MAXIMIZED);

				// Entry list column widths
				MIGRATE_CVAR_INT_DIP(elist_colsize_index, ENTRYLIST_INDEX_WIDTH);
				MIGRATE_CVAR_INT_DIP(elist_colsize_size, ENTRYLIST_SIZE_WIDTH);
				MIGRATE_CVAR_INT_DIP(elist_colsize_type, ENTRYLIST_TYPE_WIDTH);
				MIGRATE_CVAR_INT_DIP(elist_colsize_name_list, ENTRYLIST_NAME_WIDTH_LIST);
				MIGRATE_CVAR_INT_DIP(elist_colsize_name_tree, ENTRYLIST_NAME_WIDTH_TREE);

				// Entry list column visibility
				MIGRATE_CVAR_BOOL(elist_colindex_show, ENTRYLIST_INDEX_VISIBLE);
				MIGRATE_CVAR_BOOL(elist_colsize_show, ENTRYLIST_SIZE_VISIBLE);
				MIGRATE_CVAR_BOOL(elist_coltype_show, ENTRYLIST_TYPE_VISIBLE);

				// Splitter position
				MIGRATE_CVAR_INT_DIP(ap_splitter_position_list, ARCHIVEPANEL_SPLIT_POS_LIST);
				MIGRATE_CVAR_INT_DIP(ap_splitter_position_tree, ARCHIVEPANEL_SPLIT_POS_TREE);

				// Colourize/Tint Dialogs
				MIGRATE_CVAR_STRING(last_colour, COLOURISEDIALOG_LAST_COLOUR);
				MIGRATE_CVAR_STRING(last_tint_colour, TINTDIALOG_LAST_COLOUR);
				MIGRATE_CVAR_INT(last_tint_amount, TINTDIALOG_LAST_AMOUNT);

				// Zoom sliders
				MIGRATE_CVAR_INT(zoom_gfx, ZOOM_GFXCANVAS);
				MIGRATE_CVAR_INT(zoom_ctex, ZOOM_CTEXTURECANVAS);

				// Misc.
				MIGRATE_CVAR_BOOL(setup_wizard_run, SETUP_WIZARD_RUN);

				// Run Dialog
				MIGRATE_CVAR_STRING(run_last_exe, RUNDIALOG_LAST_EXE);
				MIGRATE_CVAR_INT(run_last_config, RUNDIALOG_LAST_CONFIG);
				MIGRATE_CVAR_STRING(run_last_extra, RUNDIALOG_LAST_EXTRA);
				MIGRATE_CVAR_BOOL(run_start_3d, RUNDIALOG_START_3D);

				tz.adv(2);
			}

			tz.adv(); // Skip ending }
		}

		// Migrate window size/position info
		if (tz.advIf("window_info", 2))
		{
			tz.advIf("{");
			while (!tz.check("}") && !tz.atEnd())
			{
				auto id     = tz.current().text;
				int  width  = wxWindow::ToDIP(tz.next().asInt(), nullptr);
				int  height = wxWindow::ToDIP(tz.next().asInt(), nullptr);
				int  left   = wxWindow::ToDIP(tz.next().asInt(), nullptr);
				int  top    = wxWindow::ToDIP(tz.next().asInt(), nullptr);
				ui::setWindowInfo(nullptr, id.c_str(), width, height, left, top);
				tz.adv();
			}
		}

		// Migrate recent files
		if (tz.advIf("recent_files", 2))
		{
			auto now = datetime::now();

			while (!tz.checkOrEnd("}"))
			{
				auto path = tz.current().text;

				if (archiveFileId(path) < 0)
				{
					// Recent file entry isn't already in the database, get archive info
					ArchiveFile archive_file;
					archive_file.path        = path;
					archive_file.last_opened = now++; // To keep the correct order as recent files in the config are
													  // from least to most recent

					if (fileutil::fileExists(path))
					{
						// File
						SFile file(path);
						if (!file.isOpen())
							continue;

						auto archive_format = archive::detectArchiveFormat(path);

						archive_file.size          = file.size();
						archive_file.hash          = file.calculateHash();
						archive_file.format_id     = archive::formatId(archive_format);
						archive_file.last_modified = fileutil::fileModifiedTime(path);
					}
					else if (fileutil::dirExists(path))
					{
						// Directory
						archive_file.format_id = "folder";
					}
					else
					{
						// Recent file no longer exists, don't add it
						tz.adv();
						continue;
					}

					archive_file.insert();
				}

				tz.adv();
			}

			tz.adv(); // Skip ending }
		}

		// Next token
		tz.adv();
	}
#undef MIGRATE_CVAR_BOOL
#undef MIGRATE_CVAR_INT
#undef MIGRATE_CVAR_INT_DIP
#undef MIGRATE_CVAR_STRING
}

// -----------------------------------------------------------------------------
// Returns the struct containing all database signals
// -----------------------------------------------------------------------------
database::Signals& database::signals()
{
	static Signals signals;
	return signals;
}


// -----------------------------------------------------------------------------
//
// Console Commands
//
// -----------------------------------------------------------------------------


void           c_db(const vector<string>& args);
ConsoleCommand db_cmd("db", &c_db, 1, false);
void           c_db(const vector<string>& args)
{
	const auto& command = args[0];

	try
	{
		// List tables
		if (command == "tables")
		{
			if (auto db = database::context().connectionRO())
			{
				SQLite::Statement sql(*db, "SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name");
				while (sql.executeStep())
					log::console(sql.getColumn(0).getString());
			}
		}

		// Row count of table
		else if (command == "rowcount")
		{
			if (args.size() < 2)
			{
				log::console("No table name given. Usage: db rowcount <tablename>");
				return;
			}

			if (auto db = database::context().connectionRO())
			{
				SQLite::Statement sql(*db, fmt::format("SELECT COUNT(*) FROM {}", args[1]));
				if (sql.executeStep())
					log::console("{} rows", sql.getColumn(0).getInt());
				else
					log::console("No such table");
			}
		}

		// Reset table from template
		else if (command == "reset")
		{
			if (args.size() < 2)
			{
				log::console("No table name given. Usage: db reset <tablename>");
				return;
			}

			const auto& table = args[1];

			if (auto db = database::context().connectionRW())
			{
				auto sql_entry = app::programResource()->entryAtPath(fmt::format("database/tables/{}.sql", table));
				if (!sql_entry)
				{
					log::console("Can't find table sql script for {}", table);
					return;
				}

				string sql{ reinterpret_cast<const char*>(sql_entry->data().data()), sql_entry->data().size() };
				db->exec(fmt::format("DROP TABLE IF EXISTS {}", table));
				db->exec(sql);
				log::console("Table {} recreated and reset to default", table);
			}
		}
	}
	catch (const std::exception& ex)
	{
		log::error(ex.what());
	}
}
