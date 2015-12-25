
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MathStuff.cpp
 * Description: Contains various useful math related functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "MathStuff.h"
#include "Console.h"
#include <cmath>


/*******************************************************************
 * MATHSTUFF NAMESPACE VARIABLES
 *******************************************************************/
namespace MathStuff
{
	double rad2deg = 180 / PI;
	double deg2rad = PI / 180;
}


/*******************************************************************
 * MATHSTUFF NAMESPACE FUNCTIONS
 *******************************************************************/

/* MathStuff::clamp
 * Clamps [val] to be between [min] and [max]
 *******************************************************************/
double MathStuff::clamp(double val, double min, double max)
{
	if (val < min) val = min;
	if (val > max) val = max;
	return val;
}

/* MathStuff::floor
 * Returns the integral floor of [val]
 *******************************************************************/
int MathStuff::floor(double val)
{
	if (val >= 0)
		return (int)val;
	else
		return (int)val - 1;
}

/* MathStuff::ceil
 * Returns the integral ceiling of [val]
 *******************************************************************/
int MathStuff::ceil(double val)
{
	if (val > 0)
		return (int)val + 1;
	else
		return (int)val;
}

/* MathStuff::round
 * Returns the closest integral value of [val]
 *******************************************************************/
int MathStuff::round(double val)
{
	int ret = (int)val;
	if ((val - (double)ret) >= 0.5) ret++;
	return ret;
}

/* MathStuff::distance
 * Returns the distance between [x1,y1] and [x2,y2]
 *******************************************************************/
double MathStuff::distance(double x1, double y1, double x2, double y2)
{
	return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

/* MathStuff::distance3d
 * Returns the distance between [x1,y1,z1] and [x2,y2,z2]
 *******************************************************************/
double MathStuff::distance3d(double x1, double y1, double z1, double x2, double y2, double z2)
{
	return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1));
}

/* MathStuff::lineSide
 * Returns the side of the line from [x1,y1] to [x2,y2] that the
 * point at [x,y] lies on. Positive is front, negative is back, zero
 * is on the line
 *******************************************************************/
double MathStuff::lineSide(double x, double y, double x1, double y1, double x2, double y2)
{
	return -((y-y1)*(x2-x1) - (x-x1)*(y2-y1));
}

/* MathStuff::closestPointOnLine
 * Returns the closest point to [x,y] along the line from [x1,y1] to
 * [x2,y2]
 *******************************************************************/
fpoint2_t MathStuff::closestPointOnLine(double x, double y, double x1, double y1, double x2, double y2)
{
	// Get line length
	double len = sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));

	// Calculate intersection distance
	double u = 0;
	if (len > 0)
	{
		u = ((x-x1)*(x2-x1) + (y-y1)*(y2-y1)) / (len*len);

		// Limit intersection distance to the line
		double lbound = 1 / len;
		if(u < lbound) u = lbound;
		if(u > (1.0-lbound)) u = 1.0-lbound;
	}

	// Return intersection point
	return fpoint2_t(x1 + u*(x2 - x1), y1 + u*(y2 - y1));
}

/* MathStuff::distanceToLine
 * Returns the shortest distance between the point at [x,y] and the
 * line from [x1,y1] to [x2,y2]
 *******************************************************************/
double MathStuff::distanceToLine(double x, double y, double x1, double y1, double x2, double y2)
{
	// Calculate intersection point
	fpoint2_t i = closestPointOnLine(x, y, x1, y1, x2, y2);

	// Return distance between intersection and point
	// which is the shortest distance to the line
	return sqrt((i.x-x)*(i.x-x) + (i.y-y)*(i.y-y));
}

/* MathStuff::distanceToLineFast
 * Returns the shortest 'distance' between the point at [x,y] and the
 * line from [x1,y1] to [x2,y2]. The distance returned isn't the
 * real distance, but can be used to find the 'closest' line to the
 * point
 *******************************************************************/
double MathStuff::distanceToLineFast(double x, double y, double x1, double y1, double x2, double y2)
{
	// Calculate intersection point
	fpoint2_t i = closestPointOnLine(x, y, x1, y1, x2, y2);

	// Return fast distance between intersection and point
	// which is the shortest distance to the line
	return (i.x-x)*(i.x-x) + (i.y-y)*(i.y-y);
}

