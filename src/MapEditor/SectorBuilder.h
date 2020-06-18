#pragma once

namespace slade
{
// Forward declarations
class MapLine;
class MapVertex;
class MapSector;
class MapSide;
class SLADEMap;

class SectorBuilder
{
public:
	struct Edge
	{
		MapLine* line         = nullptr;
		bool     front        = true;
		bool     side_created = false;

		Edge(MapLine* line = nullptr, bool front = true) : line{ line }, front{ front } {}
	};

	SectorBuilder()  = default;
	~SectorBuilder() = default;

	const string& error() const { return error_; }
	unsigned      nEdges() const { return sector_edges_.size(); }
	MapLine*      edgeLine(unsigned index);
	bool          edgeIsFront(unsigned index);
	bool          edgeSideCreated(unsigned index);

	bool       traceOutline(MapLine* line, bool front = true);
	int        nearestEdge(double x, double y);
	bool       pointWithinOutline(double x, double y);
	void       discardOutsideVertices();
	Edge       findOuterEdge() const;
	Edge       findInnerEdge();
	MapSector* findCopySector();
	MapSector* findExistingSector(vector<MapSide*>& sides_ignore);
	bool       isValidSector();

	bool traceSector(SLADEMap* map, MapLine* line, bool front = true);
	void createSector(MapSector* sector = nullptr, MapSector* sector_copy = nullptr);

	// Testing
	void drawResult();

private:
	vector<bool> vertex_valid_;
	SLADEMap*    map_ = nullptr;
	vector<Edge> sector_edges_;
	string       error_;

	// Current outline
	vector<Edge> o_edges_;
	bool         o_clockwise_ = false;
	BBox         o_bbox_;
	MapVertex*   vertex_right_ = nullptr;
};
} // namespace slade
