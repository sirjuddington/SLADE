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

	explicit LineDraw(MapEditContext& context) : context(context) {}

	State						state() const { return state_current; }
	unsigned					nPoints() const { return draw_points.size(); }
	fpoint2_t					point(unsigned index);
	const vector<fpoint2_t>&	points() const { return draw_points; }

	void	setState(State new_state) { state_current = new_state; }
	void	setShapeOrigin(fpoint2_t point, bool nearest = false);

	bool	addPoint(fpoint2_t point, bool nearest = false);
	void	removePoint();
	void	updateShape(fpoint2_t point);
	void	begin(bool shape = false);
	void	end(bool apply = true);

private:
	vector<fpoint2_t>	draw_points;
	fpoint2_t			draw_origin;
	MapEditContext&		context;
	State				state_current;
};
