
#include "Main.h"
#include "SectorTiltThing.h"
#include "Geometry/Geometry.h"
#include "SLADEMap/MapObject/MapSector.h"
#include "SLADEMap/MapObject/MapThing.h"
#include "SLADEMap/Types.h"
#include "Utility/MathStuff.h"

using namespace slade;

SectorTiltThing::SectorTiltThing(SectorSurfaceType surface_type) : SlopeSpecialThing{ Type::SectorTilt, surface_type }
{
}

void SectorTiltThing::apply()
{
	constexpr double TAU = math::PI * 2; // Number of radians in the unit circle

	bool floor = surface_type == SectorSurfaceType::Floor;

	// First argument is the tilt angle, but starting with 0 as straight down;
	// subtracting 90 fixes that.
	int    raw_angle = thing->arg(0);
	double angle     = thing->angle() / 360.0 * TAU;
	double tilt      = (raw_angle - 90) / 360.0 * TAU;

	// Resulting plane goes through the position of the thing
	double z = (floor ? target->floor().height : target->ceiling().height) + thing->zPos();
	Vec3d  point(thing->xPos(), thing->yPos(), z);

	double cos_angle = cos(angle);
	double sin_angle = sin(angle);
	double cos_tilt  = cos(tilt);
	double sin_tilt  = sin(tilt);
	// Need to convert these angles into vectors on the plane, so we can take a
	// normal.
	// For the first: we know that the line perpendicular to the direction the
	// thing faces lies "flat", because this is the axis the tilt thing rotates
	// around.  "Rotate" the angle a quarter turn to get this vector -- switch
	// x and y, and negate one.
	Vec3d vec1(-sin_angle, cos_angle, 0.0);

	// For the second: the tilt angle makes a triangle between the floor plane
	// and the z axis.  sin gives us the distance along the z-axis, but cos
	// only gives us the distance away /from/ the z-axis.  Break that into x
	// and y by multiplying by cos and sin of the thing's facing angle.
	Vec3d vec2(cos_tilt * cos_angle, cos_tilt * sin_angle, sin_tilt);

	if (floor)
		target->setFloorPlane(geometry::planeFromTriangle(point, point + vec1, point + vec2));
	else
		target->setCeilingPlane(geometry::planeFromTriangle(point, point + vec1, point + vec2));
}
