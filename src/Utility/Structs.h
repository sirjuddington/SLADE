
#pragma once

#include <cmath>
using std::max;
using std::min;

// This is basically a collection of handy little structs I use a lot,
// with some useful functions for each of them

// point2_t: A 2d coordinate
struct Vec2i
{
	int x, y;

	Vec2i() { x = y = 0; }
	Vec2i(int X, int Y)
	{
		x = X;
		y = Y;
	}

	void set(int X, int Y)
	{
		x = X;
		y = Y;
	}
	void set(Vec2i p)
	{
		x = p.x;
		y = p.y;
	}

	Vec2i operator+(Vec2i point) { return Vec2i(point.x + x, point.y + y); }
	Vec2i operator-(Vec2i point) { return Vec2i(x - point.x, y - point.y); }
	Vec2i operator*(double num) { return Vec2i((int)((double)x * num), (int)((double)y * num)); }
	Vec2i operator/(double num)
	{
		if (num == 0)
			return Vec2i(0, 0);
		else
			return Vec2i((int)((double)x / num), (int)((double)y / num));
	}
	bool operator==(Vec2i rhs) { return (x == rhs.x && y == rhs.y); }
	bool operator!=(Vec2i rhs) { return (x != rhs.x || y != rhs.y); }

	static Vec2i outside() { return { -1, -1 }; }
};

// fpoint2_t: A 2d coordinate (or vector) with floating-point precision
struct Vec2f
{
	double x, y;

	Vec2f() { x = y = 0.0f; }
	Vec2f(double X, double Y)
	{
		x = X;
		y = Y;
	}
	Vec2f(Vec2i p)
	{
		x = p.x;
		y = p.y;
	}

	void set(double X, double Y)
	{
		x = X;
		y = Y;
	}
	void set(Vec2f p)
	{
		x = p.x;
		y = p.y;
	}
	void set(Vec2i p)
	{
		x = p.x;
		y = p.y;
	}

	double magnitude() { return (double)sqrt((long double)(x * x + y * y)); }

	Vec2f normalized()
	{
		double mag = magnitude();
		if (mag == 0.0f)
			return Vec2f(0.0f, 0.0f);
		else
			return Vec2f(x / mag, y / mag);
	}

	void normalize()
	{
		double mag = magnitude();
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

	double distance_to(Vec2f point)
	{
		double dist_x = point.x - x;
		double dist_y = point.y - y;

		return (double)sqrt((long double)(dist_x * dist_x + dist_y * dist_y));
	}

	// aka "Manhattan" distance -- just the sum of the vertical and horizontal
	// distance, and an upper bound on the true distance
	double taxicab_distance_to(Vec2f point)
	{
		double dist;
		if (point.x < x)
			dist = x - point.x;
		else
			dist = point.x - x;
		if (point.y < y)
			dist += y - point.y;
		else
			dist += point.y - y;

		return dist;
	}

	double dot(Vec2f vec) { return x * vec.x + y * vec.y; }
	double cross(Vec2f p2) { return (x * p2.y) - (y * p2.x); }

	Vec2f operator+(Vec2f point) { return Vec2f(point.x + x, point.y + y); }
	Vec2f operator-(Vec2f point) { return Vec2f(x - point.x, y - point.y); }
	Vec2f operator*(double num) { return Vec2f(x * num, y * num); }
	Vec2f operator/(double num)
	{
		if (num == 0.0f)
			return Vec2f(0.0f, 0.0f);
		else
			return Vec2f(x / num, y / num);
	}
	bool operator==(Vec2f rhs) { return (x == rhs.x && y == rhs.y); }
	bool operator!=(Vec2f rhs) { return (x != rhs.x || y != rhs.y); }
};


// fpoint3_t: A 3d coordinate (or vector) with floating-point precision
struct Vec3f
{
	double x, y, z;

	Vec3f() { x = y = z = 0; }
	Vec3f(double X, double Y, double Z)
	{
		x = X;
		y = Y;
		z = Z;
	}
	Vec3f(Vec2f p, double Z = 0)
	{
		x = p.x;
		y = p.y;
		z = Z;
	}

	void set(double X, double Y, double Z)
	{
		x = X;
		y = Y;
		z = Z;
	}
	void set(Vec3f p)
	{
		x = p.x;
		y = p.y;
		z = p.z;
	}

	double magnitude() { return (double)sqrt((long double)(x * x + y * y + z * z)); }
	double dot(Vec3f vec) { return x * vec.x + y * vec.y + z * vec.z; }

