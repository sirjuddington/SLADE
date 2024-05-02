#pragma once

#include "Geometry/BBox.h"

namespace slade
{
namespace mapeditor
{
	class MapEditContext;
}
} // namespace slade

namespace slade::mapeditor
{
class ObjectEditGroup
{
public:
	struct Vertex
	{
		Vec2d      position;
		Vec2d      old_position;
		MapVertex* map_vertex;
		bool       ignored;
	};

	struct Line
	{
		Vertex*  v1;
		Vertex*  v2;
		MapLine* map_line;

		bool isExtra() const { return v1->ignored || v2->ignored; }
	};

	struct Thing
	{
		Vec2d     position;
		Vec2d     old_position;
		MapThing* map_thing;
		int       angle;
	};

	BBox   bbox() const { return bbox_; }
	double rotation() const { return rotation_; }

	void    addVertex(MapVertex* vertex, bool ignored = false);
	void    addConnectedLines();
	void    addThing(MapThing* thing);
	bool    hasLine(const MapLine* line) const;
	Vertex* findVertex(const MapVertex* vertex) const;
	void    clear();
	void    filterObjects(bool filter) const;
	void    resetPositions();
	bool    empty() const { return vertices_.empty() && things_.empty(); }
	bool    nearestLineEndpoints(const Vec2d& pos, double min, Vec2d& v1, Vec2d& v2) const;
	void    putMapVertices(vector<MapVertex*>& list) const;

	// Drawing
	void putVerticesToDraw(vector<Vec2d>& list) const;
	void putLinesToDraw(vector<Line>& list) const;
	void putThingsToDraw(vector<Thing>& list) const;

	// Modification
	void doMove(double xoff, double yoff);
	void doScale(double xoff, double yoff, bool left, bool top, bool right, bool bottom);
	void doRotate(const Vec2d& p1, const Vec2d& p2, bool lock45);
	void doAll(double xoff, double yoff, double xscale, double yscale, double rotation, bool mirror_x, bool mirror_y);
	void applyEdit() const;

private:
	vector<unique_ptr<Vertex>> vertices_;
	vector<Line>               lines_;
	vector<Thing>              things_;
	BBox                       bbox_;          // Current
	BBox                       old_bbox_;      // Before drag operation
	BBox                       original_bbox_; // From first init
	Vec2d                      offset_prev_ = { 0, 0 };
	double                     rotation_    = 0;
	bool                       mirrored_    = false;
};

#undef None

class ObjectEdit
{
public:
	enum class State
	{
		None,
		Move,
		Left,
		Right,
		Top,
		Bottom,
		TopLeft,
		BottomLeft,
		TopRight,
		BottomRight,
	};

	ObjectEdit(mapeditor::MapEditContext& context) : context_{ &context } {}

	ObjectEditGroup& group() { return group_; }
	State            state() const { return state_; }
	bool             rotating() const { return rotating_; }

	bool stateLeft(bool move) const
	{
		return state_ == State::Left || state_ == State::TopLeft || state_ == State::BottomLeft
			   || (move && state_ == State::Move);
	}

	bool stateTop(bool move) const
	{
		return state_ == State::Top || state_ == State::TopLeft || state_ == State::TopRight
			   || (move && state_ == State::Move);
	}

	bool stateRight(bool move) const
	{
		return state_ == State::Right || state_ == State::TopRight || state_ == State::BottomRight
			   || (move && state_ == State::Move);
	}

	bool stateBottom(bool move) const
	{
		return state_ == State::Bottom || state_ == State::BottomRight || state_ == State::BottomLeft
			   || (move && state_ == State::Move);
	}

	bool begin();
	void end(bool accept) const;
	void determineState();

private:
	mapeditor::MapEditContext* context_;
	ObjectEditGroup            group_;
	State                      state_    = State::None;
	bool                       rotating_ = false;
};
} // namespace slade::mapeditor
