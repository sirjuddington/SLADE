#pragma once

#include "MapObject.h"

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

	static const string PROP_V1;
	static const string PROP_V2;
	static const string PROP_S1;
	static const string PROP_S2;
	static const string PROP_SPECIAL;
	static const string PROP_ID;
	static const string PROP_FLAGS;
	static const string PROP_ARG0;
	static const string PROP_ARG1;
	static const string PROP_ARG2;
	static const string PROP_ARG3;
	static const string PROP_ARG4;

	MapLine(
		MapVertex* v1,
		MapVertex* v2,
		MapSide*   s1      = nullptr,
		MapSide*   s2      = nullptr,
		int        special = 0,
		int        flags   = 0,
		ArgSet     args    = {});
	MapLine(MapVertex* v1, MapVertex* v2, MapSide* s1, MapSide* s2, ParseTreeNode* udmf_def);
	~MapLine() = default;

	bool isOk() const { return vertex1_ && vertex2_; }

	MapVertex*    v1() const { return vertex1_; }
	MapVertex*    v2() const { return vertex2_; }
	MapSide*      s1() const { return side1_; }
	MapSide*      s2() const { return side2_; }
	int           special() const { return special_; }
	int           id() const { return id_; }
	int           flags() const { return flags_; }
	bool          flagSet(int flag) const { return (flags_ & flag) != 0; }
	int           arg(unsigned index) const { return index < 5 ? args_[index] : 0; }
	const ArgSet& args() const { return args_; }

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

	bool   boolProperty(const string& key) override;
	int    intProperty(const string& key) override;
	double floatProperty(const string& key) override;
	string stringProperty(const string& key) override;
	void   setBoolProperty(const string& key, bool value) override;
	void   setIntProperty(const string& key, int value) override;
	void   setFloatProperty(const string& key, double value) override;
	void   setStringProperty(const string& key, const string& value) override;
	bool   scriptCanModifyProp(const string& key) override;

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
	double distanceTo(Vec2d point);
	int    needsTexture() const;
	bool   overlaps(MapLine* other) const;
	bool   intersects(MapLine* other, Vec2d& intersect_point) const;

	void clearUnneededTextures() const;
	void resetInternals();
	void flip(bool sides = true);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;
	void copy(MapObject*) override;

	void writeUDMF(string& def) override;

	operator Debuggable() const
	{
		if (!this)
			return "<line NULL>";

		return { S_FMT("<line %u>", index_) };
	}

private:
	// Basic data
	MapVertex* vertex1_ = nullptr;
	MapVertex* vertex2_ = nullptr;
	MapSide*   side1_   = nullptr;
	MapSide*   side2_   = nullptr;
	int        special_ = 0;
	int        id_      = 0;
	int        flags_   = 0;
	ArgSet     args_    = {};

	// Internally used info
	double length_ = -1.;
	double ca_     = 0.; // Used for intersection calculations
	double sa_     = 0.; // ^^
	Vec2d  front_vec_;
};
