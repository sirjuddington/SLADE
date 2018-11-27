#pragma once

#include "Utility/Structs.h"

class MapEditContext;
class MapVertex;
class MapLine;
class MapThing;

class ObjectEditGroup
{
public:
	struct Vertex
	{
		fpoint2_t  position;
		fpoint2_t  old_position;
		MapVertex* map_vertex;
		bool       ignored;

		typedef std::unique_ptr<Vertex> UPtr;
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
		fpoint2_t position;
		fpoint2_t old_position;
		MapThing* map_thing;
		int       angle;
	};

	bbox_t bbox() const { return bbox_; }
	double rotation() const { return rotation_; }

	void    addVertex(MapVertex* vertex, bool ignored = false);
	void    addConnectedLines();
	void    addThing(MapThing* thing);
	bool    hasLine(MapLine* line);
	Vertex* findVertex(MapVertex* vertex);
	void    clear();
	void    filterObjects(bool filter);
	void    resetPositions();
	bool    empty() const { return vertices_.empty() && things_.empty(); }
	bool    getNearestLine(fpoint2_t pos, double min, fpoint2_t& v1, fpoint2_t& v2);
	void    getMapVertices(vector<MapVertex*>& list);

	// Drawing
	void getVerticesToDraw(vector<fpoint2_t>& list);
	void getLinesToDraw(vector<Line>& list);
	void getThingsToDraw(vector<Thing>& list);

	// Modification
	void doMove(double xoff, double yoff);
	void doScale(double xoff, double yoff, bool left, bool top, bool right, bool bottom);
	void doRotate(fpoint2_t p1, fpoint2_t p2, bool lock45);
	void doAll(double xoff, double yoff, double xscale, double yscale, double rotation, bool mirror_x, bool mirror_y);
	void applyEdit();

private:
	vector<Vertex::UPtr> vertices_;
	vector<Line>         lines_;
	vector<Thing>        things_;
	bbox_t               bbox_;          // Current
	bbox_t               old_bbox_;      // Before drag operation
	bbox_t               original_bbox_; // From first init
	fpoint2_t            offset_prev_ = { 0, 0 };
	double               rotation_    = 0;
	bool                 mirrored_    = false;
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

	ObjectEdit(MapEditContext& context) : context_{ context } {}

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
	void end(bool accept);
	void determineState();

private:
	MapEditContext& context_;
	ObjectEditGroup group_;
	State           state_    = State::None;
	bool            rotating_ = false;
};
