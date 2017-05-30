
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
#include "UI/MapCanvas.h"

namespace MapEditor
{
	std::unique_ptr<MapEditContext>	edit_context;
	MapTextureManager				texture_manager;
	Archive::mapdesc_t				current_map_desc;
	MapEditorWindow*				map_window;
	MapBackupManager				backup_manager;
}

MapEditContext& MapEditor::editContext()
{
	if (!edit_context)
		edit_context = std::make_unique<MapEditContext>();

	return *edit_context;
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

void ::MapEditor::setStatusText(const string &text, int column)
{
	map_window->CallAfter(&MapEditorWindow::SetStatusText, text, column);
}

void MapEditor::lockMouse(bool lock)
{
	edit_context->canvas()->lockMouse(lock);
}

void MapEditor::openContextMenu()
{
	// Context menu
	wxMenu menu_context;

	// Set 3d camera
	SAction::fromId("mapw_camera_set")->addToMenu(&menu_context, true);

	// Run from here
	SAction::fromId("mapw_run_map_here")->addToMenu(&menu_context, true);

	// Mode-specific
	bool object_selected = edit_context->selection().hasHilightOrSelection();
	if (edit_context->editMode() == Mode::Vertices)
	{
		menu_context.AppendSeparator();
		SAction::fromId("mapw_vertex_create")->addToMenu(&menu_context, true);
	}
	else if (edit_context->editMode() == Mode::Lines)
	{
		if (object_selected)
		{
			menu_context.AppendSeparator();
			SAction::fromId("mapw_line_changetexture")->addToMenu(&menu_context, true);
			SAction::fromId("mapw_line_changespecial")->addToMenu(&menu_context, true);
			SAction::fromId("mapw_line_tagedit")->addToMenu(&menu_context, true);
			SAction::fromId("mapw_line_flip")->addToMenu(&menu_context, true);
			SAction::fromId("mapw_line_correctsectors")->addToMenu(&menu_context, true);
		}
	}
	else if (edit_context->editMode() == Mode::Things)
	{
		menu_context.AppendSeparator();

		if (object_selected)
			SAction::fromId("mapw_thing_changetype")->addToMenu(&menu_context, true);

		SAction::fromId("mapw_thing_create")->addToMenu(&menu_context, true);
	}
	else if (edit_context->editMode() == Mode::Sectors)
	{
		if (object_selected)
		{
			SAction::fromId("mapw_sector_changetexture")->addToMenu(&menu_context, true);
			SAction::fromId("mapw_sector_changespecial")->addToMenu(&menu_context, true);
			if (edit_context->selection().size() > 1)
			{
				SAction::fromId("mapw_sector_join")->addToMenu(&menu_context, true);
				SAction::fromId("mapw_sector_join_keep")->addToMenu(&menu_context, true);
			}
		}

		SAction::fromId("mapw_sector_create")->addToMenu(&menu_context, true);
	}

	if (object_selected)
	{
		// General edit
		menu_context.AppendSeparator();
		SAction::fromId("mapw_edit_objects")->addToMenu(&menu_context, true);
		SAction::fromId("mapw_mirror_x")->addToMenu(&menu_context, true);
		SAction::fromId("mapw_mirror_y")->addToMenu(&menu_context, true);

		// Properties
		menu_context.AppendSeparator();
		SAction::fromId("mapw_item_properties")->addToMenu(&menu_context, true);
	}

	map_window->PopupMenu(&menu_context);
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
	// Unlock cursor if locked
	bool cursor_locked = edit_context.get()->mouseLocked();
	if (cursor_locked)
		edit_context.get()->lockMouse(false);

	// Setup texture browser
	MapTextureBrowser browser(map_window, tex_type, init_texture, &map);
	browser.SetTitle(title);

	// Get selected texture
	string tex;
	if (browser.ShowModal() == wxID_OK)
		tex = browser.getSelectedItem()->getName();

	// Re-lock cursor if needed
	if (cursor_locked)
		edit_context.get()->lockMouse(true);

	return tex;
}

int MapEditor::browseThingType(int init_type, SLADEMap& map)
{
	// Unlock cursor if locked
	bool cursor_locked = edit_context.get()->mouseLocked();
	if (cursor_locked)
		edit_context.get()->lockMouse(false);

	// Setup thing browser
	ThingTypeBrowser browser(map_window, init_type);

	// Get selected type
	int type = -1;
	if (browser.ShowModal() == wxID_OK)
		type = browser.getSelectedType();

	// Re-lock cursor if needed
	if (cursor_locked)
		edit_context.get()->lockMouse(true);

	return type;
}

bool MapEditor::editObjectProperties(vector<MapObject*>& list)
{
	string selsize = "";
	string type = edit_context->modeString(false);
	if (list.size() == 1)
		type += S_FMT(" #%d", list[0]->getIndex());
	else if (list.size() > 1)
		selsize = S_FMT("(%lu selected)", list.size());

	// Create dialog for properties panel
	SDialog dlg(
		MapEditor::window(),
		S_FMT("%s Properties %s", type, selsize),
		S_FMT("mobjprops_%s", CHR(edit_context->modeString(false))),
		-1,
		-1
	);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	// Create properties panel
	PropsPanelBase* panel_props = nullptr;
	switch (edit_context->editMode())
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
