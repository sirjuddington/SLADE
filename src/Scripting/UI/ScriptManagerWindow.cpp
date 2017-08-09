
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
#include "General/SAction.h"
#include "Graphics/Icons.h"
#include "Scripting/Lua.h"
#include "Scripting/ScriptManager.h"
#include "ScriptManagerWindow.h"
#include "ScriptPanel.h"
#include "UI/ConsolePanel.h"
#include "UI/SAuiTabArt.h"
#include "UI/STabCtrl.h"
#include "UI/SToolBar/SToolBar.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
CVAR(Bool, sm_maximized, false, CVAR_SAVE)


// ----------------------------------------------------------------------------
//
// Functions
//
// ----------------------------------------------------------------------------
namespace
{

// ----------------------------------------------------------------------------
// getOrCreateNode
//
// Returns a new or existing wxTreeItemId for [tree], at [path] from
// [parent_node]. Creates any required nodes along the way.
// ----------------------------------------------------------------------------
wxTreeItemId getOrCreateNode(wxTreeCtrl* tree, wxTreeItemId parent_node, const string& path)
{
	string path_rest;
	string name = path.BeforeFirst('/', &path_rest);

	// Find child node with name
	wxTreeItemIdValue cookie;
	wxTreeItemId child = tree->GetFirstChild(parent_node, cookie);
	while (child.IsOk())
	{
		if (S_CMPNOCASE(tree->GetItemText(child), name))
			break;

		child = tree->GetNextSibling(child);
	}

	// Not found, create child node
	if (!child.IsOk())
		child = tree->AppendItem(parent_node, name, 1);

	// Return it or go deeper into the tree
	if (path_rest.empty())
		return child;
	else
		return getOrCreateNode(tree, child, path_rest);
}

// ----------------------------------------------------------------------------
// createTreeImageList
//
// Creates the wxImageList to use for the script tree control
// ----------------------------------------------------------------------------
wxImageList* createTreeImageList()
{
	auto image_list = new wxImageList(16, 16, false, 0);
	image_list->Add(Icons::getIcon(Icons::ENTRY, "text"));
	image_list->Add(Icons::getIcon(Icons::ENTRY, "folder"));
	return image_list;
}

} // namespace (anonymous)


// ----------------------------------------------------------------------------
// ScriptTreeItemData Class
//
// Just used to store Script pointers with wxTreeCtrl items
// ----------------------------------------------------------------------------
class ScriptTreeItemData : public wxTreeItemData
{
public:
	ScriptTreeItemData(ScriptManager::Script* script) : script{ script } {}
	ScriptManager::Script* script;
};


