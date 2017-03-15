
#include "Main.h"
#include "Polygon2D.h"
#include "OpenGL/GLTexture.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MathStuff.h"
#include "OpenGL/OpenGL.h"

Polygon2D::Polygon2D()
{
	vbo_update = 2;
	colour[0] = 1.0f;
	colour[1] = 1.0f;
	colour[2] = 1.0f;
	colour[3] = 1.0f;
}

Polygon2D::~Polygon2D()
{
	clear();
}

void Polygon2D::setColour(float r, float g, float b, float a)
{
	colour[0] = r;
	colour[1] = g;
	colour[2] = b;
	colour[3] = a;
}

void Polygon2D::setZ(float z)
{
	// Go through all sub-polys
	for (unsigned a = 0; a < subpolys.size(); a++)
	{
		// Set all vertex z values
		for (unsigned v = 0; v < subpolys[a]->n_vertices; v++)
			subpolys[a]->vertices[v].z = z;
	}
}

void Polygon2D::setZ(plane_t plane)
{
	// Go through all sub-polys
	for (unsigned a = 0; a < subpolys.size(); a++)
	{
		// Set all vertex z values
		for (unsigned v = 0; v < subpolys[a]->n_vertices; v++)
			subpolys[a]->vertices[v].z = plane.height_at(subpolys[a]->vertices[v].x, subpolys[a]->vertices[v].y);
	}
}

void Polygon2D::addSubPoly()
{
	subpolys.push_back(new gl_polygon_t());
	vbo_update = 2;
}

gl_polygon_t* Polygon2D::getSubPoly(unsigned index)
{
	if (index >= subpolys.size())
		return NULL;
	else
		return subpolys[index];
}

void Polygon2D::removeSubPoly(unsigned index)
{
	if (index >= subpolys.size())
		return;

	delete subpolys[index];
	subpolys.erase(subpolys.begin() + index);

	vbo_update = 2;
}

void Polygon2D::clear()
{
	for (unsigned a = 0; a < subpolys.size(); a++)
		delete subpolys[a];
	subpolys.clear();
	vbo_update = 2;
	texture = NULL;
}

unsigned Polygon2D::totalVertices()
{
	unsigned total = 0;
	for (unsigned a = 0; a < subpolys.size(); a++)
		total += subpolys[a]->n_vertices;
	return total;
}

bool Polygon2D::openSector(MapSector* sector)
{
	// Check sector was given
	if (!sector)
		return false;

	// Init
	PolygonSplitter splitter;
	clear();

	// Get list of sides connected to this sector
	vector<MapSide*>& sides = sector->connectedSides();

	// Go through sides
	MapLine* line;
	for (unsigned a = 0; a < sides.size(); a++)
	{
		line = sides[a]->getParentLine();

		// Ignore this side if its parent line has the same sector on both sides
		if (!line || line->doubleSector())
			continue;

		// Add the edge to the splitter (direction depends on what side of the line this is)
		if (line->s1() == sides[a])
			splitter.addEdge(line->v1()->xPos(), line->v1()->yPos(), line->v2()->xPos(), line->v2()->yPos());
		else
			splitter.addEdge(line->v2()->xPos(), line->v2()->yPos(), line->v1()->xPos(), line->v1()->yPos());
	}

	// Split the polygon into convex sub-polygons
	return splitter.doSplitting(this);
}

void Polygon2D::updateTextureCoords(double scale_x, double scale_y, double offset_x, double offset_y, double rotation)
{
	// Can't do this if there is no texture
	if (!texture)
		return;

	// Check dimensions and scale
	double width = texture->getWidth();
	double height = texture->getHeight();
	if (scale_x == 0) scale_x = 1;
	if (scale_y == 0) scale_y = 1;
	if (width == 0) width = 1;
	if (height == 0) height = 1;

	// Get texture info
	double owidth = 1.0 / scale_x / width;
	double oheight = 1.0 / scale_y / height;

	// Set texture coordinates
	double x, y;
	for (unsigned p = 0; p < subpolys.size(); p++)
	{
		for (unsigned a = 0; a < subpolys[p]->n_vertices; a++)
		{
			x = subpolys[p]->vertices[a].x;
			y = subpolys[p]->vertices[a].y;

			// Apply rotation if any
			if (rotation != 0) {
				fpoint2_t np = MathStuff::rotatePoint(fpoint2_t(0, 0), fpoint2_t(x, y), rotation);
				x = np.x;
				y = np.y;
			}

			x = (scale_x*offset_x) + x;
			y = (scale_y*offset_y) - y;

			// Set texture coordinate for vertex
			subpolys[p]->vertices[a].tx = x * owidth;
			subpolys[p]->vertices[a].ty = y * oheight;
		}
	}

	// Update variables
	vbo_update = 1;
}

