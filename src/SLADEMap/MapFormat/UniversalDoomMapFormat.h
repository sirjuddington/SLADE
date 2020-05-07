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
	bool readMap(Archive::MapDesc map, MapObjectCollection& map_data, PropertyList& map_extra_props) override;

	vector<unique_ptr<ArchiveEntry>> writeMap(const MapObjectCollection& map_data, const PropertyList& map_extra_props)
		override;

	string udmfNamespace() override { return udmf_namespace_; }

private:
	string udmf_namespace_;

	unique_ptr<MapVertex> createVertex(ParseTreeNode* def) const;
	unique_ptr<MapSector> createSector(ParseTreeNode* def) const;
	unique_ptr<MapSide>   createSide(ParseTreeNode* def, const MapObjectCollection& map_data) const;
	unique_ptr<MapLine>   createLine(ParseTreeNode* def, const MapObjectCollection& map_data) const;
	unique_ptr<MapThing>  createThing(ParseTreeNode* def) const;
};
} // namespace slade
