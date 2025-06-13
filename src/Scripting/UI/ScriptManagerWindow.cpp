
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2022 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ScriptManagerWindow.cpp
// Description: SLADE Script manager window
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "ScriptManagerWindow.h"
#include "App.h"
#include "General/SAction.h"
#include "Graphics/Icons.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "ScriptPanel.h"
#include "Scripting/Lua.h"
#include "Scripting/ScriptManager.h"
#include "UI/Controls/ConsolePanel.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/SAuiTabArt.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/State.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
string docs_url       = "https://slade.readthedocs.io/en/latest";
int    layout_version = 1;
} // namespace


// -----------------------------------------------------------------------------
// NewEditorScriptDialog Class
//
// A simple dialog showing a dropdown to select an editor script type and a
// text box to enter a name for the script
// -----------------------------------------------------------------------------
class NewEditorScriptDialog : public wxDialog
{
public:
	NewEditorScriptDialog(wxWindow* parent) : wxDialog(parent, -1, wxS("New Editor Script"))
	{
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		auto gbsizer = new wxGridBagSizer(ui::pad(), ui::pad());
		sizer->Add(gbsizer, 1, wxEXPAND | wxALL, ui::padLarge());
		gbsizer->AddGrowableCol(1, 1);

		// Script type
		wxString types[] = { wxS("Custom"), wxS("Archive"), wxS("Entry"), wxS("Map Editor") };
		choice_type_     = new wxChoice(this, -1, { -1, -1 }, { -1, -1 }, 4, types);
		choice_type_->SetSelection(0);
		gbsizer->Add(new wxStaticText(this, -1, wxS("Type:")), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gbsizer->Add(choice_type_, { 0, 1 }, { 1, 1 }, wxEXPAND);

		// Script name
		text_name_ = new wxTextCtrl(this, -1, wxEmptyString, { -1, -1 }, { 200, -1 }, wxTE_PROCESS_ENTER);
		gbsizer->Add(new wxStaticText(this, -1, wxS("Name:")), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gbsizer->Add(text_name_, { 1, 1 }, { 1, 1 }, wxEXPAND);
		text_name_->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& e) { EndModal(wxID_OK); });

		// Dialog buttons
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, ui::padLarge());
		hbox->AddStretchSpacer(1);

		// OK
		hbox->Add(new wxButton(this, wxID_OK, wxS("OK")), 0, wxEXPAND | wxRIGHT, ui::padLarge());

		SetEscapeId(wxID_CANCEL);
		wxWindowBase::Layout();
		sizer->Fit(this);
	}

	scriptmanager::ScriptType selectedType() const
	{
		switch (choice_type_->GetCurrentSelection())
		{
		case 1:  return scriptmanager::ScriptType::Archive;
		case 2:  return scriptmanager::ScriptType::Entry;
		case 3:  return scriptmanager::ScriptType::Map;
		default: return scriptmanager::ScriptType::Custom;
		}
	}

	string selectedName() const { return text_name_->GetValue().utf8_string(); }

private:
	wxChoice*   choice_type_;
	wxTextCtrl* text_name_;
};


// -----------------------------------------------------------------------------
// ScriptTreeItemData Class
//
// Just used to store Script pointers with wxTreeCtrl items
// -----------------------------------------------------------------------------
class ScriptTreeItemData : public wxTreeItemData
{
public:
	ScriptTreeItemData(scriptmanager::Script* script) : script{ script } {}
	scriptmanager::Script* script;
};


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Returns a new or existing wxTreeItemId for [tree], at [path] from
// [parent_node]. Creates any required nodes along the way.
// -----------------------------------------------------------------------------
wxTreeItemId getOrCreateNode(wxTreeCtrl* tree, wxTreeItemId parent_node, string_view path)
{
	auto name      = strutil::beforeFirstV(path, '/');
	auto path_rest = strutil::afterFirstV(path, '/');

	// Find child node with name
	wxTreeItemIdValue cookie;
	auto              child = tree->GetFirstChild(parent_node, cookie);
	while (child.IsOk())
	{
		if (strutil::equalCI(tree->GetItemText(child).utf8_string(), name))
			break;

		child = tree->GetNextSibling(child);
	}

	// Not found, create child node
	if (!child.IsOk())
		child = tree->AppendItem(parent_node, wxutil::strFromView(name), 1);

	// Return it or go deeper into the tree
	if (path_rest.empty())
		return child;
	else
		return getOrCreateNode(tree, child, path_rest);
}

