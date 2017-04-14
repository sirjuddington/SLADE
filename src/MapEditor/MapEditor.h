#pragma once

class Archive;
class MapEditContext;
class MapTextureManager;
class MapEditorWindow;
class MapObject;
class ObjectEditGroup;
class UndoManager;
class MapBackupManager;

namespace MapEditor
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
		Thing,	// (also 2d things mode)

		Any
	};

	struct Item
	{
		int			index;
		ItemType	type;

		Item(int index = -1, ItemType type = ItemType::Any) : index{ index }, type{ type } {}

		// Comparison operators
		bool operator<(const Item& other) const {
			if (this->type == other.type)
				return this->index < other.index;
			else
				return this->type < other.type;
		}
		bool operator==(const Item& other) const
		{
			return index == other.index && (type == ItemType::Any || type == other.type);
		}
		bool operator!=(const Item& other) const
		{
			return !(*this == other);
		}

		// Conversion operators
		explicit operator int() const { return index; }
	};

	MapEditContext&		editContext();
	MapTextureManager&	textureManager();
	MapEditorWindow*	window();
	wxWindow*			windowWx();
	MapBackupManager&	backupManager();

	void	init();
	void	forceRefresh(bool renderer = false);
	bool	chooseMap(Archive* archive = nullptr);
	void	setUndoManager(UndoManager* manager);

	void	openObjectProperties(MapObject* object);
	void	openMultiObjectProperties(vector<MapObject*>& objects);

	void	showShapeDrawPanel(bool show = true);
	void	showObjectEditPanel(bool show = true, ObjectEditGroup* group = nullptr);

	ItemType	baseItemType(const ItemType& type);
}
