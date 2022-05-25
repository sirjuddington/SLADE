#pragma once

namespace slade
{
class Archive;
class ArchiveEntry;

namespace library
{
	struct ArchiveFile
	{
		int64_t  id = -1;
		string   path;
		unsigned size = 0;
		string   hash;
		string   format_id;
		time_t   last_opened   = 0;
		time_t   last_modified = 0;

		ArchiveFile(
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

		ArchiveFile() = default;
	};

	struct ArchiveEntry
	{
		int64_t  id;
		int64_t  archive_id;
		string   name;
		unsigned size;
		string   hash;
		string   type_id;

		ArchiveEntry(int64_t archive_id, string_view name, unsigned size, string_view hash, string_view type_id) :
			id{ -1 }, archive_id{ archive_id }, name{ name }, size{ size }, hash{ hash }, type_id{ type_id }
		{
		}
	};

	struct Signals
	{
		sigslot::signal<> updated;
	};

	void init();

	int64_t archiveFileId(const string& filename);
	int64_t findArchiveFileIdFromData(unsigned size, const string& hash);
	int64_t addArchiveCopy(string_view file_path, int64_t copy_from_id);

	int64_t        addOrUpdateArchive(string_view file_path, const Archive& archive);
	vector<string> recentFiles(unsigned count = 20);
	string         findEntryTypeId(const slade::ArchiveEntry& entry);

	Signals& signals();
} // namespace library
} // namespace slade
