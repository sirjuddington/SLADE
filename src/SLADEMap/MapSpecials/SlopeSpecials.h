#pragma once

namespace slade::map
{
enum class SectorSurfaceType : u8;

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

	bool lineUpdated(const MapLine& line, bool update_planes = true);
	bool sectorUpdated(MapSector& sector, bool update_planes = true);
	bool thingUpdated(const MapThing& thing, bool update_planes = true);

private:
	struct VertexHeightThing
	{
		SectorSurfaceType surface_type;
		const MapThing*   thing  = nullptr;
		const MapVertex*  vertex = nullptr;
	};

	struct SlopeSpecial
	{
		enum class Type : u8
		{
			PlaneAlign,
			PlaneCopy,
			SRB2Vertex,
			LineThing,
			SectorTiltThing,
			VavoomThing,
			CopyThing,
		};

		Type              type;
		SectorSurfaceType surface_type;
		MapSector*        target = nullptr;

		bool isTarget(const MapSector* sector, SectorSurfaceType surface_type) const
		{
			return sector == target && this->surface_type == surface_type;
		}
	};

	struct PlaneAlign : SlopeSpecial
	{
		const MapLine*   line  = nullptr;
		const MapSector* model = nullptr;
		PlaneAlign(SectorSurfaceType surface_type) : SlopeSpecial{ Type::PlaneAlign, surface_type } {}
	};

	struct PlaneCopy : SlopeSpecial
	{
		const MapLine*   line  = nullptr;
		const MapSector* model = nullptr;
		PlaneCopy(SectorSurfaceType surface_type) : SlopeSpecial{ Type::PlaneCopy, surface_type } {}
	};

	struct SRB2VertexSlope : SlopeSpecial
	{
		const MapLine*  line        = nullptr;
		const MapThing* vertices[3] = { nullptr, nullptr, nullptr };
		SRB2VertexSlope(SectorSurfaceType surface_type) : SlopeSpecial{ Type::SRB2Vertex, surface_type } {}
	};

	struct SlopeSpecialThing : SlopeSpecial
	{
		const MapThing* thing = nullptr;
		SlopeSpecialThing(Type type, SectorSurfaceType surface_type) : SlopeSpecial{ type, surface_type } {}
	};

	struct CopySlopeThing : SlopeSpecialThing
	{
		const MapSector* model = nullptr;
		CopySlopeThing(SectorSurfaceType surface_type) : SlopeSpecialThing{ Type::CopyThing, surface_type } {}
	};

	struct LineSlopeThing : SlopeSpecialThing
	{
		const MapLine*   line              = nullptr;
		const MapSector* containing_sector = nullptr;
		LineSlopeThing(SectorSurfaceType surface_type) : SlopeSpecialThing{ Type::LineThing, surface_type } {}
	};

	struct SectorTiltThing : SlopeSpecialThing
	{
		SectorTiltThing(SectorSurfaceType surface_type) : SlopeSpecialThing{ Type::SectorTiltThing, surface_type } {}
	};

	struct VavoomSlopeThing : SlopeSpecialThing
	{
		const MapLine* line = nullptr;
		VavoomSlopeThing(SectorSurfaceType surface_type) : SlopeSpecialThing{ Type::VavoomThing, surface_type } {}
	};

	SLADEMap*                  map_ = nullptr;
	vector<MapSector*>         sectors_to_update_;
	vector<PlaneAlign>         plane_align_specials_;
	bool                       plane_align_specials_sorted_ = false;
	vector<LineSlopeThing>     line_slope_things_;
	vector<SectorTiltThing>    sector_tilt_things_;
	vector<VavoomSlopeThing>   vavoom_things_;
	vector<SlopeSpecialThing*> sorted_slope_things_;
	vector<CopySlopeThing>     copy_slope_things_;
	bool                       copy_slope_things_sorted_ = false;
	vector<PlaneCopy>          plane_copy_specials_;
	bool                       plane_copy_specials_sorted_ = false;
	vector<VertexHeightThing>  vertex_height_things_;
	vector<SRB2VertexSlope>    srb2_vertex_slope_specials_;
	bool                       srb2_vertex_slope_specials_sorted_ = false;
	mutable bool               specials_updated_                  = false;

	// Vertex slopes
	std::optional<double> vertexHeight(const MapVertex& vertex, SectorSurfaceType surface_type) const;
	void                  applyTriangleVertexSlope(MapSector& sector, const vector<MapVertex*>& vertices) const;
	void                  applyRectangleVertexSlope(MapSector& sector, SectorSurfaceType surface_type) const;

	// Plane_Align
	void addPlaneAlign(const MapLine& line);
	void addPlaneAlign(const MapLine& line, SectorSurfaceType surface_type, int where);
	void removePlaneAlign(const MapLine& line);
	void applyPlaneAlign(const PlaneAlign& special);
	void applyPlaneAlignSpecials(const MapSector& sector);

	// SlopeThings (LineSlopeThing, SectorTiltThing, VavoomSlopeThing)
	void addLineSlopeThing(const MapThing& thing, SectorSurfaceType surface_type);
	void applyLineSlopeThing(const LineSlopeThing& special);
	void addSectorTiltThing(const MapThing& thing, SectorSurfaceType surface_type);
	void applySectorTiltThing(const SectorTiltThing& special);
	void addVavoomSlopeThing(const MapThing& thing, SectorSurfaceType surface_type);
	void applyVavoomSlopeThing(const VavoomSlopeThing& special);
	void removeSlopeThing(const MapThing& thing);
	void applySlopeThingSpecials(const MapSector& sector);

	// CopySlopeThing
	void addCopySlopeThing(const MapThing& thing, SectorSurfaceType surface_type);
	void removeCopySlopeThing(const MapThing& thing);
	void applyCopySlopeThing(const CopySlopeThing& special);
	void applyCopySlopeThingSpecials(const MapSector& sector);

	// VertexHeightThing
	void addVertexHeightThing(const MapThing& thing, SectorSurfaceType surface_type);
	void removeVertexHeightThing(const MapThing& thing);

	// Plane_Copy
	void addPlaneCopy(const MapLine& line);
	void addSRB2PlaneCopy(const MapLine& line, SectorSurfaceType surface_type);
	void removePlaneCopy(const MapLine& line);
	void applyPlaneCopy(const PlaneCopy& special);
	void applyPlaneCopySpecials(const MapSector& sector);

	// SRB2 Vertex Slope
	void addSRB2VertexSlope(const MapLine& line, SectorSurfaceType surface_type, bool front);
	void removeSRB2VertexSlope(const MapLine& line);
	void applySRB2VertexSlopeSpecials(const MapSector& sector);
};
} // namespace slade::map
