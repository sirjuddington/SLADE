
#ifndef __MAPSECTOR_H__
#define __MAPSECTOR_H__

#include "MapObject.h"
#include "Polygon2D.h"

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
	plane_t				plane_floor;
	plane_t				plane_ceiling;

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
	plane_t		getFloorPlane() { return plane_floor; }
	plane_t		getCeilingPlane() { return plane_ceiling; }
	double		floorHeightAt(double x, double y);
	double		ceilingHeightAt(double x, double y);

	string	stringProperty(string key);
	int		intProperty(string key);
	void	setStringProperty(string key, string value);
	void	setFloatProperty(string key, double value);
	void	setIntProperty(string key, int value);
	void	setFloorHeight(short height)
	{
		f_height = height;
		plane_floor.set(0, 0, 1, height);
	}
	void	setCeilingHeight(short height)
	{
		c_height = height;
		plane_ceiling.set(0, 0, 1, height);
	}
	void	setFloorPlane(plane_t p) { plane_floor = p; }
	void	setCeilingPlane(plane_t p) { plane_ceiling = p; }

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
	uint8_t				getLight(int where = 0);
	void				changeLight(int amount, int where = 0);
	rgba_t				getColour(int where = 0, bool fullbright = false);
	long				geometryUpdatedTime() { return geometry_updated; }

	void	connectSide(MapSide* side);
	void	disconnectSide(MapSide* side);

	void	updateBBox();

	void	writeBackup(mobj_backup_t* backup);
	void	readBackup(mobj_backup_t* backup);
};

#endif //__MAPSECTOR_H__
