#pragma once

#include "MapFormatHandler.h"

namespace slade
{
class MapVertex;
class MapSector;
class MapSide;
class MapLine;
class MapThing;
class ParseTreeNode;

class UniversalDoomMapFormat : public MapFormatHandler
{
public:
	bool readMap(MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props) override;

	vector<unique_ptr<ArchiveEntry>> writeMap(const MapObjectCollection& map_data, const PropertyList& map_extra_props)
		override;

	string udmfNamespace() const override { return udmf_namespace_; }
	void   setUDMFNamespace(string_view ns) override { udmf_namespace_ = ns; }

private:
	string udmf_namespace_;

	unique_ptr<MapVertex> createVertex(ParseTreeNode* def) const;
	unique_ptr<MapSector> createSector(ParseTreeNode* def) const;
	unique_ptr<MapSide>   createSide(ParseTreeNode* def, const MapObjectCollection& map_data) const;
	unique_ptr<MapLine>   createLine(ParseTreeNode* def, MapObjectCollection& map_data) const;
	unique_ptr<MapThing>  createThing(ParseTreeNode* def) const;
};
} // namespace slade
