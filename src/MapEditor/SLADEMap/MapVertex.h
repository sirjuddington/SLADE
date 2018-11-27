#pragma once

#include "MapObject.h"

class MapLine;

class MapVertex : public MapObject
{
	friend class SLADEMap;

public:
	struct DoomData
	{
		short x;
		short y;
	};

	// Those are actually fixed_t
	struct Doom64Data
	{
		int32_t x;
		int32_t y;
	};

	MapVertex(SLADEMap* parent = nullptr);
	MapVertex(double x, double y, SLADEMap* parent = nullptr);
	~MapVertex();

	double xPos() const { return x_; }
	double yPos() const { return y_; }

	fpoint2_t getPoint(Point point) override;
	fpoint2_t point();

	int    intProperty(const string& key) override;
	double floatProperty(const string& key) override;
	void   setIntProperty(const string& key, int value) override;
	void   setFloatProperty(const string& key, double value) override;
	bool   scriptCanModifyProp(const string& key) override;

	void     connectLine(MapLine* line);
	void     disconnectLine(MapLine* line);
	unsigned nConnectedLines() const { return connected_lines_.size(); }
	MapLine* connectedLine(unsigned index);

	const vector<MapLine*>& connectedLines() const { return connected_lines_; }

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	operator Debuggable() const
	{
		if (!this)
			return Debuggable("<vertex NULL>");

		return Debuggable(S_FMT("<vertex %u>", index_));
	}

private:
	// Basic data
	double x_;
	double y_;

	// Internal info
	vector<MapLine*> connected_lines_;
};
