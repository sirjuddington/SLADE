#pragma once

#include "Geometry/Rect.h"
#include "MapObject.h"
#include "SLADEMap/Types.h"

namespace slade
{
class Debuggable;

class MapLine : public MapObject
{
	friend class SLADEMap;

public:
	// Line parts
	enum Part
	{
		FrontMiddle = 0x01,
		FrontUpper  = 0x02,
		FrontLower  = 0x04,
		BackMiddle  = 0x08,
		BackUpper   = 0x10,
		BackLower   = 0x20,
	};

	inline static const string PROP_V1      = "v1";
	inline static const string PROP_V2      = "v2";
	inline static const string PROP_S1      = "sidefront";
	inline static const string PROP_S2      = "sideback";
	inline static const string PROP_SPECIAL = "special";
	inline static const string PROP_ID      = "id";
	inline static const string PROP_FLAGS   = "flags";
	inline static const string PROP_ARG0    = "arg0";
	inline static const string PROP_ARG1    = "arg1";
	inline static const string PROP_ARG2    = "arg2";
	inline static const string PROP_ARG3    = "arg3";
	inline static const string PROP_ARG4    = "arg4";

	MapLine(
		MapVertex*         v1,
		MapVertex*         v2,
		MapSide*           s1      = nullptr,
		MapSide*           s2      = nullptr,
		int                special = 0,
		int                flags   = 0,
		const map::ArgSet& args    = {});
	MapLine(MapVertex* v1, MapVertex* v2, MapSide* s1, MapSide* s2, const ParseTreeNode* udmf_def);
	~MapLine() override = default;

	bool isOk() const { return vertex1_ && vertex2_; }

	MapVertex*         v1() const { return vertex1_; }
	MapVertex*         v2() const { return vertex2_; }
	MapSide*           s1() const { return side1_; }
	MapSide*           s2() const { return side2_; }
	int                special() const { return special_; }
	int                id() const { return id_; }
	int                flags() const { return flags_; }
	bool               flagSet(int flag) const { return (flags_ & flag) != 0; }
	int                arg(unsigned index) const { return index < 5 ? args_[index] : 0; }
	const map::ArgSet& args() const { return args_; }

	MapSector* frontSector() const;
	MapSector* backSector() const;

	double x1() const;
	double y1() const;
	double x2() const;
	double y2() const;

	int v1Index() const;
	int v2Index() const;
	int s1Index() const;
	int s2Index() const;

	bool   hasProp(string_view key) const override;
	bool   boolProperty(string_view key) const override;
	int    intProperty(string_view key) const override;
	double floatProperty(string_view key) const override;
	string stringProperty(string_view key) const override;
	void   setBoolProperty(string_view key, bool value) override;
	void   setIntProperty(string_view key, int value) override;
	void   setFloatProperty(string_view key, double value) override;
	void   setStringProperty(string_view key, string_view value) override;
	bool   scriptCanModifyProp(string_view key) const override;

	void setS1(MapSide* side);
	void setS2(MapSide* side);
	void setV1(MapVertex* vertex);
	void setV2(MapVertex* vertex);
	void setSpecial(int special);
	void setId(int id);
	void setFlags(int flags);
	void setFlag(int flag);
	void clearFlag(int flag);
	void setArg(unsigned index, int value);

	Vec2d  getPoint(Point point) override;
	Vec2d  start() const;
	Vec2d  end() const;
	Seg2d  seg() const;
	double length();
	bool   doubleSector() const;
	Vec2d  frontVector();
	Vec2d  dirTabPoint(double tab_length = 0.);
	double distanceTo(const Vec2d& point);
	int    needsTexture() const;
	bool   overlaps(const MapLine* other) const;
	bool   intersects(const MapLine* other, Vec2d& intersect_point) const;

	int lowestCeiling() const;
	int highestCeiling() const;
	int lowestFloor() const;
	int highestFloor() const;

	void clearUnneededTextures() const;
	void resetInternals();
	void flip(bool sides = true);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;
	void copy(MapObject*) override;

	void writeUDMF(string& def) override;

	operator Debuggable() const;

private:
	// Basic data
	MapVertex*  vertex1_ = nullptr;
	MapVertex*  vertex2_ = nullptr;
	MapSide*    side1_   = nullptr;
	MapSide*    side2_   = nullptr;
	int         special_ = 0;
	int         id_      = 0;
	int         flags_   = 0;
	map::ArgSet args_    = {};

	// Internally used info
	double length_ = -1.;
	double ca_     = 0.; // Used for intersection calculations
	double sa_     = 0.; // ^^
	Vec2d  front_vec_;
};
} // namespace slade
