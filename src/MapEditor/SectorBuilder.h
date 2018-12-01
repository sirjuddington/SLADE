#pragma once

// Forward declarations
class MapLine;
class MapVertex;
class MapSector;
class MapSide;
class SLADEMap;

WX_DECLARE_HASH_MAP(MapLine*, int, wxPointerHash, wxPointerEqual, MapLineSet);

class SectorBuilder
{
public:
	struct Edge
	{
		MapLine* line;
		bool     front;
		bool     side_created;

		Edge(MapLine* line = nullptr, bool front = true)
		{
			this->line   = line;
			this->front  = front;
			side_created = false;
		}
	};

	SectorBuilder();
	~SectorBuilder();

	string   error() { return error_; }
	unsigned nEdges() { return sector_edges_.size(); }
	MapLine* edgeLine(unsigned index);
	bool     edgeIsFront(unsigned index);
	bool     edgeSideCreated(unsigned index);

	Edge       nextEdge(Edge edge, MapLineSet& visited_lines);
	bool       traceOutline(MapLine* line, bool front = true);
	int        nearestEdge(double x, double y);
	bool       pointWithinOutline(double x, double y);
	void       discardOutsideVertices();
	Edge       findOuterEdge();
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
	SLADEMap*    map_;
	vector<Edge> sector_edges_;
	string       error_;

	// Current outline
	vector<Edge> o_edges_;
	bool         o_clockwise_;
	BBox         o_bbox_;
	MapVertex*   vertex_right_;
};
