
#ifndef __POLYGON_2D_H__
#define __POLYGON_2D_H__

struct gl_vertex_t
{
	float x, y, z;
	float tx, ty;
	gl_vertex_t(float x = 0.0f, float y = 0.0f, float z = 0.0f)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->tx = 0.0f;
		this->ty = 0.0f;
	}
};

struct gl_polygon_t
{
	gl_vertex_t*	vertices;
	unsigned		n_vertices;
	unsigned		vbo_offset;
	unsigned		vbo_index;

	gl_polygon_t() { vertices = NULL; n_vertices = 0; vbo_offset = 0; }
	~gl_polygon_t() { if (vertices) delete[] vertices; }
};

class GLTexture;
class MapSector;
class Polygon2D
{
private:
	// Polygon data
	vector<gl_polygon_t*>	subpolys;
	GLTexture*				texture;
	float					colour[4];

	int		vbo_update;

public:
	Polygon2D();
	~Polygon2D();

	GLTexture*	getTexture() { return texture; }
	float		colRed() { return colour[0]; }
	float		colGreen() { return colour[1]; }
	float		colBlue() { return colour[2]; }
	float		colAlpha() { return colour[3]; }

	void		setTexture(GLTexture* tex) { this->texture = tex; }
	void		setColour(float r, float g, float b, float a);
	bool		hasPolygon() { return !subpolys.empty(); }
	int			vboUpdate() { return vbo_update; }
	void		setZ(float z);
	void		setZ(plane_t plane);

	unsigned		nSubPolys() { return subpolys.size(); }
	void			addSubPoly();
	gl_polygon_t*	getSubPoly(unsigned index);
	void			removeSubPoly(unsigned index);
	void			clear();
	unsigned		totalVertices();

	bool	openSector(MapSector* sector);
	void	updateTextureCoords(double scale_x = 1, double scale_y = 1, double offset_x = 0, double offset_y = 0, double rotation = 0);

	unsigned	vboDataSize();
	unsigned	writeToVBO(unsigned offset, unsigned index);
	void		updateVBOData();

	void	render();
	void	renderWireframe();
	void	renderVBO(bool colour = true);
	void	renderWireframeVBO(bool colour = true);

	static void	setupVBOPointers();
};


class PolygonSplitter
{
friend class Polygon2D;
private:
	// Structs
	struct edge_t
	{
		int		v1, v2;
		bool	ok;
		bool	done;
		bool	inpoly;
		int		sister;
	};
	struct vertex_t
	{
		double 			x, y;
		vector<int>		edges_in;
		vector<int>		edges_out;
		bool			ok;
		double			distance;
		vertex_t(double x=0, double y=0) { this->x = x; this->y = y; ok = true; }
	};
	struct poly_outline_t
	{
		vector<int>	edges;
		bbox_t		bbox;
		bool		clockwise;
		bool		convex;
	};

	// Splitter data
	vector<vertex_t>		vertices;
	vector<edge_t>			edges;
	vector<int>				concave_edges;
	vector<poly_outline_t>	polygon_outlines;
	int						split_edges_start;
	bool					verbose;
	double					last_angle;

public:
	PolygonSplitter();
	~PolygonSplitter();

	void	clear();
	void	setVerbose(bool v) { verbose = v; }

	int		addVertex(double x, double y);
	int		addEdge(double x1, double y1, double x2, double y2);
	int		addEdge(int v1, int v2);

	int		findNextEdge(int edge, bool ignore_valid = true, bool only_convex = true, bool ignore_inpoly = false);
	void	flipEdge(int edge);

	void	detectConcavity();
	bool	detectUnclosed();

	bool	tracePolyOutline(int edge_start);
	bool	testTracePolyOutline(int edge_start);

	bool	splitFromEdge(int splitter_edge);
	bool	buildSubPoly(int edge_start, gl_polygon_t* poly);
	bool	doSplitting(Polygon2D* poly);

	// Testing
	void	openSector(MapSector* sector);
	void	testRender();
};

#endif//__POLYGON_2D_H__
