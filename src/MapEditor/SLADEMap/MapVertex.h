
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
public:
	MapVertex(SLADEMap* parent = nullptr);
	MapVertex(double x, double y, SLADEMap* parent = nullptr);
	~MapVertex();

	double		xPos() const { return x; }
	double		yPos() const { return y; }

	fpoint2_t	getPoint(uint8_t point) override;
	fpoint2_t	point();

	int		intProperty(string key) override;
	double	floatProperty(string key) override;
	void	setIntProperty(string key, int value) override;
	void	setFloatProperty(string key, double value) override;
	bool	scriptCanModifyProp(const string& key) override;

	void		connectLine(MapLine* line);
	void		disconnectLine(MapLine* line);
	unsigned	nConnectedLines() const { return connected_lines.size(); }
	MapLine*	connectedLine(unsigned index);

	void	writeBackup(mobj_backup_t* backup) override;
	void	readBackup(mobj_backup_t* backup) override;

	operator Debuggable() const
	{
		if (!this)
			return Debuggable("<vertex NULL>");

		return Debuggable(S_FMT("<vertex %u>", index));
	}

private:
	// Basic data
	double		x;
	double		y;

	// Internal info
	vector<MapLine*>	connected_lines;
};

#endif //__MAPVERTEX_H__
