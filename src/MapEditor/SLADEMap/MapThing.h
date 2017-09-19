
#ifndef __MAPTHING_H__
#define __MAPTHING_H__

#include "MapObject.h"

class MapThing : public MapObject
{
	friend class SLADEMap;
public:
	struct DoomData
	{
		short	x;
		short	y;
		short	angle;
		short	type;
		short	flags;
	};

	struct HexenData
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

	struct Doom64Data
	{
		short	x;
		short	y;
		short	z;
		short	angle;
		short	type;
		short	flags;
		short	tid;
	};

	MapThing(SLADEMap* parent = nullptr);
	MapThing(double x, double y, short type, SLADEMap* parent = nullptr);
	~MapThing();

	double		xPos() const { return position_.x; }
	double		yPos() const { return position_.y; }
	fpoint2_t	pos() const { return position_; }
	int			type() const { return type_; }
	int			angle() const { return angle_; }

	void		setPos(double x, double y) { position_.set(x, y); }

	fpoint2_t	point(Point point = Point::Mid) override;

	int		intProperty(const string& key) override;
	double	floatProperty(const string& key) override;
	void	setIntProperty(const string& key, int value) override;
	void	setFloatProperty(const string& key, double value) override;

	void	copy(MapObject* c) override;

	void	setAnglePoint(fpoint2_t point);

	void	writeBackup(Backup* backup) override;
	void	readBackup(Backup* backup) override;

	operator Debuggable() const {
		if (!this)
			return Debuggable("<thing NULL>");

		return Debuggable(S_FMT("<thing %u>", index_));
	}

private:
	// Basic data
	int			type_		= 1;
	fpoint2_t	position_;
	int			angle_		= 0;
};

#endif //__MAPTHING_H__
