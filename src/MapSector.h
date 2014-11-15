
#ifndef __MAPSECTOR_H__
#define __MAPSECTOR_H__

#include "MapObject.h"
#include "Polygon2D.h"
#include "Structs.h"

class MapSide;
class MapLine;
class MapVertex;

struct doomsector_t
{
	short	f_height;
	short	c_height;
	char	f_tex[8];
	char	c_tex[8];
	short	light;
	short	special;
	short	tag;
};

struct doom64sector_t
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

enum PlaneType
{
	FLOOR_PLANE,
	CEILING_PLANE,
};

class MapSector : public MapObject
{
	friend class SLADEMap;
	friend class MapSide;
private:
	// Basic data
	string		f_tex;
	string		c_tex;
	short		f_height;
	short		c_height;
	short		light;
	short		special;
	short		tag;

	// Internal info
	vector<MapSide*>	connected_sides;
	bbox_t				bbox;
	Polygon2D			polygon;
	bool				poly_needsupdate;
	long				geometry_updated;
	fpoint2_t			text_point;

public:
	MapSector(SLADEMap* parent = NULL);
	MapSector(string f_tex, string c_tex, SLADEMap* parent = NULL);
	~MapSector();

	void	copy(MapObject* copy);

	string		getFloorTex() { return f_tex; }
	string		getCeilingTex() { return c_tex; }
	short		getFloorHeight() { return f_height; }
	short		getCeilingHeight() { return c_height; }
	short		getLightLevel() { return light; }
	short		getSpecial() { return special; }
	short		getTag() { return tag; }

	string	stringProperty(string key);
	int		intProperty(string key);
	void	setStringProperty(string key, string value);
	void	setFloatProperty(string key, double value);
	void	setIntProperty(string key, int value);

	fpoint2_t			getPoint(uint8_t point);
	void				resetBBox() { bbox.reset(); }
	bbox_t				boundingBox();
	vector<MapSide*>&	connectedSides() { return connected_sides; }
	void				resetPolygon() { poly_needsupdate = true; }
	Polygon2D*			getPolygon();
	bool				isWithin(double x, double y);
	double				distanceTo(double x, double y, double maxdist = -1);
	bool				getLines(vector<MapLine*>& list);
	bool				getVertices(vector<MapVertex*>& list);
	bool				getVertices(vector<MapObject*>& list);
	plane_t				getFloorPlane() { return getPlane<FLOOR_PLANE>(); }
	plane_t				getCeilingPlane() { return getPlane<CEILING_PLANE>(); }
	uint8_t				getLight(int where = 0);
	void				changeLight(int amount, int where = 0);
	rgba_t				getColour(int where = 0, bool fullbright = false);
	long				geometryUpdatedTime() { return geometry_updated; }

	template<PlaneType p>
	short getPlaneHeight();

	template<PlaneType p>
	plane_t getPlane();

	void	connectSide(MapSide* side);
	void	disconnectSide(MapSide* side);

	void	updateBBox();

	void	writeBackup(mobj_backup_t* backup);
	void	readBackup(mobj_backup_t* backup);
};

#endif //__MAPSECTOR_H__
