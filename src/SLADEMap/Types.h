#pragma once

namespace slade::map
{
enum class ObjectType : u8
{
	Object = 0,
	Vertex,
	Line,
	Side,
	Sector,
	Thing
};

enum class ObjectPoint
{
	Mid = 0,
	Within,
	Text
};

typedef std::array<int, 5> ArgSet;
} // namespace slade::map
