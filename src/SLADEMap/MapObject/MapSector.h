#pragma once

#include "MapObject.h"
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
		wxString texture;
		int      height = 0;
		Plane    plane  = { 0., 0., 1., 0. };

		Surface(const wxString& texture = "", int height = 0, const Plane& plane = { 0., 0., 1., 0. }) :
			texture{ texture },
			height{ height },
			plane{ plane }
		{
		}
	};

	// UDMF properties
	static const wxString PROP_TEXFLOOR;
	static const wxString PROP_TEXCEILING;
	static const wxString PROP_HEIGHTFLOOR;
	static const wxString PROP_HEIGHTCEILING;
	static const wxString PROP_LIGHTLEVEL;
	static const wxString PROP_SPECIAL;
	static const wxString PROP_ID;

	MapSector(
		int             f_height = 0,
		const wxString& f_tex    = "",
		int             c_height = 0,
		const wxString& c_tex    = "",
		short           light    = 0,
		short           special  = 0,
		short           id       = 0);
	MapSector(const wxString& f_tex, const wxString& c_tex, ParseTreeNode* udmf_def);
	~MapSector() = default;

	void copy(MapObject* obj) override;

	const Surface& floor() const { return floor_; }
	const Surface& ceiling() const { return ceiling_; }
	short          lightLevel() const { return light_; }
	short          special() const { return special_; }
	short          tag() const { return id_; }
	short          id() const { return id_; }

	wxString stringProperty(const wxString& key) override;
	int      intProperty(const wxString& key) override;

	void setStringProperty(const wxString& key, const wxString& value) override;
	void setFloatProperty(const wxString& key, double value) override;
	void setIntProperty(const wxString& key, int value) override;

	void setFloorTexture(const wxString& tex);
	void setCeilingTexture(const wxString& tex);
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

	Vec2f             getPoint(Point point) override;
	void              resetBBox() { bbox_.reset(); }
	BBox              boundingBox();
	vector<MapSide*>& connectedSides() { return connected_sides_; }
	void              resetPolygon() { poly_needsupdate_ = true; }
	Polygon2D*        polygon();
	bool              containsPoint(Vec2f point);
	double            distanceTo(Vec2f point, double maxdist = -1);
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

	void writeUDMF(wxString& def) override;

	operator Debuggable() const
	{
		if (!this)
			return { "<sector NULL>" };

		return { S_FMT("<sector %u>", index_) };
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
	Vec2f            text_point_;

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
