#pragma once

// Forward declarations
namespace slade
{
class Archive;
class MapBackupManager;
class MapEditorWindow;
class MapObject;
class MapVertex;
class MapLine;
class MapSide;
class MapSector;
class MapThing;
class MapTextureManager;
class SLADEMap;
class UndoManager;

namespace mapeditor
{
	class MapEditContext;
	class ObjectEditGroup;
} // namespace mapeditor
} // namespace slade

namespace slade::mapeditor
{
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
} // namespace slade::mapeditor
