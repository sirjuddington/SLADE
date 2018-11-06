
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
#include "General/Console/Console.h"


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
 * Returns the distance between p1 and p2
 *******************************************************************/
double MathStuff::distance(fpoint2_t p1, fpoint2_t p2)
{
	return sqrt((p2.x-p1.x)*(p2.x-p1.x) + (p2.y-p1.y)*(p2.y-p1.y));
}

/* MathStuff::distance3d
 * Returns the distance between p1 and p2
 *******************************************************************/
double MathStuff::distance3d(fpoint3_t p1, fpoint3_t p2)
{
	return sqrt((p2.x-p1.x)*(p2.x-p1.x) + (p2.y-p1.y)*(p2.y-p1.y) + (p2.z-p1.z)*(p2.z-p1.z));
}

/* MathStuff::lineSide
 * Returns the side of the line that the point lies on.  Positive is
 * front, negative is back, zero is on the line
 *******************************************************************/
double MathStuff::lineSide(fpoint2_t point, fseg2_t line)
{
	return (point.x - line.x1()) * line.height() - (point.y - line.y1()) * line.width();
}

/* MathStuff::closestPointOnLine
 * Returns the point on the given line that's closest to the given
 * point
 *******************************************************************/
fpoint2_t MathStuff::closestPointOnLine(fpoint2_t point, fseg2_t line)
{
	// Get line length
	double len = line.length();

	// Calculate intersection distance
	double u = 0;
	if (len > 0)
	{
		u = ((point.x - line.x1()) * line.width() + (point.y-line.y1()) * line.height()) / (len * len);

		// Limit intersection distance to the line
		double lbound = 1 / len;
		if(u < lbound) u = lbound;
		if(u > (1.0-lbound)) u = 1.0-lbound;
	}

	// Return intersection point
	return fpoint2_t(line.x1() + u*line.width(), line.y1() + u*line.height());
}

/* MathStuff::distanceToLine
 * Returns the shortest distance between the point at [x,y] and the
 * line from [x1,y1] to [x2,y2]
 *******************************************************************/
double MathStuff::distanceToLine(fpoint2_t point, fseg2_t line)
{
	// Calculate intersection point
	fpoint2_t i = closestPointOnLine(point, line);

	// Return distance between intersection and point
	// which is the shortest distance to the line
	return MathStuff::distance(i, point);
}

/* MathStuff::distanceToLineFast
 * Returns the shortest 'distance' between the given point and line.
 * The distance returned isn't the real distance, but can be used to
 * find the 'closest' line to the point
 *******************************************************************/
double MathStuff::distanceToLineFast(fpoint2_t point, fseg2_t line)
{
	// Calculate intersection point
	fpoint2_t i = closestPointOnLine(point, line);

	// Return fast distance between intersection and point
	// which is the shortest distance to the line
	return (i.x-point.x)*(i.x-point.x) + (i.y-point.y)*(i.y-point.y);
}

/* MathStuff::linesIntersect
 * Checks for an intersection between two lines l1 and l2.  Returns
 * true if they intersect and sets out to the intersection point
 *******************************************************************/
