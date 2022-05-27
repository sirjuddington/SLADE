
#include "Main.h"
#include "Library.h"
#include "Archive/Archive.h"
#include "General/Database.h"
#include "Utility/DateTime.h"
#include "Utility/FileUtils.h"

using namespace slade;
using namespace library;


namespace slade::library
{
Signals lib_signals;

// SQL query strings
string update_archive_file =
	"UPDATE archive_file "
	"SET path = ?, size = ?, hash = ?, format_id = ?, last_opened = ?, last_modified = ? "
	"WHERE id = ?";
string insert_archive_file =
	"REPLACE INTO archive_file (path, size, hash, format_id, last_opened, last_modified) "
	"VALUES (?,?,?,?,?,?)";
string update_archive_entry =
	"UPDATE archive_entry "
	"SET archive_id = ?, name = ?, size = ?, hash = ?, type_id = ? "
	"WHERE id = ?";
string insert_archive_entry =
	"INSERT INTO archive_entry (archive_id, name, size, hash, type_id) "
	"VALUES (?,?,?,?,?)";
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


namespace slade::library
{
ArchiveFile getArchiveFileRow(int64_t id)
{
	ArchiveFile archive_file;

	if (auto sql = database::global().cacheQuery("get_archive_file", "SELECT * FROM archive_file WHERE id = ?"))
	{
		sql->clearBindings();
		sql->bind(1, id);

		if (sql->executeStep())
		{
			archive_file.id            = id;
			archive_file.path          = sql->getColumn("path").getString();
			archive_file.size          = sql->getColumn("size").getUInt();
			archive_file.hash          = sql->getColumn("hash").getString();
			archive_file.format_id     = sql->getColumn("format_id").getString();
			archive_file.last_modified = sql->getColumn("last_modified").getInt64();
			archive_file.last_opened   = sql->getColumn("last_opened").getInt64();
		}

		sql->reset();
	}

	return archive_file;
}

int updateArchiveFileRow(const ArchiveFile& row)
{
	// Ignore invalid id
	if (row.id < 0)
		return 0;

	auto rows = 0;
	if (auto sql = database::global().cacheQuery("update_archive_file", update_archive_file.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, row.path);
		sql->bind(2, row.size);
		sql->bind(3, row.hash);
		sql->bind(4, row.format_id);
		sql->bind(5, row.last_opened);
		sql->bind(6, row.last_modified);
		sql->bind(7, row.id);

		rows = sql->exec();
		sql->reset();
	}

	if (rows > 0)
		lib_signals.archive_file_updated();

	return rows;
}

int64_t insertArchiveFileRow(const ArchiveFile& row)
{
	int64_t row_id = -1;

	if (auto sql = database::global().cacheQuery("insert_archive_file", insert_archive_file.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, row.path);
		sql->bind(2, row.size);
		sql->bind(3, row.hash);
		sql->bind(4, row.format_id);
		sql->bind(5, row.last_opened);
		sql->bind(6, row.last_modified);

		if (sql->exec() > 0)
			row_id = database::global().connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	if (row_id >= 0)
		lib_signals.archive_file_updated();

	return row_id;
}

ArchiveUIConfig getArchiveUIConfigRow(int64_t archive_id)
{
	ArchiveUIConfig row;

	if (auto sql = database::global().cacheQuery(
			"get_archive_ui_config", "SELECT * FROM archive_ui_config WHERE archive_id = ?"))
	{
		sql->clearBindings();
		sql->bind(1, archive_id);

		if (sql->executeStep())
		{
			row.archive_id            = archive_id;
			row.elist_index_visible   = sql->getColumn(1).getInt() > 0;
			row.elist_index_width     = sql->getColumn(2).getInt();
			row.elist_name_width      = sql->getColumn(3).getInt();
			row.elist_size_visible    = sql->getColumn(4).getInt() > 0;
			row.elist_size_width      = sql->getColumn(5).getInt();
			row.elist_type_visible    = sql->getColumn(6).getInt() > 0;
			row.elist_type_width      = sql->getColumn(7).getInt();
			row.elist_sort_column     = sql->getColumn(8).getString();
			row.elist_sort_descending = sql->getColumn(9).getInt() > 0;
			row.splitter_position     = sql->getColumn(10).getInt();
		}

		sql->reset();
	}

	return row;
}

int updateArchiveUIConfigRow(const ArchiveUIConfig& row)
{
	// Ignore invalid id
	if (row.archive_id < 0)
		return 0;

	auto rows = 0;
	if (auto sql = database::global().cacheQuery("update_archive_ui_config", update_archive_ui_config.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, row.elist_index_visible);
		sql->bind(2, row.elist_index_width);
		sql->bind(3, row.elist_name_width);
		sql->bind(4, row.elist_size_visible);
		sql->bind(5, row.elist_size_width);
		sql->bind(6, row.elist_type_visible);
		sql->bind(7, row.elist_type_width);
		sql->bind(8, row.elist_sort_column);
		sql->bind(9, row.elist_sort_descending);
		sql->bind(10, row.splitter_position);
		sql->bind(11, row.archive_id);

		rows = sql->exec();
		sql->reset();
	}

	return rows;
}

int64_t insertArchiveUIConfigRow(const ArchiveUIConfig& row)
{
	int64_t row_id = -1;

	if (auto sql = database::global().cacheQuery("insert_archive_ui_config", insert_archive_ui_config.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, row.archive_id);
		sql->bind(2, row.elist_index_visible);
		sql->bind(3, row.elist_index_width);
		sql->bind(4, row.elist_name_width);
		sql->bind(5, row.elist_size_visible);
		sql->bind(6, row.elist_size_width);
		sql->bind(7, row.elist_type_visible);
		sql->bind(8, row.elist_type_width);
		sql->bind(9, row.elist_sort_column);
		sql->bind(10, row.elist_sort_descending);
		sql->bind(11, row.splitter_position);

		if (sql->exec() > 0)
			row_id = database::global().connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	return row_id;
}

int updateArchiveEntryRow(const library::ArchiveEntry& row)
{
	// Ignore invalid id
	if (row.id < 0)
		return 0;

	auto rows = 0;
	if (auto sql = database::global().cacheQuery("update_archive_entry", update_archive_entry.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, row.archive_id);
		sql->bind(2, row.name);
		sql->bind(3, row.size);
		sql->bind(4, row.hash);
		sql->bind(5, row.type_id);
		sql->bind(7, row.id);

		rows = sql->exec();
		sql->reset();
	}

	return rows;
}

int64_t insertArchiveEntryRow(const library::ArchiveEntry& row)
{
	int64_t row_id = -1;

	if (auto sql = database::global().cacheQuery("insert_archive_entry", insert_archive_entry.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, row.archive_id);
		sql->bind(2, row.name);
		sql->bind(3, row.size);
		sql->bind(4, row.hash);
		sql->bind(5, row.type_id);

		if (sql->exec() > 0)
			row_id = database::global().connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	return row_id;
}

void insertArchiveEntryRows(const vector<library::ArchiveEntry>& rows)
{
	if (auto sql = database::global().cacheQuery("insert_archive_entry", insert_archive_entry.c_str(), true))
	{
		SQLite::Transaction transaction(*database::global().connectionRW());

		for (const auto& row : rows)
		{
			sql->clearBindings();

			sql->bind(1, row.archive_id);
			sql->bind(2, row.name);
			sql->bind(3, row.size);
			sql->bind(4, row.hash);
			sql->bind(5, row.type_id);

			sql->exec();
			sql->reset();
		}

		transaction.commit();
	}
}

int deleteArchiveEntryRowsByArchiveId(int64_t archive_id)
{
	int rows = 0;

	if (auto sql = database::global().cacheQuery(
			"delete_archive_entry_by_archive", "DELETE FROM archive_entry WHERE archive_id = ?", true))
	{
		sql->clearBindings();
		sql->bind(1, archive_id);
		rows = sql->exec();
		sql->reset();
	}

	return rows;
}
} // namespace slade::library

void library::init()
{
	//// Add to library when archive is opened
	// app::archiveManager().signals().archive_opened.connect(
	//	[](unsigned index)
	//	{
	//		auto archive = app::archiveManager().getArchive(index);
	//		if (!archive->parentEntry()) // Only add standalone archives for now
	//			addOrUpdateArchive(archive->filename(), *archive);
	//	});
}

int64_t library::archiveFileId(const string& filename)
{
	int64_t archive_id = -1;

	if (auto sql = database::global().cacheQuery("lib_get_archive_id", "SELECT id FROM archive_file WHERE path = ?"))
	{
		sql->bind(1, filename);

		if (sql->executeStep())
			archive_id = sql->getColumn(0);

		sql->reset();
	}

	return archive_id;
}

int64_t library::findArchiveFileIdFromData(unsigned size, const string& hash)
{
	int64_t archive_id = -1;

	if (auto sql = database::global().cacheQuery(
			"lib_find_archive_id_data", "SELECT id FROM archive_file WHERE size = ? AND hash = ?"))
	{
		sql->bind(1, size);
		sql->bind(2, hash);

		if (sql->executeStep())
			archive_id = sql->getColumn(0);

		sql->reset();
	}

	return archive_id;
}

int64_t library::addArchiveCopy(string_view file_path, int64_t copy_from_id)
{
	// Get row to copy
	auto archive_file = getArchiveFileRow(copy_from_id);
	if (archive_file.id < 0)
		return -1;

	// Set path
	archive_file.id   = -1;
	archive_file.path = file_path;

	// Add new archive_file row
	return insertArchiveFileRow(archive_file);
}

ArchiveUIConfig library::archiveUIConfig(int64_t archive_id)
{
	return getArchiveUIConfigRow(archive_id);
}

bool library::saveArchiveUIConfig(const ArchiveUIConfig& row)
{
	if (row.archive_id < 0)
		return false;

	// Check if the row for the archive already exists
	auto query  = fmt::format("SELECT EXISTS(SELECT 1 FROM archive_ui_config WHERE archive_id = {})", row.archive_id);
	auto exists = database::connectionRO()->execAndGet(query).getInt() > 0;

	log::debug("Saving entry list config for archive {}", row.archive_id);

	// Update/Insert
	if (exists)
		return updateArchiveUIConfigRow(row) > 0;
	else
		return insertArchiveUIConfigRow(row) > 0;
}

ArchiveUIConfig library::createArchiveUIConfig(int64_t archive_id, bool tree_view)
{
	ArchiveUIConfig config;

	config.archive_id          = archive_id;
	config.elist_index_visible = elist_colindex_show;
	config.elist_index_width   = elist_colsize_index;
	config.elist_name_width    = tree_view ? elist_colsize_name_tree : elist_colsize_name_list;
	config.elist_size_visible  = elist_colsize_show;
	config.elist_size_width    = elist_colsize_size;
	config.elist_type_visible  = elist_coltype_show;
	config.elist_type_width    = elist_colsize_type;
	config.splitter_position   = tree_view ? ap_splitter_position_tree : ap_splitter_position_list;

	log::debug("Created default entry list config for archive {}", archive_id);

	return config;
}

int library::archiveUIConfigSplitterPos(int64_t archive_id)
{
	int splitter_pos = -1;

	if (auto sql = database::global().cacheQuery(
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

bool library::saveArchiveUIConfigSplitterPos(int64_t archive_id, int splitter_pos)
{
	bool updated = false;

	if (auto sql = database::global().cacheQuery(
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

int64_t library::addOrUpdateArchive(string_view file_path, const Archive& archive)
{
	// sf::Clock timer;

	ArchiveFile archive_file{ file_path, 0, "", archive.formatId(), datetime::now(), 0 };

	if (archive.formatId() != "folder")
	{
		SFile file(file_path);
		archive_file.size          = file.size();
		archive_file.hash          = file.calculateHash();
		archive_file.last_modified = fileutil::fileModifiedTime(file_path);
	}

	// log::info(2, "archive_file build: {}ms", timer.getElapsedTime().asMilliseconds());
	// timer.restart();

	archive_file.id = archiveFileId(archive_file.path);

	if (archive_file.id >= 0)
		updateArchiveFileRow(archive_file);
	else
		archive_file.id = insertArchiveFileRow(archive_file);

	// log::info(2, "archive_file update: {}ms", timer.getElapsedTime().asMilliseconds());
	// timer.restart();

	// deleteArchiveEntryRowsByArchiveId(archive_file.id);

	// log::info(2, "archive_entry delete: {}ms", timer.getElapsedTime().asMilliseconds());
	// timer.restart();

	// vector<slade::ArchiveEntry*> entries;
	// archive.putEntryTreeAsList(entries);

	// vector<library::ArchiveEntry> entry_rows;
	// for (auto* entry : entries)
	//{
	//	// Ignore dirs
	//	if (entry->type() == EntryType::folderType())
	//		continue;

	//	entry_rows.emplace_back(archive_file.id, entry->path(true), entry->size(), entry->hash(), entry->type()->id());
	//}

	// log::info(2, "entry row list build: {}ms", timer.getElapsedTime().asMilliseconds());
	// timer.restart();

	// insertArchiveEntryRows(entry_rows);

	// log::info(2, "archive_entry update: {}ms", timer.getElapsedTime().asMilliseconds());
	// timer.restart();

	return archive_file.id;
}

vector<string> library::recentFiles(unsigned count)
{
	vector<string> paths;

	// Get or create cached query to select base resource paths
	if (auto sql = database::global().cacheQuery(
			"am_list_recent_files", "SELECT path FROM archive_file ORDER BY last_opened DESC LIMIT ?"))
	{
		sql->bind(1, count);

		// Execute query and add results to list
		while (sql->executeStep())
			paths.push_back(sql->getColumn(0).getString());

		sql->reset();
	}

	return paths;
}

string library::findEntryTypeId(const slade::ArchiveEntry& entry)
{
	string format_id;

	if (auto sql = database::global().cacheQuery(
			"find_entry_format_id", "SELECT type_id FROM archive_entry WHERE name = ? AND hash = ?"))
	{
		sql->bind(1, entry.path(true));
		sql->bind(2, entry.hash());

		if (sql->executeStep())
			format_id = sql->getColumn(0).getString();

		sql->reset();
	}

	return format_id;
}

Signals& library::signals()
{
	return lib_signals;
}
