
#include "Main.h"
#include "LineSlopeThing.h"
#include "Geometry/Geometry.h"
#include "SLADEMap/MapObject/MapLine.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/Types.h"

using namespace slade;


LineSlopeThing::LineSlopeThing(SectorSurfaceType surface_type) : SlopeSpecialThing{ Type::Line, surface_type } {}

void LineSlopeThing::apply()
{
	bool        floor        = surface_type == SectorSurfaceType::Floor;
	const auto& target_plane = floor ? target->plane<SectorSurfaceType::Floor>()
									 : target->plane<SectorSurfaceType::Ceiling>();

	// Need to know the containing sector's height to find the thing's true height
	auto thingz = floor ? containing_sector->plane<SectorSurfaceType::Floor>().heightAt(thing->position())
						: containing_sector->plane<SectorSurfaceType::Ceiling>().heightAt(thing->position());
	thingz += thing->zPos();

	// Three points: endpoints of the line, and the thing itself
	Vec3d p1(line->x1(), line->y1(), target_plane.heightAt(line->start()));
	Vec3d p2(line->x2(), line->y2(), target_plane.heightAt(line->end()));
	Vec3d p3(thing->xPos(), thing->yPos(), thingz);

	if (floor)
		target->setFloorPlane(geometry::planeFromTriangle(p1, p2, p3));
	else
		target->setCeilingPlane(geometry::planeFromTriangle(p1, p2, p3));
}
