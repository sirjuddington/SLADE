
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
public:
	MapSide(MapSector* sector = nullptr, SLADEMap* parent = nullptr);
	MapSide(SLADEMap* parent);
	~MapSide();

	void	copy(MapObject* c) override;

	bool	isOk() const { return !!sector; }

	MapSector*	getSector() const { return sector; }
	MapLine*	getParentLine() const { return parent; }
	string		getTexUpper() const { return tex_upper; }
	string		getTexMiddle() const { return tex_middle; }
	string		getTexLower() const { return tex_lower; }
	short		getOffsetX() const { return offset_x; }
	short		getOffsetY() const { return offset_y; }
	uint8_t		getLight();

	void	setSector(MapSector* sector);
	void	changeLight(int amount);

	int		intProperty(const string& key) override;
	void	setIntProperty(const string& key, int value) override;
	string	stringProperty(const string& key) override;
	void	setStringProperty(const string& key, const string& value) override;
	bool	scriptCanModifyProp(const string& key) override;

	void	writeBackup(Backup* backup) override;
	void	readBackup(Backup* backup) override;

private:
	// Basic data
	MapSector*	sector;
	MapLine*	parent;
	string		tex_upper;
	string		tex_middle;
	string		tex_lower;
	short		offset_x;
	short		offset_y;
};

#endif //__MAPSIDE_H__
