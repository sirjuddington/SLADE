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

	MapThing(SLADEMap* parent = nullptr) : MapObject(Type::Thing, parent) {}
	MapThing(double x, double y, short type, SLADEMap* parent = nullptr);
	~MapThing() = default;

	double xPos() const { return position_.x; }
	double yPos() const { return position_.y; }
	Vec2f  position() const { return position_; }
	short  type() const { return type_; }
	short  angle() const { return angle_; }

	Vec2f getPoint(Point point) override;

	void setPos(double x, double y) { position_.set(x, y); }

	int    intProperty(const string& key) override;
	double floatProperty(const string& key) override;
	void   setIntProperty(const string& key, int value) override;
	void   setFloatProperty(const string& key, double value) override;

	void copy(MapObject* c) override;

	void setAnglePoint(Vec2f point);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	operator Debuggable() const
	{
		if (!this)
			return { "<thing NULL>" };

		return { S_FMT("<thing %u>", index_) };
	}

private:
	// Basic data
	short type_ = 1;
	Vec2f position_;
	short angle_ = 0;
};
