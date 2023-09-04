#pragma once

#include "Structs.h"

namespace slade::math
{
constexpr double PI = 3.1415926535897932384626433832795;

double clamp(double val, double min, double max);
int    floor(double val);
int    ceil(double val);
int    round(double val);
double distance(Vec2d p1, Vec2d p2);
double distance3d(Vec3d p1, Vec3d p2);
double lineSide(Vec2d point, Seg2d line);
Vec2d  closestPointOnLine(Vec2d point, Seg2d line);
double distanceToLine(Vec2d point, Seg2d line);
double distanceToLineFast(Vec2d point, Seg2d line);
bool   linesIntersect(Seg2d line1, Seg2d line2, Vec2d& out);
double distanceRayLine(Vec2d ray_origin, Vec2d ray_dir, Vec2d seg1, Vec2d seg2);
double angle2DRad(Vec2d p1, Vec2d p2, Vec2d p3);
Vec2d  rotatePoint(Vec2d origin, Vec2d point, double angle);
Vec3d  rotateVector3D(Vec3d vector, Vec3d axis, double angle);
double degToRad(double angle);
double radToDeg(double angle);
Vec2d  vectorAngle(double angle_rad);
double distanceRayPlane(Vec3d ray_origin, Vec3d ray_dir, Plane plane);
bool   boxLineIntersect(Rectf box, Seg2d line);
Plane  planeFromTriangle(Vec3d p1, Vec3d p2, Vec3d p3);
bool   colinear(double x1, double y1, double x2, double y2, double x3, double y3);
Rectf  lineTab(const Rectf& line, float tab = 0.1f, float tab_max = 16.0f);

template<typename T> T scale(T value, double scale)
{
	return static_cast<T>(static_cast<double>(value) * scale);
}

template<typename T> T scaleInverse(T value, double scale)
{
	if (scale != 0.0)
		return static_cast<T>(static_cast<double>(value) / scale);

	return 0.0;
}

} // namespace slade::math