// -----------------------------------------------------------------------------
// Creates the wxImageList to use for the script tree control
// -----------------------------------------------------------------------------
wxImageList* createTreeImageList()
{
	auto image_list = wxutil::createSmallImageList();
	wxutil::addImageListIcon(image_list, icons::Entry, "code");
	wxutil::addImageListIcon(image_list, icons::Entry, "folder");
	return image_list;
}

} // namespace


// -----------------------------------------------------------------------------
//
// ScriptManagerWindow Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ScriptManagerWindow class constructor
// -----------------------------------------------------------------------------
ScriptManagerWindow::ScriptManagerWindow() : STopWindow("SLADE Script Manager", "scriptmanager")
{
	setupLayout();

	// Open 'scratch box' initially
	script_scratchbox_.name = "Scratch Box";
	script_scratchbox_.text =
		"-- Use this script to write ad-hoc SLADE editor scripts\n"
		"-- Note that this will not be saved between sessions\n\n";
	script_scratchbox_.read_only = true;
	openScriptTab(&script_scratchbox_);
}

// -----------------------------------------------------------------------------
// Loads the previously saved layout file for the window
// -----------------------------------------------------------------------------
void ScriptManagerWindow::loadLayout()
{
	auto* aui_mgr = wxAuiManager::GetManager(this);
	auto  layout  = ui::getWindowLayout(id_.c_str());

	for (const auto& component : layout)
		if (!component.first.empty() && !component.second.empty())
			aui_mgr->LoadPaneInfo(
				wxString::FromUTF8(component.second), aui_mgr->GetPane(wxString::FromUTF8(component.first)));
}

// -----------------------------------------------------------------------------
// Saves the current window layout to a file
// -----------------------------------------------------------------------------
void ScriptManagerWindow::saveLayout()
{
	vector<StringPair> layout;
	auto*              aui_mgr = wxAuiManager::GetManager(this);

	layout.emplace_back("console", aui_mgr->SavePaneInfo(aui_mgr->GetPane(wxS("console"))).utf8_string());
	layout.emplace_back("scripts_area", aui_mgr->SavePaneInfo(aui_mgr->GetPane(wxS("scripts_area"))).utf8_string());

	ui::setWindowLayout(id_.c_str(), layout);
}

// -----------------------------------------------------------------------------
// Sets up the wxWidgets window layout
// -----------------------------------------------------------------------------
void ScriptManagerWindow::setupLayout()
{
	// Maximize if it was last time
	if (ui::getStateBool("ScriptManagerWindowMaximized"))
		Maximize();

	// Create the wxAUI manager & related things
	auto m_mgr = new wxAuiManager(this);
	m_mgr->SetArtProvider(new SAuiDockArt(this));
	wxAuiPaneInfo p_inf;

	// Set icon
	wxutil::setWindowIcon(this, "logo");

	// -- Main Panel --
	p_inf.CenterPane();
	p_inf.Name(wxS("editor_area"));
	p_inf.PaneBorder(false);
	m_mgr->AddPane(setupMainArea(), p_inf);

	// -- Scripts Panel --
	p_inf.DefaultPane();
	p_inf.Left();
	p_inf.BestSize(wxutil::scaledSize(256, 480));
	p_inf.Caption(wxS("Scripts"));
	p_inf.Name(wxS("scripts_area"));
	p_inf.Show(true);
	p_inf.Dock();
	m_mgr->AddPane(setupScriptTreePanel(), p_inf);

	// -- Console Panel --
	auto panel_console = new ConsolePanel(this, -1);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Float();
	p_inf.FloatingSize(wxutil::scaledSize(600, 400));
	p_inf.FloatingPosition(100, 100);
	p_inf.MinSize(wxutil::scaledSize(-1, 192));
	p_inf.Show(false);
	p_inf.Caption(wxS("Console"));
	p_inf.Name(wxS("console"));
	m_mgr->AddPane(panel_console, p_inf);

	// Setup menu and toolbar
	setupMenu();
	setupToolbar();

	// Bind events
	bindEvents();

	// Load previously saved perspective string
	loadLayout();

	// Finalize
	m_mgr->Update();
	Layout();
}