unsigned Polygon2D::vboDataSize()
{
	unsigned total = 0;
	for (unsigned a = 0; a < subpolys.size(); a++)
		total += subpolys[a]->n_vertices * 20;
	return total;
}

unsigned Polygon2D::writeToVBO(unsigned offset, unsigned index)
{
	// Go through subpolys
	unsigned ofs = offset;
	unsigned i = index;
	for (unsigned a = 0; a < subpolys.size(); a++)
	{
		// Write subpoly data to VBO at the correct offset
		glBufferSubData(GL_ARRAY_BUFFER, ofs, subpolys[a]->n_vertices*20, subpolys[a]->vertices);

		// Update the subpoly vbo offset
		subpolys[a]->vbo_offset = ofs;
		subpolys[a]->vbo_index = i;
		ofs += subpolys[a]->n_vertices*20;
		i += subpolys[a]->n_vertices;
	}

	// Update variables
	vbo_update = 0;

	// Return the offset to the end of the data
	return ofs;
}

void Polygon2D::updateVBOData()
{
	// Go through subpolys
	for (unsigned a = 0; a < subpolys.size(); a++)
		glBufferSubData(GL_ARRAY_BUFFER, subpolys[a]->vbo_offset, subpolys[a]->n_vertices*20, subpolys[a]->vertices);

	// Update variables
	vbo_update = 0;
}

void Polygon2D::render()
{
	// Go through sub-polys
	for (unsigned a = 0; a < subpolys.size(); a++)
	{
		gl_polygon_t* poly = subpolys[a];
		glBegin(GL_TRIANGLE_FAN);
		for (unsigned v = 0; v < poly->n_vertices; v++)
		{
			glTexCoord2f(poly->vertices[v].tx, poly->vertices[v].ty);
			glVertex3d(poly->vertices[v].x, poly->vertices[v].y, poly->vertices[v].z);
		}
		glEnd();
	}
}

void Polygon2D::renderWireframe()
{
	// Go through sub-polys
	for (unsigned a = 0; a < subpolys.size(); a++)
	{
		gl_polygon_t* poly = subpolys[a];
		glBegin(GL_LINE_LOOP);
		for (unsigned v = 0; v < poly->n_vertices; v++)
		{
			glTexCoord2f(poly->vertices[v].tx, poly->vertices[v].ty);
			glVertex2d(poly->vertices[v].x, poly->vertices[v].y);
		}
		glEnd();
	}
}

void Polygon2D::renderVBO(bool colour)
{
	// Render
	//glColor4f(this->colour[0], this->colour[1], this->colour[2], this->colour[3]);
	for (unsigned a = 0; a < subpolys.size(); a++)
		glDrawArrays(GL_TRIANGLE_FAN, subpolys[a]->vbo_index, subpolys[a]->n_vertices);
}

void Polygon2D::renderWireframeVBO(bool colour)
{
}

void Polygon2D::setupVBOPointers()
{
	glVertexPointer(3, GL_FLOAT, 20, 0);
	glTexCoordPointer(2, GL_FLOAT, 20, ((char*)NULL + 12));
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}







PolygonSplitter::PolygonSplitter()
{
	verbose = false;
}

PolygonSplitter::~PolygonSplitter()
{
}

void PolygonSplitter::clear()
{
	vertices.clear();
	edges.clear();
	polygon_outlines.clear();
}

int PolygonSplitter::addVertex(double x, double y)
{
	// Check vertex doesn't exist
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (vertices[a].x == x && vertices[a].y == y)
			return a;
	}

	// Add vertex
	vertices.push_back(vertex_t(x, y));
	vertices.back().distance = 999999;
	return vertices.size() - 1;
}

int PolygonSplitter::addEdge(double x1, double y1, double x2, double y2)
{
	// Add edge vertices
	int v1 = addVertex(x1, y1);
	int v2 = addVertex(x2, y2);

	// Add edge
	return addEdge(v1, v2);
}

