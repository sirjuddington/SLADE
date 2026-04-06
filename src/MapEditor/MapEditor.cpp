
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapEditor.cpp
// Description: MapEditor namespace - general map editor functions and types,
//              mostly to interact with the map editor window without needing to
//              access it (and wx) directly
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "General/SAction.h"
#include "ItemSelection.h"
#include "MapBackupManager.h"
#include "MapEditContext.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "MapEditor/UI/Dialogs/ThingTypeBrowser.h"
#include "MapEditor/UI/PropsPanel/LinePropsPanel.h"
#include "MapEditor/UI/PropsPanel/SectorPropsPanel.h"
#include "MapEditor/UI/PropsPanel/ThingPropsPanel.h"
#include "MapTextureManager.h"
#include "SLADEMap/MapObject/MapObject.h"
#include "UI/Browser/BrowserItem.h"
#include "UI/Layout.h"
#include "UI/MapCanvas.h"
#include "UI/MapEditorWindow.h"
#include "UI/PropsPanel/MapObjectPropsPanel.h"
#include "UI/SDialog.h"
#include "UI/WxUtils.h"

using namespace slade;
using namespace mapeditor;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::mapeditor
{
unique_ptr<MapEditContext> edit_context;
MapTextureManager          texture_manager;
MapDesc                    current_map_desc;
MapEditorWindow*           map_window;
MapBackupManager           backup_manager;
} // namespace slade::mapeditor


// -----------------------------------------------------------------------------
//
// MapEditor Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the current map editor context
// -----------------------------------------------------------------------------
MapEditContext& mapeditor::editContext()
{
	if (!edit_context)
		edit_context = std::make_unique<MapEditContext>();

	return *edit_context;
}

// -----------------------------------------------------------------------------
// Returns the map editor texture manager
// -----------------------------------------------------------------------------
MapTextureManager& mapeditor::textureManager()
{
	return texture_manager;
}

// -----------------------------------------------------------------------------
// Returns the map editor window
// -----------------------------------------------------------------------------
MapEditorWindow* mapeditor::window()
{
	if (!map_window)
		init();

	return map_window;
}

// -----------------------------------------------------------------------------
// Returns the map editor window (as a wxWindow)
// -----------------------------------------------------------------------------
wxWindow* mapeditor::windowWx()
{
	if (!map_window)
		init();

	return map_window;
}

// -----------------------------------------------------------------------------
// Returns the map editor backup manager
// -----------------------------------------------------------------------------
MapBackupManager& mapeditor::backupManager()
{
	return backup_manager;
}

bool mapeditor::windowCreated()
{
	return map_window != nullptr;
}

// -----------------------------------------------------------------------------
// Initialises the map editor
// -----------------------------------------------------------------------------
void mapeditor::init()
{
	map_window = new MapEditorWindow();
	texture_manager.init();
}

// -----------------------------------------------------------------------------
// Forces a refresh of the map editor window
// (and the renderer if [renderer] is true)
// -----------------------------------------------------------------------------
void mapeditor::forceRefresh(bool renderer)
{
	if (map_window)
		map_window->forceRefresh(renderer);
}

// -----------------------------------------------------------------------------
// Opens the map editor launcher dialog to create or open a map
// -----------------------------------------------------------------------------
bool mapeditor::chooseMap(Archive* archive)
{
	if (!map_window)
		init();

	return map_window->chooseMap(archive);
}

// -----------------------------------------------------------------------------
// Sets the active undo [manager] for the map editor
// -----------------------------------------------------------------------------
void mapeditor::setUndoManager(UndoManager* manager)
{
	map_window->setUndoManager(manager);
}

// -----------------------------------------------------------------------------
// Sets the map editor window status bar [text] at [column]
// -----------------------------------------------------------------------------
void ::mapeditor::setStatusText(string_view text, int column)
{
	map_window->CallAfter(&MapEditorWindow::SetStatusText, wxutil::strFromView(text), column);
}

// -----------------------------------------------------------------------------
// Locks or unlocks the mouse cursor
// -----------------------------------------------------------------------------
void mapeditor::lockMouse(bool lock)
{
	edit_context->canvas()->lockMouse(lock);
}

// -----------------------------------------------------------------------------
// Pops up the context menu for the current edit mode
// -----------------------------------------------------------------------------
void mapeditor::openContextMenu()
{
	wxMenu menu_context;
	if (edit_context->populateContextMenu(menu_context))
		map_window->PopupMenu(&menu_context);
}

