#pragma once

class MapEditContext;

class LineDraw
{
public:
	LineDraw(MapEditContext& context) : context(context) {}

	unsigned	nPoints() const { return draw_points.size(); }
	fpoint2_t	point(unsigned index);
	bool		addPoint(fpoint2_t point, bool nearest = false);
	void		removePoint();
	void		setShapeOrigin(fpoint2_t point, bool nearest = false);
	void		updateShape(fpoint2_t point);
	void		end(bool apply = true);

private:
	vector<fpoint2_t>	draw_points;
	fpoint2_t			draw_origin;
	MapEditContext&		context;
};
