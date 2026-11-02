#pragma once

namespace slade::map
{
struct SectorLighting
{
	u8      brightness = 255;
	ColRGBA colour;
	ColRGBA fog;
};
} // namespace slade::map
