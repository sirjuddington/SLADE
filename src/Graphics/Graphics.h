#pragma once

namespace slade
{
class ArchiveEntry;

namespace gfx
{
	// PNG
	Vec2i                pngGetSize(const MemChunk& png_data);
	std::optional<Vec2i> pngGetgrAb(const MemChunk& png_data);
	bool                 pngSetgrAb(MemChunk& png_data, int xoff, int yoff);
	bool                 pngGettRNS(const MemChunk& png_data);
	bool                 pngSettRNS(MemChunk& png_data, bool value);
	bool                 pngGetalPh(const MemChunk& png_data);
	bool                 pngSetalPh(MemChunk& png_data, bool value);

	// Offsets
	enum class OffsetType
	{
		Monster,
		MonsterGL,
		Projectile,
		WeaponFull,
		WeaponDoom,
		WeaponHeretic,
		WeaponHexen
	};
	Vec2i                calculateOffsets(int width, int height, OffsetType type);
	std::optional<Vec2i> getImageOffsets(MemChunk& img_data);
	bool                 setImageOffsets(MemChunk& img_data, int xoff, int yoff);
	bool                 supportsOffsets(const ArchiveEntry& entry);
} // namespace gfx
} // namespace slade