int PolygonSplitter::addEdge(int v1, int v2)
{
	// Check for duplicate edge
	for (unsigned a = 0; a < edges.size(); a++)
	{
		if (edges[a].v1 == v1 && edges[a].v2 == v2)
			return a;
	}

	// Create edge
	edge_t edge;
	edge.v1 = v1;
	edge.v2 = v2;
	edge.ok = true;
	edge.done = false;
	edge.inpoly = false;
	edge.sister = -1;

	// Add edge to list
	edges.push_back(edge);

	// Add edge to its vertices' edge lists (heh)
	int index = edges.size() - 1;
	vertices[v1].edges_out.push_back(index);
	vertices[v2].edges_in.push_back(index);

	// Return edge index
	return index;
}

int PolygonSplitter::findNextEdge(int edge, bool ignore_done, bool only_convex, bool ignore_inpoly)
{
	edge_t& e = edges[edge];
	vertex_t& v2 = vertices[e.v2];
	vertex_t& v1 = vertices[e.v1];

	// Go through all edges starting from the end of this one
	double min_angle = 2*PI;
	int next = -1;
	for (unsigned a = 0; a < v2.edges_out.size(); a++)
	{
		edge_t& out = edges[v2.edges_out[a]];

		// Ignore 'done' edges
		if (ignore_done && edges[v2.edges_out[a]].done)
			continue;

		// Ignore 'inpoly' edges
		if (ignore_inpoly && edges[v2.edges_out[a]].inpoly)
			continue;

		// Ignore edges on the reverse-side of this
		if (out.v1 == e.v2 && out.v2 == e.v1)
			continue;

		// Ignore invalid edges
		if (!out.ok)
			continue;

		// Determine angle between edges
		double angle = MathStuff::angle2DRad(fpoint2_t(v1.x, v1.y), fpoint2_t(v2.x, v2.y), fpoint2_t(vertices[out.v2].x, vertices[out.v2].y));
		if (angle < min_angle)
		{
			min_angle = angle;
			next = v2.edges_out[a];
		}
	}

	last_angle = min_angle;
	if (only_convex && min_angle > PI)
		return -1;
	else
		return next;
}

void PolygonSplitter::flipEdge(int edge)
{
	edge_t& e = edges[edge];
	vertex_t& v1 = vertices[e.v1];
	vertex_t& v2 = vertices[e.v2];

	// Remove the edge from its vertices' edge lists
	for (unsigned a = 0; a < v1.edges_out.size(); a++)
	{
		if (v1.edges_out[a] == edge)
		{
			v1.edges_out.erase(v1.edges_out.begin() + a);
			break;
		}
	}
	for (unsigned a = 0; a < v2.edges_in.size(); a++)
	{
		if (v2.edges_in[a] == edge)
		{
			v2.edges_in.erase(v2.edges_in.begin() + a);
			break;
		}
	}

	// Flip the edge
	int temp = e.v2;
	e.v2 = e.v1;
	e.v1 = temp;

	// Add the edge to its new vertices' edge lists
	v1.edges_in.push_back(edge);
	v2.edges_out.push_back(edge);
}

void PolygonSplitter::detectConcavity()
{
	concave_edges.clear();

	// Go through all edges
	for (unsigned a = 0; a < edges.size(); a++)
	{
		if (!edges[a].ok)
			continue;

		// Find the next edge with the lowest angle (ignore edges with angle > 180)
		int next = findNextEdge(a, false);
		if (next < 0)
		{
			// If no edge with an angle < 180 was found, this edge is concave
			concave_edges.push_back(a);
		}
	}
}

