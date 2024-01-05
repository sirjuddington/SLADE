#pragma once

namespace slade
{
class Archive;
class MapBackupManager;
class MapEditContext;
class MapEditorWindow;
class MapObject;
class MapVertex;
class MapLine;
class MapSide;
class MapSector;
class MapThing;
class MapTextureManager;
class ObjectEditGroup;
class SLADEMap;
class UndoManager;

namespace mapeditor
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

	enum class Mode
	{
		Vertices,
		Lines,
		Sectors,
		Things,
		Visual
	};

	enum class SectorMode
	{
		Both,
		Floor,
		Ceiling
	};

	enum class TextureType
	{
		Texture,
		Flat
	};

	struct Item
	{
		int      index;
		ItemType type;
		int      real_index;
		int      control_line;

		Item(int index = -1, ItemType type = ItemType::Any) :
			index{ index }, type{ type }, real_index{ -1 }, control_line{ -1 }
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
			return index == other.index && (type == ItemType::Any || type == other.type)
				   && real_index == other.real_index;
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

	MapEditContext&    editContext();
	MapTextureManager& textureManager();
	MapEditorWindow*   window();
	wxWindow*          windowWx();
	MapBackupManager&  backupManager();
	bool               windowCreated();

	void init();
	void forceRefresh(bool renderer = false);
	bool chooseMap(Archive* archive = nullptr);
	void setUndoManager(UndoManager* manager);

	// UI
	void setStatusText(string_view text, int column);
	void lockMouse(bool lock);
	void openContextMenu();

	// Properties Panel
	void openObjectProperties(MapObject* object);
	void openMultiObjectProperties(vector<MapObject*>& objects);
	bool editObjectProperties(vector<MapObject*>& list);
	void resetObjectPropertiesPanel();

	// Other Panels
	void showShapeDrawPanel(bool show = true);
	void showObjectEditPanel(bool show = true, ObjectEditGroup* group = nullptr);

	// Browser
	string browseTexture(
		string_view init_texture,
		TextureType tex_type,
		SLADEMap&   map,
		string_view title = "Browse Texture");
	int browseThingType(int init_type, SLADEMap& map);

	// Misc
	ItemType baseItemType(const ItemType& type);
	ItemType itemTypeFromObject(const MapObject* object);
} // namespace mapeditor
} // namespace slade
