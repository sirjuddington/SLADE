
#ifndef __MAP_RENDERER_2D__
#define __MAP_RENDERER_2D__

#include "SLADEMap.h"

class ThingType;
class GLTexture;
class ObjectEditGroup;
class MapRenderer2D
{
private:
	SLADEMap*	map;
	GLTexture*	tex_last;
	long		vertices_updated;
	long		lines_updated;
	long		flats_updated;

	// VBOs etc
	GLuint	vbo_vertices;
	GLuint	vbo_lines;
	GLuint	vbo_flats;

	// Display lists
	GLuint	list_vertices;
	GLuint	list_lines;

	// Visibility
	enum
	{
	    VIS_LEFT	= 1,
	    VIS_RIGHT	= 2,
	    VIS_ABOVE	= 4,
	    VIS_BELOW	= 8,
	    VIS_SMALL	= 16,
	};
	vector<uint8_t>	vis_v;
	vector<uint8_t>	vis_l;
	vector<uint8_t>	vis_t;
	vector<uint8_t>	vis_s;

	// Structs
	struct glvert_t
	{
		GLfloat x, y;
		GLfloat r, g, b, a;
	};
	struct glline_t
	{
		glvert_t v1, v2;	// The line itself
		glvert_t dv1, dv2;	// Direction tab
	};

	// Other
	bool	lines_dirs;
	int		n_vertices;
	int		n_lines;
	int		n_things;
	double	view_scale;
	double	view_scale_inv;
	bool	things_angles;

	vector<GLTexture*>	tex_flats;
	int					last_flat_type;

public:
	MapRenderer2D(SLADEMap* map);
	~MapRenderer2D();

	// Vertices
	bool	setupVertexRendering(float size_scale);
	void	renderVertices(float alpha = 1.0f);
	void	renderVerticesVBO();
	void	renderVerticesImmediate();
	void	renderVertexHilight(int index, float fade);
	void	renderVertexSelection(vector<int>& selection, float fade = 1.0f);

	// Lines
	rgba_t	lineColour(MapLine* line, bool ignore_filter = false);
	void	renderLines(bool show_direction, float alpha = 1.0f);
	void	renderLinesVBO(bool show_direction, float alpha);
	void	renderLinesImmediate(bool show_direction, float alpha);
	void	renderLineHilight(int index, float fade);
	void	renderLineSelection(vector<int>& selection, float fade = 1.0f);
	void	renderTaggedLines(vector<MapLine*>& lines, float fade);
	void	renderTaggingLines(vector<MapLine*>& lines, float fade);

	// Things
	bool	setupThingOverlay();
	void	renderThingOverlay(double x, double y, double radius, bool point);
	void	renderRoundThing(double x, double y, double angle, ThingType* type, float alpha = 1.0f);
	bool	renderSpriteThing(double x, double y, double angle, ThingType* type, float alpha = 1.0f, bool fitradius = false);
	void	renderSimpleSquareThing(double x, double y, double angle, ThingType* type, float alpha = 1.0f);
	bool	renderSquareThing(double x, double y, double angle, ThingType* type, float alpha = 1.0f, bool showicon = true, bool framed = false);
	void	renderThings(float alpha = 1.0f, bool force_dir = false);
	void	renderThingsImmediate(float alpha);
	void	renderThingHilight(int index, float fade);
	void	renderThingSelection(vector<int>& selection, float fade = 1.0f);
	void	renderTaggedThings(vector<MapThing*>& things, float fade);
	void	renderTaggingThings(vector<MapThing*>& things, float fade);
	void	renderPathedThings(vector<MapThing*>& things);

	// Flats (sectors)
	void	renderFlats(int type = 0, bool texture = true, float alpha = 1.0f);
	void	renderFlatsImmediate(int type, bool texture, float alpha);
	void	renderFlatsVBO(int type, bool texture, float alpha);
	void	renderFlatHilight(int index, float fade);
	void	renderFlatSelection(vector<int>& selection, float fade = 1.0f);
	void	renderTaggedFlats(vector<MapSector*>& sectors, float fade);

	// Moving
	void	renderMovingVertices(vector<int>& vertices, fpoint2_t move_vec);
	void	renderMovingLines(vector<int>& lines, fpoint2_t move_vec);
	void	renderMovingSectors(vector<int>& sectors, fpoint2_t move_vec);
	void	renderMovingThings(vector<int>& things, fpoint2_t move_vec);

	// Paste
	void	renderPasteThings(vector<MapThing*>& things, fpoint2_t pos);

	// Object Edit
	void	renderObjectEditGroup(ObjectEditGroup* group);


	// VBOs
	void	updateVerticesVBO();
	void	updateLinesVBO(bool show_direction, float alpha);
	void	updateFlatsVBO();

	// Misc
	void	setScale(double scale) { view_scale = scale; view_scale_inv = 1.0 / scale; }
	void	updateVisibility(fpoint2_t view_tl, fpoint2_t view_br);
	void	forceUpdate(float line_alpha = 1.0f);
	double	scaledRadius(int radius);
	bool	visOK();
	void	clearTextureCache() { tex_flats.clear(); }
};

#endif//__MAP_RENDERER_2D__