bool PolygonSplitter::detectUnclosed()
{
	vector<int> end_verts;
	vector<int> start_verts;

	// Go through all vertices
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		// If the vertex has no outgoing edges, we have an unclosed polygon
		if (vertices[a].edges_out.empty())
			end_verts.push_back(a);
		// Same if it has no incoming
		else if (vertices[a].edges_in.empty())
			start_verts.push_back(a);
	}

	// If there are no end/start vertices, the polygon is closed
	if (end_verts.empty() && start_verts.empty())
		return false;
	else if (verbose)
	{
		// Print invalid vertices info if verbose
		string info = "Vertices with no outgoing edges: ";
		for (unsigned a = 0; a < end_verts.size(); a++)
		{
			info += S_FMT("%1.2f", vertices[end_verts[a]].x);
			info += ",";
			info += S_FMT("%1.2f", vertices[end_verts[a]].y);
			info += " ";
		}
		wxLogMessage(info);
		info = "Vertices with no incoming edges: ";
		for (unsigned a = 0; a < start_verts.size(); a++)
		{
			info += S_FMT("%1.2f", vertices[start_verts[a]].x);
			info += ",";
			info += S_FMT("%1.2f", vertices[start_verts[a]].y);
			info += " ";
		}
		wxLogMessage(info);
	}

	// Check if any of this is caused by flipped edges
	for (unsigned a = 0; a < end_verts.size(); a++)
	{
		vertex_t& ev = vertices[end_verts[a]];

		// Check all the edges coming out of this vertex,
		// and see if any go into another 'unattacted' vertex
		for (unsigned e = 0; e < ev.edges_in.size(); e++)
		{
			edge_t& edge = edges[ev.edges_in[e]];

			bool flipped = false;
			for (unsigned b = 0; b < start_verts.size(); b++)
			{
				vertex_t& sv = vertices[start_verts[b]];

				if (edge.v1 == start_verts[b] && edge.v2 == end_verts[a])
					flipEdge(ev.edges_in[e]);	// Flip the edge
			}
		}
	}

	// Re-check vertices
	end_verts.clear();
	start_verts.clear();
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (!vertices[a].ok)
			continue;

		// If the vertex has no outgoing edges, we have an unclosed polygon
		if (vertices[a].edges_out.empty())
			end_verts.push_back(a);
		else if (vertices[a].edges_in.empty())
			start_verts.push_back(a);
	}

	// If there are no end/start vertices, the polygon is closed
	if (end_verts.empty() && start_verts.empty())
		return false;

	// If it still isn't closed, check for completely detached edges and 'remove' them
	for (unsigned a = 0; a < edges.size(); a++)
	{
		if (vertices[edges[a].v1].edges_in.empty() &&
		        vertices[edges[a].v2].edges_out.empty())
		{
			// Invalidate edge
			edges[a].ok = false;

			// Invalidate vertices
			vertices[edges[a].v1].ok = false;
			vertices[edges[a].v2].ok = false;
		}
	}

	// Re-check vertices
	end_verts.clear();
	start_verts.clear();
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (!vertices[a].ok)
			continue;

		// If the vertex has no outgoing edges, we have an unclosed polygon
		if (vertices[a].edges_out.empty())
			end_verts.push_back(a);
		else if (vertices[a].edges_in.empty())
			start_verts.push_back(a);
	}

	// If there are no end/start vertices, the polygon is closed
	if (end_verts.empty() && start_verts.empty())
		return false;

	// Not closed
	return true;
}

bool PolygonSplitter::tracePolyOutline(int edge_start)
{
	polygon_outlines.push_back(poly_outline_t());
	poly_outline_t& poly = polygon_outlines.back();
	poly.convex = true;
	double edge_sum = 0;

	int edge = edge_start;
	int v1, v2, next;
	//while (true) {
	unsigned a = 0;
	for (a = 0; a < 100000; a++)
	{
		v1 = edges[edge].v1;
		v2 = edges[edge].v2;
		next = -1;

		// Add current edge
		poly.edges.push_back(edge);
		if (edge == edge_start) poly.bbox.extend(vertices[v1].x, vertices[v1].y);
		else edges[edge].inpoly = true;
		poly.bbox.extend(vertices[v2].x, vertices[v2].y);
		edge_sum += vertices[v1].x*vertices[v2].y - vertices[v2].x*vertices[v1].y;

		// Find the next edge with the lowest angle
		next = findNextEdge(edge, true, false, true);

		// Abort if no next edge was found
		if (next < 0)
		{
			for (unsigned b = 0; b < poly.edges.size(); b++)
				edges[poly.edges[b]].inpoly = false;

			polygon_outlines.pop_back();
			return false;
		}

		// Check for concavity
		if (last_angle > PI)
			poly.convex = false;

		// Stop if we're back at the start
		if (next == edge_start)
			break;

		// Continue loop
		edge = next;
	}

	if (a >= 99999)
	{
		if (verbose) wxLogMessage("Possible infinite loop in tracePolyOutline");
		return false;
	}

	// Determine if this is an 'outer' (clockwise) or 'inner' (anti-clockwise) polygon
	poly.clockwise = (edge_sum < 0);

	// Set all polygon edges 'inpoly' to true (so they are ignored when tracing future polylines
	edges[edge_start].inpoly = true;
	//for (unsigned a = 0; a < poly.edges.size(); a++)
	//	edges[poly.edges[a]].inpoly = true;

	if (verbose)
	{
		string info = "Traced polygon outline: ";
		info += S_FMT("%lu edges, ", poly.edges.size());
		if (poly.convex) info += "convex, ";
		else info += "concave, ";
		if (poly.clockwise) info += "clockwise";
		else info += "anticlockwise";
		wxLogMessage(info);
	}

	return true;
}

