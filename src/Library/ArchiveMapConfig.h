#pragma once

namespace slade
{
namespace database
{
	class Context;
}

namespace library
{
	struct ArchiveMapConfigRow
	{
		int64_t archive_id = -1;
		string  game;
		string  port;

		ArchiveMapConfigRow() = default;
		ArchiveMapConfigRow(database::Context& db, int64_t archive_id);
		ArchiveMapConfigRow(int64_t archive_id) : archive_id{ archive_id } {}

		int64_t insert() const;
		bool    update() const;
		bool    remove();
	};

	ArchiveMapConfigRow getArchiveMapConfig(int64_t archive_id);
	bool                saveArchiveMapConfig(const ArchiveMapConfigRow& row);
} // namespace library
} // namespace slade
