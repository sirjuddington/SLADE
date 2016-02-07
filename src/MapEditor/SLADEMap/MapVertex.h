
#ifndef __MAPVERTEX_H__
#define __MAPVERTEX_H__

#include "MapObject.h"

class MapLine;

struct doomvertex_t
{
	short x;
	short y;
};

// Those are actually fixed_t
struct doom64vertex_t
{
	int32_t x;
	int32_t y;
};

class MapVertex : public MapObject
{
	friend class SLADEMap;
private:
	// Basic data
	double		x;
	double		y;

	// Internal info
	vector<MapLine*>	connected_lines;

public:
	MapVertex(SLADEMap* parent = NULL);
	MapVertex(double x, double y, SLADEMap* parent = NULL);
	~MapVertex();

	double		xPos() { return x; }
	double		yPos() { return y; }

	fpoint2_t	getPoint(uint8_t point);
	fpoint2_t	point();

	int		intProperty(string key);
	double	floatProperty(string key);
	void	setIntProperty(string key, int value);
	void	setFloatProperty(string key, double value);

	void		connectLine(MapLine* line);
	void		disconnectLine(MapLine* line);
	unsigned	nConnectedLines() { return connected_lines.size(); }
	MapLine*	connectedLine(unsigned index);

	void	writeBackup(mobj_backup_t* backup);
	void	readBackup(mobj_backup_t* backup);

	operator Debuggable() const {
		if (!this)
			return Debuggable("<vertex NULL>");

		return Debuggable(S_FMT("<vertex %u>", index));
	}
};

#endif //__MAPVERTEX_H__