bool PolygonSplitter::testTracePolyOutline(int edge_start)
{
	int edge = edge_start;
	int v1, v2, next;
	unsigned a = 0;
	for (a = 0; a < 100000; a++)
	{
		v1 = edges[edge].v1;
		v2 = edges[edge].v2;

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
		if (verbose) wxLogMessage("Possible infinite loop in tracePolyOutline");
		return false;
	}

	return true;
}


struct vdist_t
{
	int		index;
	double	distance;
	vdist_t(int index, double distance) { this->index = index; this->distance = distance; }
	bool operator<(const vdist_t& right) const { return distance < right.distance; }
};
bool PolygonSplitter::splitFromEdge(int splitter_edge)
{
	// Get vertices
	int v1 = edges[splitter_edge].v1;
	int v2 = edges[splitter_edge].v2;

	// First up, find the closest vertex on the front side of the edge
	double min_dist = 999999;
	int closest = -1;
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (MathStuff::lineSide(vertices[a], fseg2_t(vertices[v1], vertices[v2])) > 0 && vertices[a].ok)
		{
			vertices[a].distance = MathStuff::distance(vertices[v2], vertices[a]);
			if (vertices[a].distance < min_dist)
			{
				min_dist = vertices[a].distance;
				closest = a;
			}
		}
		else
			vertices[a].distance = 999999;
	}

	// If there's nothing on the front side, something is wrong
	if (closest == -1)
		return false;

	// See if we can split to here without crossing anything
	// (this will be the case most of the time)
	bool intersect = false;
	fpoint2_t pointi;
	for (unsigned a = 0; a < edges.size(); a++)
	{
		// Ignore edge if adjacent to the vertices we are looking at
		if (edges[a].v1 == closest || edges[a].v2 == closest || edges[a].v1 == v2 || edges[a].v2 == v2 || !edges[a].ok)
			continue;

		// Intersection test
		if (MathStuff::linesIntersect(fseg2_t(vertices[v2], vertices[closest]), fseg2_t(vertices[edges[a].v1], vertices[edges[a].v2]), pointi))
		{
			intersect = true;
			break;
		}
	}
	if (!intersect)
	{
		// No edge intersections, create split
		int e1 = addEdge(v2, closest);
		int e2 = addEdge(closest, v2);
		edges[e1].sister = e2;
		edges[e2].sister = e1;

		return true;
	}


	// Otherwise, we'll have to find the next closest vertex
	vector<vdist_t> sorted_verts;

	// Build a list of potential vertices, ordered by distance
	for (unsigned a = 0; a < vertices.size(); a++)
	{
		if (vertices[a].distance < 999999)
			sorted_verts.push_back(vdist_t(a, vertices[a].distance));
	}

	// Go through potential split vertices, closest first
	std::sort(sorted_verts.begin(), sorted_verts.end());
	for (unsigned a = 0; a < sorted_verts.size(); a++)
	{
		int index = sorted_verts[a].index;
		vertex_t& vert = vertices[index];

		// Check if a split from the edge to this vertex would cross any other edges
		intersect = false;
		for (unsigned a = 0; a < edges.size(); a++)
		{
			// Ignore edge if adjacent to the vertices we are looking at
			if (edges[a].v1 == index || edges[a].v2 == index || edges[a].v1 == v2 || edges[a].v2 == v2 || !edges[a].ok)
				continue;

			// Intersection test
			if (MathStuff::linesIntersect(fseg2_t(vertices[v2], vert), fseg2_t(vertices[edges[a].v1], vertices[edges[a].v2]), pointi))
			{
				intersect = true;
				break;
			}
		}

		if (!intersect)
		{
			// No edge intersections, create split
			int e1 = addEdge(v2, index);
			int e2 = addEdge(index, v2);
			edges[e1].sister = e2;
			edges[e2].sister = e1;

			return true;
		}
	}

	// No split created
	return false;
}

