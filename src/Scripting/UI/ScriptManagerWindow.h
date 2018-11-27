#pragma once

#include "General/SAction.h"
#include "Scripting/ScriptManager.h"
#include "UI/STopWindow.h"

class STabCtrl;
class ScriptPanel;

class ScriptManagerWindow : public STopWindow, SActionHandler
{
public:
	ScriptManagerWindow();
	virtual ~ScriptManagerWindow() {}

	void                   openScriptTab(ScriptManager::Script* script) const;
	ScriptManager::Script* currentScript() const;
	string                 currentScriptText() const;

private:
	ScriptManager::Script  script_scratchbox_;
	ScriptManager::Script* script_clicked_ = nullptr;

	std::map<ScriptManager::ScriptType, wxTreeItemId> editor_script_nodes_;

	// Documentation tab
#if USE_WEBVIEW_STARTPAGE
	wxWebView* webview_docs_;
#endif

	// Widgets
	STabCtrl*   tabs_scripts_;
	wxTreeCtrl* tree_scripts_;

	// Layout
	void loadLayout();
	void saveLayout();

	// Setup
	void         setupLayout();
	wxPanel*     setupMainArea();
	void         setupMenu();
	void         setupToolbar();
	void         bindEvents();
	wxPanel*     setupScriptTreePanel();
	void         populateEditorScriptsTree(ScriptManager::ScriptType type);
	void         addEditorScriptsNode(wxTreeItemId parent_node, ScriptManager::ScriptType type, const string& name);
	void         populateScriptsTree();
	ScriptPanel* currentPage() const;
	void         closeScriptTab(ScriptManager::Script* script);
	void         showDocs(string url = "");

	// SActionHandler
	bool handleAction(string id) override;
};
