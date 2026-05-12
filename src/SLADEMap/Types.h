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

	enum class SectorPart : u8
	{
		Interior = 0,
		Floor    = 1,
		Ceiling  = 2
	};

	enum class SidePart : u8
	{
		Upper  = 0,
		Middle = 1,
		Lower  = 2
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
