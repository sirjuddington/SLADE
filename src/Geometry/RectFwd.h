#pragma once

namespace slade
{
template<typename T> struct Rect;

typedef Rect<int>    Recti;
typedef Rect<float>  Rectf;
typedef Rect<double> Rectd;

// Rectangle is not really any different from a 2D segment, but using it to
// mean that can be confusing, so here's an alias.
template<typename T> using Seg2 = Rect<T>;
typedef Seg2<int>    Seg2i;
typedef Seg2<double> Seg2d;
typedef Seg2<float>  Seg2f;
} // namespace slade
