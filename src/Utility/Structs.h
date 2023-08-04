
#pragma once

#include <cmath>
using std::max;
using std::min;

// This is basically a collection of handy little structs I use a lot,
// with some useful functions for each of them

namespace slade
{
// 2D Vector/Point
template<typename T> struct Vec2
{
	T x, y;

	Vec2<T>() : x{}, y{} {}
	Vec2<T>(T X, T Y) : x{ X }, y{ Y } {}

	void set(T X, T Y)
	{
		x = X;
		y = Y;
	}
	void set(const Vec2<T>& v)
	{
		x = v.x;
		y = v.y;
	}

	Vec2<T> operator+(const Vec2<T>& v) const { return { v.x + x, v.y + y }; }
	Vec2<T> operator-(const Vec2<T>& v) const { return { x - v.x, y - v.y }; }
	Vec2<T> operator*(T num) const { return { x * num, y * num }; }
	Vec2<T> operator/(T num) const
	{
		if (num == 0)
			return Vec2<T>{};

		return Vec2<T>{ x / num, y / num };
	}
	bool     operator==(const Vec2<T>& rhs) const { return (x == rhs.x && y == rhs.y); }
	bool     operator!=(const Vec2<T>& rhs) const { return (x != rhs.x || y != rhs.y); }
	Vec2<T>& operator=(const Vec2<T>& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		return *this;
	}

	double magnitude() const { return std::sqrt((x * x) + (y * y)); }

	Vec2<T> normalized() const
	{
		auto mag = magnitude();
		if (mag == 0)
			return {};

		return { T(x / mag), T(y / mag) };
	}

	void normalize()
	{
		auto mag = magnitude();
		if (mag == 0)
		{
			x = 0;
			y = 0;
		}
		else
		{
			x /= mag;
			y /= mag;
		}
	}

	double distanceTo(const Vec2<T> other) const
	{
		T dist_x = other.x - x;
		T dist_y = other.y - y;

		return std::sqrt((dist_x * dist_x) + (dist_y * dist_y));
	}

	// aka "Manhattan" distance -- just the sum of the vertical and horizontal
	// distance, and an upper bound on the true distance
	T taxicabDistanceTo(const Vec2<T>& other) const
	{
		T dist;
		if (other.x < x)
			dist = x - other.x;
		else
			dist = other.x - x;
		if (other.y < y)
			dist += y - other.y;
		else
			dist += other.y - y;

		return dist;
	}

	T dot(const Vec2<T>& other) const { return x * other.x + y * other.y; }

	T cross(const Vec2<T>& other) const { return (x * other.y) - (y * other.x); }
};

typedef Vec2<int>    Vec2i;
typedef Vec2<float>  Vec2f;
typedef Vec2<double> Vec2d;

template<typename T> using Point2 = Vec2<T>;
typedef Point2<int>    Point2i;
typedef Point2<float>  Point2f;
typedef Point2<double> Point2d;


// 3D Vector/Point
template<typename T> struct Vec3
{
	T x, y, z;

	Vec3<T>() : x{}, y{}, z{} {}
	Vec3<T>(T x, T y, T z) : x{ x }, y{ y }, z{ z } {}
	Vec3<T>(const Vec2<T>& p, T z = {}) : x{ p.x }, y{ p.y }, z{ z } {}

	void set(T x, T y, T z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	void set(const Vec3<T>& p)
	{
		x = p.x;
		y = p.y;
		z = p.z;
	}

	Vec3<T>& operator=(const Vec3<T>& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}

	double magnitude() const { return sqrt((x * x) + (y * y) + (z * z)); }

	T dot(const Vec3<T>& vec) const { return x * vec.x + y * vec.y + z * vec.z; }

	Vec3<T> normalized() const
	{
		auto mag = magnitude();
		if (mag == 0)
			return {};
		else
			return { T(x / mag), T(y / mag), T(z / mag) };
	}

	void normalize()
	{
		double mag = magnitude();
		if (mag == 0)
			set(0, 0, 0);
		else
		{
			x /= mag;
			y /= mag;
			z /= mag;
		}
	}

	double distanceTo(const Vec3<T>& point) const
	{
		auto dist_x = point.x - x;
		auto dist_y = point.y - y;
		auto dist_z = point.z - z;

		return sqrt((dist_x * dist_x) + (dist_y * dist_y) + (dist_z * dist_z));
	}

	Vec3<T> operator+(const Vec3<T>& point) const { return { point.x + x, point.y + y, point.z + z }; }

	Vec3<T> operator-(const Vec3<T>& point) const { return { x - point.x, y - point.y, z - point.z }; }

	Vec3<T> operator*(T num) const { return { x * num, y * num, z * num }; }

	Vec3<T> operator/(T num) const
	{
		if (num == 0)
			return {};
		else
			return { x / num, y / num, z / num };
	}

	Vec3<T> cross(const Vec3<T>& p2) const
	{
		Vec3<T> cross_product;

		cross_product.x = ((y * p2.z) - (z * p2.y));
		cross_product.y = ((z * p2.x) - (x * p2.z));
		cross_product.z = ((x * p2.y) - (y * p2.x));

		return cross_product;
	}

	Vec2<T> get2d() const { return { x, y }; }
};
typedef Vec3<int>    Vec3i;
typedef Vec3<float>  Vec3f;
typedef Vec3<double> Vec3d;

template<typename T> using Point3 = Vec3<T>;
typedef Point3<int>    Point3i;
typedef Point3<float>  Point3f;
typedef Point3<double> Point3d;


// Rectangle
template<typename T> struct Rect
{
	Vec2<T> tl, br;

	// Constructors
	Rect() = default;
	Rect(const Vec2<T>& tl, const Vec2<T>& br) : tl{ tl }, br{ br } {}
	Rect(T x1, T y1, T x2, T y2) : tl{ x1, y1 }, br{ x2, y2 } {}
	Rect(T x, T y, T width, T height, bool center)
	{
		if (center)
		{
			tl.set(x - (width * 0.5), y - (height * 0.5));
			br.set(x + (width * 0.5), y + (height * 0.5));
		}
		else
		{
			tl.set(x, y);
			br.set(x + width, y + height);
		}
	}

	// Functions
	void set(const Vec2<T>& tl, const Vec2<T>& br)
	{
		this->tl = tl;
		this->br = br;
	}

	void set(T x1, T y1, T x2, T y2)
	{
		tl.set(x1, y1);
		br.set(x2, y2);
	}

	void set(const Rect<T>& rect)
	{
		tl.set(rect.tl);
		br.set(rect.br);
	}

	// TR/BL aliases that make more sense for line segments
	const Vec2<T>& start() const { return tl; }
	const Vec2<T>& end() const { return br; }

	T x1() const { return tl.x; }
	T y1() const { return tl.y; }
	T x2() const { return br.x; }
	T y2() const { return br.y; }

	T left() const { return std::min<T>(tl.x, br.x); }
	T top() const { return std::min<T>(tl.y, br.y); }
	T right() const { return std::max<T>(br.x, tl.x); }
	T bottom() const { return std::max<T>(br.y, tl.y); }

	T width() const { return br.x - tl.x; }
	T height() const { return br.y - tl.y; }

	T awidth() const { return std::max<T>(br.x, tl.x) - std::min<T>(tl.x, br.x); }
	T aheight() const { return std::max<T>(br.y, tl.y) - std::min<T>(tl.y, br.y); }

	Vec2<T> middle() const { return { left() + T(awidth() * 0.5), top() + T(aheight() * 0.5) }; }

	void expand(T x, T y)
	{
		if (tl.x < br.x)
		{
			tl.x -= x;
			br.x += x;
		}
		else
		{
			tl.x += x;
			br.x -= x;
		}

		if (tl.y < br.y)
		{
			tl.y -= y;
			br.y += y;
		}
		else
		{
			tl.y += y;
			br.y -= y;
		}
	}

	T length() const
	{
		T dist_x = br.x - tl.x;
		T dist_y = br.y - tl.y;

		return sqrt(dist_x * dist_x + dist_y * dist_y);
	}

	bool contains(Vec2<T> point) const
	{
		return (point.x >= left() && point.x <= right() && point.y >= top() && point.y <= bottom());
	}

	void move(T x_offset, T y_offset)
	{
		tl.x += x_offset;
		tl.y += y_offset;
		br.x += x_offset;
		br.y += y_offset;
	}
};

typedef Rect<int>    Recti;
typedef Rect<float>  Rectf;
typedef Rect<double> Rectd;

// Rectangle is not really any different from a 2D segment, but using it to
// mean that can be confusing, so here's an alias.
template<typename T> using Seg2 = Rect<T>;
typedef Seg2<int>    Seg2i;
typedef Seg2<double> Seg2d;
typedef Seg2<float>  Seg2f;


// Plane: A 3d plane
struct Plane
{
	double a, b, c, d;

	Plane() : a(0.0), b(0.0), c(0.0), d(0.0) {}
	Plane(double a, double b, double c, double d) : a(a), b(b), c(c), d(d) {}

	/** Construct a flat plane (perpendicular to the z axis) at the given height.
	 */
	static Plane flat(float height) { return { 0.0, 0.0, 1.0, height }; }

	bool operator==(const Plane& rhs) const { return a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d; }
	bool operator!=(const Plane& rhs) const { return !(*this == rhs); }

	void set(double a, double b, double c, double d)
	{
		this->a = a;
		this->b = b;
		this->c = c;
		this->d = d;
	}

	Vec3d normal() const
	{
		Vec3d norm(a, b, c);
		norm.normalize();
		return norm;
	}

	void normalize()
	{
		Vec3d  vec(a, b, c);
		double mag = vec.magnitude();
		a          = a / mag;
		b          = b / mag;
		c          = c / mag;
		d          = d / mag;
	}

	double heightAt(Vec2d point) const { return heightAt(point.x, point.y); }
	double heightAt(double x, double y) const { return ((-a * x) + (-b * y) + d) / c; }
};


// BBox: A simple bounding box with related functions
struct BBox
{
	Vec2d min;
	Vec2d max;

	BBox() { reset(); }
	BBox(const Vec2d& min, const Vec2d& max) : min{ min }, max{ max } {}
	BBox(double min_x, double min_y, double max_x, double max_y) : min{ min_x, min_y }, max{ max_x, max_y } {}

	void reset()
	{
		min.set(0, 0);
		max.set(0, 0);
	}

	void extend(double x, double y)
	{
		// Init bbox if it has been reset last
		if (min.x == 0 && min.y == 0 && max.x == 0 && max.y == 0)
		{
			min.set(x, y);
			max.set(x, y);
			return;
		}

		// Extend to fit the point [x,y]
		if (x < min.x)
			min.x = x;
		if (x > max.x)
			max.x = x;
		if (y < min.y)
			min.y = y;
		if (y > max.y)
			max.y = y;
	}

	void extend(const Vec2d& other) { extend(other.x, other.y); }

	void extend(const BBox& other)
	{
		if (other.min.x < min.x)
			min.x = other.min.x;
		if (other.min.y < min.y)
			min.y = other.min.y;

		if (other.max.x > max.x)
			max.x = other.max.x;
		if (other.max.y > max.y)
			max.y = other.max.y;
	}

	bool pointWithin(double x, double y) const { return (x >= min.x && x <= max.x && y >= min.y && y <= max.y); }
	bool contains(Vec2d point) const { return pointWithin(point.x, point.y); }
	bool isWithin(Vec2d bmin, Vec2d bmax) const
	{
		return (min.x >= bmin.x && max.x <= bmax.x && min.y >= bmin.y && max.y <= bmax.y);
	}

	bool isValid() const { return ((max.x - min.x > 0) && (max.y - min.y) > 0); }

	Vec2d  size() const { return { max.x - min.x, max.y - min.y }; }
	double width() const { return max.x - min.x; }
	double height() const { return max.y - min.y; }

	Vec2d  mid() const { return { midX(), midY() }; }
	double midX() const { return min.x + ((max.x - min.x) * 0.5); }
	double midY() const { return min.y + ((max.y - min.y) * 0.5); }

	Seg2d leftSide() const { return { min.x, min.y, min.x, max.y }; }
	Seg2d rightSide() const { return { max.x, min.y, max.x, max.y }; }
	Seg2d bottomSide() const { return { min.x, max.y, max.x, max.y }; }
	Seg2d topSide() const { return { min.x, min.y, max.x, min.y }; }
};

// Formerly key_value_t
typedef std::pair<string, string> StringPair;

// Simple templated struct for string+value pairs
template<typename T> struct Named
{
	string name;
	T      value;

	Named(string_view name, const T& value) : name{ name }, value{ value } {}

	// For sorting
	bool operator<(const Named<T>& rhs) { return name < rhs.name; }
	bool operator<=(const Named<T>& rhs) { return name <= rhs.name; }
	bool operator>(const Named<T>& rhs) { return name > rhs.name; }
	bool operator>=(const Named<T>& rhs) { return name >= rhs.name; }
};

} // namespace slade
