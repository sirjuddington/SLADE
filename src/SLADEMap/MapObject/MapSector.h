#pragma once

#include "MapObject.h"
#include "Utility/Colour.h"
#include "Utility/Polygon2D.h"

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
		std::string texture;
		int         height = 0;
		Plane       plane  = { 0., 0., 1., 0. };

		Surface(std::string_view texture = "", int height = 0, const Plane& plane = { 0., 0., 1., 0. }) :
			texture{ texture },
			height{ height },
			plane{ plane }
		{
		}
	};

	// UDMF properties
	inline static const std::string PROP_TEXFLOOR      = "texturefloor";
	inline static const std::string PROP_TEXCEILING    = "textureceiling";
	inline static const std::string PROP_HEIGHTFLOOR   = "heightfloor";
	inline static const std::string PROP_HEIGHTCEILING = "heightceiling";
	inline static const std::string PROP_LIGHTLEVEL    = "lightlevel";
	inline static const std::string PROP_SPECIAL       = "special";
	inline static const std::string PROP_ID            = "id";

	MapSector(
		int              f_height = 0,
		std::string_view f_tex    = "",
		int              c_height = 0,
		std::string_view c_tex    = "",
		short            light    = 0,
		short            special  = 0,
		short            id       = 0);
	MapSector(std::string_view f_tex, std::string_view c_tex, ParseTreeNode* udmf_def);
	~MapSector() = default;

	void copy(MapObject* obj) override;

	const Surface& floor() const { return floor_; }
	const Surface& ceiling() const { return ceiling_; }
	short          lightLevel() const { return light_; }
	short          special() const { return special_; }
	short          tag() const { return id_; }
	short          id() const { return id_; }

	std::string stringProperty(std::string_view key) override;
	int         intProperty(std::string_view key) override;

	void setStringProperty(std::string_view key, std::string_view value) override;
	void setFloatProperty(std::string_view key, double value) override;
	void setIntProperty(std::string_view key, int value) override;

	void setFloorTexture(std::string_view tex);
	void setCeilingTexture(std::string_view tex);
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

	Vec2d             getPoint(Point point) override;
	void              resetBBox() { bbox_.reset(); }
	BBox              boundingBox();
	vector<MapSide*>& connectedSides() { return connected_sides_; }
	void              resetPolygon() { poly_needsupdate_ = true; }
	Polygon2D*        polygon();
	bool              containsPoint(Vec2d point);
	double            distanceTo(Vec2d point, double maxdist = -1);
	bool              putLines(vector<MapLine*>& list);
	bool              putVertices(vector<MapVertex*>& list);
	bool              putVertices(vector<MapObject*>& list);
	uint8_t           lightAt(int where = 0);
	void              changeLight(int amount, int where = 0);
	ColRGBA           colourAt(int where = 0, bool fullbright = false);
	ColRGBA           fogColour();
	long              geometryUpdatedTime() const { return geometry_updated_; }
	void              findTextPoint();

	void connectSide(MapSide* side);
	void disconnectSide(MapSide* side);
	void clearConnectedSides() { connected_sides_.clear(); }

	void updateBBox();

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(std::string& def) override;

	operator Debuggable() const
	{
		if (!this)
			return { "<sector NULL>" };

		return { fmt::format("<sector {}>", index_) };
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
	bool             poly_needsupdate_ = true;
	long             geometry_updated_ = 0;
	Vec2d            text_point_;

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
