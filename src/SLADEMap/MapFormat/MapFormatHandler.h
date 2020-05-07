#pragma once

#include "Archive/Archive.h"

namespace slade
{
class MapObjectCollection;

class MapFormatHandler
{
public:
	virtual ~MapFormatHandler() = default;

	virtual bool readMap(Archive::MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props) = 0;
	virtual vector<unique_ptr<ArchiveEntry>> writeMap(
		const MapObjectCollection& map_data,
		const PropertyList&        map_extra_props) = 0;

	virtual string udmfNamespace() = 0;

	static unique_ptr<MapFormatHandler> get(MapFormat format);
};
} // namespace slade
