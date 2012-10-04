
#ifndef __MAPTHING_H__
#define __MAPTHING_H__

#include "MapObject.h"

struct doomthing_t
{
	short	x;
	short	y;
	short	angle;
	short	type;
	short	flags;
};

struct hexenthing_t
{
	short	tid;
	short	x;
	short	y;
	short	z;
	short	angle;
	short	type;
	short	flags;
	uint8_t	special;
	uint8_t	args[5];
};

struct doom64thing_t
{
	short	x;
	short	y;
	short	z;
	short	angle;
	short	type;
	short	flags;
	short	tid;
};

class MapThing : public MapObject {
friend class SLADEMap;
private:
	// Basic data
	short		type;
	double		x;
	double		y;

public:
	MapThing(SLADEMap* parent = NULL);
	MapThing(double x, double y, short type, SLADEMap* parent = NULL);
	~MapThing();

	double		xPos() { return x; }
	double		yPos() { return y; }
	void		setPos(double x, double y) { this->x = x; this->y = y; }

	fpoint2_t	midPoint() { return fpoint2_t(x, y); }

	short	getType() { return type; }

	int		intProperty(string key);
	double	floatProperty(string key);
	void	setIntProperty(string key, int value);
	void	setFloatProperty(string key, double value);

	void	copy(MapObject* c);

	void	setAnglePoint(fpoint2_t point);
};

#endif //__MAPTHING_H__