bool MathStuff::linesIntersect(fseg2_t l1, fseg2_t l2, fpoint2_t& out)
{
	// First, simple check for two parallel horizontal or vertical lines
	if ((l1.x1() == l1.x2() && l2.x1() == l2.x2()) || (l1.y1() == l1.y2() && l2.y1() == l2.y2()))
		return false;

	// Second, check if the lines share any endpoints
	if ((l1.x1() == l2.x1() && l1.y1() == l2.y1()) ||
			(l1.x2() == l2.x2() && l1.y2() == l2.y2()) ||
			(l1.x1() == l2.x2() && l1.y1() == l2.y2()) ||
			(l1.x2() == l2.x1() && l1.y2() == l2.y1()))
		return false;

	// Third, check bounding boxes
	if (max(l1.x1(), l1.x2()) < min(l2.x1(), l2.x2()) ||
			max(l2.x1(), l2.x2()) < min(l1.x1(), l1.x2()) ||
			max(l1.y1(), l1.y2()) < min(l2.y1(), l2.y2()) ||
			max(l2.y1(), l2.y2()) < min(l1.y1(), l1.y2()))
		return false;

	// Fourth, check for two perpendicular horizontal or vertical lines
	if (l1.x1() == l1.x2() && l2.y1() == l2.y2())
	{
		out.x = l1.x1();
		out.y = l2.y1();
		return true;
	}
	if (l1.y1() == l1.y2() && l2.x1() == l2.x2())
	{
		out.x = l2.x1();
		out.y = l1.y1();
		return true;
	}

	// Not a simple case, do full intersection calculation

	// Calculate some values
	double a1 = l1.y2() - l1.y1();
	double a2 = l2.y2() - l2.y1();
	double b1 = l1.x1() - l1.x2();
	double b2 = l2.x1() - l2.x2();
	double c1 = (a1 * l1.x1()) + (b1 * l1.y1());
	double c2 = (a2 * l2.x1()) + (b2 * l2.y1());
	double det = a1*b2 - a2*b1;

	// Check for no intersection
	if (det == 0)
		return false;

	// Calculate intersection point
	out.x = (b2*c1 - b1*c2) / det;
	out.y = (a1*c2 - a2*c1) / det;

	// Round to nearest 3 decimal places
	out.x = std::floor(out.x * 1000.0 + 0.5) / 1000.0;
	out.y = std::floor(out.y * 1000.0 + 0.5) / 1000.0;

	// Check that the intersection point is on both lines
	if (min(l1.x1(), l1.x2()) <= out.x && out.x <= max(l1.x1(), l1.x2()) &&
		min(l1.y1(), l1.y2()) <= out.y && out.y <= max(l1.y1(), l1.y2()) &&
		min(l2.x1(), l2.x2()) <= out.x && out.x <= max(l2.x1(), l2.x2()) &&
		min(l2.y1(), l2.y2()) <= out.y && out.y <= max(l2.y1(), l2.y2()))
		return true;

	// Intersection point does not lie on both lines
	return false;
}

/* MathStuff::distanceRayLine
 * Returns the distance between the ray [r1 -> r2] and the line
 * segment [seg1 -> seg2]
 *******************************************************************/
double MathStuff::distanceRayLine(fpoint2_t r1, fpoint2_t r2, fpoint2_t s1, fpoint2_t s2)
{
	// Calculate the intersection distance from the ray
	double u_ray = ((s2.x - s1.x) * (r1.y - s1.y) - (s2.y - s1.y) * (r1.x - s1.x)) /
				   ((s2.y - s1.y) * (r2.x - r1.x) - (s2.x - s1.x) * (r2.y - r1.y));

	// Calculate the intersection distance from the line
	double u_line = ((r2.x - r1.x) * (r1.y - s1.y) - (r2.y - r1.y) * (r1.x - s1.x)) /
					((s2.y - s1.y) * (r2.x - r1.x) - (s2.x - s1.x) * (r2.y - r1.y));

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
bool MathStuff::boxLineIntersect(frect_t box, fseg2_t line)
{
	// Find min and max X for the segment
	double minX = line.x1();
	double maxX = line.x2();
	if (line.x1() > line.x2())
	{
		minX = line.x2();
		maxX = line.x1();
	}

	// Find the intersection of the segment's and rectangle's x-projections
	if (maxX > box.x2())
		maxX = box.x2();
	if (minX < box.x1())
		minX = box.x1();

	// If their projections do not intersect return false
	if (minX > maxX)
		return false;

	// Find corresponding min and max Y for min and max X we found before
	double minY = line.y1();
	double maxY = line.y2();
	double dx = line.x2() - line.x1();

	if (fabs(dx) > 0.0000001)
	{
		double a = (line.y2() - line.y1()) / dx;
		double b = line.y1() - a * line.x1();
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
	if (maxY > box.y2())
		maxY = box.y2();
	if (minY < box.y1())
		minY = box.y1();

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
	v1.normalize();
	v2.normalize();
	fpoint3_t normal = v1.cross(v2);
	normal.normalize();

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
	LOG_MESSAGE(1, "Angle = %1.4f", ang);
}