	Vec3f normalized()
	{
		double mag = magnitude();
		if (mag == 0)
			return Vec3f(0, 0, 0);
		else
			return Vec3f(x / mag, y / mag, z / mag);
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

	double distance_to(Vec3f point)
	{
		double dist_x = point.x - x;
		double dist_y = point.y - y;
		double dist_z = point.z - z;

		return (double)sqrt((long double)(dist_x * dist_x + dist_y * dist_y + dist_z * dist_z));
	}

	Vec3f operator+(Vec3f point) { return Vec3f(point.x + x, point.y + y, point.z + z); }
	Vec3f operator-(Vec3f point) { return Vec3f(x - point.x, y - point.y, z - point.z); }
	Vec3f operator*(double num) { return Vec3f(x * num, y * num, z * num); }
	Vec3f operator/(double num)
	{
		if (num == 0)
			return Vec3f(0, 0, 0);
		else
			return Vec3f(x / num, y / num, z / num);
	}

	Vec3f cross(Vec3f p2)
	{
		Vec3f cross_product;

		cross_product.x = ((y * p2.z) - (z * p2.y));
		cross_product.y = ((z * p2.x) - (x * p2.z));
		cross_product.z = ((x * p2.y) - (y * p2.x));

		return cross_product;
	}

	Vec2f get2d() { return Vec2f(x, y); }
};


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

	float fr() { return (float)r / 255.0f; }
	float fg() { return (float)g / 255.0f; }
	float fb() { return (float)b / 255.0f; }
	float fa() { return (float)a / 255.0f; }

	double dr() { return (double)r / 255.0; }
	double dg() { return (double)g / 255.0; }
	double db() { return (double)b / 255.0; }
	double da() { return (double)a / 255.0; }

	bool equals(ColRGBA rhs, bool alpha = false, bool index = false)
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
	ColRGBA amp(int R, int G, int B, int A)
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

		return ColRGBA((uint8_t)nr, (uint8_t)ng, (uint8_t)nb, (uint8_t)na, blend, -1);
	}

	// Amplify/fade colour components by factors
	ColRGBA ampf(float fr, float fg, float fb, float fa)
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

