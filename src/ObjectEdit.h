
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
	};

	ObjectEditGroup();
	~ObjectEditGroup();

	bbox_t	getBBox() { return bbox; }

	void		addVertex(MapVertex* v, bool ignored = false);
	void		addConnectedLines();
	void		addThing(MapThing* t);
	bool		hasLine(MapLine* l);
	vertex_t*	findVertex(MapVertex* v);
	void		clear();
	void		filterObjects(bool filter);
	void		resetPositions();

	// Drawing
	void	getVerticesToDraw(vector<fpoint2_t>& list);
	void	getLinesToDraw(vector<line_t>& list);
	void	getThingsToDraw(vector<thing_t>& list);

	// Modification
	void	doMove(double xoff, double yoff);
	void	doScale(double xoff, double yoff, bool left, bool top, bool right, bool bottom);
	void	doAll(double xoff, double yoff, double xscale, double yscale, double rotation);
	void	applyEdit();

private:
	vector<vertex_t>	vertices;
	vector<line_t>		lines;
	vector<thing_t>		things;
	bbox_t				bbox;			// Current
	bbox_t				old_bbox;		// Before drag operation
	bbox_t				original_bbox;	// From first init
	double				xoff_prev;
	double				yoff_prev;
};

#endif//__OBJECT_EDIT_H__
