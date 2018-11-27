#pragma once

#include "MapObject.h"

class MapThing : public MapObject
{
	friend class SLADEMap;

public:
	struct DoomData
	{
		short x;
		short y;
		short angle;
		short type;
		short flags;
	};

	struct HexenData
	{
		short   tid;
		short   x;
		short   y;
		short   z;
		short   angle;
		short   type;
		short   flags;
		uint8_t special;
		uint8_t args[5];
	};

	struct Doom64Data
	{
		short x;
		short y;
		short z;
		short angle;
		short type;
		short flags;
		short tid;
	};

	MapThing(SLADEMap* parent = nullptr);
	MapThing(double x, double y, short type, SLADEMap* parent = nullptr);
	~MapThing();

	double xPos() { return x_; }
	double yPos() { return y_; }
	void   setPos(double x, double y)
	{
		this->x_ = x;
		this->y_ = y;
	}

	fpoint2_t getPoint(Point point) override;
	fpoint2_t point();

	short getType() const { return type_; }
	short getAngle() const { return angle_; }

	int    intProperty(const string& key) override;
	double floatProperty(const string& key) override;
	void   setIntProperty(const string& key, int value) override;
	void   setFloatProperty(const string& key, double value) override;

	void copy(MapObject* c) override;

	void setAnglePoint(fpoint2_t point);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	operator Debuggable() const
	{
		if (!this)
			return Debuggable("<thing NULL>");

		return Debuggable(S_FMT("<thing %u>", index_));
	}

private:
	// Basic data
	short  type_;
	double x_;
	double y_;
	short  angle_;
};
