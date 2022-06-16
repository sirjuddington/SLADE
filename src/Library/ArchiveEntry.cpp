
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ArchiveEntry.cpp
// Description: ArchiveEntryRow struct and related functions
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
#include "Archive/ArchiveEntry.h"
#include "App.h"
#include "ArchiveEntry.h"
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
string update_archive_entry =
	"UPDATE archive_entry "
	"SET path = ?, [index] = ?, name = ?, size = ?, hash = ?, type_id = ? "
	"WHERE archive_id = ? AND id = ?";
string insert_archive_entry =
	"INSERT INTO archive_entry (archive_id, id, path, [index], name, size, hash, type_id) "
	"VALUES (?,?,?,?,?,?,?,?)";
string copy_archive_entries =
	"INSERT INTO archive_entry (archive_id, id, path, [index], name, size, hash, type_id) "
	"            SELECT ?, id, path, [index], name, size, hash, type_id "
	"            FROM archive_entry WHERE archive_id = ?";
string insert_archive_entry_property = "INSERT INTO archive_entry_property VALUES (?,?,?,?,?)";
string get_archive_entry_properties =
	"SELECT key, value_type, value FROM archive_entry_property WHERE archive_id = ? AND entry_id = ?";

vector<string> saved_ex_props = { "TextPosition", "TextLanguage" };
} // namespace slade::library


// -----------------------------------------------------------------------------
//
// ArchiveEntryRow Struct Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// ArchiveEntryRow constructor
// Initializes the row for [archive_id]+[id] with info from [entry]
// -----------------------------------------------------------------------------
ArchiveEntryRow::ArchiveEntryRow(int64_t archive_id, int64_t id, const ArchiveEntry& entry) :
	archive_id{ archive_id },
	id{ id },
	path{ entry.path() },
	index{ entry.index() },
	name{ entry.name() },
	size{ entry.size() },
	hash{ entry.hash() },
	type_id{ entry.type()->id() }
{
}

// -----------------------------------------------------------------------------
// ArchiveEntryRow constructor
// Reads existing data from the database. If row [archive_id]+[id] doesn't exist
// in the database, the row id will be set to -1
// -----------------------------------------------------------------------------
ArchiveEntryRow::ArchiveEntryRow(db::Context& db, int64_t archive_id, int64_t id) : archive_id{ archive_id }, id{ id }
{
	if (auto sql = db.cacheQuery("get_archive_entry", "SELECT * FROM archive_entry WHERE archive_id = ? AND id = ?"))
	{
		sql->clearBindings();
		sql->bind(1, archive_id);
		sql->bind(2, id);

		if (sql->executeStep())
		{
			path    = sql->getColumn(2).getString();
			index   = sql->getColumn(3).getInt();
			name    = sql->getColumn(4).getString();
			size    = sql->getColumn(5).getUInt();
			hash    = sql->getColumn(6).getString();
			type_id = sql->getColumn(7).getString();
		}
		else
		{
			log::warning("archive_entry row with archive_id {}, id {} does not exist in the database", archive_id, id);
			this->id = -1;
		}

		sql->reset();
	}
}

// -----------------------------------------------------------------------------
// Inserts this row into the database.
// If successful, the inserted row id is returned, otherwise returns -1.
// (note the returned row id isn't the 'id' column since that is not a primary
//  key, it returns the sqlite rowid of the row)
// -----------------------------------------------------------------------------
int64_t ArchiveEntryRow::insert() const
{
	if (id >= 0)
	{
		log::warning("Trying to insert archive_entry id {} that already exists", id);
		return id;
	}

	int64_t row_id = -1;
	if (auto sql = db::cacheQuery("insert_archive_entry", insert_archive_entry, true))
	{
		sql->clearBindings();

		sql->bind(1, archive_id);
		sql->bind(2, id);
		sql->bind(3, path);
		sql->bind(4, index);
		sql->bind(5, name);
		sql->bind(6, size);
		sql->bind(7, hash);
		sql->bind(8, type_id);

		if (sql->exec() > 0)
			row_id = db::connectionRW()->getLastInsertRowid();

		sql->reset();
	}

	return row_id;
}

