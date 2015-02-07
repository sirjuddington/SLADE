
#ifndef __OBJECT_EDIT_H__
#define __OBJECT_EDIT_H__

class MapVertex;
class MapLine;
class MapThing;
class ObjectEditGroup
{
public:
	struct vertex_t
	{
		fpoint2_t	position;
		fpoint2_t	old_position;
		MapVertex*	map_vertex;
		bool		ignored;

		vertex_t() { map_vertex = NULL; ignored = false; }
	};

	struct line_t
	{
		vertex_t*	v1;
		vertex_t*	v2;
		MapLine*	map_line;

		bool	isExtra() { return v1->ignored || v2->ignored; }
	};

	struct thing_t
	{
		fpoint2_t	position;
		fpoint2_t	old_position;
		MapThing*	map_thing;
		int			angle;
	};

	ObjectEditGroup();
	~ObjectEditGroup();

	bbox_t	getBBox() { return bbox; }
	double	getRotation() { return rotation; }
	void	getVertices(vector<MapVertex*>& list);

	void		addVertex(MapVertex* v, bool ignored = false);
	void		addConnectedLines();
	void		addThing(MapThing* t);
	bool		hasLine(MapLine* l);
	vertex_t*	findVertex(MapVertex* v);
	void		clear();
	void		filterObjects(bool filter);
	void		resetPositions();
	bool		empty() { return vertices.empty() && things.empty(); }
	bool		getNearestLine(fpoint2_t pos, double min, fpoint2_t& v1, fpoint2_t& v2);

	// Drawing
	void	getVerticesToDraw(vector<fpoint2_t>& list);
	void	getLinesToDraw(vector<line_t>& list);
	void	getThingsToDraw(vector<thing_t>& list);

	// Modification
	void	doMove(double xoff, double yoff);
	void	doScale(double xoff, double yoff, bool left, bool top, bool right, bool bottom);
	void	doRotate(fpoint2_t p1, fpoint2_t p2, bool lock45);
	void	doAll(double xoff, double yoff, double xscale, double yscale, double rotation, bool mirror_x, bool mirror_y);
	void	applyEdit();

private:
	vector<vertex_t*>	vertices;
	vector<line_t>		lines;
	vector<thing_t>		things;
	bbox_t				bbox;			// Current
	bbox_t				old_bbox;		// Before drag operation
	bbox_t				original_bbox;	// From first init
	double				xoff_prev;
	double				yoff_prev;
	double				rotation;
	bool				mirrored;
};

#endif//__OBJECT_EDIT_H__
