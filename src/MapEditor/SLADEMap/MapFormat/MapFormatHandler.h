#pragma once

#include "Archive/Archive.h"

class MapObjectCollection;

class MapFormatHandler
{
public:
	typedef std::unique_ptr<MapFormatHandler> UPtr;

	virtual ~MapFormatHandler() = default;

	virtual bool readMap(Archive::MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props) = 0;
	virtual vector<ArchiveEntry::UPtr> writeMap(
		const MapObjectCollection& map_data,
		const PropertyList&        map_extra_props) = 0;

	virtual string udmfNamespace() = 0;

	static UPtr get(MapFormat format);
};
