#pragma once

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
	bool		openMap(Archive::MapDesc map);
	void		loadMapScripts(Archive::MapDesc map);
	WadArchive*	writeMap(string name="MAP01", bool nodes = true);
	bool		saveMap();
	bool		saveMapAs();
	void		closeMap();
	void		forceRefresh(bool renderer = false);
	void		refreshToolBar();
	void		setUndoManager(UndoManager* manager);
	bool		tryClose();
	bool		hasMapOpen(Archive* archive);
	void		reloadScriptsMenu();

	MapObjectPropsPanel*	propsPanel() const { return panel_obj_props_; }
	ObjectEditPanel*		objectEditPanel() const { return panel_obj_edit_; }
	
	void	showObjectEditPanel(bool show, ObjectEditGroup* group);
	void	showShapeDrawPanel(bool show = true);

	// SAction handler
	bool	handleAction(string id) override;

private:
	MapCanvas*					map_canvas_				= nullptr;
	MapObjectPropsPanel*		panel_obj_props_		= nullptr;
	ScriptEditorPanel*			panel_script_editor_	= nullptr;
	vector<ArchiveEntry*>		map_data_;
	ObjectEditPanel*			panel_obj_edit_			= nullptr;
	MapChecksPanel*				panel_checks_			= nullptr;
	UndoManagerHistoryPanel*	panel_undo_history_		= nullptr;
	wxMenu*						menu_scripts_			= nullptr;

	void	buildNodes(Archive* wad);
	void	lockMapEntries(bool lock = true);

	// Events
	void	onClose(wxCloseEvent& e);
	void	onSize(wxSizeEvent& e);
};
