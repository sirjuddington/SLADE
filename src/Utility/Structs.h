
#pragma once

#include <cmath>
using std::max;
using std::min;

// This is basically a collection of handy little structs I use a lot,
// with some useful functions for each of them

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


// ColRGBA: A 32-bit colour definition
struct ColRGBA
{
	uint8_t r, g, b, a;
	int16_t index; // -1=not indexed
	char    blend; // 0=normal, 1=additive

	// Constructors
	ColRGBA()
	{
		r     = 0;
		g     = 0;
		b     = 0;
		a     = 0;
		blend = -1;
		index = -1;
	}

	ColRGBA(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255, char BLEND = -1, int16_t INDEX = -1)
	{
		r     = R;
		g     = G;
		b     = B;
		a     = A;
		blend = BLEND;
		index = INDEX;
	}

	// Functions
	void set(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255, char BLEND = -1, int16_t INDEX = -1)
	{
		r     = R;
		g     = G;
		b     = B;
		a     = A;
		blend = BLEND;
		index = INDEX;
	}

	void set(ColRGBA colour)
	{
		r     = colour.r;
		g     = colour.g;
		b     = colour.b;
		a     = colour.a;
		blend = colour.blend;
		index = colour.index;
	}

	float fr() const { return (float)r / 255.0f; }
	float fg() const { return (float)g / 255.0f; }
	float fb() const { return (float)b / 255.0f; }
	float fa() const { return (float)a / 255.0f; }

	double dr() const { return (double)r / 255.0; }
	double dg() const { return (double)g / 255.0; }
	double db() const { return (double)b / 255.0; }
	double da() const { return (double)a / 255.0; }

	bool equals(ColRGBA rhs, bool alpha = false, bool index = false) const
	{
		bool col_equal = (r == rhs.r && g == rhs.g && b == rhs.b);

		if (index)
			col_equal &= (this->index == rhs.index);
		if (alpha)
			return col_equal && (a == rhs.a);
		else
			return col_equal;
	}

	// Amplify/fade colour components by given amounts
	ColRGBA amp(int R, int G, int B, int A) const
	{
		int nr = r + R;
		int ng = g + G;
		int nb = b + B;
		int na = a + A;

		if (nr > 255)
			nr = 255;
		if (nr < 0)
			nr = 0;
		if (ng > 255)
			ng = 255;
		if (ng < 0)
			ng = 0;
		if (nb > 255)
			nb = 255;
		if (nb < 0)
			nb = 0;
		if (na > 255)
			na = 255;
		if (na < 0)
			na = 0;

		return { (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, (uint8_t)na, blend, -1 };
	}

	// Amplify/fade colour components by factors
	ColRGBA ampf(float fr, float fg, float fb, float fa) const
	{
		int nr = (int)(r * fr);
		int ng = (int)(g * fg);
		int nb = (int)(b * fb);
		int na = (int)(a * fa);

		if (nr > 255)
			nr = 255;
		if (nr < 0)
			nr = 0;
		if (ng > 255)
			ng = 255;
		if (ng < 0)
			ng = 0;
		if (nb > 255)
			nb = 255;
		if (nb < 0)
			nb = 0;
		if (na > 255)
			na = 255;
		if (na < 0)
			na = 0;

		return { (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, (uint8_t)na, blend, -1 };
	}

	void write(uint8_t* ptr) const
	{
		if (ptr)
		{
			ptr[0] = r;
			ptr[1] = g;
			ptr[2] = b;
			ptr[3] = a;
		}
	}

	// Returns a copy of this colour as greyscale (using 'common' component coefficients)
	ColRGBA greyscale() const
	{
		uint8_t l = (uint8_t)round(r * 0.3 + g * 0.59 + b * 0.11);
		return { l, l, l, a, blend };
	}
};

// Some basic colours
#define COL_WHITE ColRGBA(255, 255, 255, 255, 0)
#define COL_BLACK ColRGBA(0, 0, 0, 255, 0)
#define COL_RED ColRGBA(255, 0, 0, 255, 0)
#define COL_GREEN ColRGBA(0, 255, 0, 255, 0)
#define COL_BLUE ColRGBA(0, 0, 255, 255, 0)
#define COL_YELLOW ColRGBA(255, 255, 0, 255, 0)
#define COL_PURPLE ColRGBA(255, 0, 255, 255, 0)
#define COL_CYAN ColRGBA(0, 255, 255, 255, 0)

// Convert ColRGBA to wxColor
#define WXCOL(rgba) wxColor((rgba).r, (rgba).g, (rgba).b, (rgba).a)
#define COLWX(wxcol) wxcol.Red(), wxcol.Green(), wxcol.Blue()

// ColHSL: Represents a colour in HSL format, generally used for calculations
struct ColHSL
{
	double h, s, l;

	ColHSL() { h = s = l = 0; }
	ColHSL(double h, double s, double l)
	{
		this->h = h;
		this->s = s;
		this->l = l;
	}
};

// ColLAB: Represents a colour in CIE-L*a*b format, generally used for calculations
struct ColLAB
{
	double l, a, b;

	ColLAB() { l = a = b = 0; }
	ColLAB(double l, double a, double b)
	{
		this->l = l;
		this->a = a;
		this->b = b;
	}
};

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

	void set(int x1, int y1, int x2, int y2)
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