// ----------------------------------------------------------------------------
//
// ScriptManagerWindow Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ScriptManagerWindow::ScriptManagerWindow
//
// ScriptManagerWindow class constructor
// ----------------------------------------------------------------------------
ScriptManagerWindow::ScriptManagerWindow() :
	STopWindow("SLADE Script Manager", "scriptmanager")
{
	setupLayout();

	// Open 'scratch box' initially
	script_scratchbox_.name = "Scratch Box";
	script_scratchbox_.text =
		"-- Use this script to write ad-hoc SLADE editor scripts\n"
		"-- Note that this will not be saved between sessions\n\n";
	script_scratchbox_.read_only = true;
	openScriptTab(&script_scratchbox_);

	// Bind events
	tabs_scripts_->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, [&](wxAuiNotebookEvent& e)
	{
		auto page = currentPage();
		if (page && !page->close())
			e.Veto();
	});
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::loadLayout
//
// Loads the previously saved layout file for the window
// ----------------------------------------------------------------------------
void ScriptManagerWindow::loadLayout()
{
	// Open layout file
	wxFile file(App::path("scriptmanager.layout", App::Dir::User), wxFile::read);

	// Read component layout
	if (file.IsOpened())
	{
		string layout;
		file.ReadAll(&layout);
		wxAuiManager::GetManager(this)->LoadPerspective(layout);
	}

	// Close file
	file.Close();
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::saveLayout
//
// Saves the current window layout to a file
// ----------------------------------------------------------------------------
void ScriptManagerWindow::saveLayout()
{
	// Open layout file
	wxFile file(App::path("scriptmanager.layout", App::Dir::User), wxFile::write);

	// Write component layout
	file.Write(wxAuiManager::GetManager(this)->SavePerspective());

	// Close file
	file.Close();
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::setupLayout
//
// Sets up the wxWidgets window layout
// ----------------------------------------------------------------------------
void ScriptManagerWindow::setupLayout()
{
	// Maximize if it was last time
	if (sm_maximized)
		Maximize();

	// Create the wxAUI manager & related things
	auto m_mgr = new wxAuiManager(this);
	m_mgr->SetArtProvider(new SAuiDockArt());
	wxAuiPaneInfo p_inf;

	// Set icon
	auto icon_filename = App::path("slade.ico", App::Dir::Temp);
	App::archiveManager().programResourceArchive()->getEntry("slade.ico")->exportFile(icon_filename);
	SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
	wxRemoveFile(icon_filename);

	// -- Main Panel --
	p_inf.CenterPane();
	p_inf.Name("editor_area");
	p_inf.PaneBorder(false);
	m_mgr->AddPane(setupMainArea(), p_inf);

	// -- Scripts Panel --
	p_inf.DefaultPane();
	p_inf.Left();
	p_inf.BestSize(256, 480);
	p_inf.Caption("Scripts");
	p_inf.Name("scripts_area");
	p_inf.Show(true);
	p_inf.Dock();
	m_mgr->AddPane(setupScriptTreePanel(), p_inf);

	// -- Console Panel --
	auto panel_console = new ConsolePanel(this, -1);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Float();
	p_inf.FloatingSize(600, 400);
	p_inf.FloatingPosition(100, 100);
	p_inf.MinSize(-1, 192);
	p_inf.Show(false);
	p_inf.Caption("Console");
	p_inf.Name("console");
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

// ----------------------------------------------------------------------------
// ScriptManagerWindow::setupMainArea
//
// Creates and returns the script manager 'main' area as a wxPanel
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// ScriptManagerWindow::setupMenu
//
// Sets up the script manager window menu
// ----------------------------------------------------------------------------
void ScriptManagerWindow::setupMenu()
{
	// -- Menu bar --
	auto menu = new wxMenuBar();
	menu->SetThemeEnabled(false);

	// File menu
	auto fileMenu = new wxMenu();
	auto newMenu = new wxMenu();
	SAction::fromId("scrm_newscript_custom")->addToMenu(newMenu, true, "&Custom Script");
	SAction::fromId("scrm_newscript_archive")->addToMenu(newMenu, true, "&Archive Script");
	fileMenu->AppendSubMenu(newMenu, "&New");
	menu->Append(fileMenu, "&File");

	// Script menu
	auto scriptMenu = new wxMenu();
	SAction::fromId("scrm_run")->addToMenu(scriptMenu);
	SAction::fromId("scrm_save")->addToMenu(scriptMenu);
	menu->Append(scriptMenu, "&Script");

	// Text menu
	auto textMenu = new wxMenu();
	SAction::fromId("scrm_find_replace")->addToMenu(textMenu);
	SAction::fromId("scrm_jump_to_line")->addToMenu(textMenu);
	wxMenu* menu_fold = new wxMenu();
	textMenu->AppendSubMenu(menu_fold, "Code Folding");
	SAction::fromId("scrm_fold_foldall")->addToMenu(menu_fold);
	SAction::fromId("scrm_fold_unfoldall")->addToMenu(menu_fold);
	textMenu->AppendSeparator();
	SAction::fromId("scrm_wrap")->addToMenu(textMenu);
	menu->Append(textMenu, "&Text");

	// View menu
	auto viewMenu = new wxMenu();
	SAction::fromId("scrm_showscripts")->addToMenu(viewMenu);
	SAction::fromId("scrm_showconsole")->addToMenu(viewMenu);
	menu->Append(viewMenu, "&View");

	// Set the menu
	SetMenuBar(menu);
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::setupToolbar
//
// Sets up the script manager window toolbar
// ----------------------------------------------------------------------------
void ScriptManagerWindow::setupToolbar()
{
	toolbar = new SToolBar(this, true);

	// Create File toolbar
	auto tbg_script = new SToolBarGroup(toolbar, "_Script");
	tbg_script->addActionButton("scrm_run");
	tbg_script->addActionButton("scrm_save");
	toolbar->addGroup(tbg_script);

	// Add toolbar
	wxAuiManager::GetManager(this)->AddPane(
		toolbar,
		wxAuiPaneInfo()
			.Top()
			.CaptionVisible(false)
			.MinSize(-1, SToolBar::getBarHeight())
			.Resizable(false)
			.PaneBorder(false)
			.Name("toolbar")
	);
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::bindEvents
//
// Bind events for wx controls
// ----------------------------------------------------------------------------
void ScriptManagerWindow::bindEvents()
{
	// Tree item activate
	tree_scripts_->Bind(wxEVT_TREE_ITEM_ACTIVATED, [=](wxTreeEvent e)
	{
		auto data = (ScriptTreeItemData*)tree_scripts_->GetItemData(e.GetItem());
		if (data && data->script)
			openScriptTab(data->script);
			//text_editor_->SetText(data->script->text);
		else if (tree_scripts_->ItemHasChildren(e.GetItem()))
			tree_scripts_->Toggle(e.GetItem());
	});

	// Window close
	Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent e)
	{
		// Save Layout
		saveLayout();
		sm_maximized = IsMaximized();
		if (!IsMaximized())
			Misc::setWindowInfo(id, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);

		// Hide
		Show(false);
	});
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::setupScriptTreePanel
//
// Creates and returns the script tree area as a wxPanel
// ----------------------------------------------------------------------------
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
		{ 200, -1 },
		wxTR_DEFAULT_STYLE | wxTR_NO_LINES | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT
	);
#if wxMAJOR_VERSION > 3 || (wxMAJOR_VERSION == 3 && wxMINOR_VERSION >= 1)
	tree_scripts_->EnableSystemTheme(true);
#endif
	tree_scripts_->SetImageList(createTreeImageList());
	populateScriptsTree();
	sizer->Add(tree_scripts_, 1, wxEXPAND | wxALL, 10);

	return panel;
}

void ScriptManagerWindow::populateCustomScripts(wxTreeItemId tree_node)
{
	static wxTreeItemId custom_scripts;
	if (tree_node.IsOk())
		custom_scripts = tree_node;

	tree_scripts_->DeleteChildren(custom_scripts);
	for (auto& script : ScriptManager::customScripts())
		tree_scripts_->AppendItem(
			custom_scripts,
			script->name,
			0,
			0,
			new ScriptTreeItemData(script.get())
		);
}

void ScriptManagerWindow::populateArchiveScripts(wxTreeItemId tree_node)
{
	static wxTreeItemId archive_scripts;
	if (tree_node.IsOk())
		archive_scripts = tree_node;

	tree_scripts_->DeleteChildren(archive_scripts);
	for (auto& script : ScriptManager::archiveScripts())
		tree_scripts_->AppendItem(
			archive_scripts,
			script->name.empty() ? "UNSAVED" : script->name,
			0,
			0,
			new ScriptTreeItemData(script.get())
		);
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::populateScriptsTree
//
// Loads scripts from slade.pk3 into the scripts tree control
// ----------------------------------------------------------------------------
void ScriptManagerWindow::populateScriptsTree()
{
	// Clear tree
	tree_scripts_->DeleteAllItems();

	// Populate it
	auto root = tree_scripts_->AddRoot("Scripts");

	// Editor scripts (general)
	auto editor_scripts = tree_scripts_->AppendItem(root, "SLADE Editor Scripts", 1);
	tree_scripts_->AppendItem(editor_scripts, "Scratch Box", 0, 0, new ScriptTreeItemData(&script_scratchbox_));
	for (auto& script : ScriptManager::editorScripts())
		tree_scripts_->AppendItem(
			getOrCreateNode(tree_scripts_, editor_scripts, script->path),
			script->name,
			0,
			0,
			new ScriptTreeItemData(script.get())
		);

	// Custom scripts
	auto custom_scripts = tree_scripts_->AppendItem(editor_scripts, "Custom Scripts", 1);
	populateCustomScripts(custom_scripts);

	// Global scripts
	auto global_scripts = tree_scripts_->AppendItem(editor_scripts, "Global Scripts", 1);

	// Archive scripts
	auto archive_scripts = tree_scripts_->AppendItem(editor_scripts, "Archive Scripts", 1);
	populateArchiveScripts(archive_scripts);

	// Entry scripts
	auto entry_scripts = tree_scripts_->AppendItem(editor_scripts, "Entry Scripts", 1);

	// Expand editor scripts initially
	tree_scripts_->Expand(editor_scripts);
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::currentPage
//
// Returns the currently open/focused ScriptPanel
// ----------------------------------------------------------------------------
ScriptPanel* ScriptManagerWindow::currentPage() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page->GetName() == "script")
		return (ScriptPanel*)page;

	return nullptr;
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::openScriptTab
//
// Opens the tab for [script], or creates a new tab for it if needed
// ----------------------------------------------------------------------------
void ScriptManagerWindow::openScriptTab(ScriptManager::Script* script) const
{
	// Find existing tab
	for (unsigned a = 0; a < tabs_scripts_->GetPageCount(); a++)
	{
		auto page = tabs_scripts_->GetPage(a);
		if (page->GetName() == "script")
			if (((ScriptPanel*)page)->script() == script)
			{
				tabs_scripts_->ChangeSelection(a);
				return;
			}
	}

	// Not found, create new tab for script
	tabs_scripts_->AddPage(
		new ScriptPanel(tabs_scripts_, script),
		script->name.empty() ? "UNSAVED" : script->name,
		true,
		Icons::getIcon(Icons::ENTRY, "text")
	);
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::currentScript
//
// Returns the currently open/focused script, or nullptr if none are open
// ----------------------------------------------------------------------------
ScriptManager::Script* ScriptManagerWindow::currentScript() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page->GetName() == "script")
		return ((ScriptPanel*)page)->script();

	return nullptr;
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::currentScriptText
//
// Returns the currently open/focused script text
// ----------------------------------------------------------------------------
string ScriptManagerWindow::currentScriptText() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page->GetName() == "script")
		return ((ScriptPanel*)page)->currentText();

	return wxEmptyString;
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::handleAction
//
// Handles the SAction [id].
// Returns true if the action was handled, false otherwise
// ----------------------------------------------------------------------------
bool ScriptManagerWindow::handleAction(string id)
{
	// We're only interested in "scrm_" actions
	if (!id.StartsWith("scrm_"))
		return false;

	// Send to current ScriptPanel first
	auto current = currentPage();
	if (current && current->handleAction(id))
		return true;

	// File->New->Custom Script
	if (id == "scrm_newscript_custom")
	{
		string name = wxGetTextFromUser("Enter a name for the script", "New Custom Script");
		if (!name.empty())
		{
			auto script = ScriptManager::createCustomScript(name);
			populateCustomScripts();
			openScriptTab(script);
		}
		return true;
	}

	// File->New->Archive Script
	if (id == "scrm_newscript_archive")
	{
		string name = wxGetTextFromUser("Enter a name for the script", "New Archive Script");
		if (!name.empty())
		{
			auto script = ScriptManager::createArchiveScript(name);
			populateArchiveScripts();
			openScriptTab(script);
		}
		return true;
	}

	// Script->Run
	if (id == "scrm_run")
	{
		Lua::setCurrentWindow(this);
		if (!Lua::run(currentScriptText()))
			Lua::showErrorDialog();

		return true;
	}

	// View->Scripts
	if (id == "scrm_showscripts")
	{
		auto m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("scripts_area");
		p_inf.Show(!p_inf.IsShown());
		m_mgr->Update();
		return true;
	}

	// View->Console
	if (id == "scrm_showconsole")
	{
		auto m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("console");
		p_inf.Show(!p_inf.IsShown());
		p_inf.MinSize(200, 128);
		m_mgr->Update();
		return true;
	}

	return false;
}
