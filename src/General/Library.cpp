
#include "Main.h"
#include "Library.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveManager.h"
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

} // namespace slade::library


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

int updateArchiveFileRow(const ArchiveFile& archive_file)
{
	// Ignore invalid id
	if (archive_file.id < 0)
		return 0;

	auto rows = 0;
	if (auto sql = database::global().cacheQuery("update_archive_file", update_archive_file.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, archive_file.path);
		sql->bind(2, archive_file.size);
		sql->bind(3, archive_file.hash);
		sql->bind(4, archive_file.format_id);
		sql->bind(5, archive_file.last_opened);
		sql->bind(6, archive_file.last_modified);
		sql->bind(7, archive_file.id);

		rows = sql->exec();
		sql->reset();
	}

	if (rows > 0)
		lib_signals.updated();

	return rows;
}

int64_t insertArchiveFileRow(const ArchiveFile& archive_file)
{
	int64_t row_id = -1;

	if (auto sql = database::global().cacheQuery("insert_archive_file", insert_archive_file.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, archive_file.path);
		sql->bind(2, archive_file.size);
		sql->bind(3, archive_file.hash);
		sql->bind(4, archive_file.format_id);
		sql->bind(5, archive_file.last_opened);
		sql->bind(6, archive_file.last_modified);

		if (sql->exec() > 0)
			row_id = database::global().connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	if (row_id >= 0)
		lib_signals.updated();

	return row_id;
}

int updateArchiveEntryRow(const library::ArchiveEntry& archive_entry)
{
	// Ignore invalid id
	if (archive_entry.id < 0)
		return 0;

	auto rows = 0;
	if (auto sql = database::global().cacheQuery("update_archive_entry", update_archive_entry.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, archive_entry.archive_id);
		sql->bind(2, archive_entry.name);
		sql->bind(3, archive_entry.size);
		sql->bind(4, archive_entry.hash);
		sql->bind(5, archive_entry.type_id);
		sql->bind(7, archive_entry.id);

		rows = sql->exec();
		sql->reset();
	}

	return rows;
}

int64_t insertArchiveEntryRow(const library::ArchiveEntry& archive_entry)
{
	int64_t row_id = -1;

	if (auto sql = database::global().cacheQuery("insert_archive_entry", insert_archive_entry.c_str(), true))
	{
		sql->clearBindings();

		sql->bind(1, archive_entry.archive_id);
		sql->bind(2, archive_entry.name);
		sql->bind(3, archive_entry.size);
		sql->bind(4, archive_entry.hash);
		sql->bind(5, archive_entry.type_id);

		if (sql->exec() > 0)
			row_id = database::global().connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	return row_id;
}

void insertArchiveEntryRows(const vector<library::ArchiveEntry>& archive_entries)
{
	if (auto sql = database::global().cacheQuery("insert_archive_entry", insert_archive_entry.c_str(), true))
	{
		SQLite::Transaction transaction(*database::global().connectionRW());

		for (const auto& archive_entry : archive_entries)
		{
			sql->clearBindings();

			sql->bind(1, archive_entry.archive_id);
			sql->bind(2, archive_entry.name);
			sql->bind(3, archive_entry.size);
			sql->bind(4, archive_entry.hash);
			sql->bind(5, archive_entry.type_id);

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
	// Add to library when archive is opened
	app::archiveManager().signals().archive_opened.connect(
		[](unsigned index)
		{
			auto archive = app::archiveManager().getArchive(index);
			if (!archive->parentEntry()) // Only add standalone archives for now
				addOrUpdateArchive(archive->filename(), *archive);
		});
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
	archive_file.id = -1;
	archive_file.path = file_path;

	// Add new archive_file row
	return insertArchiveFileRow(archive_file);
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
