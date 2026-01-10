
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Polygon.cpp
// Description: Sector polygon triangulation and related functions
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Polygon.h"
#include "Geometry/Geometry.h"
#include "MathStuff.h"
#include "OpenGL/GLTexture.h"
#include "OpenGL/VertexBuffer2D.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapSide.h"
#include "Vector.h"

using namespace slade;
using namespace polygon;


// -----------------------------------------------------------------------------
//
// Structs
//
// -----------------------------------------------------------------------------
namespace slade::polygon
{
struct VDist
{
	int    index;
	double distance;

	VDist(int index, double distance) : index{ index }, distance{ distance } {}
	bool operator<(const VDist& right) const { return distance < right.distance; }
};
} // namespace slade::polygon


// -----------------------------------------------------------------------------
//
// Triangulator Class
//
// -----------------------------------------------------------------------------
namespace slade::polygon
{
class Triangulator
{
public:
	Triangulator(bool verbose = false) : verbose_{ verbose } {}
	~Triangulator() = default;

	int addEdge(double x1, double y1, double x2, double y2)
	{
		// Add edge vertices
		int v1 = addVertex(x1, y1);
		int v2 = addVertex(x2, y2);

		// Add edge
		return addEdge(v1, v2);
	}

	vector<glm::vec2> triangulate();

private:
	struct Edge
	{
		int  v1, v2;
		bool ok     = true;
		bool done   = false;
		bool inpoly = false;
		int  sister = -1;

		Edge(int v1, int v2) : v1{ v1 }, v2{ v2 } {}
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

	vector<Vertex>  vertices_;
	vector<Edge>    edges_;
	vector<int>     concave_edges_;
	vector<Outline> polygon_outlines_;
	int             split_edges_start_ = 0;
	bool            verbose_           = false;
	double          last_angle_        = 0.;

