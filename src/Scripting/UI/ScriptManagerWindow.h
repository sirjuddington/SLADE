#pragma once

#include "UI/STopWindow.h"
#include "General/SAction.h"
#include "Scripting/ScriptManager.h"

class STabCtrl;

class ScriptManagerWindow : public STopWindow, SActionHandler
{
public:
	ScriptManagerWindow();
	virtual ~ScriptManagerWindow() {}

	void					openScriptTab(ScriptManager::Script* script);
	ScriptManager::Script*	currentScript();

private:
	ScriptManager::Script	script_scratchbox_;

	// Widgets
	STabCtrl*	tabs_scripts_;
	wxTreeCtrl*	tree_scripts_;

	// Layout
	void	loadLayout();
	void	saveLayout();

	// Setup
	void		setupLayout();
	wxPanel*	setupMainArea();
	void		setupMenu();
	void		setupToolbar();
	void		bindEvents();
	wxPanel*	setupScriptTreePanel();
	void		populateScriptsTree();

	// SActionHandler
	bool	handleAction(string id);
};
