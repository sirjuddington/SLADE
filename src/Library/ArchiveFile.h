#pragma once

namespace slade
{
namespace database
{
	class Context;
}

namespace library
{
	// Database model for rows in the archive_file table in the database
	struct ArchiveFileRow
	{
		int64_t  id = -1;
		string   path;
		unsigned size = 0;
		string   hash;
		string   format_id;
		time_t   last_opened   = 0;
		time_t   last_modified = 0;

		ArchiveFileRow() = default;
		ArchiveFileRow(
			string_view path,
			unsigned    size,
			string_view hash,
			string_view format_id,
			time_t      last_opened,
			time_t      last_modified) :
			path{ path },
			size{ size },
			hash{ hash },
			format_id{ format_id },
			last_opened{ last_opened },
			last_modified{ last_modified }
		{
		}
		ArchiveFileRow(string_view file_path, string_view format_id);
		ArchiveFileRow(database::Context& db, int64_t id);

		int64_t insert();
		bool    update() const;
		bool    remove();
	};

	int64_t archiveFileId(const string& filename);
	int64_t findArchiveFileIdFromData(unsigned size, const string& hash);
	bool    saveArchiveFile(ArchiveFileRow& row);
	int64_t copyArchiveFile(string_view file_path, int64_t copy_from_id);
	void    removeArchiveFile(int64_t id);
	time_t  archiveFileLastOpened(int64_t id);
} // namespace library
} // namespace slade
