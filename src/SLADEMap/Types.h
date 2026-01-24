#pragma once

namespace slade
{
namespace map
{
	enum class SectorSurfaceType : u8
	{
		Floor = 1,
		Ceiling
	};

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
