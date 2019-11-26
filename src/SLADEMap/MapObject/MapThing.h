#pragma once

#include "MapObject.h"

class MapThing : public MapObject
{
	friend class SLADEMap;

public:
	// UDMF property names
	inline static const string PROP_X       = "x";
	inline static const string PROP_Y       = "y";
	inline static const string PROP_Z       = "height";
	inline static const string PROP_TYPE    = "type";
	inline static const string PROP_ANGLE   = "angle";
	inline static const string PROP_FLAGS   = "flags";
	inline static const string PROP_ARG0    = "arg0";
	inline static const string PROP_ARG1    = "arg1";
	inline static const string PROP_ARG2    = "arg2";
	inline static const string PROP_ARG3    = "arg3";
	inline static const string PROP_ARG4    = "arg4";
	inline static const string PROP_ID      = "id";
	inline static const string PROP_SPECIAL = "special";

	MapThing(
		const Vec3d&  pos     = { 0., 0., 0. },
		short         type    = -1,
		short         angle   = 0,
		short         flags   = 0,
		const ArgSet& args    = {},
		int           id      = 0,
		int           special = 0);
	MapThing(const Vec3d& pos, short type, ParseTreeNode* def);
	~MapThing() = default;

	double        xPos() const { return position_.x; }
	double        yPos() const { return position_.y; }
	double        zPos() const { return z_; }
	Vec2d         position() const { return position_; }
	double        height() const { return z_; }
	short         type() const { return type_; }
	short         angle() const { return angle_; }
	int           flags() const { return flags_; }
	bool          flagSet(int flag) const { return (flags_ & flag) != 0; }
	int           arg(unsigned index) const { return index < 5 ? args_[index] : 0; }
	const ArgSet& args() const { return args_; }
	int           id() const { return id_; }
	int           special() const { return special_; }

	Vec2d getPoint(Point point) override;

	int    intProperty(string_view key) override;
	double floatProperty(string_view key) override;
	void   setIntProperty(string_view key, int value) override;
	void   setFloatProperty(string_view key, double value) override;

	void copy(MapObject* c) override;

	void move(Vec2d pos, bool modify = true);
	void setZ(double z);
	void setType(int type);
	void setAngle(int angle, bool modify = true);
	void setAnglePoint(Vec2d point, bool modify = true);
	void setId(int id);
	void setFlags(int flags);
	void setFlag(int flag);
	void clearFlag(int flag);
	void setArg(unsigned index, int value);
	void setSpecial(int special);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(string& def) override;

	operator Debuggable() const
	{
		if (!this)
			return { "<thing NULL>" };

		return { fmt::format("<thing {}>", index_) };
	}

private:
	// Basic data
	short  type_ = 1;
	Vec2d  position_;
	double z_       = 0.;
	short  angle_   = 0;
	int    flags_   = 0;
	ArgSet args_    = {};
	int    id_      = 0;
	int    special_ = 0;
};
