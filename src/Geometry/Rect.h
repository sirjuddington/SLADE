#pragma once

#include "RectFwd.h"

namespace slade
{
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
			tl = { x - (width * 0.5), y - (height * 0.5) };
			br = { x + (width * 0.5), y + (height * 0.5) };
		}
		else
		{
			tl = { x, y };
			br = { x + width, y + height };
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
		tl = { x1, y1 };
		br = { x2, y2 };
	}

	void set(const Rect<T>& rect)
	{
		tl = rect.tl;
		br = rect.br;
	}

	// TR/BL aliases that make more sense for line segments
	const Vec2<T>& start() const { return tl; }
	const Vec2<T>& end() const { return br; }

	Rect<T> flip() const { return Rect<T>(br, tl); }

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
} // namespace slade
