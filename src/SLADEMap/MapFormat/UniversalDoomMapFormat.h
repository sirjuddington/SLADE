#pragma once

#include "MapFormatHandler.h"

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

	vector<ArchiveEntry::UPtr> writeMap(const MapObjectCollection& map_data, const PropertyList& map_extra_props)
		override;

	wxString udmfNamespace() override { return udmf_namespace_; }

private:
	wxString udmf_namespace_;

	std::unique_ptr<MapVertex> createVertex(ParseTreeNode* def) const;
	std::unique_ptr<MapSector> createSector(ParseTreeNode* def) const;
	std::unique_ptr<MapSide>   createSide(ParseTreeNode* def, const MapObjectCollection& map_data) const;
	std::unique_ptr<MapLine>   createLine(ParseTreeNode* def, const MapObjectCollection& map_data) const;
	std::unique_ptr<MapThing>  createThing(ParseTreeNode* def) const;
};
