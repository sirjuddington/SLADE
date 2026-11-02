#pragma once

#include "General/SActionHandler.h"
#include "UI/STopWindow.h"

// Forward declarations
namespace slade
{
class STabCtrl;
class ScriptPanel;
namespace scriptmanager
{
	enum class ScriptType;
	struct Script;
} // namespace scriptmanager
} // namespace slade

namespace slade
{
class ScriptManagerWindow : public STopWindow, SActionHandler
{
public:
	ScriptManagerWindow();
	~ScriptManagerWindow() override = default;

	void                   openScriptTab(scriptmanager::Script* script) const;
	scriptmanager::Script* currentScript() const;
	string                 currentScriptText() const;

private:
	unique_ptr<scriptmanager::Script> script_scratchbox_;
	scriptmanager::Script*            script_clicked_ = nullptr;

	std::map<scriptmanager::ScriptType, wxTreeItemId> editor_script_nodes_;

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
	void         closeScriptTab(const scriptmanager::Script* script) const;
	void         showDocs(const string& url = "");

	// SActionHandler
	bool handleAction(string_view id) override;
};
} // namespace slade