bool PolygonSplitter::buildSubPoly(int edge_start, gl_polygon_t* poly)
{
	// Check polygon was given
	if (!poly)
		return false;

	// Loop of death
	int edge = edge_start;
	//int v1 = edges[edge].v1;
	//int v = 0;
	vector<int> verts;
	for (unsigned a = 0; a < 1000; a++)
	{
		// Add vertex
		verts.push_back(edges[edge].v1);

		// Fill triangle
		// (doesn't seem to be any kind of performance increase using triangles over
		//	just rendering a GL_TRIANGLE_FAN polygon, not worth the memory usage increase)
		//v++;
		//if (v > 2) {
		//	verts.push_back(v1);
		//	verts.push_back(edges[edge].v1);
		//}

		// Add edge to 'valid' edges list, so it is ignored when building further polygons
		if (edge != edge_start) edges[edge].done = true;

		// Get 'next' edge
		edge = findNextEdge(edge);

		// If no next edge is found, something is wrong, so abort building the polygon
		if (edge < 0)
			return false;

		// If we're back at the start, finish
		if (edge == edge_start)
			break;
	}

	// Set starting edge to valid
	edges[edge_start].done = true;

	// Check if the polygon is valid
	if (verts.size() >= 3)
	{
		// Allocate polygon vertex data
		poly->n_vertices = verts.size();
		poly->vertices = new gl_vertex_t[poly->n_vertices];

		// Add vertex data to polygon
		for (unsigned a = 0; a < verts.size(); a++)
		{
			poly->vertices[a].x = vertices[verts[a]].x;
			poly->vertices[a].y = vertices[verts[a]].y;
		}

		return true;
	}
	else
		return false;
}

