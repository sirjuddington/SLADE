#pragma once

#include "MapObject.h"
#include "Utility/Polygon2D.h"

class MapSide;
class MapLine;
class MapVertex;

class MapSector : public MapObject
{
	friend class SLADEMap;
	friend class MapSide;

public:
	enum SurfaceType
	{
		Floor = 1,
		Ceiling
	};

	struct Surface
	{
		string texture;
		int    height = 0;
		Plane  plane  = { 0., 0., 1., 0. };
	};

	struct DoomData
	{
		short f_height;
		short c_height;
		char  f_tex[8];
		char  c_tex[8];
		short light;
		short special;
		short tag;
	};

	struct Doom64Data
	{
		short    f_height;
		short    c_height;
		uint16_t f_tex;
		uint16_t c_tex;
		uint16_t color[5];
		short    special;
		short    tag;
		uint16_t flags;
	};

	struct ExFloorType
	{
		// TODO merge with surface?
		enum
		{
			// TODO how does vavoom work?  their wiki is always broken
		// VAVOOM,
		SOLID     = 1,
		SWIMMABLE = 2,
		NONSOLID  = 3,

		DISABLE_LIGHTING = 1,
		LIGHTING_INSIDE_ONLY = 2,
		INNER_FOG_EFFECT = 4,
		FLAT_AT_CEILING = 8,
		USE_UPPER_TEXTURE = 16,
		USE_LOWER_TEXTURE = 32,
		ADDITIVE_TRANSPARENCY = 64,
	};

	Plane  floor_plane;
	Plane  ceiling_plane;
	short    effective_height;
	short    floor_light;
	short    ceiling_light;
	unsigned control_sector_index;
	unsigned control_line_index;
	int floor_type;
	float alpha;
	bool draw_inside;
	unsigned char flags;

	bool disableLighting() { return flags & ExFloorType::DISABLE_LIGHTING; }
	bool lightingInsideOnly() { return flags & ExFloorType::LIGHTING_INSIDE_ONLY; }
	bool ceilingOnly() { return flags & ExFloorType::FLAT_AT_CEILING; }
	bool useUpperTexture() { return flags & ExFloorType::USE_UPPER_TEXTURE; }
	bool useLowerTexture() { return flags & ExFloorType::USE_LOWER_TEXTURE; }
	bool additiveTransparency() { return flags & ExFloorType::ADDITIVE_TRANSPARENCY; }
};

	enum PlaneType
	{
		FLOOR_PLANE,
		CEILING_PLANE,
	};

	// TODO maybe make this private, maybe
	vector<ExFloorType> extra_floors;

	MapSector(SLADEMap* parent = nullptr);
	MapSector(string f_tex, string c_tex, SLADEMap* parent = nullptr);
	~MapSector();

	void copy(MapObject* copy) override;

	const Surface& floor() const { return floor_; }
	const Surface& ceiling() const { return ceiling_; }
	short   lightLevel() const { return light_; }
	short   special() const { return special_; }
	short   tag() const { return id_; }

	string stringProperty(const string& key) override;
	int    intProperty(const string& key) override;

	void setStringProperty(const string& key, const string& value) override;
	void setFloatProperty(const string& key, double value) override;
	void setIntProperty(const string& key, int value) override;

	void setFloorTexture(const string& tex);
	void setCeilingTexture(const string& tex);
	void setFloorHeight(short height);
	void setCeilingHeight(short height);
	void setFloorPlane(const Plane& p);
	void setCeilingPlane(const Plane& p);

	template<SurfaceType p> short planeHeight();
	template<SurfaceType p> Plane plane();
	template<SurfaceType p> void  setPlane(const Plane& plane);



	Vec2f             getPoint(Point point) override;
	void              resetBBox() { bbox_.reset(); }
	BBox              boundingBox();
	vector<MapSide*>& connectedSides() { return connected_sides_; }
	void              resetPolygon() { poly_needsupdate_ = true; }
	Polygon2D*        polygon();
	bool              isWithin(Vec2f point);
	double            distanceTo(Vec2f point, double maxdist = -1);
	bool              putLines(vector<MapLine*>& list);
	bool              putVertices(vector<MapVertex*>& list);
	bool              putVertices(vector<MapObject*>& list);
	uint8_t           lightAt(int where = 0, int extra_floor_index = -1);
	void              changeLight(int amount, int where = 0);
	ColRGBA           colourAt(int where = 0, bool fullbright = false);
	ColRGBA           fogColour();
	long              geometryUpdatedTime() const { return geometry_updated_; }
	void              setGeometryUpdated();

	void connectSide(MapSide* side);
	void disconnectSide(MapSide* side);

	void updateBBox();

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	operator Debuggable() const
	{
		if (!this)
			return Debuggable("<sector NULL>");

		return Debuggable(S_FMT("<sector %u>", index_));
	}

private:
	// Basic data
	Surface floor_;
	Surface ceiling_;
	short   light_   = 0;
	short   special_ = 0;
	short   id_      = 0;

	// Internal info
	vector<MapSide*> connected_sides_;
	BBox             bbox_;
	Polygon2D        polygon_;
	bool             poly_needsupdate_;
	long             geometry_updated_;

	Vec2f        text_point_;
	// Computed properties from MapSpecials, not directly stored in the map data
	Plane plane_floor;

	Plane plane_ceiling;
};

// Note: these MUST be inline, or the linker will complain
template<> inline short MapSector::planeHeight<MapSector::Floor>()
{
	return floor_.height;
}
template<> inline short MapSector::planeHeight<MapSector::Ceiling>()
{
	return ceiling_.height;
}
template<> inline Plane MapSector::plane<MapSector::Floor>()
{
	return floor_.plane;
}
template<> inline Plane MapSector::plane<MapSector::Ceiling>()
{
	return ceiling_.plane;
}
template<> inline void MapSector::setPlane<MapSector::Floor>(const Plane& plane)
{
	setFloorPlane(plane);
}
template<> inline void MapSector::setPlane<MapSector::Ceiling>(const Plane& plane)
{
	setCeilingPlane(plane);
}
