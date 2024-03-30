#pragma once

#include "MapObject.h"

namespace slade
{
class Debuggable;
class VertexList;

class MapVertex : public MapObject
{
	friend class SLADEMap;
	friend class VertexList;

public:
	inline static const string PROP_X = "x";
	inline static const string PROP_Y = "y";

	MapVertex(const Vec2d& pos);
	MapVertex(const Vec2d& pos, const ParseTreeNode* udmf_def);
	~MapVertex() override = default;

	double xPos() const { return position_.x; }
	double yPos() const { return position_.y; }
	Vec2d  position() const { return position_; }

	Vec2d getPoint(Point point) override;

	void move(double nx, double ny);

	int    intProperty(string_view key) override;
	double floatProperty(string_view key) override;
	void   setIntProperty(string_view key, int value) override;
	void   setFloatProperty(string_view key, double value) override;
	bool   scriptCanModifyProp(string_view key) override;

	void     connectLine(MapLine* line);
	void     disconnectLine(const MapLine* line);
	unsigned nConnectedLines() const { return connected_lines_.size(); }
	MapLine* connectedLine(unsigned index) const;
	void     clearConnectedLines() { connected_lines_.clear(); }
	bool     isDetached() const { return connected_lines_.empty(); }

	const vector<MapLine*>& connectedLines() const { return connected_lines_; }

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(string& def) override;

	operator Debuggable() const;

private:
	// Basic data
	Vec2d position_;

	// Internal info
	vector<MapLine*> connected_lines_;
};
} // namespace slade