// -----------------------------------------------------------------------------
// Updates this row in the database
// -----------------------------------------------------------------------------
bool ArchiveEntryRow::update() const
{
	// Ignore invalid id
	if (id < 0)
	{
		log::warning("Trying to update archive_entry row with no id");
		return false;
	}

	auto rows = 0;
	if (auto sql = db::cacheQuery("update_archive_entry", update_archive_entry, true))
	{
		sql->clearBindings();

		sql->bind(1, path);
		sql->bind(2, index);
		sql->bind(3, name);
		sql->bind(4, size);
		sql->bind(5, hash);
		sql->bind(6, type_id);
		sql->bind(7, archive_id);
		sql->bind(8, id);

		rows = sql->exec();
		sql->reset();
	}

	return rows > 0;
}

// -----------------------------------------------------------------------------
// Removes this row from the database.
// If successful, id will be set to -1
// -----------------------------------------------------------------------------
bool ArchiveEntryRow::remove()
{
	// Ignore invalid id
	if (id < 0)
	{
		log::warning("Trying to remove archive_entry row with no id");
		return false;
	}

	auto rows = 0;

	if (auto sql = db::cacheQuery("delete_archive_entry", "DELETE FROM archive_entry WHERE archive_id = ? AND id = ?"))
	{
		sql->bind(1, archive_id);
		sql->bind(2, id);
		rows = sql->exec();
		sql->reset();
	}

	if (rows > 0)
	{
		id = -1;
		return true;
	}

	return false;
}


