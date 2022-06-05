#pragma once

namespace slade
{
class ArchiveEntry;

namespace database
{
	class Context;
}

namespace library
{
	struct ArchiveEntryRow
	{
		int64_t  archive_id = -1;
		int64_t  id         = -1;
		string   path;
		int      index = -1;
		string   name;
		unsigned size = 0;
		string   hash;
		string   type_id;

		ArchiveEntryRow() = default;
		ArchiveEntryRow(
			int64_t     archive_id,
			int64_t     id,
			string_view path,
			int         index,
			string_view name,
			unsigned    size,
			string_view hash,
			string_view type_id) :
			archive_id{ archive_id },
			id{ id },
			path{ path },
			index{ index },
			name{ name },
			size{ size },
			hash{ hash },
			type_id{ type_id }
		{
		}
		ArchiveEntryRow(int64_t archive_id, int64_t id, const ArchiveEntry& entry);
		ArchiveEntryRow(database::Context& db, int64_t archive_id, int64_t id);

		int64_t insert() const;
		bool    update() const;
		bool    remove();
	};

	int64_t findEntryId(int64_t archive_id, const string& name, const string& path, const string& hash);
	int64_t findFolderEntryId(int64_t archive_id, const string& path, const string& name);
	void    removeArchiveEntry(int64_t archive_id, int64_t id);
	int     copyArchiveEntries(int64_t from_archive_id, int64_t to_archive_id);
	void    rebuildEntries(int64_t archive_id, const vector<ArchiveEntry*>& entries);

	void readEntryInfo(int64_t archive_id, const vector<ArchiveEntry*>& entries);

} // namespace library
} // namespace slade
