
#include "Main.h"
#include "MapBackupManager.h"
#include "MapEditContext.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "MapEditor/UI/Dialogs/ThingTypeBrowser.h"
#include "MapEditor/UI/PropsPanel/LinePropsPanel.h"
#include "MapEditor/UI/PropsPanel/SectorPropsPanel.h"
#include "MapEditor/UI/PropsPanel/ThingPropsPanel.h"
#include "MapTextureManager.h"
#include "UI/MapEditorWindow.h"
#include "UI/PropsPanel/MapObjectPropsPanel.h"
#include "UI/SDialog.h"

namespace MapEditor
{
	MapEditContext		edit_context;
	MapTextureManager	texture_manager;
	Archive::mapdesc_t	current_map_desc;
	MapEditorWindow*	map_window = nullptr;
	MapBackupManager	backup_manager;
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

MapBackupManager& MapEditor::backupManager()
{
	return backup_manager;
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

string MapEditor::browseTexture(const string &init_texture, int tex_type, SLADEMap& map, const string& title)
{
	// Open texture browser
	MapTextureBrowser browser(map_window, tex_type, init_texture, &map);
	browser.SetTitle(title);

	// Return selected texture if not cancelled
	if (browser.ShowModal() == wxID_OK)
		return browser.getSelectedItem()->getName();

	return wxEmptyString;
}

int MapEditor::browseThingType(int init_type, SLADEMap& map)
{
	// Open thing browser
	ThingTypeBrowser browser(map_window, init_type);

	if (browser.ShowModal() == wxID_OK)
		return browser.getSelectedType();

	return -1;
}

bool MapEditor::editObjectProperties(vector<MapObject*>& list)
{
	string selsize = "";
	string type = edit_context.modeString(false);
	if (list.size() == 1)
		type += S_FMT(" #%d", list[0]->getIndex());
	else if (list.size() > 1)
		selsize = S_FMT("(%lu selected)", list.size());

	// Create dialog for properties panel
	SDialog dlg(
		MapEditor::window(),
		S_FMT("%s Properties %s", type, selsize),
		S_FMT("mobjprops_%s", CHR(edit_context.modeString(false))),
		-1,
		-1
	);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	// Create properties panel
	PropsPanelBase* panel_props = nullptr;
	switch (edit_context.editMode())
	{
	case Mode::Lines:
		sizer->Add(panel_props = new LinePropsPanel(&dlg), 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10); break;
	case Mode::Sectors:
		sizer->Add(panel_props = new SectorPropsPanel(&dlg), 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10); break;
	case Mode::Things:
		sizer->Add(panel_props = new ThingPropsPanel(&dlg), 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10); break;
	default:
		sizer->Add(panel_props = new MapObjectPropsPanel(&dlg, true), 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
	}

	// Add dialog buttons
	sizer->AddSpacer(4);
	sizer->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Open current selection
	panel_props->openObjects(list);

	// Open the dialog and apply changes if OK was clicked
	dlg.SetMinClientSize(sizer->GetMinSize());
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		panel_props->applyChanges();
		return true;
	}

	return false;
}

MapEditor::ItemType MapEditor::baseItemType(const ItemType& type)
{
	switch (type)
	{
	case ItemType::Vertex:		return ItemType::Vertex;
	case ItemType::Line:		return ItemType::Line;
	case ItemType::Side:
	case ItemType::WallBottom:
	case ItemType::WallMiddle:
	case ItemType::WallTop:		return ItemType::Side;
	case ItemType::Sector:
	case ItemType::Ceiling:
	case ItemType::Floor:		return ItemType::Sector;
	case ItemType::Thing:		return ItemType::Thing;
	default:					return ItemType::Any;
	}
}
