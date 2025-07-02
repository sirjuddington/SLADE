
#include "Main.h"
#include "PlaneAlignSpecial.h"
#include "Geometry/Geometry.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapVertex.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// PlaneAlignSpecial Struct Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// PlaneAlignSpecial constructor
// -----------------------------------------------------------------------------
PlaneAlignSpecial::PlaneAlignSpecial(const MapLine& line, SectorSurfaceType surface_type, int where) :
	SlopeSpecial{ surface_type },
	line{ &line }
{
	if (where == 1)
	{
		target = line.frontSector();
		model  = line.backSector();
	}
	else if (where == 2)
	{
		target = line.backSector();
		model  = line.frontSector();
	}
	else
		log::warning(
			"Invalid Plane_Align special on line {}: arg{} must be 1 (front) or 2 (back)",
			line.index(),
			surface_type == SectorSurfaceType::Floor ? 0 : 1);
}

void PlaneAlignSpecial::apply()
{
	bool floor = surface_type == SectorSurfaceType::Floor;

	vector<MapVertex*> vertices;
	target->putVertices(vertices);

	Vec2d mid    = line->getPoint(MapObject::Point::Mid);
	Vec2d v1_pos = glm::normalize(line->start() - mid);
	Vec2d v2_pos = glm::normalize(line->end() - mid);

	// Extend the line to the sector boundaries
	double max_dot_1 = 0.0;
	double max_dot_2 = 0.0;
	for (auto& vertex : vertices)
	{
		Vec2d vert = vertex->position() - mid;

		double dot = glm::dot(vert, v1_pos);

		double& max_dot = dot > 0 ? max_dot_1 : max_dot_2;

		dot     = std::fabs(dot);
		max_dot = std::max(dot, max_dot);
	}

	v1_pos = v1_pos * max_dot_1 + mid;
	v2_pos = v2_pos * max_dot_2 + mid;

	// The slope is between the line with Plane_Align, and the point in the
	// sector furthest away from it, which can only be at a vertex
	double     furthest_dist   = 0.0;
	MapVertex* furthest_vertex = nullptr;
	Seg2d      seg(v1_pos, v2_pos);
	for (auto& vertex : vertices)
	{
		double dist = geometry::distanceToLine(vertex->position(), seg);

		if (!geometry::colinear(vertex->xPos(), vertex->yPos(), v1_pos.x, v1_pos.y, v2_pos.x, v2_pos.y)
			&& dist > furthest_dist)
		{
			furthest_vertex = vertex;
			furthest_dist   = dist;
		}
	}

	if (!furthest_vertex || furthest_dist < 0.01)
	{
		log::warning(
			"Ignoring Plane_Align on line {}; sector {} has no appropriate reference vertex",
			line->index(),
			target->index());
		return;
	}

	// Calculate slope plane from our three points: this line's endpoints
	// (at the model sector's height) and the found vertex (at this sector's height).
	double modelz  = floor ? model->floor().height : model->ceiling().height;
	double targetz = floor ? target->floor().height : target->ceiling().height;

	Vec3d p1(v1_pos, modelz);
	Vec3d p2(v2_pos, modelz);
	Vec3d p3(furthest_vertex->position(), targetz);

	// Apply to sector floor/ceiling plane
	if (floor)
		target->setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
	else
		target->setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
}
