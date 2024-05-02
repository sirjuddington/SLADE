#pragma once

#include "RectFwd.h"

namespace slade
{
struct BBox
{
	Vec2d min;
	Vec2d max;

	BBox();
	BBox(const Vec2d& min, const Vec2d& max);
	BBox(double min_x, double min_y, double max_x, double max_y);

	void reset();

	void extend(double x, double y);
	void extend(const Vec2d& point);
	void extend(const BBox& other);

	bool pointWithin(double x, double y) const;
	bool contains(const Vec2d& point) const;
	bool isWithin(const Vec2d& bmin, const Vec2d& bmax) const;

	bool isValid() const;

	Vec2d  size() const;
	double width() const;
	double height() const;

	Vec2d  mid() const;
	double midX() const;
	double midY() const;

	Seg2d leftSide() const;
	Seg2d rightSide() const;
	Seg2d bottomSide() const;
	Seg2d topSide() const;
};
} // namespace slade
