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
	virtual ~ScriptManagerWindow() = default;

	void                   openScriptTab(ScriptManager::Script* script) const;
	ScriptManager::Script* currentScript() const;
	std::string               currentScriptText() const;

private:
	ScriptManager::Script  script_scratchbox_;
	ScriptManager::Script* script_clicked_ = nullptr;

	std::map<ScriptManager::ScriptType, wxTreeItemId> editor_script_nodes_;

	// Documentation tab
#if USE_WEBVIEW_STARTPAGE
	wxWebView* webview_docs_ = nullptr;
#endif

	// Widgets
	STabCtrl*   tabs_scripts_ = nullptr;
	wxTreeCtrl* tree_scripts_ = nullptr;

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
	void         addEditorScriptsNode(wxTreeItemId parent_node, ScriptManager::ScriptType type, const wxString& name);
	void         populateScriptsTree();
	ScriptPanel* currentPage() const;
	void         closeScriptTab(ScriptManager::Script* script) const;
	void         showDocs(const wxString& url = "");

	// SActionHandler
	bool handleAction(std::string_view id) override;
};
