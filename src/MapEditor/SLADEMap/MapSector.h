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
		string  texture;
		int     height = 0;
		plane_t plane  = { 0., 0., 1., 0. };
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

	const Surface& floor() const { return floor_; }
	const Surface& ceiling() const { return ceiling_; }
	short          getLightLevel() const { return light_; }
	short          getSpecial() const { return special_; }
	short          getTag() const { return id_; }

	string stringProperty(const string& key) override;
	int    intProperty(const string& key) override;

	void setStringProperty(const string& key, const string& value) override;
	void setFloatProperty(const string& key, double value) override;
	void setIntProperty(const string& key, int value) override;

	void setFloorTexture(const string& tex);
	void setCeilingTexture(const string& tex);
	void setFloorHeight(short height);
	void setCeilingHeight(short height);
	void setFloorPlane(const plane_t& p);
	void setCeilingPlane(const plane_t& p);

	template<SurfaceType p> short   getPlaneHeight();
	template<SurfaceType p> plane_t getPlane();
	template<SurfaceType p> void    setPlane(const plane_t& plane);

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
	Surface floor_;
	Surface ceiling_;
	short   light_   = 0;
	short   special_ = 0;
	short   id_      = 0;

	// Internal info
	vector<MapSide*> connected_sides_;
	bbox_t           bbox_;
	Polygon2D        polygon_;
	bool             poly_needsupdate_;
	long             geometry_updated_;
	fpoint2_t        text_point_;

	void setGeometryUpdated();
};

// Note: these MUST be inline, or the linker will complain
template<> inline short MapSector::getPlaneHeight<MapSector::Floor>()
{
	return floor_.height;
}
template<> inline short MapSector::getPlaneHeight<MapSector::Ceiling>()
{
	return ceiling_.height;
}
template<> inline plane_t MapSector::getPlane<MapSector::Floor>()
{
	return floor_.plane;
}
template<> inline plane_t MapSector::getPlane<MapSector::Ceiling>()
{
	return ceiling_.plane;
}
template<> inline void MapSector::setPlane<MapSector::Floor>(const plane_t& plane)
{
	setFloorPlane(plane);
}
template<> inline void MapSector::setPlane<MapSector::Ceiling>(const plane_t& plane)
{
	setCeilingPlane(plane);
}