/* MathStuff::linesIntersect
 * Checks for an intersection between two lines [l1x1,l1y1]-[l1x2,l1y2]
 * and [l2x1,l2y1]-[l2x2,l2y2]. Returns true if they intersect and
 * sets [x,y] to the intersection point
 *******************************************************************/
bool MathStuff::linesIntersect(double l1x1, double l1y1, double l1x2, double l1y2,
							   double l2x1, double l2y1, double l2x2, double l2y2,
							   double& x, double& y)
{
	// First, simple check for two parallel horizontal or vertical lines
	if ((l1x1 == l1x2 && l2x1 == l2x2) || (l1y1 == l1y2 && l2y1 == l2y2))
		return false;

	// Second, check if the lines share any endpoints
	if ((l1x1 == l2x1 && l1y1 == l2y1) ||
			(l1x2 == l2x2 && l1y2 == l2y2) ||
			(l1x1 == l2x2 && l1y1 == l2y2) ||
			(l1x2 == l2x1 && l1y2 == l2y1))
		return false;

	// Third, check bounding boxes
	if (max(l1x1, l1x2) < min(l2x1, l2x2) ||
			max(l2x1, l2x2) < min(l1x1, l1x2) ||
			max(l1y1, l1y2) < min(l2y1, l2y2) ||
			max(l2y1, l2y2) < min(l1y1, l1y2))
		return false;

	// Fourth, check for two perpendicular horizontal or vertical lines
	if (l1x1 == l1x2 && l2y1 == l2y2)
	{
		x = l1x1;
		y = l2y1;
		return true;
	}
	if (l1y1 == l1y2 && l2x1 == l2x2)
	{
		x = l2x1;
		y = l1y1;
		return true;
	}

	// Not a simple case, do full intersection calculation

	// Calculate some values
	double a1 = l1y2 - l1y1;
	double a2 = l2y2 - l2y1;
	double b1 = l1x1 - l1x2;
	double b2 = l2x1 - l2x2;
	double c1 = (a1 * l1x1) + (b1 * l1y1);
	double c2 = (a2 * l2x1) + (b2 * l2y1);
	double det = a1*b2 - a2*b1;

	// Check for no intersection
	if (det == 0)
		return false;

	// Calculate intersection point
	x = (b2*c1 - b1*c2) / det;
	y = (a1*c2 - a2*c1) / det;

	// Round to nearest 3 decimal places
	x = std::floor(x * 1000.0 + 0.5) / 1000.0;
	y = std::floor(y * 1000.0 + 0.5) / 1000.0;

	// Check that the intersection point is on both lines
	if (min(l1x1, l1x2) <= x && x <= max(l1x1, l1x2) &&
			min(l1y1, l1y2) <= y && y <= max(l1y1, l1y2) &&
			min(l2x1, l2x2) <= x && x <= max(l2x1, l2x2) &&
			min(l2y1, l2y2) <= y && y <= max(l2y1, l2y2))
		return true;

	// Intersection point does not lie on both lines
	return false;
}

/* MathStuff::distanceRayLine
 * Returns the distance between the ray [r1 -> r2] and the line
 * segment [x1,y1]-[x2,y2]
 *******************************************************************/
double MathStuff::distanceRayLine(fpoint2_t r1, fpoint2_t r2, double x1, double y1, double x2, double y2)
{
	// Calculate the intersection distance from the ray
	double u_ray = ((x2 - x1) * (r1.y - y1) - (y2 - y1) * (r1.x - x1)) /
				   ((y2 - y1) * (r2.x - r1.x) - (x2 - x1) * (r2.y - r1.y));

	// Calculate the intersection distance from the line
	double u_line = ((r2.x - r1.x) * (r1.y - y1) - (r2.y - r1.y) * (r1.x - x1)) /
					((y2 - y1) * (r2.x - r1.x) - (x2 - x1) * (r2.y - r1.y));

	// Return the distance on the ray if intersecting, or return -1
	if((u_ray >= 0)/* && (u_ray <= 1024) */&& (u_line >= 0) && (u_line <= 1)) return u_ray; else return -1;
}

