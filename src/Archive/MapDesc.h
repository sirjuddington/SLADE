#pragma once

#include "General/Defs.h"

namespace slade
{
struct MapDesc
{
	string                 name;
	weak_ptr<ArchiveEntry> head;
	weak_ptr<ArchiveEntry> end;     // The last entry of the map data
	MapFormat              format;  // See MapTypes enum
	bool                   archive; // True if head is an archive (for maps in zips)

	vector<ArchiveEntry*> unk; // Unknown map lumps (must be preserved for UDMF compliance)

	MapDesc()
	{
		archive = false;
		format  = MapFormat::Unknown;
	}

	vector<ArchiveEntry*> entries(const Archive& parent, bool include_head = false) const;
	void                  updateMapFormatHints() const;
};
} // namespace slade