// -----------------------------------------------------------------------------
//
// library Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::library
{
// -----------------------------------------------------------------------------
// Returns a list of all ArchiveEntryRows for [archive_id]
// -----------------------------------------------------------------------------
vector<ArchiveEntryRow> getArchiveEntryRows(int64_t archive_id)
{
	vector<ArchiveEntryRow> rows;

	if (auto sql = db::cacheQuery(
			"get_archive_entries_for_archive", "SELECT * FROM archive_entry WHERE archive_id = ?"))
	{
		sql->clearBindings();
		sql->bind(1, archive_id);

		while (sql->executeStep())
		{
			rows.emplace_back(
				archive_id,
				sql->getColumn(1).getInt64(),
				sql->getColumn(2).getString(),
				sql->getColumn(3).getInt(),
				sql->getColumn(4).getString(),
				sql->getColumn(5).getUInt(),
				sql->getColumn(6).getString(),
				sql->getColumn(7).getString());
		}

		sql->reset();
	}

	return rows;
}

// -----------------------------------------------------------------------------
// Mass updates the given [rows] in the database
// -----------------------------------------------------------------------------
void updateArchiveEntryRows(const vector<ArchiveEntryRow>& rows)
{
	if (auto sql = db::cacheQuery("update_archive_entry", update_archive_entry, true))
	{
		// Begin transaction if none currently active
		db::Transaction transaction(db::connectionRW(), false);
		transaction.beginIfNoActiveTransaction();

		for (const auto& row : rows)
		{
			sql->clearBindings();

			sql->bind(1, row.path);
			sql->bind(2, row.index);
			sql->bind(3, row.name);
			sql->bind(4, row.size);
			sql->bind(5, row.hash);
			sql->bind(6, row.type_id);
			sql->bind(7, row.archive_id);
			sql->bind(8, row.id);

			sql->exec();

			sql->reset();
		}

		transaction.commit();
	}
}

// -----------------------------------------------------------------------------
// Mass inserts the given [rows] in the database
// -----------------------------------------------------------------------------
void insertArchiveEntryRows(const vector<ArchiveEntryRow>& rows)
{
	if (auto sql = db::cacheQuery("insert_archive_entry", insert_archive_entry, true))
	{
		// Begin transaction if none currently active
		db::Transaction transaction(db::connectionRW(), false);
		transaction.beginIfNoActiveTransaction();

		for (auto& row : rows)
		{
			sql->clearBindings();

			sql->bind(1, row.archive_id);
			sql->bind(2, row.id);
			sql->bind(3, row.path);
			sql->bind(4, row.index);
			sql->bind(5, row.name);
			sql->bind(6, row.size);
			sql->bind(7, row.hash);
			sql->bind(8, row.type_id);

			sql->exec();

			sql->reset();
		}

		transaction.commit();
	}
}

// -----------------------------------------------------------------------------
// Mass deletes all archive_entry rows for [archive_id] in the database
// -----------------------------------------------------------------------------
int deleteArchiveEntryRowsByArchiveId(int64_t archive_id)
{
	int rows = 0;

	if (auto sql = db::cacheQuery(
			"delete_archive_entry_by_archive", "DELETE FROM archive_entry WHERE archive_id = ?", true))
	{
		sql->clearBindings();
		sql->bind(1, archive_id);
		rows = sql->exec();
		sql->reset();
	}

	return rows;
}

// -----------------------------------------------------------------------------
// Returns true if [row] exactly matches the given entry details.
//
// Note this currently ignores index so that if an entry has been moved up/down
// externally it will still be able to be matched (it's very likely to be the
// same entry)
// -----------------------------------------------------------------------------
inline bool entryRowExactMatch(
	const ArchiveEntryRow& row,
	string_view            entry_path,
	string_view            entry_name,
	unsigned               entry_size,
	string_view            entry_hash)
{
	return row.path == entry_path && row.name == entry_name && row.size == entry_size && row.hash == entry_hash;
}

void writeEntryProperties(SQLite::Statement* sql, int64_t archive_id, const ArchiveEntry* entry)
{
	for (const auto& prop : entry->exProps().properties())
	{
		if (VECTOR_EXISTS(saved_ex_props, prop.name))
		{
			sql->bind(1, archive_id);
			sql->bind(2, entry->libraryId());
			sql->bind(3, prop.name);
			sql->bind(4, static_cast<unsigned>(prop.value.index()));
			switch (prop.value.index())
			{
			case 0: sql->bind(5, std::get<bool>(prop.value)); break;
			case 1: sql->bind(5, std::get<int>(prop.value)); break;
			case 2: sql->bind(5, std::get<unsigned int>(prop.value)); break;
			case 3: sql->bind(5, std::get<double>(prop.value)); break;
			case 4: sql->bind(5, std::get<string>(prop.value)); break;
			default: sql->bind(5, 0); break; // Shouldn't happen
			}
			sql->exec();
			sql->reset();
		}
	}
}

void readEntryProperties(int64_t archive_id, ArchiveEntry* entry)
{
	if (auto sql = db::cacheQuery("get_archive_entry_properties", get_archive_entry_properties))
	{
		sql->bind(1, archive_id);
		sql->bind(2, entry->libraryId());

		while (sql->executeStep())
		{
			auto key = sql->getColumn(0).getString();
			switch (sql->getColumn(1).getInt())
			{
			case 0: entry->exProp(key) = sql->getColumn(2).getInt() > 0; break;
			case 1: entry->exProp(key) = sql->getColumn(2).getInt(); break;
			case 2: entry->exProp(key) = sql->getColumn(2).getUInt(); break;
			case 3: entry->exProp(key) = sql->getColumn(2).getDouble(); break;
			case 4: entry->exProp(key) = sql->getColumn(2).getString(); break;
			default: break; // Shouldn't happen
			}
		}

		sql->reset();
	}
}
} // namespace slade::library

