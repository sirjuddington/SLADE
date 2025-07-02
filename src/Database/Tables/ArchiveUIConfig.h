#pragma once

namespace slade::database
{
class Context;
class Statement;

struct ArchiveUIConfig
{
	i64    archive_id          = -1;
	bool   elist_index_visible = false;
	int    elist_index_width   = -1;
	int    elist_name_width    = -1;
	bool   elist_size_visible  = true;
	int    elist_size_width    = -1;
	bool   elist_type_visible  = true;
	int    elist_type_width    = -1;
	string elist_sort_column;
	bool   elist_sort_descending = false;
	int    splitter_position     = -1;

	void read(Statement& ps);
	void write();

	i64  insert();
	void update() const;
	void remove();

	void setDefaults(bool tree_view);
};

ArchiveUIConfig getArchiveUIConfig(i64 archive_id);
int             archiveUIConfigSplitterPos(i64 archive_id);
void            saveArchiveUIConfigSplitterPos(i64 archive_id, int splitter_pos);
} // namespace slade::database
