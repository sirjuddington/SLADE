#pragma once

// General definitions that don't really fit in one specific place,
// but also don't need to be included *everywhere* (Main.h)

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
}
