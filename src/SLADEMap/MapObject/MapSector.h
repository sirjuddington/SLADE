#pragma once

#include "Geometry/BBox.h"
#include "Geometry/Plane.h"
#include "MapObject.h"

namespace slade
{
class Debuggable;

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

		Surface(string_view texture = "", int height = 0, const Plane& plane = { 0., 0., 1., 0. }) :
			texture{ texture },
			height{ height },
			plane{ plane }
		{
		}
	};

	struct ExtraFloor
	{
		// TODO merge with surface?
		enum
		{
			// TODO how does vavoom work?  their wiki is always broken
			// VAVOOM,
			SOLID     = 1,
			SWIMMABLE = 2,
			NONSOLID  = 3,

			DISABLE_LIGHTING      = 1,
			LIGHTING_INSIDE_ONLY  = 2,
			INNER_FOG_EFFECT      = 4,
			FLAT_AT_CEILING       = 8,
			USE_UPPER_TEXTURE     = 16,
			USE_LOWER_TEXTURE     = 32,
			ADDITIVE_TRANSPARENCY = 64,
		};

		Plane         floor_plane;
		Plane         ceiling_plane;
		short         effective_height;
		short         floor_light;
		short         ceiling_light;
		unsigned      control_sector_index;
		unsigned      control_line_index;
		int           floor_type;
		float         alpha;
		bool          draw_inside;
		unsigned char flags;

		bool disableLighting() const { return flags & ExtraFloor::DISABLE_LIGHTING; }
		bool lightingInsideOnly() const { return flags & ExtraFloor::LIGHTING_INSIDE_ONLY; }
		bool ceilingOnly() const { return flags & ExtraFloor::FLAT_AT_CEILING; }
		bool useUpperTexture() const { return flags & ExtraFloor::USE_UPPER_TEXTURE; }
		bool useLowerTexture() const { return flags & ExtraFloor::USE_LOWER_TEXTURE; }
		bool additiveTransparency() const { return flags & ExtraFloor::ADDITIVE_TRANSPARENCY; }
	};

	// UDMF properties
	inline static const string PROP_TEXFLOOR      = "texturefloor";
	inline static const string PROP_TEXCEILING    = "textureceiling";
	inline static const string PROP_HEIGHTFLOOR   = "heightfloor";
	inline static const string PROP_HEIGHTCEILING = "heightceiling";
	inline static const string PROP_LIGHTLEVEL    = "lightlevel";
	inline static const string PROP_SPECIAL       = "special";
	inline static const string PROP_ID            = "id";

	MapSector(
		int         f_height = 0,
		string_view f_tex    = "",
		int         c_height = 0,
		string_view c_tex    = "",
		short       light    = 0,
		short       special  = 0,
		short       id       = 0);
	MapSector(string_view f_tex, string_view c_tex, const ParseTreeNode* udmf_def);
	~MapSector() override;

	void copy(MapObject* obj) override;

	const Surface& floor() const { return floor_; }
	const Surface& ceiling() const { return ceiling_; }
	short          lightLevel() const { return light_; }
	short          special() const { return special_; }
	short          tag() const { return id_; }
	short          id() const { return id_; }

	string stringProperty(string_view key) override;
	int    intProperty(string_view key) override;

	void setStringProperty(string_view key, string_view value) override;
	void setFloatProperty(string_view key, double value) override;
	void setIntProperty(string_view key, int value) override;

	void setFloorTexture(string_view tex);
	void setCeilingTexture(string_view tex);
	void setFloorHeight(short height);
	void setCeilingHeight(short height);
	void setFloorPlane(const Plane& p);
	void setCeilingPlane(const Plane& p);
	void setLightLevel(int light);
	void setSpecial(int special);
	void setTag(int tag);

	template<SurfaceType p> short planeHeight();
	template<SurfaceType p> Plane plane();
	template<SurfaceType p> void  setPlane(const Plane& plane);

	Vec2d                    getPoint(Point point) override;
	void                     resetBBox() { bbox_.reset(); }
	BBox                     boundingBox();
	vector<MapSide*>&        connectedSides() { return connected_sides_; }
	const vector<MapSide*>&  connectedSides() const { return connected_sides_; }
	const vector<glm::vec2>& polygonVertices();
	void                     resetPolygon() { poly_needsupdate_ = true; }
	bool                     containsPoint(const Vec2d& point);
	double                   distanceTo(const Vec2d& point, double maxdist = -1);
	bool                     putLines(vector<MapLine*>& list) const;
	bool                     putVertices(vector<MapVertex*>& list) const;
	bool                     putVertices(vector<MapObject*>& list) const;
	uint8_t                  lightAt(int where = 0, int extra_floor_index = -1);
	void                     changeLight(int amount, int where = 0);
	ColRGBA                  colourAt(int where = 0, bool fullbright = false);
	ColRGBA                  fogColour();
	long                     geometryUpdatedTime() const { return geometry_updated_; }
	void                     findTextPoint();

	void connectSide(MapSide* side);
	void disconnectSide(const MapSide* side);
	void clearConnectedSides() { connected_sides_.clear(); }

	void updateBBox();

	// Extra floors
	const vector<ExtraFloor>& extraFloors() const { return extra_floors_; }
	void                      clearExtraFloors() { extra_floors_.clear(); }
	void                      addExtraFloor(const ExtraFloor& extra_floor, const MapSector& control_sector);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(string& def) override;

	operator Debuggable() const;

private:
	// Basic data
	Surface floor_;
	Surface ceiling_;
	short   light_   = 0;
	short   special_ = 0;
	short   id_      = 0;

	// Internal info
	vector<MapSide*>   connected_sides_;
	BBox               bbox_;
	vector<glm::vec2>  polygon_triangles_;
	bool               poly_needsupdate_ = true;
	long               geometry_updated_ = 0;
	Vec2d              text_point_       = {};
	vector<ExtraFloor> extra_floors_;

	void setGeometryUpdated();
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
} // namespace slade
