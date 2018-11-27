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

	MapSector(SLADEMap* parent = nullptr);
	MapSector(string f_tex, string c_tex, SLADEMap* parent = nullptr);
	~MapSector();

	void copy(MapObject* copy) override;

	string  getFloorTex() const { return f_tex_; }
	string  getCeilingTex() const { return c_tex_; }
	short   getFloorHeight() const { return f_height_; }
	short   getCeilingHeight() const { return c_height_; }
	short   getLightLevel() const { return light_; }
	short   getSpecial() const { return special_; }
	short   getTag() const { return tag_; }
	plane_t getFloorPlane() const { return plane_floor_; }
	plane_t getCeilingPlane() const { return plane_ceiling_; }

	string stringProperty(const string& key) override;
	int    intProperty(const string& key) override;
	void   setStringProperty(const string& key, const string& value) override;
	void   setFloatProperty(const string& key, double value) override;
	void   setIntProperty(const string& key, int value) override;
	void   setFloorHeight(short height);
	void   setCeilingHeight(short height);
	void   setFloorPlane(plane_t p)
	{
		if (plane_floor_ != p)
			setGeometryUpdated();
		plane_floor_ = p;
	}
	void setCeilingPlane(plane_t p)
	{
		if (plane_ceiling_ != p)
			setGeometryUpdated();
		plane_ceiling_ = p;
	}

	template<SurfaceType p> short   getPlaneHeight();
	template<SurfaceType p> plane_t getPlane();
	template<SurfaceType p> void    setPlane(plane_t plane);

	fpoint2_t         getPoint(Point point) override;
	void              resetBBox() { bbox_.reset(); }
	bbox_t            boundingBox();
	vector<MapSide*>& connectedSides() { return connected_sides_; }
	void              resetPolygon() { poly_needsupdate_ = true; }
	Polygon2D*        getPolygon();
	bool              isWithin(fpoint2_t point);
	double            distanceTo(fpoint2_t point, double maxdist = -1);
	bool              getLines(vector<MapLine*>& list);
	bool              getVertices(vector<MapVertex*>& list);
	bool              getVertices(vector<MapObject*>& list);
	uint8_t           getLight(int where = 0);
	void              changeLight(int amount, int where = 0);
	rgba_t            getColour(int where = 0, bool fullbright = false);
	rgba_t            getFogColour();
	long              geometryUpdatedTime() const { return geometry_updated_; }

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
	string f_tex_;
	string c_tex_;
	short  f_height_;
	short  c_height_;
	short  light_;
	short  special_;
	short  tag_;

	// Internal info
	vector<MapSide*> connected_sides_;
	bbox_t           bbox_;
	Polygon2D        polygon_;
	bool             poly_needsupdate_;
	long             geometry_updated_;
	fpoint2_t        text_point_;
	plane_t          plane_floor_;
	plane_t          plane_ceiling_;

	void setGeometryUpdated();
};

// Note: these MUST be inline, or the linker will complain
template<> inline short MapSector::getPlaneHeight<MapSector::Floor>()
{
	return getFloorHeight();
}
template<> inline short MapSector::getPlaneHeight<MapSector::Ceiling>()
{
	return getCeilingHeight();
}
template<> inline plane_t MapSector::getPlane<MapSector::Floor>()
{
	return getFloorPlane();
}
template<> inline plane_t MapSector::getPlane<MapSector::Ceiling>()
{
	return getCeilingPlane();
}
template<> inline void MapSector::setPlane<MapSector::Floor>(plane_t plane)
{
	setFloorPlane(plane);
}
template<> inline void MapSector::setPlane<MapSector::Ceiling>(plane_t plane)
{
	setCeilingPlane(plane);
}