// -----------------------------------------------------------------------------
// Copies all archive_entry rows from [from_archive_id] and inserts them for
// [to_archive_id]
// -----------------------------------------------------------------------------
int library::copyArchiveEntries(int64_t from_archive_id, int64_t to_archive_id)
{
	// Check args
	if (from_archive_id < 0 || to_archive_id < 0)
		return 0;

	int n_copied = 0;

	if (auto sql = db::cacheQuery("copy_archive_entries", copy_archive_entries, true))
	{
		sql->bind(1, to_archive_id);
		sql->bind(2, from_archive_id);

		n_copied = sql->exec();

		sql->reset();
	}

	return n_copied;
}

// -----------------------------------------------------------------------------
// Rebuilds all archive_entry rows for [archive_id] using the given [entries]
// -----------------------------------------------------------------------------
void library::rebuildEntries(int64_t archive_id, const vector<ArchiveEntry*>& entries)
{
	auto start_time = app::runTimer();

	// Delete all existing archive_entry rows for archive_id
	deleteArchiveEntryRowsByArchiveId(archive_id);

	// Build list of entry rows to add
	vector<ArchiveEntryRow> rows;
	for (auto i = 0; i < entries.size(); ++i)
	{
		rows.emplace_back(archive_id, i, *entries[i]);
		entries[i]->setLibraryId(i);
	}

	// Add rows to database
	insertArchiveEntryRows(rows);

	// Write entry properties to database
	saveAllEntryProperties(archive_id, entries);

	log::debug("library::rebuildEntries took {}ms", app::runTimer() - start_time);
}

void library::saveEntryProperties(int64_t archive_id, const ArchiveEntry& entry)
{
	if (entry.libraryId() < 0)
		return;

	// Delete existing rows
	db::connectionRW()->exec(fmt::format(
		"DELETE FROM archive_entry_property WHERE archive_id = {} AND entry_id = {}", archive_id, entry.libraryId()));

	// Insert prop rows
	if (auto sql = db::cacheQuery("insert_archive_entry_property", insert_archive_entry_property, true))
		writeEntryProperties(sql, archive_id, &entry);
}

void library::saveAllEntryProperties(int64_t archive_id, const vector<ArchiveEntry*>& entries)
{
	// Delete existing rows
	db::connectionRW()->exec(fmt::format("DELETE FROM archive_entry_property WHERE archive_id = {}", archive_id));

	// Insert prop rows for all entries
	if (auto sql = db::cacheQuery("insert_archive_entry_property", insert_archive_entry_property, true))
	{
		db::Transaction transaction(db::connectionRW(), false);
		transaction.beginIfNoActiveTransaction();

		for (auto entry : entries)
			writeEntryProperties(sql, archive_id, entry);

		transaction.commit();
	}
}