// -----------------------------------------------------------------------------
// Creates and returns the script manager 'main' area as a wxPanel
// -----------------------------------------------------------------------------
wxPanel* ScriptManagerWindow::setupMainArea()
{
	auto panel = new wxPanel(this);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Tabs
	tabs_scripts_ = new STabCtrl(panel, true, true, -1, true, true);
	sizer->Add(tabs_scripts_, 1, wxEXPAND);

	return panel;
}

// -----------------------------------------------------------------------------
// Sets up the script manager window menu
// -----------------------------------------------------------------------------
void ScriptManagerWindow::setupMenu()
{
	// -- Menu bar --
	auto menu = new wxMenuBar();
	menu->SetThemeEnabled(false);

	// File menu
	auto file_menu = new wxMenu();
	SAction::fromId("scrm_newscript_editor")->addToMenu(file_menu);
	file_menu->AppendSeparator();
	SAction::fromId("scrm_close")->addToMenu(file_menu);
	menu->Append(file_menu, wxS("&File"));

	// Script menu
	auto script_menu = new wxMenu();
	SAction::fromId("scrm_run")->addToMenu(script_menu);
	SAction::fromId("scrm_save")->addToMenu(script_menu);
	// SAction::fromId("scrm_rename")->addToMenu(scriptMenu);
	// SAction::fromId("scrm_delete")->addToMenu(scriptMenu);
	menu->Append(script_menu, wxS("&Script"));

	// Text menu
	auto text_menu = new wxMenu();
	SAction::fromId("scrm_find_replace")->addToMenu(text_menu);
	SAction::fromId("scrm_jump_to_line")->addToMenu(text_menu);
	auto menu_fold = new wxMenu();
	text_menu->AppendSubMenu(menu_fold, wxS("Code Folding"));
	SAction::fromId("scrm_fold_foldall")->addToMenu(menu_fold);
	SAction::fromId("scrm_fold_unfoldall")->addToMenu(menu_fold);
	text_menu->AppendSeparator();
	SAction::fromId("scrm_wrap")->addToMenu(text_menu);
	menu->Append(text_menu, wxS("&Text"));

	// View menu
	auto view_menu = new wxMenu();
	SAction::fromId("scrm_showscripts")->addToMenu(view_menu);
	SAction::fromId("scrm_showconsole")->addToMenu(view_menu);
	if (app::useWebView())
		SAction::fromId("scrm_showdocs")->addToMenu(view_menu);
	menu->Append(view_menu, wxS("&View"));

	// Set the menu
	SetMenuBar(menu);
}

// -----------------------------------------------------------------------------
// Sets up the script manager window toolbar
// -----------------------------------------------------------------------------
void ScriptManagerWindow::setupToolbar()
{
	toolbar_ = new SToolBar(this, true);

	// Create File toolbar
	auto tbg_file = new SToolBarGroup(toolbar_, "_File");
	tbg_file->addActionButton("scrm_newscript_editor");
	toolbar_->addGroup(tbg_file);

	// Add toolbar
	wxAuiManager::GetManager(this)->AddPane(
		toolbar_,
		wxAuiPaneInfo()
			.Top()
			.CaptionVisible(false)
			.MinSize(-1, SToolBar::getBarHeight())
			.Resizable(false)
			.PaneBorder(false)
			.Name(wxS("toolbar")));
}

