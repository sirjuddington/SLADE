#pragma once

#include "Structs.h"

namespace MathStuff
{
constexpr double PI = 3.1415926535897932384;

double    clamp(double val, double min, double max);
int       floor(double val);
int       ceil(double val);
int       round(double val);
double    distance(Vec2f p1, Vec2f p2);
double    distance3d(Vec3f p1, Vec3f p2);
double    lineSide(Vec2f point, Seg2f line);
Vec2f closestPointOnLine(Vec2f point, Seg2f line);
double    distanceToLine(Vec2f point, Seg2f line);
double    distanceToLineFast(Vec2f point, Seg2f line);
bool      linesIntersect(Seg2f line1, Seg2f line2, Vec2f& out);
double    distanceRayLine(Vec2f ray_origin, Vec2f ray_dir, Vec2f seg1, Vec2f seg2);
double    angle2DRad(Vec2f p1, Vec2f p2, Vec2f p3);
Vec2f rotatePoint(Vec2f origin, Vec2f point, double angle);
Vec3f rotateVector3D(Vec3f vector, Vec3f axis, double angle);
double    degToRad(double angle);
double    radToDeg(double angle);
Vec2f vectorAngle(double angle_rad);
double    distanceRayPlane(Vec3f ray_origin, Vec3f ray_dir, Plane plane);
bool      boxLineIntersect(Rectf box, Seg2f line);
Plane   planeFromTriangle(Vec3f p1, Vec3f p2, Vec3f p3);

} // namespace MathStuff
