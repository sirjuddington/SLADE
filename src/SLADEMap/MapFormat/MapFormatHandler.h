#pragma once

namespace slade
{
class MapObjectCollection;
class PropertyList;
enum class MapFormat;
struct MapDesc;

class MapFormatHandler
{
public:
	virtual ~MapFormatHandler() = default;

	virtual bool readMap(MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props) = 0;
	virtual vector<unique_ptr<ArchiveEntry>> writeMap(
		const MapObjectCollection& map_data,
		const PropertyList&        map_extra_props) = 0;

	virtual string udmfNamespace() const { return {}; }
	virtual void   setUDMFNamespace(string_view ns) {}

	static unique_ptr<MapFormatHandler> get(MapFormat format);
};
} // namespace slade
