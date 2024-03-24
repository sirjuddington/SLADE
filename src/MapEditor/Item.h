#pragma once

namespace slade::mapeditor
{
enum class ItemType
{
	// 2d modes
	Vertex,
	Line,
	Sector,

	// 3d mode
	Side,
	WallTop,
	WallMiddle,
	WallBottom,
	Floor,
	Ceiling,
	Thing, // (also 2d things mode)

	Any
};

struct Item
{
	int      index;
	ItemType type;
	int      real_index;
	int      control_line;

	Item(int index = -1, ItemType type = ItemType::Any) :
		index{ index },
		type{ type },
		real_index{ -1 },
		control_line{ -1 }
	{
	}

	// Comparison operators
	bool operator<(const Item& other) const
	{
		if (type == other.type)
		{
			if (index == other.index)
				return real_index < other.real_index;
			else
				return index < other.index;
		}
		else
			return type < other.type;
	}
	bool operator==(const Item& other) const
	{
		return index == other.index && (type == ItemType::Any || type == other.type) && real_index == other.real_index;
	}
	bool operator!=(const Item& other) const { return !(*this == other); }

	// Conversion operators
	explicit operator int() const { return index; }

	MapVertex* asVertex(const SLADEMap& map) const;
	MapLine*   asLine(const SLADEMap& map) const;
	MapSide*   asSide(const SLADEMap& map) const;
	MapSector* asSector(const SLADEMap& map) const;
	MapThing*  asThing(const SLADEMap& map) const;
	MapObject* asObject(const SLADEMap& map) const;
};

ItemType baseItemType(const ItemType& type);
ItemType itemTypeFromObject(const MapObject* object);
} // namespace slade::mapeditor