// -----------------------------------------------------------------------------
// Bind events for wx controls
// -----------------------------------------------------------------------------
void ScriptManagerWindow::bindEvents()
{
	// Tree item activate
	tree_scripts_->Bind(
		wxEVT_TREE_ITEM_ACTIVATED,
		[=](wxTreeEvent& e)
		{
			auto data = dynamic_cast<ScriptTreeItemData*>(tree_scripts_->GetItemData(e.GetItem()));
			if (data && data->script)
				openScriptTab(data->script);
			else if (tree_scripts_->ItemHasChildren(e.GetItem()))
				tree_scripts_->Toggle(e.GetItem());
		});

	// Tree item right click
	tree_scripts_->Bind(
		wxEVT_TREE_ITEM_RIGHT_CLICK,
		[&](wxTreeEvent& e)
		{
			auto data = dynamic_cast<ScriptTreeItemData*>(tree_scripts_->GetItemData(e.GetItem()));
			if (data && data->script && !data->script->read_only)
			{
				script_clicked_ = data->script;
				wxMenu popup;
				SAction::fromId("scrm_rename")->addToMenu(&popup);
				SAction::fromId("scrm_delete")->addToMenu(&popup);
				PopupMenu(&popup);
			}
		});

	// Window close
	Bind(
		wxEVT_CLOSE_WINDOW,
		[=](wxCloseEvent&)
		{
			// Save Layout
			saveLayout();
			ui::saveStateBool("ScriptManagerWindowMaximized", IsMaximized());
			const wxSize size = GetSize() * GetContentScaleFactor();
			if (!IsMaximized())
				ui::setWindowInfo(
					id_,
					size.x,
					size.y,
					GetPosition().x * GetContentScaleFactor(),
					GetPosition().y * GetContentScaleFactor());

			// Hide
			Show(false);
		});

	// Tab closing
	tabs_scripts_->Bind(
		wxEVT_AUINOTEBOOK_PAGE_CLOSE,
		[&](wxAuiNotebookEvent& e)
		{
			auto page = currentPage();
			if (page && !page->close())
				e.Veto();
		});
}

// -----------------------------------------------------------------------------
// Creates and returns the script tree area as a wxPanel
// -----------------------------------------------------------------------------
wxPanel* ScriptManagerWindow::setupScriptTreePanel()
{
	auto panel = new wxPanel(this);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	panel->SetSizer(sizer);

	// Scripts Tree
	tree_scripts_ = new wxTreeCtrl(
		panel,
		-1,
		wxDefaultPosition,
		wxutil::scaledSize(200, -1),
		wxTR_DEFAULT_STYLE | wxTR_NO_LINES | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT);
#if wxMAJOR_VERSION > 3 || (wxMAJOR_VERSION == 3 && wxMINOR_VERSION >= 1)
	tree_scripts_->EnableSystemTheme(true);
#endif
	tree_scripts_->SetImageList(createTreeImageList());
	populateScriptsTree();
	sizer->Add(tree_scripts_, 1, wxEXPAND | wxALL, ui::pad());

	return panel;
}

// -----------------------------------------------------------------------------
// Populates the editor scripts wxTreeCtrl node for [type]
// -----------------------------------------------------------------------------
void ScriptManagerWindow::populateEditorScriptsTree(scriptmanager::ScriptType type)
{
	if (!editor_script_nodes_[type].IsOk())
		return;

	tree_scripts_->DeleteChildren(editor_script_nodes_[type]);
	for (auto& script : scriptmanager::editorScripts(type))
		tree_scripts_->AppendItem(
			editor_script_nodes_[type], wxString::FromUTF8(script->name), 0, 0, new ScriptTreeItemData(script.get()));
}

// -----------------------------------------------------------------------------
// Adds the editor scripts wxTreeCtrl node for [type] with [name], under
// [parent_node] and populates it
// -----------------------------------------------------------------------------
void ScriptManagerWindow::addEditorScriptsNode(
	wxTreeItemId              parent_node,
	scriptmanager::ScriptType type,
	const wxString&           name)
{
	editor_script_nodes_[type] = tree_scripts_->AppendItem(parent_node, name, 1);
	populateEditorScriptsTree(type);
}