/* MathStuff::angle2DRad
 * Returns the angle between the 2d points [p1], [p2] and [p3]
 *******************************************************************/
double MathStuff::angle2DRad(fpoint2_t p1, fpoint2_t p2, fpoint2_t p3)
{
	// From: http://stackoverflow.com/questions/3486172/angle-between-3-points
	// modified not to bother converting to degrees
	fpoint2_t ab(p2.x - p1.x, p2.y - p1.y);
	fpoint2_t cb(p2.x - p3.x, p2.y - p3.y);

	// dot product
	double dot = (ab.x * cb.x + ab.y * cb.y);

	// length square of both vectors
	double abSqr = ab.x * ab.x + ab.y * ab.y;
	double cbSqr = cb.x * cb.x + cb.y * cb.y;

	// square of cosine of the needed angle
	double cosSqr = dot * dot / abSqr / cbSqr;

	// this is a known trigonometric equality:
	// cos(alpha * 2) = [ cos(alpha) ]^2 * 2 - 1
	double cos2 = 2 * cosSqr - 1;

	// Here's the only invocation of the heavy function.
	// It's a good idea to check explicitly if cos2 is within [-1 .. 1] range
	double alpha2 =
		(cos2 <= -1) ? PI :
		(cos2 >= 1) ? 0 :
		acosf(cos2);

	//double rslt = alpha2 / 2;
	//double rs = rslt * rad2deg;
	double rs = alpha2 / 2;

	// Now revolve the ambiguities.
	// 1. If dot product of two vectors is negative - the angle is definitely
	// above 90 degrees. Still we have no information regarding the sign of the angle.

	// NOTE: This ambiguity is the consequence of our method: calculating the cosine
	// of the double angle. This allows us to get rid of calling sqrt.
	if (dot < 0)
		rs = PI - rs;

	// 2. Determine the sign. For this we'll use the Determinant of two vectors.
	double det = (ab.x * cb.y - ab.y * cb.x);
	if (det < 0)
		rs = (2*PI) - rs;

	return rs;
}

/* MathStuff::rotatePoint
 * Rotates [point] around [origin] by [angle] and returns the newly
 * rotated point
 *******************************************************************/
fpoint2_t MathStuff::rotatePoint(fpoint2_t origin, fpoint2_t point, double angle)
{
	// Translate to origin
	double x = point.x - origin.x;
	double y = point.y - origin.y;

	// Maths yay
	double srot = sin(angle * deg2rad);
	double crot = cos(angle * deg2rad);
	double nx = crot * x - srot * y;
	double ny = srot * x + crot * y;

	// Return rotated point
	return fpoint2_t(nx + origin.x, ny + origin.y);
}

/* MathStuff::distanceRayLine
 * Rotates [vector] around [axis] by [angle] and returns the
 * resulting rotated vector
 *******************************************************************/
fpoint3_t MathStuff::rotateVector3D(fpoint3_t vector, fpoint3_t axis, double angle)
{
	fpoint3_t rvec;

	// Calculate the sine and cosine of the angle once
	float crot = (float)cos(angle);
	float srot = (float)sin(angle);

	// Rotate x
	rvec.x  = (crot + (1 - crot) * axis.x * axis.x)				* vector.x;
	rvec.x += ((1 - crot) * axis.x * axis.y - axis.z * srot)	* vector.y;
	rvec.x += ((1 - crot) * axis.x * axis.z + axis.y * srot)	* vector.z;

	// Rotate y
	rvec.y  = ((1 - crot) * axis.x * axis.y + axis.z * srot)	* vector.x;
	rvec.y += (crot + (1 - crot) * axis.y * axis.y)				* vector.y;
	rvec.y += ((1 - crot) * axis.y * axis.z - axis.x * srot)	* vector.z;

	// Rotate z
	rvec.z  = ((1 - crot) * axis.x * axis.z - axis.y * srot)	* vector.x;
	rvec.z += ((1 - crot) * axis.y * axis.z + axis.x * srot)	* vector.y;
	rvec.z += (crot + (1 - crot) * axis.z * axis.z)				* vector.z;

	return rvec;
}

