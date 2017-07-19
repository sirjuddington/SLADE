#pragma once

#include "UI/STopWindow.h"
#include "General/SAction.h"

class TextEditor;

class ScriptManagerWindow : public STopWindow, SActionHandler
{
public:
	ScriptManagerWindow();
	virtual ~ScriptManagerWindow() {}

private:
	// Widgets
	TextEditor*	text_editor_;
	wxButton*	btn_run_;
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
	void		populateScriptsTree();

	// SActionHandler
	bool	handleAction(string id);
};
