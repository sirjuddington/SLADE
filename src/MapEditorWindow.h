
#ifndef __MAPEDITORWINDOW_H__
#define __MAPEDITORWINDOW_H__

#include "MapCanvas.h"
#include "MapEditor.h"
#include "MapTextureManager.h"
#include "MainApp.h"
#include "STopWindow.h"

class MapObject;
class MapObjectPropsPanel;
class SToolBar;
class ScriptEditorPanel;
class MapEditorWindow : public STopWindow, public SActionHandler
{
private:
	MapCanvas*				map_canvas;
	MapEditor				editor;
	MapTextureManager		tex_man;
	MapObjectPropsPanel*	panel_obj_props;
	ScriptEditorPanel*		panel_script_editor;
	Archive::mapdesc_t		mdesc_current;

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

	MapEditor&			mapEditor() { return editor; }
	MapTextureManager&	textureManager() { return tex_man; }
	Archive::mapdesc_t&	currentMapDesc() { return mdesc_current; }

	// Layout save/load
	void	loadLayout();
	void	saveLayout();

	void	setupLayout();
	bool	openMap(Archive::mapdesc_t map);
	void	loadMapScripts(Archive::mapdesc_t map);
	bool	saveMap();
	bool	saveMapAs();
	void	closeMap();
	void	forceRefresh(bool renderer = false);
	void	refreshToolBar();

	MapObjectPropsPanel*	propsPanel() { return panel_obj_props; }

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
