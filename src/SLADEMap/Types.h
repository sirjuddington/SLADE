#pragma once

namespace slade
{
// TODO: Move to map namespace
enum class SectorSurfaceType : u8
{
	Floor = 1,
	Ceiling
};

namespace map
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

	enum class ObjectPoint : u8
	{
		Mid = 0,
		Within,
		Text
	};

	typedef std::array<int, 5> ArgSet;
} // namespace map
} // namespace slade
