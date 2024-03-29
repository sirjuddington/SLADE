#pragma once

namespace slade
{
// 2D Vectors
template<typename T> using Vec2 = glm::vec<2, T>;
typedef Vec2<int>    Vec2i;
typedef Vec2<float>  Vec2f;
typedef Vec2<double> Vec2d;

// 2D 'Point' alias
template<typename T> using Point2 = glm::vec<2, T>;
typedef Point2<int>    Point2i;
typedef Point2<float>  Point2f;
typedef Point2<double> Point2d;

// 3D Vectors
template<typename T> using Vec3 = glm::vec<3, T>;
typedef Vec3<int>    Vec3i;
typedef Vec3<float>  Vec3f;
typedef Vec3<double> Vec3d;

// 3D 'Point' alias
template<typename T> using Point3 = glm::vec<3, T>;
typedef Point3<int>    Point3i;
typedef Point3<float>  Point3f;
typedef Point3<double> Point3d;
} // namespace slade
