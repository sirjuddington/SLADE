#pragma once

#include "MapObject.h"

class MapThing : public MapObject
{
	friend class SLADEMap;

public:
	static const wxString PROP_X;
	static const wxString PROP_Y;
	static const wxString PROP_Z;
	static const wxString PROP_TYPE;
	static const wxString PROP_ANGLE;
	static const wxString PROP_FLAGS;
	static const wxString PROP_ARG0;
	static const wxString PROP_ARG1;
	static const wxString PROP_ARG2;
	static const wxString PROP_ARG3;
	static const wxString PROP_ARG4;
	static const wxString PROP_ID;
	static const wxString PROP_SPECIAL;

	MapThing(
		const Vec3f&  pos     = { 0., 0., 0. },
		short         type    = -1,
		short         angle   = 0,
		short         flags   = 0,
		const ArgSet& args    = {},
		int           id      = 0,
		int           special = 0);
	MapThing(const Vec3f& pos, short type, ParseTreeNode* def);
	~MapThing() = default;

	double        xPos() const { return position_.x; }
	double        yPos() const { return position_.y; }
	double        zPos() const { return z_; }
	Vec2f         position() const { return position_; }
	double        height() const { return z_; }
	short         type() const { return type_; }
	short         angle() const { return angle_; }
	int           flags() const { return flags_; }
	bool          flagSet(int flag) const { return (flags_ & flag) != 0; }
	int           arg(unsigned index) const { return index < 5 ? args_[index] : 0; }
	const ArgSet& args() const { return args_; }
	int           id() const { return id_; }
	int           special() const { return special_; }

	Vec2f getPoint(Point point) override;

	int    intProperty(const wxString& key) override;
	double floatProperty(const wxString& key) override;
	void   setIntProperty(const wxString& key, int value) override;
	void   setFloatProperty(const wxString& key, double value) override;

	void copy(MapObject* c) override;

	void move(Vec2f pos, bool modify = true);
	void setZ(double z);
	void setType(int type);
	void setAngle(int angle, bool modify = true);
	void setAnglePoint(Vec2f point, bool modify = true);
	void setId(int id);
	void setFlags(int flags);
	void setFlag(int flag);
	void clearFlag(int flag);
	void setArg(unsigned index, int value);
	void setSpecial(int special);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(wxString& def) override;

	operator Debuggable() const
	{
		if (!this)
			return { "<thing NULL>" };

		return { wxString::Format("<thing %u>", index_) };
	}

private:
	// Basic data
	short  type_ = 1;
	Vec2f  position_;
	double z_       = 0.;
	short  angle_   = 0;
	int    flags_   = 0;
	ArgSet args_    = {};
	int    id_      = 0;
	int    special_ = 0;
};
