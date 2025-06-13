#pragma once

namespace slade::database
{
class Context;

// Database model for a row in the archive_file table
struct ArchiveFile
{
	i64      id = -1;
	string   path;
	unsigned size = 0;
	string   hash;
	string   format_id;
	time_t   last_opened   = 0;
	time_t   last_modified = 0;
	i64      parent_id     = -1;

	ArchiveFile() = default;
	ArchiveFile(
		string_view path,
		unsigned    size,
		string_view hash,
		string_view format_id,
		time_t      last_opened,
		time_t      last_modified,
		i64         parent_id) :
		path{ path },
		size{ size },
		hash{ hash },
		format_id{ format_id },
		last_opened{ last_opened },
		last_modified{ last_modified },
		parent_id{ parent_id }
	{
	}
	ArchiveFile(Context& db, i64 id);

	i64  insert(Context& db);
	void update(Context& db) const;
	void remove(Context& db);
};

} // namespace slade::database