// -----------------------------------------------------------------------------
// Loads scripts from slade.pk3 into the scripts tree control
// -----------------------------------------------------------------------------
void ScriptManagerWindow::populateScriptsTree()
{
	using namespace scriptmanager;

	// Clear tree
	tree_scripts_->DeleteAllItems();

	// Populate it
	auto root = tree_scripts_->AddRoot(wxS("Scripts"));

	// Editor scripts (general)
	auto editor_scripts = tree_scripts_->AppendItem(root, wxS("SLADE Editor Scripts"), 1);
	tree_scripts_->AppendItem(editor_scripts, wxS("Scratch Box"), 0, 0, new ScriptTreeItemData(&script_scratchbox_));
	for (auto& script : scriptmanager::editorScripts())
		tree_scripts_->AppendItem(
			getOrCreateNode(tree_scripts_, editor_scripts, script->path),
			wxString::FromUTF8(script->name),
			0,
			0,
			new ScriptTreeItemData(script.get()));

	// Editor scripts
	addEditorScriptsNode(editor_scripts, ScriptType::Custom, wxS("Custom Scripts"));
	addEditorScriptsNode(editor_scripts, ScriptType::Archive, wxS("Archive Scripts"));
	addEditorScriptsNode(editor_scripts, ScriptType::Entry, wxS("Entry Scripts"));
	addEditorScriptsNode(editor_scripts, ScriptType::Map, wxS("Map Editor Scripts"));

	// Expand editor scripts node initially
	tree_scripts_->Expand(editor_scripts);
}

// -----------------------------------------------------------------------------
// Returns the currently open/focused ScriptPanel
// -----------------------------------------------------------------------------
ScriptPanel* ScriptManagerWindow::currentPage() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page && page->GetName() == wxS("script"))
		return dynamic_cast<ScriptPanel*>(page);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Closes the tab for [script] if it is currently open
// -----------------------------------------------------------------------------
void ScriptManagerWindow::closeScriptTab(scriptmanager::Script* script) const
{
	// Find existing tab
	for (unsigned a = 0; a < tabs_scripts_->GetPageCount(); a++)
	{
		auto page = tabs_scripts_->GetPage(a);
		if (page->GetName() == wxS("script"))
			if (dynamic_cast<ScriptPanel*>(page)->script() == script)
			{
				tabs_scripts_->RemovePage(a);
				return;
			}
	}
}

// -----------------------------------------------------------------------------
// Shows the scripting documentation tab or creates it if it isn't currently
// open. If [url] is specified, navigates to <scripting docs url>/[url]
// -----------------------------------------------------------------------------
void ScriptManagerWindow::showDocs(const string& url)
{
#ifdef USE_WEBVIEW_STARTPAGE

	// Find existing tab
	bool found = false;
	for (unsigned a = 0; a < tabs_scripts_->GetPageCount(); a++)
	{
		auto page = tabs_scripts_->GetPage(a);
		if (page->GetName() == wxS("docs"))
		{
			tabs_scripts_->SetSelection(a);
			found = true;
			break;
		}
	}

	if (!found)
	{
		// Tab not open, create it
		webview_docs_ = wxWebView::New(this, -1, wxEmptyString);
		webview_docs_->SetName(wxS("docs"));

		// Bind HTML link click event
		webview_docs_->Bind(
			wxEVT_WEBVIEW_NAVIGATING,
			[&](wxEvent& e)
			{
				auto& ev   = dynamic_cast<wxWebViewEvent&>(e);
				auto  href = ev.GetURL();

				// Open external links externally
				if (!href.StartsWith(wxString::FromUTF8(docs_url)))
				{
					wxLaunchDefaultBrowser(href);
					ev.Veto();
				}
			});

		tabs_scripts_->AddPage(
			webview_docs_, wxS("Scripting Documentation"), true, icons::getIcon(icons::General, "wiki"));
	}

	// Load page if set
	if (!found || !url.empty())
		webview_docs_->LoadURL(wxString::FromUTF8(docs_url + "/" + url));

#endif
}

