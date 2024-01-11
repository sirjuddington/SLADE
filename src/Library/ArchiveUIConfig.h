#pragma once

namespace slade
{
namespace database
{
	class Context;
}

namespace library
{
	// Database model for rows in the archive_ui_config table in the database
	struct ArchiveUIConfigRow
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

		ArchiveUIConfigRow() = default;
		ArchiveUIConfigRow(int64_t archive_id) : archive_id{ archive_id } {}
		ArchiveUIConfigRow(
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
		ArchiveUIConfigRow(database::Context& db, int64_t archive_id);
		ArchiveUIConfigRow(int64_t archive_id, bool tree_view);

		int64_t insert() const;
		bool    update() const;
		bool    remove();
	};

	ArchiveUIConfigRow getArchiveUIConfig(int64_t archive_id);
	bool               saveArchiveUIConfig(const ArchiveUIConfigRow& row);
	int                archiveUIConfigSplitterPos(int64_t archive_id);
	bool               saveArchiveUIConfigSplitterPos(int64_t archive_id, int splitter_pos);

} // namespace library
} // namespace slade
