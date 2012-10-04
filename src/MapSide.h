
#ifndef __MAPSIDE_H__
#define __MAPSIDE_H__

#include "MapObject.h"

class MapSector;
class MapLine;

struct doomside_t
{
	short	x_offset;
	short	y_offset;
	char	tex_upper[8];
	char	tex_lower[8];
	char	tex_middle[8];
	short	sector;
};

struct doom64side_t
{
	short		x_offset;
	short		y_offset;
	uint16_t	tex_upper;
	uint16_t	tex_lower;
	uint16_t	tex_middle;
	short		sector;
};

class MapSide : public MapObject {
friend class SLADEMap;
friend class MapLine;
private:
	// Basic data
	MapSector*	sector;
	MapLine*	parent;

public:
	MapSide(MapSector* sector = NULL, SLADEMap* parent = NULL);
	~MapSide();

	bool	isOk() { return !!sector; }

	MapSector*	getSector() { return sector; }
	MapLine*	getParentLine() { return parent; }

	void	setSector(MapSector* sector);

	int		intProperty(string key);
	void	setIntProperty(string key, int value);
};

#endif //__MAPSIDE_H__
