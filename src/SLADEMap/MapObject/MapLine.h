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

	static const wxString PROP_V1;
	static const wxString PROP_V2;
	static const wxString PROP_S1;
	static const wxString PROP_S2;
	static const wxString PROP_SPECIAL;
	static const wxString PROP_ID;
	static const wxString PROP_FLAGS;
	static const wxString PROP_ARG0;
	static const wxString PROP_ARG1;
	static const wxString PROP_ARG2;
	static const wxString PROP_ARG3;
	static const wxString PROP_ARG4;

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

	bool     boolProperty(const wxString& key) override;
	int      intProperty(const wxString& key) override;
	double   floatProperty(const wxString& key) override;
	wxString stringProperty(const wxString& key) override;
	void     setBoolProperty(const wxString& key, bool value) override;
	void     setIntProperty(const wxString& key, int value) override;
	void     setFloatProperty(const wxString& key, double value) override;
	void     setStringProperty(const wxString& key, const wxString& value) override;
	bool     scriptCanModifyProp(const wxString& key) override;

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

	Vec2f  getPoint(Point point) override;
	Vec2f  start() const;
	Vec2f  end() const;
	Seg2f  seg() const;
	double length();
	bool   doubleSector() const;
	Vec2f  frontVector();
	Vec2f  dirTabPoint(double tab_length = 0.);
	double distanceTo(Vec2f point);
	int    needsTexture() const;
	bool   overlaps(MapLine* other) const;
	bool   intersects(MapLine* other, Vec2f& intersect_point) const;

	void clearUnneededTextures() const;
	void resetInternals();
	void flip(bool sides = true);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;
	void copy(MapObject*) override;

	void writeUDMF(wxString& def) override;

	operator Debuggable() const
	{
		if (!this)
			return "<line NULL>";

		return { wxString::Format("<line %u>", index_) };
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
	Vec2f  front_vec_;
};
