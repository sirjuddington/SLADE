#pragma once

#include "Archive/Archive.h"
#include "General/Defs.h"

namespace slade
{
namespace database
{
	class Context;
}

namespace library
{
	struct ArchiveMapRow
	{
		int64_t   archive_id      = -1;
		int64_t   header_entry_id = -1;
		string    name;
		MapFormat format = MapFormat::Unknown;

		ArchiveMapRow(database::Context& db, int64_t archive_id, int64_t header_entry_id);
		ArchiveMapRow(int64_t archive_id, const Archive::MapDesc& map_desc);

		int64_t insert();
		bool    update() const;
		bool    remove();
	};

	void updateArchiveMaps(int64_t archive_id, const Archive& archive);
} // namespace library
} // namespace slade
