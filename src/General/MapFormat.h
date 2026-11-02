#pragma once

namespace slade
{
enum class MapFormat
{
	Doom = 0,
	Hexen,
	Doom64,
	UDMF,
	Doom32X,
	Unknown, // Needed for maps in zip archives
};

MapFormat mapFormatFromId(string_view id);
string    mapFormatId(MapFormat format);
} // namespace slade