		return ColRGBA((uint8_t)nr, (uint8_t)ng, (uint8_t)nb, (uint8_t)na, blend, -1);
	}

	void write(uint8_t* ptr)
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
	ColRGBA greyscale()
	{
		uint8_t l = (uint8_t)round(r * 0.3 + g * 0.59 + b * 0.11);
		return ColRGBA(l, l, l, a, blend);
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
#define WXCOL(rgba) wxColor(rgba.r, rgba.g, rgba.b, rgba.a)
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

// Recti: A rectangle (int points)
struct Recti
{
	Vec2i tl, br;

	// Constructors
	Recti()
	{
		tl.set(0, 0);
		br.set(0, 0);
	}

	Recti(Vec2i TL, Vec2i BR)
	{
		tl.set(TL);
		br.set(BR);
	}

	Recti(int x1, int y1, int x2, int y2)
	{
		tl.set(x1, y1);
		br.set(x2, y2);
	}

	Recti(int x, int y, int width, int height, int align)
	{
		// Top-left
		if (align == 0)
		{
			tl.set(x, y);
			br.set(x + width, y + height);
		}

		// Centered
		else if (align == 1)
		{
			tl.set(x - (width / 2), y - (height / 2));
			br.set(x + (width / 2), y + (height / 2));
		}
	}

	// Functions
	void set(Vec2i TL, Vec2i BR)
	{
		tl.set(TL);
		br.set(BR);
	}

	void set(int x1, int y1, int x2, int y2)
	{
		tl.set(x1, y1);
		br.set(x2, y2);
	}

	void set(Recti rect)
	{
		tl.set(rect.tl);
		br.set(rect.br);
	}

	int x1() { return tl.x; }
	int y1() { return tl.y; }
	int x2() { return br.x; }
	int y2() { return br.y; }

	int left() { return min(tl.x, br.x); }
	int top() { return min(tl.y, br.y); }
	int right() { return max(br.x, tl.x); }
	int bottom() { return max(br.y, tl.y); }

	int width() { return br.x - tl.x; }
	int height() { return br.y - tl.y; }

	int awidth() { return max(br.x, tl.x) - min(tl.x, br.x); }
	int aheight() { return max(br.y, tl.y) - min(tl.y, br.y); }

	Vec2i middle() { return Vec2i(left() + (awidth() / 2), top() + (aheight() / 2)); }

	void expand(int x, int y)
	{
		if (x1() < x2())
		{
			tl.x -= x;
			br.x += x;
		}
		else
		{
			tl.x += x;
			br.x -= x;
		}

		if (y1() < y2())
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

	double length()
	{
		double dist_x = x2() - x1();
		double dist_y = y2() - y1();

		return (double)sqrt((long double)(dist_x * dist_x + dist_y * dist_y));
	}

	bool contains(Vec2i point)
	{
		return (point.x >= left() && point.x <= right() && point.y >= top() && point.y <= bottom());
	}
};


// Rectf: A rectangle (float points)
struct Rectf
{
	Vec2f tl, br;

	// Constructors
	Rectf()
	{
		tl.set(0, 0);
		br.set(0, 0);
	}

	Rectf(Recti rect)
	{
		tl.set(rect.tl);
		br.set(rect.br);
	}

	Rectf(Vec2f TL, Vec2f BR)
	{
		tl.set(TL);
		br.set(BR);
	}

	Rectf(Vec2i TL, Vec2i BR)
	{
		tl.set(TL);
		br.set(BR);
	}

	Rectf(double x1, double y1, double x2, double y2)
	{
		tl.set(x1, y1);
		br.set(x2, y2);
	}

	Rectf(double x, double y, double width, double height, int align)
	{
		// Top-left
		if (align == 0)
		{
			tl.set(x, y);
			br.set(x + width, y + height);
		}

		// Centered
		else if (align == 1)
		{
			tl.set(x - (width / 2), y - (height / 2));
			br.set(x + (width / 2), y + (height / 2));
		}
	}

	// Functions
	void set(Recti rect)
	{
		tl.set(rect.tl);
		br.set(rect.br);
	}

	void set(Vec2f TL, Vec2f BR)
	{
		tl.set(TL);
		br.set(BR);
	}

	void set(Vec2i TL, Vec2i BR)
	{
		tl.set(TL);
		br.set(BR);
	}

	void set(double x1, double y1, double x2, double y2)
	{
		tl.set(x1, y1);
		br.set(x2, y2);
	}

	void set(Rectf rect)
	{
		tl.set(rect.tl);
		br.set(rect.br);
	}

	double x1() const { return tl.x; }
	double y1() const { return tl.y; }
	double x2() const { return br.x; }
	double y2() const { return br.y; }
	Vec2f  p1() const { return tl; }
	Vec2f  p2() const { return br; }

	double left() const { return min(tl.x, br.x); }
	double top() const { return min(tl.y, br.y); }
	double right() const { return max(br.x, tl.x); }
	double bottom() const { return max(br.y, tl.y); }

	double width() const { return br.x - tl.x; }
	double height() const { return br.y - tl.y; }

	double awidth() const { return max(br.x, tl.x) - min(tl.x, br.x); }
	double aheight() const { return max(br.y, tl.y) - min(tl.y, br.y); }

	Vec2f middle() { return Vec2f(left() + (awidth() / 2), top() + (aheight() / 2)); }

	void expand(double x, double y)
	{
		if (x1() < x2())
		{
			tl.x -= x;
			br.x += x;
		}
		else
		{
			tl.x += x;
			br.x -= x;
		}

		if (y1() < y2())
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

	double length()
	{
		double dist_x = x2() - x1();
		double dist_y = y2() - y1();

		return (double)sqrt((long double)(dist_x * dist_x + dist_y * dist_y));
	}

	bool contains(Vec2f point) const
	{
		return (point.x >= left() && point.x <= right() && point.y >= top() && point.y <= bottom());
	}
};
// Rectangle is not really any different from a 2D segment, but using it to
// mean that can be confusing, so here's an alias.
typedef Rectf Seg2f;


// Plane: A 3d plane
struct Plane
{
	double a, b, c, d;

	Plane() : a(0.0), b(0.0), c(0.0), d(0.0) {}
	Plane(double a, double b, double c, double d) : a(a), b(b), c(c), d(d) {}

	/** Construct a flat plane (perpendicular to the z axis) at the given height.
	 */
	static Plane flat(float height) { return Plane(0.0, 0.0, 1.0, height); }

	bool operator==(const Plane& rhs) const { return a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d; }
	bool operator!=(const Plane& rhs) const { return !(*this == rhs); }

	void set(double a, double b, double c, double d)
	{
		this->a = a;
		this->b = b;
		this->c = c;
		this->d = d;
	}

	Vec3f normal() const
	{
		Vec3f norm(a, b, c);
		norm.normalize();
		return norm;
	}

	void normalize()
	{
		Vec3f  vec(a, b, c);
		double mag = vec.magnitude();
		a          = a / mag;
		b          = b / mag;
		c          = c / mag;
		d          = d / mag;
	}

	double height_at(Vec2f point) const { return height_at(point.x, point.y); }
	double height_at(double x, double y) const { return ((-a * x) + (-b * y) + d) / c; }
};


// BBox: A simple bounding box with related functions
struct BBox
{
	Vec2f min;
	Vec2f max;

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

	bool pointWithin(double x, double y) { return (x >= min.x && x <= max.x && y >= min.y && y <= max.y); }
	bool contains(Vec2f point) { return pointWithin(point.x, point.y); }
	bool isWithin(Vec2f bmin, Vec2f bmax)
	{
		return (min.x >= bmin.x && max.x <= bmax.x && min.y >= bmin.y && max.y <= bmax.y);
	}

	bool isValid() { return ((max.x - min.x > 0) && (max.y - min.y) > 0); }

	Vec2f  size() { return Vec2f(max.x - min.x, max.y - min.y); }
	double width() { return max.x - min.x; }
	double height() { return max.y - min.y; }

	Vec2f  mid() { return Vec2f(midX(), midY()); }
	double midX() { return min.x + ((max.x - min.x) * 0.5); }
	double midY() { return min.y + ((max.y - min.y) * 0.5); }

	Seg2f leftSide() { return Seg2f(min.x, min.y, min.x, max.y); }
	Seg2f rightSide() { return Seg2f(max.x, min.y, max.x, max.y); }
	Seg2f bottomSide() { return Seg2f(min.x, max.y, max.x, max.y); }
	Seg2f topSide() { return Seg2f(min.x, min.y, max.x, min.y); }
};

// Formerly key_value_t
typedef std::pair<string, string> StringPair;
