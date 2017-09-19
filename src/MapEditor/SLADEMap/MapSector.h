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
	enum Surface
	{
		Floor,
		Ceiling,
	};

	struct DoomData
	{
		short	f_height;
		short	c_height;
		char	f_tex[8];
		char	c_tex[8];
		short	light;
		short	special;
		short	tag;
	};

	struct Doom64Data
	{
		short		f_height;
		short		c_height;
		uint16_t	f_tex;
		uint16_t	c_tex;
		uint16_t	color[5];
		short		special;
		short		tag;
		uint16_t	flags;
	};

	MapSector(SLADEMap* parent = nullptr);
	MapSector(string f_tex, string c_tex, SLADEMap* parent = nullptr);
	~MapSector();

	void	copy(MapObject* copy) override;

	string	texFloor() const { return tex_floor_; }
	string	texCeiling() const { return tex_ceiling_; }
	short	heightFloor() const { return height_floor_; }
	short	heightCeiling() const { return height_ceiling_; }
	short	lightLevel() const { return light_; }
	short	special() const { return special_; }
	short	id() const { return id_; }
	plane_t	planeFloor() const { return plane_floor_; }
	plane_t	planeCeiling() const { return plane_ceiling_; }

	string	stringProperty(const string& key) override;
	int		intProperty(const string& key) override;
	void	setStringProperty(const string& key, const string& value) override;
	void	setFloatProperty(const string& key, double value) override;
	void	setIntProperty(const string& key, int value) override;
	void	setFloorHeight(short height);
	void	setCeilingHeight(short height);
	void	setFloorPlane(plane_t p) { if (plane_floor_ != p) setGeometryUpdated(); plane_floor_ = p; }
	void	setCeilingPlane(plane_t p) { if (plane_ceiling_ != p) setGeometryUpdated(); plane_ceiling_ = p; }

	template<Surface p> short	planeHeight();
	template<Surface p> plane_t	plane();
	template<Surface p> void	setPlane(plane_t plane);

	fpoint2_t			point(Point point = Point::Mid) override;
	void				resetBBox() { bbox_.reset(); }
	bbox_t				boundingBox();
	vector<MapSide*>&	connectedSides() { return connected_sides_; }
	void				resetPolygon() { poly_needsupdate_ = true; }
	Polygon2D*			getPolygon();
	bool				isWithin(fpoint2_t point);
	double				distanceTo(fpoint2_t point, double maxdist = -1);
	bool				getLines(vector<MapLine*>& list);
	bool				getVertices(vector<MapVertex*>& list);
	bool				getVertices(vector<MapObject*>& list);
	uint8_t				getLight(int where = 0);
	void				changeLight(int amount, int where = 0);
	rgba_t				getColour(int where = 0, bool fullbright = false);
	rgba_t				getFogColour();
	long				geometryUpdatedTime() const { return geometry_updated_; }

	void	connectSide(MapSide* side);
	void	disconnectSide(MapSide* side);

	void	updateBBox();

	void	writeBackup(Backup* backup) override;
	void	readBackup(Backup* backup) override;

	operator Debuggable() const {
		if (!this)
			return Debuggable("<sector NULL>");

		return Debuggable(S_FMT("<sector %u>", index_));
	}

private:
	// Basic data
	string	tex_floor_;
	string	tex_ceiling_;
	int		height_floor_	= 0;
	int		height_ceiling_	= 0;
	int		light_			= 0;
	int		special_		= 0;
	int		id_				= 0;

	// Internal info
	vector<MapSide*>	connected_sides_;
	bbox_t				bbox_;
	Polygon2D			polygon_;
	bool				poly_needsupdate_	= true;
	long				geometry_updated_	= 0;
	fpoint2_t			text_point_;
	plane_t				plane_floor_		= { 0, 0, 1, 0 };
	plane_t				plane_ceiling_		= { 0, 0, 1, 0 };

	void	setGeometryUpdated();
};

// Note: these MUST be inline, or the linker will complain
template<> inline short MapSector::planeHeight<MapSector::Floor>() { return heightFloor(); }
template<> inline short MapSector::planeHeight<MapSector::Ceiling>() { return heightCeiling(); }
template<> inline plane_t MapSector::plane<MapSector::Floor>() { return planeFloor(); }
template<> inline plane_t MapSector::plane<MapSector::Ceiling>() { return planeCeiling(); }
template<> inline void MapSector::setPlane<MapSector::Floor>(plane_t plane) { setFloorPlane(plane); }
template<> inline void MapSector::setPlane<MapSector::Ceiling>(plane_t plane) { setCeilingPlane(plane); }