// -----------------------------------------------------------------------------
// Reads all entry info from the library for [archive_id] into the given
// [entries].
//
// Will attempt to match any archive_entry rows that don't match exactly to an
// entry but are close enough (eg. if the entry was renamed or moved since the
// archive was last recorded in the library)
// -----------------------------------------------------------------------------
void library::readEntryInfo(int64_t archive_id, const vector<ArchiveEntry*>& entries)
{
	// Get existing archive_entry rows for the archive
	auto existing_rows = getArchiveEntryRows(archive_id);
	if (existing_rows.empty())
		return; // No rows, so no existing info to read

	auto start_time = app::runTimer();

	// Find all exact matches (path+name+size+hash is close enough to exact)
	// This is likely to account a majority of the entries unless the archive
	// has changed a *lot* since it was last saved to the library
	unsigned n_unmatched_entries = 0;
	unsigned row_start_index     = 0;
	for (auto entry : entries)
	{
		entry->setLibraryId(-1);

		for (unsigned i = row_start_index; i < existing_rows.size(); ++i)
		{
			// Ignore rows already matched to an entry (id < 0)
			if (existing_rows[i].id < 0)
			{
				// If this is the first row being checked, don't check it again
				if (row_start_index == i)
					++row_start_index;

				continue;
			}

			if (entryRowExactMatch(existing_rows[i], entry->path(), entry->name(), entry->size(), entry->hash()))
			{
				entry->setLibraryId(existing_rows[i].id);
				existing_rows[i].id = -1;
				break;
			}
		}

		if (entry->libraryId() < 0)
			++n_unmatched_entries;
	}

	log::debug("archive_entry matching (exact) took {}ms", app::runTimer() - start_time);


	// For any remaining unmatched entries, try to determine what archive_entry
	// row most closely matches the entry (and can reasonably be considered the
	// 'same' entry - ideally we should have no match over matching to the wrong
	// entry)
	if (n_unmatched_entries > 0)
	{
		start_time = app::runTimer();

		log::debug(
			"Found {} entries in archive {} with no exact-matching archive_entry row", n_unmatched_entries, archive_id);

		// Get list of unmatched entries
		vector<ArchiveEntry*> unmatched_entries;
		unmatched_entries.reserve(n_unmatched_entries);
		for (auto entry : entries)
			if (entry->libraryId() < 0)
				unmatched_entries.push_back(entry);

		// Get list of unmatched rows
		vector<ArchiveEntryRow*> unmatched_rows;
		for (auto& row : existing_rows)
			if (row.id >= 0)
				unmatched_rows.push_back(&row);

		string           entry_path, entry_type;
		ArchiveEntryRow* current_match;
		uint8_t          match_score;
		bool             match_path, match_name, match_data, match_type;
		for (auto entry : unmatched_entries)
		{
			entry_path    = entry->path();
			entry_type    = entry->type()->id();
			current_match = nullptr;
			match_score   = 0;

			// Find best match in unmatched rows (if any)
			for (auto row : unmatched_rows)
			{
				if (row->id < 0)
					continue;

				// Check what matches the entry
				// (zero-sized entries can't be matched by data)
				match_path = row->path == entry_path;
				match_name = row->name == entry->name();
				match_data = entry->size() > 0 && row->size > 0 && row->hash == entry->hash();
				match_type = row->type_id == entry_type;

				// Renamed entry
				if (match_path && match_data)
				{
					current_match = row;
					match_score   = 10;
				}

				// Moved entry
				if (match_score < 9 && match_name && match_data)
				{
					current_match = row;
					match_score   = 9;
				}

				// Modified entry (same type)
				if (match_score < 8 && match_path && match_name && match_type)
				{
					current_match = row;
					match_score   = 8;
				}

				// Moved+Renamed entry
				if (match_score < 7 && match_data)
				{
					current_match = row;
					match_score   = 7;
				}
			}

			if (current_match)
			{
#ifdef _DEBUG
				static string match_desc;
				switch (match_score)
				{
				case 10: match_desc = "entry renamed"; break;
				case 9: match_desc = "entry moved"; break;
				case 8: match_desc = "entry modified (same type)"; break;
				case 7: match_desc = "entry moved & renamed"; break;
				default: match_desc = "unknown match"; break;
				}

				log::debug(
					"Matched entry {} to row {} ({}) - {}",
					entry->path(true),
					current_match->id,
					current_match->name,
					match_desc);
#endif

				entry->setLibraryId(current_match->id);
				current_match->id = -1;
			}
#ifdef _DEBUG
			else
				log::debug("No matching row found for entry {}", entry->path(true));
#endif
		}

		log::debug("archive_entry matching (remaining unmatched) took {}ms", app::runTimer() - start_time);
	}
	else
		log::debug("All archive_entry rows in archive {} matched", archive_id);


	// Load entry properties
	for (auto entry : entries)
		readEntryProperties(archive_id, entry);


	// Rebuild entry rows if there were any mismatches
	if (n_unmatched_entries > 0)
		rebuildEntries(archive_id, entries);
}