// -----------------------------------------------------------------------------
// Opens the tab for [script], or creates a new tab for it if needed
// -----------------------------------------------------------------------------
void ScriptManagerWindow::openScriptTab(scriptmanager::Script* script) const
{
	// Find existing tab
	for (unsigned a = 0; a < tabs_scripts_->GetPageCount(); a++)
	{
		auto page = tabs_scripts_->GetPage(a);
		if (page->GetName() == wxS("script"))
			if (dynamic_cast<ScriptPanel*>(page)->script() == script)
			{
				tabs_scripts_->ChangeSelection(a);
				return;
			}
	}

	// Not found, create new tab for script
	tabs_scripts_->AddPage(
		new ScriptPanel(tabs_scripts_, script),
		wxString::FromUTF8(script->name.empty() ? "UNSAVED" : script->name),
		true,
		icons::getIcon(icons::Entry, "code"));
	tabs_scripts_->Layout();
}

// -----------------------------------------------------------------------------
// Returns the currently open/focused script, or nullptr if none are open
// -----------------------------------------------------------------------------
scriptmanager::Script* ScriptManagerWindow::currentScript() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page && page->GetName() == wxS("script"))
		return dynamic_cast<ScriptPanel*>(page)->script();

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the currently open/focused script text
// -----------------------------------------------------------------------------
string ScriptManagerWindow::currentScriptText() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page->GetName() == wxS("script"))
		return dynamic_cast<ScriptPanel*>(page)->currentText();

	return {};
}

// -----------------------------------------------------------------------------
// Handles the SAction [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool ScriptManagerWindow::handleAction(string_view id)
{
	using namespace scriptmanager;

	// We're only interested in "scrm_" actions
	if (!strutil::startsWith(id, "scrm_"))
		return false;

	// Send to current ScriptPanel first
	auto current = currentPage();
	if (current && current->handleAction(id))
		return true;

	// File->New Editor Script
	if (id == "scrm_newscript_editor")
	{
		NewEditorScriptDialog dlg(this);
		dlg.CenterOnParent();
		if (dlg.ShowModal() == wxID_OK)
		{
			auto name = dlg.selectedName();
			auto type = dlg.selectedType();

			if (!name.empty())
			{
				auto script = scriptmanager::createEditorScript(name, type);
				populateEditorScriptsTree(type);
				openScriptTab(script);

				if (type == ScriptType::Map)
					mapeditor::window()->reloadScriptsMenu();
			}
		}

		return true;
	}

	// File->Close
	if (id == "scrm_close")
	{
		auto script = currentScript();

		if (script)
		{
			closeScriptTab(script);
		}
		else
		{
			wxWindow::Close();
		}

		return true;
	}

	// Script->Run
	if (id == "scrm_run")
	{
		lua::setCurrentWindow(this);

		if (!lua::run(script_clicked_ ? script_clicked_->text : currentScriptText()))
			lua::showErrorDialog();

		script_clicked_ = nullptr;

		return true;
	}

	// Script->Rename
	if (id == "scrm_rename")
	{
		auto script = script_clicked_ ? script_clicked_ : currentScript();

		if (script)
		{
			auto name = wxGetTextFromUser(
				wxS("Enter a new name for the script"), wxS("Rename Script"), wxString::FromUTF8(script->name));

			if (!name.empty())
			{
				scriptmanager::renameScript(script, name.utf8_string());
				populateEditorScriptsTree(script->type);
			}
		}

		script_clicked_ = nullptr;

		return true;
	}

	// Script->Delete
	if (id == "scrm_delete")
	{
		auto script = script_clicked_ ? script_clicked_ : currentScript();

		if (script)
		{
			if (scriptmanager::deleteScript(script))
			{
				closeScriptTab(script);
				populateEditorScriptsTree(script->type);
			}
		}

		script_clicked_ = nullptr;

		return true;
	}

	// View->Scripts
	if (id == "scrm_showscripts")
	{
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane(wxS("scripts_area"));
		p_inf.Show(!p_inf.IsShown());
		m_mgr->Update();
		return true;
	}

	// View->Console
	if (id == "scrm_showconsole")
	{
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane(wxS("console"));
		p_inf.Show(!p_inf.IsShown());
		p_inf.MinSize(200, 128);
		dynamic_cast<ConsolePanel*>(p_inf.window)->focusInput();
		m_mgr->Update();
		return true;
	}

	// View->Documentation
	if (id == "scrm_showdocs")
	{
		showDocs();
		return true;
	}

	return false;
}
