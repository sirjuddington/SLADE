#pragma once

#include "General/SAction.h"
#include "Scripting/ScriptManager.h"
#include "UI/STopWindow.h"

namespace slade
{
class STabCtrl;
class ScriptPanel;

class ScriptManagerWindow : public STopWindow, SActionHandler
{
public:
	ScriptManagerWindow();
	virtual ~ScriptManagerWindow() = default;

	void                   openScriptTab(scriptmanager::Script* script) const;
	scriptmanager::Script* currentScript() const;
	string                 currentScriptText() const;

private:
	scriptmanager::Script  script_scratchbox_;
	scriptmanager::Script* script_clicked_ = nullptr;

	std::map<scriptmanager::ScriptType, wxTreeItemId> editor_script_nodes_;

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
	void         populateEditorScriptsTree(scriptmanager::ScriptType type);
	void         addEditorScriptsNode(wxTreeItemId parent_node, scriptmanager::ScriptType type, const wxString& name);
	void         populateScriptsTree();
	ScriptPanel* currentPage() const;
	void         closeScriptTab(scriptmanager::Script* script) const;
	void         showDocs(const wxString& url = "");

	// SActionHandler
	bool handleAction(string_view id) override;
};
} // namespace slade
