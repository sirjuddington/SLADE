#pragma once

class Archive;
class MapBackupManager;
class MapEditContext;
class MapEditorWindow;
class MapObject;
class MapTextureManager;
class ObjectEditGroup;
class SLADEMap;
class UndoManager;

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

struct Item
{
	int      index;
	ItemType type;
	int      extra_floor_index;
//  int      control_line;
	int      real_index;

	Item(int index = -1, ItemType type = ItemType::Any, int extra_floor_index = -1) :
		 index{ index },
		 type{ type },
		 extra_floor_index{ extra_floor_index },
		 real_index{ -1 } {}

	// Comparison operators
	bool operator<(const Item& other) const
	{
		if (this->type == other.type)
		{
			if (this->index == other.index)
			{
				if(this->extra_floor_index == other.extra_floor_index)
					return this->real_index < other.real_index;
				else
					return this->extra_floor_index < other.extra_floor_index;
			}
			else
			{
				return this->index < other.index;
			}
		}
		else
		{
			return this->type < other.type;
		}
	}

	bool operator==(const Item& other) const
	{
		return index == other.index && (type == ItemType::Any || type == other.type) && this->extra_floor_index == other.extra_floor_index && this->real_index == other.real_index;
	}

	bool operator!=(const Item& other) const
	{
		return !(*this == other);
	}

	// Conversion operators
	explicit operator int() const { return index; }
};

MapEditContext&    editContext();
MapTextureManager& textureManager();
MapEditorWindow*   window();
wxWindow*          windowWx();
MapBackupManager&  backupManager();

void init();
void forceRefresh(bool renderer = false);
bool chooseMap(Archive* archive = nullptr);
void setUndoManager(UndoManager* manager);

// UI
void setStatusText(const string& text, int column);
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
string browseTexture(const string& init_texture, int tex_type, SLADEMap& map, const string& title = "Browse Texture");
int    browseThingType(int init_type, SLADEMap& map);

// Misc
ItemType baseItemType(const ItemType& type);
ItemType itemTypeFromObject(const MapObject* object);
} // namespace MapEditor
