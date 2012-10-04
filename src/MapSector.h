
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

class MapSector : public MapObject {
friend class SLADEMap;
friend class MapSide;
private:
	// Basic data
	string		f_tex;
	string		c_tex;

	// Internal info
	vector<MapSide*>	connected_sides;
	bbox_t				bbox;
	Polygon2D			polygon;
	bool				poly_needsupdate;

public:
	MapSector(SLADEMap* parent = NULL);
	MapSector(string f_tex, string c_tex, SLADEMap* parent = NULL);
	~MapSector();

	void	copy(MapObject* copy);

	string		floorTexture() { return f_tex; }
	string		ceilingTexture() { return c_tex; }

	string	stringProperty(string key);
	void	setStringProperty(string key, string value);
	void	setFloatProperty(string key, double value);

	fpoint2_t			midPoint();
	void				resetBBox() { bbox.reset(); }
	bbox_t				boundingBox();
	vector<MapSide*>&	connectedSides() { return connected_sides; }
	void				resetPolygon() { poly_needsupdate = true; }
	Polygon2D*			getPolygon();
	bool				isWithin(double x, double y);
	double				distanceTo(double x, double y, double maxdist = -1);
	bool				getLines(vector<MapLine*>& list);
	bool				getVertices(vector<MapVertex*>& list);
	uint8_t				getLight(int where = 0);
	void				changeLight(int amount, int where = 0);
	rgba_t				getColour(int where = 0, bool fullbright = false);

	void	connectSide(MapSide* side);
	void	disconnectSide(MapSide* side);

	void	updateBBox();
};

#endif //__MAPSECTOR_H__
