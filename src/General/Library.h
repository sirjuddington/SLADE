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

	struct ArchiveUIConfig
	{
		int64_t archive_id          = -1;
		bool    elist_index_visible = false;
		int     elist_index_width   = -1;
		int     elist_name_width    = -1;
		bool    elist_size_visible  = true;
		int     elist_size_width    = -1;
		bool    elist_type_visible  = true;
		int     elist_type_width    = -1;
		string  elist_sort_column;
		bool    elist_sort_descending = false;
		int     splitter_position     = -1;

		ArchiveUIConfig() = default;
		ArchiveUIConfig(int64_t archive_id) : archive_id{ archive_id } {}
		ArchiveUIConfig(
			int64_t     archive_id,
			bool        elist_index_visible,
			int         elist_index_width,
			int         elist_name_width,
			bool        elist_size_visible,
			int         elist_size_width,
			bool        elist_type_visible,
			int         elist_type_width,
			string_view elist_sort_column,
			bool        elist_sort_descending,
			int         splitter_position) :
			archive_id{ archive_id },
			elist_index_visible{ elist_index_visible },
			elist_index_width{ elist_index_width },
			elist_name_width{ elist_name_width },
			elist_size_visible{ elist_size_visible },
			elist_size_width{ elist_size_width },
			elist_type_visible{ elist_type_visible },
			elist_type_width{ elist_type_width },
			elist_sort_column{ elist_sort_column },
			elist_sort_descending{ elist_sort_descending },
			splitter_position{ splitter_position }
		{
		}
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
		sigslot::signal<> archive_file_updated;
	};

	void init();

	// ArchiveFile
	int64_t archiveFileId(const string& filename);
	int64_t findArchiveFileIdFromData(unsigned size, const string& hash);
	int64_t addArchiveCopy(string_view file_path, int64_t copy_from_id);

	// ArchiveUIConfig
	ArchiveUIConfig archiveUIConfig(int64_t archive_id);
	bool            saveArchiveUIConfig(const ArchiveUIConfig& row);
	ArchiveUIConfig createArchiveUIConfig(int64_t archive_id, bool tree_view);
	int             archiveUIConfigSplitterPos(int64_t archive_id);
	bool            saveArchiveUIConfigSplitterPos(int64_t archive_id, int splitter_pos);

	// General
	int64_t        addOrUpdateArchive(string_view file_path, const Archive& archive);
	vector<string> recentFiles(unsigned count = 20);
	string         findEntryTypeId(const slade::ArchiveEntry& entry);

	Signals& signals();
} // namespace library
} // namespace slade
