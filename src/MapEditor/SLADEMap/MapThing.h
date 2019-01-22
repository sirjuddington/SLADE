#pragma once

#include "MapObject.h"

class MapThing : public MapObject
{
	friend class SLADEMap;

public:
	static const string PROP_X;
	static const string PROP_Y;
	static const string PROP_TYPE;
	static const string PROP_ANGLE;
	static const string PROP_FLAGS;

	MapThing(const Vec2f& pos = { 0, 0 }, short type = -1, short angle = 0, short flags = 0);
	MapThing(const Vec2f& pos, short type, ParseTreeNode* def);
	~MapThing() = default;

	double xPos() const { return position_.x; }
	double yPos() const { return position_.y; }
	Vec2f  position() const { return position_; }
	short  type() const { return type_; }
	short  angle() const { return angle_; }

	Vec2f getPoint(Point point) override;

	int    intProperty(const string& key) override;
	double floatProperty(const string& key) override;
	void   setIntProperty(const string& key, int value) override;
	void   setFloatProperty(const string& key, double value) override;

	void copy(MapObject* c) override;

	void move(Vec2f pos, bool modify = true);
	void setAngle(int angle, bool modify = true);
	void setAnglePoint(Vec2f point, bool modify = true);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(string& def) override;

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
