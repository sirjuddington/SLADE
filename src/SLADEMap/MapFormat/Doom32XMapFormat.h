#pragma once

#include "DoomMapFormat.h"

namespace slade
{
class ThingList;
class SectorList;
class LineList;
class SideList;
class VertexList;

class Doom32XMapFormat : public DoomMapFormat
{
public:
	struct Vertex32BE
	{
		int32_t x;
		int32_t y;
	};

protected:
	bool readVERTEXES(ArchiveEntry* entry, MapObjectCollection& map_data) const override;
};
} // namespace slade
