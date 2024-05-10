#pragma once

namespace slade
{
namespace database
{
	class Context;
}

namespace library
{
	struct ArchiveRunConfigRow
	{
		int64_t archive_id = -1;
		string  executable_id;
		int     run_config = 0;
		string  run_extra;
		string  iwad_path;

		ArchiveRunConfigRow() = default;
		ArchiveRunConfigRow(database::Context& db, int64_t archive_id);
		ArchiveRunConfigRow(int64_t archive_id) : archive_id{ archive_id } {}

		int64_t insert() const;
		bool    update() const;
		bool    remove();
	};

	ArchiveRunConfigRow getArchiveRunConfig(int64_t archive_id);
	bool                saveArchiveRunConfig(const ArchiveRunConfigRow& row);
} // namespace library
} // namespace slade
