#pragma once

#include "Geometry/BBox.h"
#include "Geometry/Plane.h"
#include "MapObject.h"
#include "SLADEMap/Types.h"

namespace slade
{
class Debuggable;

class MapSector : public MapObject
{
	friend class SLADEMap;
	friend class MapSide;

public:
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
	short          id() const { return id_; }
	bool           hasId(int id) const;

	string stringProperty(string_view key) const override;
	int    intProperty(string_view key) const override;

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

	template<SectorSurfaceType p> short planeHeight() const;
	template<SectorSurfaceType p> Plane plane() const;
	template<SectorSurfaceType p> void  setPlane(const Plane& plane);
	bool                                ceilingHasSlope() const;
	bool                                floorHasSlope() const;

	Vec2d                    getPoint(Point point) const override;
	void                     resetBBox() const { bbox_.reset(); }
	BBox                     boundingBox() const;
	vector<MapSide*>&        connectedSides() { return connected_sides_; }
	const vector<MapSide*>&  connectedSides() const { return connected_sides_; }
	const vector<glm::vec2>& polygonVertices() const;
	void                     resetPolygon() const { poly_needsupdate_ = true; }
	bool                     containsPoint(const Vec2d& point) const;
	double                   distanceTo(const Vec2d& point, double maxdist = -1) const;
	bool                     putLines(vector<MapLine*>& list) const;
	bool                     putVertices(vector<MapVertex*>& list) const;
	bool                     putVertices(vector<MapObject*>& list) const;
	uint8_t                  lightAt(int where = 0, int extra_floor_index = -1) const;
	void                     changeLight(int amount, int where = 0);
	ColRGBA                  colourAt(int where = 0, bool fullbright = false) const;
	ColRGBA                  fogColour() const;
	long                     geometryUpdatedTime() const { return geometry_updated_; }
	void                     findTextPoint() const;

	void connectSide(MapSide* side);
	void disconnectSide(const MapSide* side);
	void clearConnectedSides() { connected_sides_.clear(); }

	void updateBBox() const;

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
	vector<MapSide*>          connected_sides_;
	mutable BBox              bbox_;
	mutable vector<glm::vec2> polygon_triangles_;
	mutable bool              poly_needsupdate_ = true;
	mutable long              geometry_updated_ = 0;
	mutable Vec2d             text_point_       = {};

	void setGeometryUpdated() const;
};

// Note: these MUST be inline, or the linker will complain
template<> inline short MapSector::planeHeight<SectorSurfaceType::Floor>() const
{
	return floor_.height;
}
template<> inline short MapSector::planeHeight<SectorSurfaceType::Ceiling>() const
{
	return ceiling_.height;
}
template<> inline Plane MapSector::plane<SectorSurfaceType::Floor>() const
{
	return floor_.plane;
}
template<> inline Plane MapSector::plane<SectorSurfaceType::Ceiling>() const
{
	return ceiling_.plane;
}
template<> inline void MapSector::setPlane<SectorSurfaceType::Floor>(const Plane& plane)
{
	setFloorPlane(plane);
}
template<> inline void MapSector::setPlane<SectorSurfaceType::Ceiling>(const Plane& plane)
{
	setCeilingPlane(plane);
}
} // namespace slade
