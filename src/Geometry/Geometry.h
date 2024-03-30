#pragma once

#include "RectFwd.h"

namespace slade
{
struct Plane;
}

namespace slade::geometry
{
double taxicabDistance(const Point2d& p1, const Point2d& p2);
double lineSide(const Vec2d& point, const Seg2d& line);
Vec2d  closestPointOnLine(const Vec2d& point, const Seg2d& line);
double distanceToLine(const Vec2d& point, const Seg2d& line);
double distanceToLineFast(const Vec2d& point, const Seg2d& line);
bool   linesIntersect(const Seg2d& line1, const Seg2d& line2, Vec2d& out);
double distanceRayLine(const Vec2d& ray_origin, const Vec2d& ray_dir, const Vec2d& seg1, const Vec2d& seg2);
double angle2DRad(const Vec2d& p1, const Vec2d& p2, const Vec2d& p3);
Vec2d  rotatePoint(const Vec2d& origin, const Vec2d& point, double angle);
Vec3d  rotateVector3D(const Vec3d& vector, const Vec3d& axis, double angle);
double degToRad(double angle);
double radToDeg(double angle);
Vec2d  vectorAngle(double angle_rad);
double distanceRayPlane(const Vec3d& ray_origin, const Vec3d& ray_dir, const Plane& plane);
bool   boxLineIntersect(const Rectf& box, const Seg2d& line);
Plane  planeFromTriangle(const Vec3d& p1, const Vec3d& p2, const Vec3d& p3);
bool   colinear(double x1, double y1, double x2, double y2, double x3, double y3);
} // namespace slade::geometry
