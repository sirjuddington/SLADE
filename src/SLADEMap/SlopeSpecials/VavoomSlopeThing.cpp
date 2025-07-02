
#include "Main.h"
#include "VavoomSlopeThing.h"
#include "Geometry/Geometry.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/Types.h"

using namespace slade;

VavoomSlopeThing::VavoomSlopeThing(SectorSurfaceType surface_type) : SlopeSpecialThing{ Type::Vavoom, surface_type } {}

void VavoomSlopeThing::apply()
{
	bool floor = surface_type == SectorSurfaceType::Floor;

	// Vavoom things use the plane defined by the thing and its two
	// endpoints, based on the sector's original (flat) plane and treating
	// the thing's height as absolute
	short height = floor ? target->floor().height : target->ceiling().height;
	Vec3d p1(thing->xPos(), thing->yPos(), thing->zPos());
	Vec3d p2(line->x1(), line->y1(), height);
	Vec3d p3(line->x2(), line->y2(), height);

	if (floor)
		target->setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
	else
		target->setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
}
