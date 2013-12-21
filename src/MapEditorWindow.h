
#ifndef __MAPEDITORWINDOW_H__
#define __MAPEDITORWINDOW_H__

#include "MapEditor.h"
#include "MapTextureManager.h"
#include "MainApp.h"
#include "STopWindow.h"

class MapObject;
class MapObjectPropsPanel;
class SToolBar;
class ScriptEditorPanel;
class ObjectEditPanel;
class ObjectEditGroup;
class WadArchive;
class MapCanvas;
class MapChecksPanel;
class MapEditorWindow : public STopWindow, public SActionHandler
{
private:
	MapCanvas*				map_canvas;
	MapEditor				editor;
	MapTextureManager		tex_man;
	MapObjectPropsPanel*	panel_obj_props;
	ScriptEditorPanel*		panel_script_editor;
	Archive::mapdesc_t		mdesc_current;
	ObjectEditPanel*		panel_obj_edit;
	MapChecksPanel*			panel_checks;

	// Singleton instance
	static MapEditorWindow*		instance;

	void	buildNodes(Archive* wad);
	void	lockMapEntries(bool lock = true);

public:
	MapEditorWindow();
	~MapEditorWindow();

	// Singleton implementation
	static MapEditorWindow* getInstance()
	{
		if (!instance)
			instance = new MapEditorWindow();

		return instance;
	}
	static void deleteInstance()
	{
		if (instance)
		{
			instance->Close();
			delete instance;
		}
		instance = NULL;
	}

	MapEditor&			mapEditor() { return editor; }
	MapTextureManager&	textureManager() { return tex_man; }
	Archive::mapdesc_t&	currentMapDesc() { return mdesc_current; }

	// Layout save/load
	void	loadLayout();
	void	saveLayout();

	void		setupLayout();
	bool		createMap();
	bool		openMap(Archive::mapdesc_t map);
	void		loadMapScripts(Archive::mapdesc_t map);
	WadArchive*	writeMap();
	bool		saveMap();
	bool		saveMapAs();
	void		closeMap();
	void		forceRefresh(bool renderer = false);
	void		refreshToolBar();
	void		editObjectProperties(vector<MapObject*>& objects);

	MapObjectPropsPanel*	propsPanel() { return panel_obj_props; }
	ObjectEditPanel*		objectEditPanel() { return panel_obj_edit; }
	
	void	showObjectEditPanel(bool show, ObjectEditGroup* group);
	void	showShapeDrawPanel(bool show = true);

	// SAction handler
	bool	handleAction(string id);

	// Events
	void	onClose(wxCloseEvent& e);
	void	onSize(wxSizeEvent& e);
	void	onMove(wxMoveEvent& e);
};

// Define for less cumbersome MapEditorWindow::getInstance()
#define theMapEditor MapEditorWindow::getInstance()

enum ThingDrawTypes
{
	TDT_SQUARE,
	TDT_ROUND,
	TDT_SPRITE,
	TDT_SQUARESPRITE,
	TDT_FRAMEDSPRITE,
};

#endif //__MAPEDITORWINDOW_H__
