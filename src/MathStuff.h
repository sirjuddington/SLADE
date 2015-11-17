
#ifndef __MATHSTUFF_H__
#define __MATHSTUFF_H__

#include "Structs.h"

#define PI	3.1415926535897932384

namespace MathStuff
{
	double		clamp(double val, double min, double max);
	int			floor(double val);
	int			ceil(double val);
	int			round(double val);
	double		distance(fpoint2_t p1, fpoint2_t p2);
	double		distance3d(fpoint3_t p1, fpoint3_t p2);
	double		lineSide(fpoint2_t point, fseg2_t line);
	fpoint2_t	closestPointOnLine(fpoint2_t point, fseg2_t line);
	double		distanceToLine(fpoint2_t point, fseg2_t line);
	double		distanceToLineFast(fpoint2_t point, fseg2_t line);
	bool		linesIntersect(fseg2_t line1, fseg2_t line2, fpoint2_t& out);
	double		distanceRayLine(fpoint2_t ray_origin, fpoint2_t ray_dir, fpoint2_t seg1, fpoint2_t seg2);
	double		angle2DRad(fpoint2_t p1, fpoint2_t p2, fpoint2_t p3);
	fpoint2_t	rotatePoint(fpoint2_t origin, fpoint2_t point, double angle);
	fpoint3_t	rotateVector3D(fpoint3_t vector, fpoint3_t axis, double angle);
	double		degToRad(double angle);
	double		radToDeg(double angle);
	fpoint2_t	vectorAngle(double angle_rad);
	double		distanceRayPlane(fpoint3_t ray_origin, fpoint3_t ray_dir, plane_t plane);
	bool		boxLineIntersect(frect_t box, fseg2_t line);
	plane_t		planeFromTriangle(fpoint3_t p1, fpoint3_t p2, fpoint3_t p3);
}

#endif//__MATHSTUFF_H__
