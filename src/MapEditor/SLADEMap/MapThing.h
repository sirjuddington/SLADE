
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

class MapThing : public MapObject
{
	friend class SLADEMap;
private:
	// Basic data
	short		type;
	double		x;
	double		y;
	short		angle;

public:
	MapThing(SLADEMap* parent = NULL);
	MapThing(double x, double y, short type, SLADEMap* parent = NULL);
	~MapThing();

	double		xPos() { return x; }
	double		yPos() { return y; }
	void		setPos(double x, double y) { this->x = x; this->y = y; }

	fpoint2_t	getPoint(uint8_t point) override;
	fpoint2_t	point();

	short	getType() const { return type; }
	short	getAngle() const { return angle; }

	int		intProperty(const string& key) override;
	double	floatProperty(const string& key) override;
	void	setIntProperty(const string& key, int value) override;
	void	setFloatProperty(const string& key, double value) override;

	void	copy(MapObject* c) override;

	void	setAnglePoint(fpoint2_t point);

	void	writeBackup(mobj_backup_t* backup) override;
	void	readBackup(mobj_backup_t* backup) override;

	operator Debuggable() const {
		if (!this)
			return Debuggable("<thing NULL>");

		return Debuggable(S_FMT("<thing %u>", index));
	}
};

#endif //__MAPTHING_H__
