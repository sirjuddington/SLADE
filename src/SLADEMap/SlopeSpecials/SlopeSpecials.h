#pragma once

namespace slade
{
enum class SectorSurfaceType : u8;
struct PlaneAlignSpecial;
struct PlaneCopySpecial;
struct LineSlopeThing;
struct SlopeSpecialThing;
struct SRB2VertexSlopeSpecial;

class SlopeSpecials
{
public:
	explicit SlopeSpecials(SLADEMap& map);
	~SlopeSpecials() = default;

	void processLineSpecial(const MapLine& line);
	void processThing(const MapThing& thing);
	void clearSpecials();

	void updateSectorPlanes(MapSector& sector);
	void updateOutdatedSectorPlanes();

	void lineUpdated(const MapLine& line, bool update_planes = true);
	void sectorUpdated(MapSector& sector, bool update_planes = true);
	void thingUpdated(const MapThing& thing, bool update_planes = true);

private:
	struct VertexHeightThing
	{
		SectorSurfaceType surface_type;
		const MapThing*   thing  = nullptr;
		const MapVertex*  vertex = nullptr;
	};

	SLADEMap*                                  map_ = nullptr;
	vector<MapSector*>                         sectors_to_update_;
	vector<unique_ptr<PlaneAlignSpecial>>      plane_align_specials_;
	bool                                       plane_align_specials_sorted_ = false;
	vector<unique_ptr<SlopeSpecialThing>>      slope_things_;
	bool                                       slope_things_sorted_ = false;
	vector<unique_ptr<SlopeSpecialThing>>      copy_slope_things_;
	bool                                       copy_slope_things_sorted_ = false;
	vector<unique_ptr<PlaneCopySpecial>>       plane_copy_specials_;
	bool                                       plane_copy_specials_sorted_ = false;
	vector<VertexHeightThing>                  vertex_height_things_;
	vector<unique_ptr<SRB2VertexSlopeSpecial>> srb2_vertex_slope_specials_;
	bool                                       srb2_vertex_slope_specials_sorted_ = false;

	std::optional<double> vertexHeight(const MapVertex& vertex, SectorSurfaceType surface_type) const;
	void                  applyTriangleVertexSlope(MapSector& sector, const vector<MapVertex*>& vertices) const;
	void                  applyRectangleVertexSlope(MapSector& sector, SectorSurfaceType surface_type) const;

	// Plane_Align
	void addPlaneAlign(const MapLine& line);
	void addPlaneAlign(const MapLine& line, SectorSurfaceType surface_type, int where);
	void removePlaneAlign(const MapLine& line);
	void applyPlaneAlign(const MapSector& sector);

	// SlopeThings (LineSlopeThing, SectorTiltThing, VavoomSlopeThing)
	void addLineSlopeThing(const MapThing& thing, SectorSurfaceType surface_type);
	void addSectorTiltThing(const MapThing& thing, SectorSurfaceType surface_type);
	void addVavoomSlopeThing(const MapThing& thing, SectorSurfaceType surface_type);
	void removeSlopeThing(const MapThing& thing);
	void applySlopeThings(const MapSector& sector);

	// CopySlopeThing
	void addCopySlopeThing(const MapThing& thing, SectorSurfaceType surface_type);
	void removeCopySlopeThing(const MapThing& thing);
	void applyCopySlopeThings(const MapSector& sector);

	// VertexHeightThing
	void addVertexHeightThing(const MapThing& thing, SectorSurfaceType surface_type);
	void removeVertexHeightThing(const MapThing& thing);

	// Plane_Copy
	void addPlaneCopy(const MapLine& line);
	void addSRB2PlaneCopy(const MapLine& line, SectorSurfaceType surface_type);
	void removePlaneCopy(const MapLine& line);
	void applyPlaneCopy(const MapSector& sector);

	// SRB2 Vertex Slope
	void addSRB2VertexSlope(const MapLine& line, SectorSurfaceType surface_type, bool front);
	void removeSRB2VertexSlope(const MapLine& line);
	void applySRB2VertexSlopes(const MapSector& sector);
};
} // namespace slade