	int  addVertex(double x, double y);
	int  addEdge(int v1, int v2);
	int  findNextEdge(int edge, bool ignore_done = true, bool only_convex = true, bool ignore_inpoly = false);
	bool tracePolyOutline(int edge_start);
	void detectConcavity();
	bool splitFromEdge(int splitter_edge);
	bool testTracePolyOutline(int edge_start);
	vector<glm::vec2> buildConvexPolygon(int edge_start);
};
} // namespace slade::polygon


// -----------------------------------------------------------------------------
//
// Triangulator Class Functions
//
// -----------------------------------------------------------------------------
namespace slade::polygon
{
int Triangulator::addVertex(double x, double y)
{
	// Check vertex doesn't exist
	for (unsigned a = 0; a < vertices_.size(); a++)
	{
		if (vertices_[a].x == x && vertices_[a].y == y)
			return a;
	}

	// Add vertex
	vertices_.emplace_back(x, y);
	vertices_.back().distance = 999999;
	return vertices_.size() - 1;
}

int Triangulator::addEdge(int v1, int v2)
{
	// Check for duplicate edge
	for (unsigned a = 0; a < edges_.size(); a++)
	{
		if (edges_[a].v1 == v1 && edges_[a].v2 == v2)
			return a;
	}

	// Add edge to list
	edges_.emplace_back(v1, v2);

	// Add edge to its vertices' edge lists (heh)
	int index = edges_.size() - 1;
	vertices_[v1].edges_out.push_back(index);
	vertices_[v2].edges_in.push_back(index);

	// Return edge index
	return index;
}

int Triangulator::findNextEdge(int edge, bool ignore_done, bool only_convex, bool ignore_inpoly)
{
	auto& e  = edges_[edge];
	auto& v2 = vertices_[e.v2];
	auto& v1 = vertices_[e.v1];

	// Go through all edges starting from the end of this one
	double min_angle = 2 * math::PI;
	int    next      = -1;
	for (unsigned a = 0; a < v2.edges_out.size(); a++)
	{
		auto& out = edges_[v2.edges_out[a]];

		// Ignore 'done' edges
		if (ignore_done && edges_[v2.edges_out[a]].done)
			continue;

		// Ignore 'inpoly' edges
		if (ignore_inpoly && edges_[v2.edges_out[a]].inpoly)
			continue;

		// Ignore edges on the reverse-side of this
		if (out.v1 == e.v2 && out.v2 == e.v1)
			continue;

		// Ignore invalid edges
		if (!out.ok)
			continue;

		// Determine angle between edges
		double angle = geometry::angle2DRad(
			Vec2d(v1.x, v1.y), Vec2d(v2.x, v2.y), Vec2d(vertices_[out.v2].x, vertices_[out.v2].y));
		if (angle < min_angle)
		{
			min_angle = angle;
			next      = v2.edges_out[a];
		}
	}

	last_angle_ = min_angle;
	return only_convex && min_angle > math::PI ? -1 : next;
}

bool Triangulator::tracePolyOutline(int edge_start)
{
	polygon_outlines_.emplace_back();
	auto& poly      = polygon_outlines_.back();
	poly.convex     = true;
	double edge_sum = 0;

	int edge = edge_start;
	int v1, v2, next;
	// while (true) {
	unsigned a = 0;
	for (a = 0; a < 100000; a++)
	{
		v1   = edges_[edge].v1;
		v2   = edges_[edge].v2;
		next = -1;

		// Add current edge
		poly.edges.push_back(edge);
		if (edge == edge_start)
			poly.bbox.extend(vertices_[v1].x, vertices_[v1].y);
		else
			edges_[edge].inpoly = true;
		poly.bbox.extend(vertices_[v2].x, vertices_[v2].y);
		edge_sum += vertices_[v1].x * vertices_[v2].y - vertices_[v2].x * vertices_[v1].y;

		// Find the next edge with the lowest angle
		next = findNextEdge(edge, true, false, true);

		// Abort if no next edge was found
		if (next < 0)
		{
			for (int ei : poly.edges)
				edges_[ei].inpoly = false;

			polygon_outlines_.pop_back();
			return false;
		}

		// Check for concavity
		if (last_angle_ > math::PI)
			poly.convex = false;

		// Stop if we're back at the start
		if (next == edge_start)
			break;

		// Continue loop
		edge = next;
	}

	if (a >= 99999)
	{
		if (verbose_)
			log::info("Possible infinite loop in tracePolyOutline");
		return false;
	}

	// Determine if this is an 'outer' (clockwise) or 'inner' (anti-clockwise) polygon
	poly.clockwise = (edge_sum < 0);

	// Set all polygon edges 'inpoly' to true (so they are ignored when tracing future polylines
	edges_[edge_start].inpoly = true;
	// for (unsigned a = 0; a < poly.edges.size(); a++)
	//	edges[poly.edges[a]].inpoly = true;

	if (verbose_)
	{
		string info = "Traced polygon outline: ";
		info += fmt::format("{} edges, ", poly.edges.size());
		if (poly.convex)
			info += "convex, ";
		else
			info += "concave, ";
		if (poly.clockwise)
			info += "clockwise";
		else
			info += "anticlockwise";
		log::info(info);
	}

	return true;
}

void Triangulator::detectConcavity()
{
	concave_edges_.clear();

	// Go through all edges
	for (unsigned a = 0; a < edges_.size(); a++)
	{
		if (!edges_[a].ok)
			continue;

		// Find the next edge with the lowest angle (ignore edges with angle > 180)
		int next = findNextEdge(a, false);
		if (next < 0)
		{
			// If no edge with an angle < 180 was found, this edge is concave
			concave_edges_.push_back(a);
		}
	}
}

bool Triangulator::splitFromEdge(int splitter_edge)
{
	// Get vertices
	int v1 = edges_[splitter_edge].v1;
	int v2 = edges_[splitter_edge].v2;

	// First up, find the closest vertex on the front side of the edge
	double min_dist = 999999;
	int    closest  = -1;
	for (unsigned a = 0; a < vertices_.size(); a++)
	{
		if (geometry::lineSide(vertices_[a], Seg2d(vertices_[v1], vertices_[v2])) > 0 && vertices_[a].ok)
		{
			vertices_[a].distance = glm::distance(static_cast<Vec2d>(vertices_[v2]), static_cast<Vec2d>(vertices_[a]));
			if (vertices_[a].distance < min_dist)
			{
				min_dist = vertices_[a].distance;
				closest  = a;
			}
		}
		else
			vertices_[a].distance = 999999;
	}

	// If there's nothing on the front side, something is wrong
	if (closest == -1)
		return false;

	// See if we can split to here without crossing anything
	// (this will be the case most of the time)
	bool  intersect = false;
	Vec2d pointi;
	for (auto& edge : edges_)
	{
		// Ignore edge if adjacent to the vertices we are looking at
		if (edge.v1 == closest || edge.v2 == closest || edge.v1 == v2 || edge.v2 == v2 || !edge.ok)
			continue;

		// Intersection test
		if (geometry::linesIntersect(
				Seg2d(vertices_[v2], vertices_[closest]), Seg2d(vertices_[edge.v1], vertices_[edge.v2]), pointi))
		{
			intersect = true;
			break;
		}
	}
	if (!intersect)
	{
		// No edge intersections, create split
		int e1            = addEdge(v2, closest);
		int e2            = addEdge(closest, v2);
		edges_[e1].sister = e2;
		edges_[e2].sister = e1;

		return true;
	}


	// Otherwise, we'll have to find the next closest vertex
	vector<VDist> sorted_verts;

	// Build a list of potential vertices, ordered by distance
	for (unsigned a = 0; a < vertices_.size(); a++)
	{
		if (vertices_[a].distance < 999999)
			sorted_verts.emplace_back(a, vertices_[a].distance);
	}

	// Go through potential split vertices, closest first
	std::sort(sorted_verts.begin(), sorted_verts.end());
	for (auto& sorted_vertex : sorted_verts)
	{
		int   index = sorted_vertex.index;
		auto& vert  = vertices_[index];

		// Check if a split from the edge to this vertex would cross any other edges
		intersect = false;
		for (auto& edge : edges_)
		{
			// Ignore edge if adjacent to the vertices we are looking at
			if (edge.v1 == index || edge.v2 == index || edge.v1 == v2 || edge.v2 == v2 || !edge.ok)
				continue;

			// Intersection test
			if (geometry::linesIntersect(
					Seg2d(vertices_[v2], vert), Seg2d(vertices_[edge.v1], vertices_[edge.v2]), pointi))
			{
				intersect = true;
				break;
			}
		}

		if (!intersect)
		{
			// No edge intersections, create split
			int e1            = addEdge(v2, index);
			int e2            = addEdge(index, v2);
			edges_[e1].sister = e2;
			edges_[e2].sister = e1;

			return true;
		}
	}

	// No split created
	return false;
}

bool Triangulator::testTracePolyOutline(int edge_start)
{
	int      edge = edge_start;
	int      v1, v2, next;
	unsigned a = 0;
	for (a = 0; a < 100000; a++)
	{
		v1 = edges_[edge].v1;
		v2 = edges_[edge].v2;

		// Find the next convex edge with the lowest angle
		next = findNextEdge(edge, false, true);

		// Abort if no next edge was found
		if (next < 0)
			return false;

		// Stop if we're back at the start
		if (next == edge_start)
			break;

		// Continue loop
		edge = next;
	}

	if (a >= 99999)
	{
		if (verbose_)
			log::info("Possible infinite loop in tracePolyOutline");
		return false;
	}

	return true;
}

vector<glm::vec2> Triangulator::buildConvexPolygon(int edge_start)
{
	// Loop of death
	int         edge = edge_start;
	int         v1   = edges_[edge].v1;
	int         v    = 0;
	vector<int> verts;
	for (unsigned a = 0; a < 1000; a++)
	{
		// Add vertex
		verts.push_back(edges_[edge].v1);

		// Fill triangle
		if (++v > 2)
		{
			verts.push_back(v1);
			verts.push_back(edges_[edge].v1);
		}

		// Add edge to 'valid' edges list, so it is ignored when building further polygons
		if (edge != edge_start)
			edges_[edge].done = true;

		// Get 'next' edge
		edge = findNextEdge(edge);

		// If no next edge is found, something is wrong, so abort building the polygon
		if (edge < 0)
			return {};

		// If we're back at the start, finish
		if (edge == edge_start)
			break;
	}

	// Remove last edge if there were multiple triangles
	if (v > 2)
	{
		verts.pop_back();
		verts.pop_back();
	}

	// Set starting edge to valid
	edges_[edge_start].done = true;

	// Check if the polygon is valid
	if (verts.size() >= 3)
	{
		// Allocate polygon vertex data
		vector<glm::vec2> vertices(verts.size());

		// Add vertex data to polygon
		for (unsigned a = 0; a < verts.size(); a++)
		{
			vertices[a].x = vertices_[verts[a]].x;
			vertices[a].y = vertices_[verts[a]].y;
		}

		return vertices;
	}

	return {};
}

vector<glm::vec2> Triangulator::triangulate()
{
	// Init
	split_edges_start_ = edges_.size();

	// Trace polygon outlines
	for (unsigned a = 0; a < edges_.size(); a++)
	{
		if (edges_[a].inpoly || !edges_[a].ok)
			continue;
		tracePolyOutline(a);
	}
	if (verbose_)
		log::info("{} Polygon outlines detected", polygon_outlines_.size());

	// Check if any edges are not part of a polygon outline
	for (auto& edge : edges_)
	{
		if (!edge.inpoly)
			edge.ok = false; // Invalidate it
	}

	// Let's check for some cases where we can 'throw away' edges/vertices from further consideration
	for (unsigned a = 0; a < polygon_outlines_.size(); a++)
	{
		// Check if this polygon intersects with any others
		bool separate = true;
		for (unsigned b = 0; b < polygon_outlines_.size(); b++)
		{
			if (b == a)
				continue;
			auto& bb1 = polygon_outlines_[a].bbox;
			auto& bb2 = polygon_outlines_[b].bbox;
			if (!(bb2.min.x > bb1.max.x || bb2.max.x < bb1.min.x || bb2.min.y > bb1.max.y || bb2.max.y < bb1.min.y))
			{
				separate = false;
				break;
			}
		}

		// If the polygon didn't intersect, and is convex and clockwise ('outer')
		if (separate && polygon_outlines_[a].clockwise && polygon_outlines_[a].convex)
		{
			if (verbose_)
				log::info("Separate, convex polygon exists, cutting (valid)");
			for (int edge : polygon_outlines_[a].edges)
			{
				// Set the edge to 'done' so it is ignored, but still used to build polygons
				edges_[edge].done = true;

				// If the edge's vertices aren't attached to anything else, also preclude these from later calculations
				int v1 = edges_[edge].v1;
				if (vertices_[v1].edges_in.size() == 1 && vertices_[v1].edges_out.size() == 1)
					vertices_[v1].ok = false;

				int v2 = edges_[edge].v2;
				if (vertices_[v2].edges_in.size() == 1 && vertices_[v2].edges_out.size() == 1)
					vertices_[v2].ok = false;
			}
		}

		// If the polygon didn't intersect, and is anticlockwise (inner), it is invalid
		else if (separate && !polygon_outlines_[a].clockwise)
		{
			if (verbose_)
				log::info("Separate, anticlockwise polygon exists, cutting (invalid)");
			for (int edge : polygon_outlines_[a].edges)
			{
				// Set the edge to 'done' so it is ignored, but still used to build polygons
				edges_[edge].ok = false;

				// If the edge's vertices aren't attached to anything else, also preclude these from later calculations
				int v1 = edges_[edge].v1;
				if (vertices_[v1].edges_in.size() == 1 && vertices_[v1].edges_out.size() == 1)
					vertices_[v1].ok = false;

				int v2 = edges_[edge].v2;
				if (vertices_[v2].edges_in.size() == 1 && vertices_[v2].edges_out.size() == 1)
					vertices_[v2].ok = false;
			}
		}
	}

	// Detect concave edges/vertices
	detectConcavity();

	// Keep splitting until we have no concave edges left
	// (we'll limit the number of rounds to 100 to avoid infinite loops, just in case)
	for (unsigned loop = 0; loop < 100; loop++)
	{
		for (int concave_edge : concave_edges_)
			splitFromEdge(concave_edge);

		detectConcavity();
		if (concave_edges_.empty())
			break;
	}

	// Remove unnecessary splits
	for (unsigned a = split_edges_start_; a < edges_.size(); a++)
	{
		if (!edges_[a].ok)
			continue;

		// Invalidate split
		edges_[a].ok                = false;
		edges_[edges_[a].sister].ok = false;

		// Check poly is still convex without split
		int next = findNextEdge(a, false, true);
		if (next >= 0)
		{
			if (testTracePolyOutline(next))
				continue;
		}

		// Not convex, split is needed
		edges_[a].ok                = true;
		edges_[edges_[a].sister].ok = true;
	}

	// Reset edge 'done' status
	for (auto& edge : edges_)
		edge.done = false;

	// Build polygons
	vector<glm::vec2> triangles;
	for (unsigned a = 0; a < edges_.size(); a++)
	{
		if (edges_[a].done || !edges_[a].ok)
			continue;

		auto poly_verts = buildConvexPolygon(a);
		if (!poly_verts.empty())
			vectorConcat(triangles, poly_verts);
	}

	return triangles;
}
} // namespace slade::polygon


// -----------------------------------------------------------------------------
//
// Polygon Namespace Functions
//
// -----------------------------------------------------------------------------
namespace slade::polygon
{
glm::vec2 calculateTexCoords(
	float x,
	float y,
	float tex_width,
	float tex_height,
	float scale_x,
	float scale_y,
	float offset_x,
	float offset_y,
	float rotation)
{
	// Check dimensions and scale
	if (scale_x == 0)
		scale_x = 1;
	if (scale_y == 0)
		scale_y = 1;
	if (tex_width == 0)
		tex_width = 1;
	if (tex_height == 0)
		tex_height = 1;

	// Get texture info
	auto owidth  = 1.0f / scale_x / tex_width;
	auto oheight = 1.0f / scale_y / tex_height;

	// Apply rotation if any
	if (rotation != 0)
	{
		auto np = geometry::rotatePoint(Vec2d(0, 0), Vec2d(x, y), rotation);
		x       = np.x;
		y       = np.y;
	}

	x = (scale_x * offset_x) + x;
	y = (scale_y * offset_y) - y;

	// Set texture coordinate for vertex
	return { x * owidth, y * oheight };
}

vector<glm::vec2> generateSectorTriangles(const MapSector& sector)
{
	Triangulator triangulator;

	// Get list of sides connected to this sector
	auto& sides = sector.connectedSides();

	// Go through sides
	for (auto& side : sides)
	{
		auto line = side->parentLine();

		// Ignore this side if its parent line has the same sector on both sides
		if (!line || line->doubleSector())
			continue;

		// Add the edge to the splitter (direction depends on what side of the line this is)
		if (line->s1() == side)
			triangulator.addEdge(line->x1(), line->y1(), line->x2(), line->y2());
		else
			triangulator.addEdge(line->x2(), line->y2(), line->x1(), line->y1());
	}

	// Split the sector polygon into triangles
	return triangulator.triangulate();
}

bool generateTextureCoords(
	vector<gl::Vertex2D>& vertices,
	unsigned              texture,
	float                 scale_x,
	float                 scale_y,
	float                 offset_x,
	float                 offset_y,
	float                 rotation)
{
	// Can't do this if there is no texture
	if (!texture)
		return false;

	// Get texture info
	auto& tex_info = gl::Texture::info(texture);
	auto  width    = static_cast<float>(tex_info.size.x);
	auto  height   = static_cast<float>(tex_info.size.y);

	// Calculate texture coords
	for (auto& v : vertices)
		v.tex_coord = calculateTexCoords(
			v.position.x, v.position.y, width, height, scale_x, scale_y, offset_x, offset_y, rotation);

	return true;
}
} // namespace slade::polygon
