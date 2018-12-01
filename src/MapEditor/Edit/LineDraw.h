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
	Vec2f                point(unsigned index);
	const vector<Vec2f>& points() const { return draw_points_; }

	void setState(State new_state) { state_current_ = new_state; }
	void setShapeOrigin(Vec2f point, bool nearest = false);

	bool addPoint(Vec2f point, bool nearest = false);
	void removePoint();
	void updateShape(Vec2f point);
	void begin(bool shape = false);
	void end(bool apply = true);

private:
	vector<Vec2f> draw_points_;
	Vec2f         draw_origin_;
	MapEditContext&   context_;
	State             state_current_;
};