bool PolygonSplitter::doSplitting(Polygon2D* poly)
{
	// Init
	split_edges_start = edges.size();

	// Trace polygon outlines
	for (unsigned a = 0; a < edges.size(); a++)
	{
		if (edges[a].inpoly || !edges[a].ok)
			continue;
		tracePolyOutline(a);
	}
	if (verbose) wxLogMessage("%lu Polygon outlines detected", polygon_outlines.size());

	// Check if any edges are not part of a polygon outline
	for (unsigned a = 0; a < edges.size(); a++)
	{
		if (!edges[a].inpoly)
			edges[a].ok = false;	// Invalidate it
	}

	// Let's check for some cases where we can 'throw away' edges/vertices from further consideration
	for (unsigned a = 0; a < polygon_outlines.size(); a++)
	{
		// Check if this polygon intersects with any others
		bool separate = true;
		for (unsigned b = 0; b < polygon_outlines.size(); b++)
		{
			if (b == a) continue;
			bbox_t& bb1 = polygon_outlines[a].bbox;
			bbox_t& bb2 = polygon_outlines[b].bbox;
			if (!(bb2.min.x > bb1.max.x || bb2.max.x < bb1.min.x ||
			        bb2.min.y > bb1.max.y || bb2.max.y < bb1.min.y))
			{
				separate = false;
				break;
			}
		}

		// If the polygon didn't intersect, and is convex and clockwise ('outer')
		if (separate && polygon_outlines[a].clockwise && polygon_outlines[a].convex)
		{
			if (verbose) wxLogMessage("Separate, convex polygon exists, cutting (valid)");
			for (unsigned b = 0; b < polygon_outlines[a].edges.size(); b++)
			{
				// Set the edge to 'done' so it is ignored, but still used to build polygons
				edges[polygon_outlines[a].edges[b]].done = true;

				// If the edge's vertices aren't attached to anything else, also preclude these from later calculations
				int v1 = edges[polygon_outlines[a].edges[b]].v1;
				if (vertices[v1].edges_in.size() == 1 && vertices[v1].edges_out.size() == 1)
					vertices[v1].ok = false;

				int v2 = edges[polygon_outlines[a].edges[b]].v2;
				if (vertices[v2].edges_in.size() == 1 && vertices[v2].edges_out.size() == 1)
					vertices[v2].ok = false;
			}
		}

		// If the polygon didn't intersect, and is anticlockwise (inner), it is invalid
		else if (separate && !polygon_outlines[a].clockwise)
		{
			if (verbose) wxLogMessage("Separate, anticlockwise polygon exists, cutting (invalid)");
			for (unsigned b = 0; b < polygon_outlines[a].edges.size(); b++)
			{
				// Set the edge to 'done' so it is ignored, but still used to build polygons
				edges[polygon_outlines[a].edges[b]].ok = false;

				// If the edge's vertices aren't attached to anything else, also preclude these from later calculations
				int v1 = edges[polygon_outlines[a].edges[b]].v1;
				if (vertices[v1].edges_in.size() == 1 && vertices[v1].edges_out.size() == 1)
					vertices[v1].ok = false;

				int v2 = edges[polygon_outlines[a].edges[b]].v2;
				if (vertices[v2].edges_in.size() == 1 && vertices[v2].edges_out.size() == 1)
					vertices[v2].ok = false;
			}
		}
	}

	// Detect concave edges/vertices
	detectConcavity();

	// Keep splitting until we have no concave edges left
	// (we'll limit the number of rounds to 100 to avoid infinite loops, just in case)
	for (unsigned loop = 0; loop < 100; loop++)
	{
		for (unsigned a = 0; a < concave_edges.size(); a++)
			splitFromEdge(concave_edges[a]);

		detectConcavity();
		if (concave_edges.empty())
			break;
	}

	// Remove unnecessary splits
	for (unsigned a = split_edges_start; a < edges.size(); a++)
	{
		if (!edges[a].ok)
			continue;

		// Invalidate split
		edges[a].ok = false;
		edges[edges[a].sister].ok = false;

		// Check poly is still convex without split
		int next = findNextEdge(a, false, true);
		if (next >= 0)
		{
			if (testTracePolyOutline(next))
				continue;
		}

		// Not convex, split is needed
		edges[a].ok = true;
		edges[edges[a].sister].ok = true;
	}

	// Reset edge 'done' status
	for (unsigned a = 0; a < edges.size(); a++)
		edges[a].done = false;

	// Build polygons
	for (unsigned a = 0; a < edges.size(); a++)
	{
		if (edges[a].done || !edges[a].ok)
			continue;

		poly->addSubPoly();
		if (!buildSubPoly(a, poly->getSubPoly(poly->nSubPolys() - 1)))
			poly->removeSubPoly(poly->nSubPolys() - 1);
	}

	return true;
}

void PolygonSplitter::openSector(MapSector* sector)
{
	// Check sector was given
	if (!sector)
		return;

	// Init
	clear();

	// Get list of sides connected to this sector
	vector<MapSide*>& sides = sector->connectedSides();

	// Go through sides
	MapLine* line;
	for (unsigned a = 0; a < sides.size(); a++)
	{
		line = sides[a]->getParentLine();

		// Ignore this side if its parent line has the same sector on both sides
		if (!line || line->doubleSector())
			continue;

		// Add the edge to the splitter (direction depends on what side of the line this is)
		if (line->s1() == sides[a])
			addEdge(line->v1()->xPos(), line->v1()->yPos(), line->v2()->xPos(), line->v2()->yPos());
		else
			addEdge(line->v2()->xPos(), line->v2()->yPos(), line->v1()->xPos(), line->v1()->yPos());
	}
}

void PolygonSplitter::testRender()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Draw vertices
	OpenGL::setColour(255, 255, 255, 255, 0);
	glBegin(GL_POINTS);
	for (unsigned a = 0; a < vertices.size(); a++)
		glVertex2d(vertices[a].x, vertices[a].y);
	glEnd();

	// Draw original edges
	glColor3f(0.0f, 0.0f, 1.0f);
	glBegin(GL_LINES);
	for (int a = 0; a < split_edges_start; a++)
	{
		glVertex2d(vertices[edges[a].v1].x, vertices[edges[a].v1].y);
		glVertex2d(vertices[edges[a].v2].x, vertices[edges[a].v2].y);
	}
	glEnd();

	// Draw split edges
	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINES);
	for (unsigned a = split_edges_start; a < edges.size(); a++)
	{
		glVertex2d(vertices[edges[a].v1].x, vertices[edges[a].v1].y);
		glVertex2d(vertices[edges[a].v2].x, vertices[edges[a].v2].y);
	}
	glEnd();
}
