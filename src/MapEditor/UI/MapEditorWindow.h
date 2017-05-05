
#ifndef __MAPEDITORWINDOW_H__
#define __MAPEDITORWINDOW_H__

#include "General/SAction.h"
#include "UI/STopWindow.h"
#include "Archive/Archive.h"

class MapObject;
class MapObjectPropsPanel;
class ScriptEditorPanel;
class ObjectEditPanel;
class ObjectEditGroup;
class WadArchive;
class MapCanvas;
class MapChecksPanel;
class UndoManagerHistoryPanel;
class UndoManager;
class ArchiveEntry;

class MapEditorWindow : public STopWindow, public SActionHandler
{
public:
	MapEditorWindow();
	~MapEditorWindow();

	// Layout save/load
	void	loadLayout();
	void	saveLayout();

	void		setupMenu();
	void		setupLayout();
	bool		chooseMap(Archive* archive = nullptr);
	bool		openMap(Archive::mapdesc_t map);
	void		loadMapScripts(Archive::mapdesc_t map);
	WadArchive*	writeMap(string name="MAP01", bool nodes = true);
	bool		saveMap();
	bool		saveMapAs();
	void		closeMap();
	void		forceRefresh(bool renderer = false);
	void		refreshToolBar();
	void		setUndoManager(UndoManager* manager);
	bool		tryClose();
	bool		hasMapOpen(Archive* archive);

	MapObjectPropsPanel*	propsPanel() { return panel_obj_props; }
	ObjectEditPanel*		objectEditPanel() { return panel_obj_edit; }
	
	void	showObjectEditPanel(bool show, ObjectEditGroup* group);
	void	showShapeDrawPanel(bool show = true);

	// SAction handler
	bool	handleAction(string id);

private:
	MapCanvas*					map_canvas;
	MapObjectPropsPanel*		panel_obj_props;
	ScriptEditorPanel*			panel_script_editor;
	vector<ArchiveEntry*>		map_data;
	ObjectEditPanel*			panel_obj_edit;
	MapChecksPanel*				panel_checks;
	UndoManagerHistoryPanel*	panel_undo_history;

	void	buildNodes(Archive* wad);
	void	lockMapEntries(bool lock = true);

	// Events
	void	onClose(wxCloseEvent& e);
	void	onSize(wxSizeEvent& e);
};

#endif //__MAPEDITORWINDOW_H__
