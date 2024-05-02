#pragma once

namespace slade
{
struct Plane
{
	double a, b, c, d;

	Plane();
	Plane(double a, double b, double c, double d);

	bool operator==(const Plane& rhs) const { return a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d; }
	bool operator!=(const Plane& rhs) const { return !(*this == rhs); }

	void set(double a, double b, double c, double d);

	Vec3d normal() const;
	void  normalize();

	double heightAt(const Vec2d& point) const;
	double heightAt(double x, double y) const;

	static Plane flat(float height);
};
} // namespace slade