/* MathStuff::degToRad
 * Converts [angle] from degrees to radians
 *******************************************************************/
double MathStuff::degToRad(double angle)
{
	return angle * deg2rad;
}

/* MathStuff::radtoDeg
 * Converts [angle] from radians to degrees
 *******************************************************************/
double MathStuff::radToDeg(double angle)
{
	return angle * rad2deg;
}

/* MathStuff::vectorAngle
 * Converts [angle] from degrees to radians
 *******************************************************************/
fpoint2_t MathStuff::vectorAngle(double angle_rad)
{
	return fpoint2_t(cos(-angle_rad), -sin(-angle_rad));
}

/* MathStuff::distanceRayPlane
 * Returns the distance along the ray [r_o -> r_v] to [plane]
 *******************************************************************/
double MathStuff::distanceRayPlane(fpoint3_t r_o, fpoint3_t r_v, plane_t plane)
{
	fpoint3_t p_normal = plane.normal();
	double cos_a = r_v.dot(p_normal);

	// parallel to the plane (alpha=90)
	if (cos_a == 0)
		return -1;

	return ((plane.d - r_o.dot(p_normal)) / cos_a);
}

/* MathStuff::boxLineIntersect
 * Returns true if the box from [box_x1,box_y1] to [box_x2,box_y2]
 * intersects with the line from [line_x1,line_y1] to
 * [line_x2,line_y2]. Box values must be from min to max.
 * Taken from http://stackoverflow.com/a/100165
 *******************************************************************/
bool MathStuff::boxLineIntersect(double box_x1, double box_y1, double box_x2, double box_y2,
						double line_x1, double line_y1, double line_x2, double line_y2)
{
	// Find min and max X for the segment
	double minX = line_x1;
	double maxX = line_x2;
	if (line_x1 > line_x2)
	{
		minX = line_x2;
		maxX = line_x1;
	}

	// Find the intersection of the segment's and rectangle's x-projections
	if (maxX > box_x2)
		maxX = box_x2;
	if (minX < box_x1)
		minX = box_x1;

	// If their projections do not intersect return false
	if (minX > maxX)
		return false;

	// Find corresponding min and max Y for min and max X we found before
	double minY = line_y1;
	double maxY = line_y2;
	double dx = line_x2 - line_x1;

	if (fabs(dx) > 0.0000001)
	{
		double a = (line_y2 - line_y1) / dx;
		double b = line_y1 - a * line_x1;
		minY = a * minX + b;
		maxY = a * maxX + b;
	}
	if (minY > maxY)
	{
		double tmp = maxY;
		maxY = minY;
		minY = tmp;
	}

	// Find the intersection of the segment's and rectangle's y-projections
	if (maxY > box_y2)
		maxY = box_y2;
	if (minY < box_y1)
		minY = box_y1;

	// If Y-projections do not intersect return false
	if (minY > maxY)
		return false;

	return true;
}

/* MathStuff::planeFromTriangle
 * Calculates a plane from the given points [p1,p2,p3]
 *******************************************************************/
plane_t MathStuff::planeFromTriangle(fpoint3_t p1, fpoint3_t p2, fpoint3_t p3)
{
	fpoint3_t v1 = p3 - p1;
	fpoint3_t v2 = p2 - p1;
	v1 = v1.normalize();
	v2 = v2.normalize();
	fpoint3_t normal = v1.cross(v2);
	normal.set(normal.normalize());

	plane_t plane;
	plane.a = normal.x;
	plane.b = normal.y;
	plane.c = normal.z;
	plane.d = (normal.x * p1.x) + (normal.y * p1.y) + (normal.z * p1.z);

	return plane;
}



CONSOLE_COMMAND(angle2d, 6, false)
{
	double vals[6];
	for (unsigned a = 0; a < args.size(); a++)
	{
		args[a].ToDouble(&vals[a]);
	}

	double ang = MathStuff::angle2DRad(fpoint2_t(vals[0], vals[1]), fpoint2_t(vals[2], vals[3]), fpoint2_t(vals[4], vals[5]));
	wxLogMessage("Angle = %1.4f", ang);
}
