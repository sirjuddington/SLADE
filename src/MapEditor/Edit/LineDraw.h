#pragma once

class MapEditContext;

class LineDraw
{
public:
	enum class State
	{
		Line,
		ShapeOrigin,
		ShapeEdge
	};

	explicit LineDraw(MapEditContext& context) : context_(context) {}

	State                    state() const { return state_current_; }
	unsigned                 nPoints() const { return draw_points_.size(); }
	fpoint2_t                point(unsigned index);
	const vector<fpoint2_t>& points() const { return draw_points_; }

	void setState(State new_state) { state_current_ = new_state; }
	void setShapeOrigin(fpoint2_t point, bool nearest = false);

	bool addPoint(fpoint2_t point, bool nearest = false);
	void removePoint();
	void updateShape(fpoint2_t point);
	void begin(bool shape = false);
	void end(bool apply = true);

private:
	vector<fpoint2_t> draw_points_;
	fpoint2_t         draw_origin_;
	MapEditContext&   context_;
	State             state_current_;
};
