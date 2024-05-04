
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "Context.h"
#include "General/Console.h"
#include "General/UI.h"
#include "UI/State.h"
#include "Utility/DateTime.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include "thirdparty/sqlitecpp/sqlite3/sqlite3.h"
#include <SQLiteCpp/Database.h>

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::database
{
int     db_version = 1;
int64_t session_id = -1;
} // namespace slade::database


// -----------------------------------------------------------------------------
//
// Database Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::database
{
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
bool createMissingTables(SQLite::Database& db)
{
	// Get slade.pk3 dir with table definition scripts
	auto tables_dir = app::programResource()->dirAtPath("database/tables");
	if (!tables_dir)
	{
		global::error = "Unable to initialize SLADE database: no table definitions in slade.pk3";
		return false;
	}

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
		catch (const SQLite::Exception& ex)
		{
			global::error = fmt::format("Failed to create database table {}: {}", table_name, ex.what());
			return false;
		}
	}

	// Get slade.pk3 dir with view definition scripts
	auto views_dir = app::programResource()->dirAtPath("database/views");
	if (views_dir)
	{
		for (const auto& entry : views_dir->entries())
		{
			// Check view exists
			string view_name{ strutil::Path::fileNameOf(entry->name(), false) };
			if (viewExists(view_name, db))
				continue;

			// Doesn't exist, create view
			string sql{ reinterpret_cast<const char*>(entry->data().data()), entry->data().size() };
			try
			{
				db.exec(sql);
				log::info("Created database view {}", view_name);
			}
			catch (const SQLite::Exception& ex)
			{
				global::error = fmt::format("Failed to create database view {}: {}", view_name, ex.what());
				return false;
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Creates and initializes a new program database file at [file_path]
// -----------------------------------------------------------------------------
bool createDatabase(const string& file_path)
{
	SQLite::Database db(file_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

	// Create tables
	if (!createMissingTables(db))
		return false;

	// Init db_info table
	try
	{
		Statement sql{ db, "INSERT INTO db_info (version) VALUES (?)" };
		sql.bind(1, db_version);
		sql.exec();
	}
	catch (const SQLite::Exception& ex)
	{
		global::error = fmt::format("Failed to initialize database: {}", ex.what());
		log::error(global::error);
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Updates the program database tables
// -----------------------------------------------------------------------------
bool updateDatabase(int prev_version)
{
	log::info("Updating database from v{} to v{}...", prev_version, db_version);

	// Create missing tables
	if (!createMissingTables(*connectionRW()))
		return false;

	// Update db_info.version
	exec(fmt::format("UPDATE db_info SET version = {}", db_version));

	// Done
	log::info("Database updated to v{} successfully", db_version);
	return true;
}

// -----------------------------------------------------------------------------
// Writes a new session row to the database
// -----------------------------------------------------------------------------
bool beginSession()
{
	try
	{
		Statement sql_new_session{ *connectionRW(),
								   "INSERT INTO session (opened_time, closed_time, version_major, version_minor, "
								   "version_revision, version_beta) "
								   "VALUES (?,?,?,?,?,?)" };

		sql_new_session.bind(1, datetime::now());
		sql_new_session.bind(2); // NULL closed time
		sql_new_session.bind(3, static_cast<int32_t>(app::version().major));
		sql_new_session.bind(4, static_cast<int32_t>(app::version().minor));
		sql_new_session.bind(5, static_cast<int32_t>(app::version().revision));
		sql_new_session.bind(6, static_cast<int32_t>(app::version().beta));

		if (sql_new_session.exec() == 0)
		{
			global::error = "Failed to initialize database: Session row creation failed";
			return false;
		}

		session_id = connectionRO()->execAndGet("SELECT MAX(id) FROM session").getInt64();

		return true;
	}
	catch (const SQLite::Exception& ex)
	{
		global::error = fmt::format("Failed to initialize database: {}", ex.what());
		log::error(global::error);
		return false;
	}
}

// -----------------------------------------------------------------------------
// Sets the closed time in the current session row
// -----------------------------------------------------------------------------
bool endSession()
{
	try
	{
		Statement sql_close_session{ *connectionRW(), "UPDATE session SET closed_time = ? WHERE id = ?" };
		sql_close_session.bind(1, datetime::now());
		sql_close_session.bind(2, session_id);
		sql_close_session.exec();

		session_id = -1;

		return true;
	}
	catch (const SQLite::Exception& ex)
	{
		log::error(ex.what());
		return false;
	}
}
} // namespace slade::database

// -----------------------------------------------------------------------------
// Executes an sql [query] on the database using the given [connection].
// If [connection] is null, the global read+write connection is used.
// Returns the number of rows modified/created by the query, or 0 if the global
// connection context is not connected
// -----------------------------------------------------------------------------
int database::exec(const string& query, SQLite::Database* connection)
{
	if (!connection)
		connection = connectionRW();
	if (!connection)
		return 0;

	return connection->exec(query);
}
int database::exec(const char* query, SQLite::Database* connection)
{
	if (!connection)
		connection = connectionRW();
	if (!connection)
		return 0;

	return connection->exec(query);
}

// -----------------------------------------------------------------------------
// Returns true if a view with [view_name] exists in the database [connection]
// -----------------------------------------------------------------------------
bool database::viewExists(string_view view_name, const SQLite::Database& connection)
{
	Statement query(connection, "SELECT count(*) FROM sqlite_master WHERE type='view' AND name=?");
	query.bind(1, view_name);
	(void)query.executeStep(); // Cannot return false, as the above query always return a result
	return (1 == query.getColumn(0).getInt());
}

// -----------------------------------------------------------------------------
// Returns true if a row exists in [table_name] where [id_col] = [id].
// The column must be an integer column for this to work correctly
// -----------------------------------------------------------------------------
bool database::rowIdExists(string_view table_name, int64_t id, string_view id_col)
{
	return global().rowIdExists(table_name, id, id_col);
}

// -----------------------------------------------------------------------------
// Returns the cached query [id] if it exists, otherwise creates a new cached
// query from the given [sql] string and returns it.
// If [writes] is true, the created query will use the read+write connection.
// -----------------------------------------------------------------------------
database::Statement* database::cacheQuery(string_view id, string_view sql, bool writes)
{
	return global().cacheQuery(id, sql, writes);
}

// -----------------------------------------------------------------------------
// Returns the read-only connection to the database (for the calling thread)
// -----------------------------------------------------------------------------
SQLite::Database* database::connectionRO()
{
	return global().connectionRO();
}

// -----------------------------------------------------------------------------
// Returns the read+write connection to the database (for the calling thread)
// -----------------------------------------------------------------------------
SQLite::Database* database::connectionRW()
{
	return global().connectionRW();
}

// -----------------------------------------------------------------------------
// Returns true if the program database file exists
// -----------------------------------------------------------------------------
bool database::fileExists()
{
	return fileutil::fileExists(app::path("slade.sqlite", app::Dir::User));
}

// -----------------------------------------------------------------------------
// Returns true if a transaction (BEGIN -> COMMIT/ROLLBACK) is currently active
// on [connection]
// -----------------------------------------------------------------------------
bool database::isTransactionActive(const SQLite::Database* connection)
{
	return sqlite3_get_autocommit(connection->getHandle()) == 0;
}

// -----------------------------------------------------------------------------
// Returns the path to the program database file
// -----------------------------------------------------------------------------
string database::programDatabasePath()
{
	return app::path("slade.sqlite", app::Dir::User);
}

// -----------------------------------------------------------------------------
// Returns the current database session id
// -----------------------------------------------------------------------------
int64_t database::sessionId()
{
	return session_id;
}

// -----------------------------------------------------------------------------
// Initialises the program database, creating it if it doesn't exist and opening
// the 'global' connection context.
// Returns false if the database couldn't be created or the global context
// failed to open, true otherwise
// -----------------------------------------------------------------------------
bool database::init()
{
	auto db_path = app::path("slade.sqlite", app::Dir::User);

	// Create database if needed
	bool created = false;
	if (!fileutil::fileExists(db_path))
	{
		if (!createDatabase(db_path))
			return false;

		created = true;
	}

	// Open global connections to database (for main thread usage only)
	if (!global().open(db_path))
	{
		global::error = "Unable to open global database connections";
		return false;
	}

	// Migrate pre-3.3.0 config stuff to database
	if (created)
		migrateConfigs();

	// Update the database if needed
	auto existing_version = connectionRO()->execAndGet("SELECT version FROM db_info").getInt();
	if (existing_version < db_version)
		return updateDatabase(existing_version);

	// Write new session
	if (!beginSession())
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Closes the global connection context to the database
// -----------------------------------------------------------------------------
void database::close()
{
	endSession();
	global().vacuum(); // Shrink size on disk
	global().close();
}

// -----------------------------------------------------------------------------
// Migrates various configurations from text/cfg files (pre-3.3.0) to the
// SLADE program database
// -----------------------------------------------------------------------------
void database::migrateConfigs()
{
#define MIGRATE_CVAR_BOOL(cvar, state) else if (tz.check(#cvar)) ui::saveStateBool(#state, tz.peek().asBool())
#define MIGRATE_CVAR_INT(cvar, state) else if (tz.check(#cvar)) ui::saveStateInt(#state, tz.peek().asInt())
#define MIGRATE_CVAR_STRING(cvar, state) else if (tz.check(#cvar)) ui::saveStateString(#state, tz.peek().text)

	// Migrate window layouts from .layout files
	migrateWindowLayout("mainwindow.layout", "main");
	migrateWindowLayout("mapwindow.layout", "map");
	migrateWindowLayout("scriptmanager.layout", "scriptmanager");

	// Migrate various things from SLADE.cfg
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
					ui::saveStateString("ArchiveLastCreatedFormat", tz.peek().text);

				// Window maximized flags
				MIGRATE_CVAR_BOOL(browser_maximised, BrowserWindowMaximized);
				MIGRATE_CVAR_BOOL(mw_maximized, MainWindowMaximized);
				MIGRATE_CVAR_BOOL(mew_maximized, MapEditorWindowMaximized);
				MIGRATE_CVAR_BOOL(sm_maximized, ScriptManagerWindowMaximized);

				// Entry list column widths
				MIGRATE_CVAR_INT(elist_colsize_index, EntryListIndexWidth);
				MIGRATE_CVAR_INT(elist_colsize_size, EntryListSizeWidth);
				MIGRATE_CVAR_INT(elist_colsize_type, EntryListTypeWidth);
				MIGRATE_CVAR_INT(elist_colsize_name_list, EntryListNameWidthList);
				MIGRATE_CVAR_INT(elist_colsize_name_tree, EntryListNameWidthTree);

				// Entry list column visibility
				MIGRATE_CVAR_BOOL(elist_colindex_show, EntryListIndexVisible);
				MIGRATE_CVAR_BOOL(elist_colsize_show, EntryListSizeVisible);
				MIGRATE_CVAR_BOOL(elist_coltype_show, EntryListTypeVisible);

				// Splitter position
				MIGRATE_CVAR_INT(ap_splitter_position_list, ArchivePanelSplitPosList);
				MIGRATE_CVAR_INT(ap_splitter_position_tree, ArchivePanelSplitPosTree);

				// Colourize/Tint Dialogs
				MIGRATE_CVAR_STRING(last_colour, ColouriseDialogLastColour);
				MIGRATE_CVAR_STRING(last_tint_colour, TintDialogLastColour);
				MIGRATE_CVAR_INT(last_tint_amount, TintDialogLastAmount);

				// Zoom sliders
				MIGRATE_CVAR_INT(zoom_gfx, ZoomGfxCanvas);
				MIGRATE_CVAR_INT(zoom_ctex, ZoomCTextureCanvas);

				// Misc.
				MIGRATE_CVAR_BOOL(setup_wizard_run, SetupWizardRun);

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
				int  width  = tz.next().asInt();
				int  height = tz.next().asInt();
				int  left   = tz.next().asInt();
				int  top    = tz.next().asInt();
				ui::setWindowInfo(id.c_str(), width, height, left, top);
				tz.adv();
			}
		}

		// Next token
		tz.adv();
	}
#undef MIGRATE_CVAR_BOOL
#undef MIGRATE_CVAR_INT
#undef MIGRATE_CVAR_STRING
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
			if (auto db = database::connectionRO())
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

			if (auto db = database::connectionRO())
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

			if (auto db = database::connectionRW())
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
	catch (const SQLite::Exception& ex)
	{
		log::error(ex.what());
	}
}
