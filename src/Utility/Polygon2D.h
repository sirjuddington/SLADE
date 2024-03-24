#pragma once

namespace slade
{
class Polygon2D
{
public:
	struct Vertex
	{
		float x = 0.f, y = 0.f, z = 0.f;
		float tx = 0.f, ty = 0.f;
		Vertex(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x{ x }, y{ y }, z{ z } {}
	};

	struct SubPoly
	{
		vector<Vertex> vertices;
	};

	Polygon2D() = default;
	~Polygon2D() { clear(); }

	unsigned texture() const { return texture_; }
	float    colRed() const { return colour_[0]; }
	float    colGreen() const { return colour_[1]; }
	float    colBlue() const { return colour_[2]; }
	float    colAlpha() const { return colour_[3]; }

	void setTexture(unsigned tex) { texture_ = tex; }
	void setColour(float r, float g, float b, float a);
	bool hasPolygon() const { return !subpolys_.empty(); }
	int  vboUpdate() const { return vbo_update_; }
	void setZ(float z);
	void setZ(const Plane& plane);

	unsigned nSubPolys() const { return subpolys_.size(); }
	void     addSubPoly();
	SubPoly* subPoly(unsigned index);
	void     removeSubPoly(unsigned index);
	void     clear();
	unsigned totalVertices() const;

	bool openSector(MapSector* sector);
	void updateTextureCoords(
		double scale_x  = 1,
		double scale_y  = 1,
		double offset_x = 0,
		double offset_y = 0,
		double rotation = 0);

	unsigned vboDataSize() const;
	unsigned writeToVBO(unsigned offset);

	void render() const;
	void renderWireframe() const;
	void renderVBO(unsigned offset) const;
	void renderWireframeVBO(bool colour = true) const;

	static void setupVBOPointers();

private:
	// Polygon data
	vector<SubPoly> subpolys_;
	unsigned        texture_   = 0;
	float           colour_[4] = { 1.f, 1.f, 1.f, 1.f };

	int vbo_update_ = 2;
};


class PolygonSplitter
{
	friend class Polygon2D;

public:
	PolygonSplitter()  = default;
	~PolygonSplitter() = default;

	void clear();
	void setVerbose(bool v) { verbose_ = v; }

	int addVertex(double x, double y);
	int addEdge(double x1, double y1, double x2, double y2);
	int addEdge(int v1, int v2);

	int  findNextEdge(int edge, bool ignore_done = true, bool only_convex = true, bool ignore_inpoly = false);
	void flipEdge(int edge);

	void detectConcavity();
	bool detectUnclosed();

	bool tracePolyOutline(int edge_start);
	bool testTracePolyOutline(int edge_start);

	bool splitFromEdge(int splitter_edge);
	bool buildSubPoly(int edge_start, Polygon2D::SubPoly* poly);
	bool doSplitting(Polygon2D* poly);

	// Testing
	void openSector(MapSector* sector);
	void testRender() const;

private:
	// Structs
	struct Edge
	{
		int  v1, v2;
		bool ok;
		bool done;
		bool inpoly;
		int  sister;
	};
	struct Vertex
	{
		double      x, y;
		vector<int> edges_in;
		vector<int> edges_out;
		bool        ok       = true;
		double      distance = 0;
		Vertex(double x = 0, double y = 0) : x{ x }, y{ y } {}

		operator Vec2d() const { return { x, y }; }
	};
	struct Outline
	{
		vector<int> edges;
		BBox        bbox;
		bool        clockwise;
		bool        convex;
	};

	// Splitter data
	vector<Vertex>  vertices_;
	vector<Edge>    edges_;
	vector<int>     concave_edges_;
	vector<Outline> polygon_outlines_;
	int             split_edges_start_ = 0;
	bool            verbose_           = false;
	double          last_angle_        = 0.;
};
} // namespace slade
