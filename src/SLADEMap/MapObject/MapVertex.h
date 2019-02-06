#pragma once

#include "MapObject.h"

class VertexList;

class MapVertex : public MapObject
{
	friend class SLADEMap;
	friend class VertexList;

public:
	typedef std::unique_ptr<MapVertex> UPtr;

	static const wxString PROP_X;
	static const wxString PROP_Y;

	MapVertex(const Vec2f& pos);
	MapVertex(const Vec2f& pos, ParseTreeNode* udmf_def);
	~MapVertex() = default;

	double xPos() const { return position_.x; }
	double yPos() const { return position_.y; }
	Vec2f  position() const { return position_; }

	Vec2f getPoint(Point point) override;

	void move(double nx, double ny);

	int    intProperty(const wxString& key) override;
	double floatProperty(const wxString& key) override;
	void   setIntProperty(const wxString& key, int value) override;
	void   setFloatProperty(const wxString& key, double value) override;
	bool   scriptCanModifyProp(const wxString& key) override;

	void     connectLine(MapLine* line);
	void     disconnectLine(MapLine* line);
	unsigned nConnectedLines() const { return connected_lines_.size(); }
	MapLine* connectedLine(unsigned index);
	void     clearConnectedLines() { connected_lines_.clear(); }
	bool     isDetached() const { return connected_lines_.empty(); }

	const vector<MapLine*>& connectedLines() const { return connected_lines_; }

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	void writeUDMF(wxString& def) override;

	operator Debuggable() const
	{
		if (!this)
			return { "<vertex NULL>" };

		return { S_FMT("<vertex %u>", index_) };
	}

private:
	// Basic data
	Vec2f position_;

	// Internal info
	vector<MapLine*> connected_lines_;
};
