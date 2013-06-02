
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

class MapSide : public MapObject
{
	friend class SLADEMap;
	friend class MapLine;
private:
	// Basic data
	MapSector*	sector;
	MapLine*	parent;
	string		tex_upper;
	string		tex_middle;
	string		tex_lower;
	short		offset_x;
	short		offset_y;

public:
	MapSide(MapSector* sector = NULL, SLADEMap* parent = NULL);
	MapSide(SLADEMap* parent);
	~MapSide();

	void	copy(MapObject* c);

	bool	isOk() { return !!sector; }

	MapSector*	getSector() { return sector; }
	MapLine*	getParentLine() { return parent; }
	string		getTexUpper() { return tex_upper; }
	string		getTexMiddle() { return tex_middle; }
	string		getTexLower() { return tex_lower; }
	short		getOffsetX() { return offset_x; }
	short		getOffsetY() { return offset_y; }

	void	setSector(MapSector* sector);

	int		intProperty(string key);
	void	setIntProperty(string key, int value);
	string	stringProperty(string key);
	void	setStringProperty(string key, string value);

	void	writeBackup(mobj_backup_t* backup);
	void	readBackup(mobj_backup_t* backup);
};

#endif //__MAPSIDE_H__
