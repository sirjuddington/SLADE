
#include "Main.h"
#include "MapEditContext.h"
#include "MapEditor.h"
#include "MapTextureManager.h"
#include "UI/MapEditorWindow.h"
#include "UI/PropsPanel/MapObjectPropsPanel.h"

namespace MapEditor
{
	MapEditContext		edit_context;
	MapTextureManager	texture_manager;
	Archive::mapdesc_t	current_map_desc;
	MapEditorWindow*	map_window = nullptr;
}

MapEditContext& MapEditor::editContext()
{
	return edit_context;
}

MapTextureManager& MapEditor::textureManager()
{
	return texture_manager;
}

MapEditorWindow* MapEditor::window()
{
	if (!map_window)
		init();

	return map_window;
}

wxWindow* MapEditor::windowWx()
{
	if (!map_window)
		init();

	return map_window;
}

void MapEditor::init()
{
	map_window = new MapEditorWindow();
	texture_manager.init();
}

void MapEditor::forceRefresh(bool renderer)
{
	if (map_window)
		map_window->forceRefresh(renderer);
}

bool MapEditor::chooseMap(Archive* archive)
{
	if (!map_window)
		init();

	return map_window->chooseMap(archive);
}

void MapEditor::setUndoManager(UndoManager* manager)
{
	map_window->setUndoManager(manager);
}

void MapEditor::openObjectProperties(MapObject* object)
{
	map_window->propsPanel()->openObject(object);
}

void MapEditor::openMultiObjectProperties(vector<MapObject*>& objects)
{
	map_window->propsPanel()->openObjects(objects);
}

void MapEditor::showShapeDrawPanel(bool show)
{
	map_window->showShapeDrawPanel(show);
}

void MapEditor::showObjectEditPanel(bool show, ObjectEditGroup* group)
{
	map_window->showObjectEditPanel(show, group);
}