// -----------------------------------------------------------------------------
// Opens [object] in the map editor object properties panel
// -----------------------------------------------------------------------------
void mapeditor::openObjectProperties(MapObject* object)
{
	map_window->propsPanel()->openObject(object);
}

// -----------------------------------------------------------------------------
// Opens multiple [objects] in the map editor object properties panel
// -----------------------------------------------------------------------------
void mapeditor::openMultiObjectProperties(vector<MapObject*>& objects)
{
	map_window->propsPanel()->openObjects(objects);
}

// -----------------------------------------------------------------------------
// Shows or hides the shape draw panel
// -----------------------------------------------------------------------------
void mapeditor::showShapeDrawPanel(bool show)
{
	map_window->showShapeDrawPanel(show);
}

// -----------------------------------------------------------------------------
// Shows or hides the object edit panel and opens [group] if being shown
// -----------------------------------------------------------------------------
void mapeditor::showObjectEditPanel(bool show, ObjectEditGroup* group)
{
	map_window->showObjectEditPanel(show, group);
}

// -----------------------------------------------------------------------------
// Opens the texture browser for [tex_type] textures, with [init_texture]
// initially selected. Returns the selected texture
// -----------------------------------------------------------------------------
string mapeditor::browseTexture(string_view init_texture, TextureType tex_type, SLADEMap& map, string_view title)
{
	// Unlock cursor if locked
	bool cursor_locked = edit_context->mouseLocked();
	if (cursor_locked)
		edit_context->lockMouse(false);

	// Setup texture browser
	MapTextureBrowser browser(map_window, tex_type, init_texture, &map);
	browser.SetTitle(wxutil::strFromView(title));

	// Get selected texture
	string tex{ init_texture };
	if (browser.ShowModal() == wxID_OK && browser.selectedItem())
		tex = browser.selectedItem()->name();

	// Re-lock cursor if needed
	if (cursor_locked)
		edit_context->lockMouse(true);

	return tex;
}

// -----------------------------------------------------------------------------
// Opens the thing type browser with [init_type] initially selected.
// Returns the selected type
// -----------------------------------------------------------------------------
int mapeditor::browseThingType(int init_type, SLADEMap& map)
{
	// Unlock cursor if locked
	bool cursor_locked = edit_context->mouseLocked();
	if (cursor_locked)
		edit_context->lockMouse(false);

	// Setup thing browser
	ThingTypeBrowser browser(map_window, init_type);

	// Get selected type
	int type = -1;
	if (browser.ShowModal() == wxID_OK)
		type = browser.selectedType();

	// Re-lock cursor if needed
	if (cursor_locked)
		edit_context->lockMouse(true);

	return type;
}

// -----------------------------------------------------------------------------
// Opens the appropriate properties dialog for objects in [list].
// Returns true if the property edit was applied
// -----------------------------------------------------------------------------
bool mapeditor::editObjectProperties(vector<MapObject*>& list)
{
	if (list.empty())
		return false;

	string selsize;
	string type = list[0]->typeName();
	if (list.size() == 1)
		type += fmt::format(" #{}", list[0]->index());
	else if (list.size() > 1)
		selsize = fmt::format("({} selected)", list.size());

	// Create dialog for properties panel
	SDialog dlg(
		mapeditor::window(),
		fmt::format("{} Properties {}", type, selsize),
		fmt::format("mobjprops_{}", list[0]->typeName()),
		-1,
		-1);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	// Create/add properties panel
	auto            lh          = ui::LayoutHelper(&dlg);
	PropsPanelBase* panel_props = nullptr;
	switch (list[0]->objType())
	{
	case MapObject::Type::Line:   panel_props = new LinePropsPanel(&dlg); break;
	case MapObject::Type::Sector: panel_props = new SectorPropsPanel(&dlg); break;
	case MapObject::Type::Thing:  panel_props = new ThingPropsPanel(&dlg); break;
	default:                      panel_props = new MapObjectPropsPanel(&dlg, true); break;
	}
	sizer->Add(panel_props, lh.sfWithLargeBorder(1, wxLEFT | wxRIGHT | wxTOP).Expand());

	// Add dialog buttons
	sizer->AddSpacer(lh.pad());
	sizer->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), lh.sfWithLargeBorder(0, wxLEFT | wxRIGHT | wxBOTTOM).Expand());

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

// -----------------------------------------------------------------------------
// Resets/clears the object properties panel
// -----------------------------------------------------------------------------
void mapeditor::resetObjectPropertiesPanel()
{
	map_window->propsPanel()->clearGrid();
}
