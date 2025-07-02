#pragma once

namespace slade::database
{
class Statement;

struct ArchiveRunConfig
{
	i64    archive_id = -1;
	string executable_id;
	int    run_config = 0;
	string run_extra;
	string iwad_path;

	void read(Statement& ps);
	void write();

	i64  insert();
	void update() const;
	void remove();
};

ArchiveRunConfig getArchiveRunConfig(i64 archive_id);
} // namespace slade::database
