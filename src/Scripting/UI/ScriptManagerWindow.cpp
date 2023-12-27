
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
#include "Archive/ArchiveManager.h"
#include "General/Misc.h"
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
wxString docs_url       = "https://slade.readthedocs.io/en/latest";
int      layout_version = 1;
} // namespace

CVAR(Bool, sm_maximized, false, CVar::Flag::Save)


// -----------------------------------------------------------------------------
// NewEditorScriptDialog Class
//
// A simple dialog showing a dropdown to select an editor script type and a
// text box to enter a name for the script
// -----------------------------------------------------------------------------
class NewEditorScriptDialog : public wxDialog
{
public:
	NewEditorScriptDialog(wxWindow* parent) : wxDialog(parent, -1, "New Editor Script")
	{
		auto sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		auto gbsizer = new wxGridBagSizer(ui::pad(), ui::pad());
		sizer->Add(gbsizer, 1, wxEXPAND | wxALL, ui::padLarge());
		gbsizer->AddGrowableCol(1, 1);

		// Script type
		wxString types[] = { "Custom", "Archive", "Entry", "Map Editor" };
		choice_type_     = new wxChoice(this, -1, { -1, -1 }, { -1, -1 }, 4, types);
		choice_type_->SetSelection(0);
		gbsizer->Add(new wxStaticText(this, -1, "Type:"), { 0, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gbsizer->Add(choice_type_, { 0, 1 }, { 1, 1 }, wxEXPAND);

		// Script name
		text_name_ = new wxTextCtrl(this, -1, wxEmptyString, { -1, -1 }, { 200, -1 }, wxTE_PROCESS_ENTER);
		gbsizer->Add(new wxStaticText(this, -1, "Name:"), { 1, 0 }, { 1, 1 }, wxALIGN_CENTER_VERTICAL);
		gbsizer->Add(text_name_, { 1, 1 }, { 1, 1 }, wxEXPAND);
		text_name_->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& e) { EndModal(wxID_OK); });

		// Dialog buttons
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, ui::padLarge());
		hbox->AddStretchSpacer(1);

		// OK
		hbox->Add(new wxButton(this, wxID_OK, "OK"), 0, wxEXPAND | wxRIGHT, ui::padLarge());

		SetEscapeId(wxID_CANCEL);
		wxWindowBase::Layout();
		sizer->Fit(this);
	}

	scriptmanager::ScriptType selectedType() const
	{
		switch (choice_type_->GetCurrentSelection())
		{
		case 1: return scriptmanager::ScriptType::Archive;
		case 2: return scriptmanager::ScriptType::Entry;
		case 3: return scriptmanager::ScriptType::Map;
		default: return scriptmanager::ScriptType::Custom;
		}
	}

	wxString selectedName() const { return text_name_->GetValue(); }

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
wxTreeItemId getOrCreateNode(wxTreeCtrl* tree, wxTreeItemId parent_node, const wxString& path)
{
	wxString path_rest;
	wxString name = path.BeforeFirst('/', &path_rest);

	// Find child node with name
	wxTreeItemIdValue cookie;
	auto              child = tree->GetFirstChild(parent_node, cookie);
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
	// Open layout file
	wxFile file(app::path("scriptmanager.layout", app::Dir::User), wxFile::read);

	// Read component layout
	if (file.IsOpened())
	{
		wxString text, layout;
		file.ReadAll(&text);

		// Get layout version
		wxString version = text.BeforeFirst('\n', &layout);

		// Check version
		long val;
		if (version.ToLong(&val))
		{
			// Load layout only if correct version
			if (val == layout_version)
				wxAuiManager::GetManager(this)->LoadPerspective(layout);
		}
	}

	// Close file
	file.Close();

	// Force calculated toolbar size
	wxAuiManager::GetManager(this)->GetPane("toolbar").MinSize(-1, SToolBar::getBarHeight());
}

// -----------------------------------------------------------------------------
// Saves the current window layout to a file
// -----------------------------------------------------------------------------
void ScriptManagerWindow::saveLayout()
{
	// Open layout file
	wxFile file(app::path("scriptmanager.layout", app::Dir::User), wxFile::write);

	// Write component layout
	file.Write(wxString::Format("%d\n", layout_version));
	file.Write(wxAuiManager::GetManager(this)->SavePerspective());

	// Close file
	file.Close();
}

// -----------------------------------------------------------------------------
// Sets up the wxWidgets window layout
// -----------------------------------------------------------------------------
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
	auto icon_filename = app::path(app::iconFile(), app::Dir::Temp);
	app::archiveManager().programResourceArchive()->entry(app::iconFile())->exportFile(icon_filename);
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
	p_inf.BestSize(wxutil::scaledSize(256, 480));
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
	p_inf.FloatingSize(wxutil::scaledSize(600, 400));
	p_inf.FloatingPosition(100, 100);
	p_inf.MinSize(wxutil::scaledSize(-1, 192));
	p_inf.Show(false);
	p_inf.Caption("Console");
	p_inf.Name("console");
	m_mgr->AddPane(panel_console, p_inf);

	// Setup menu and toolbar
	setupMenu();
	setupToolbar();

	// Status Bar
	CreateStatusBar(3);

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
	menu->Append(file_menu, "&File");

	// Script menu
	auto script_menu = new wxMenu();
	SAction::fromId("scrm_run")->addToMenu(script_menu);
	SAction::fromId("scrm_save")->addToMenu(script_menu);
	// SAction::fromId("scrm_rename")->addToMenu(scriptMenu);
	// SAction::fromId("scrm_delete")->addToMenu(scriptMenu);
	menu->Append(script_menu, "&Script");

	// Text menu
	auto text_menu = new wxMenu();
	SAction::fromId("scrm_find_replace")->addToMenu(text_menu);
	SAction::fromId("scrm_jump_to_line")->addToMenu(text_menu);
	auto menu_fold = new wxMenu();
	text_menu->AppendSubMenu(menu_fold, "Code Folding");
	SAction::fromId("scrm_fold_foldall")->addToMenu(menu_fold);
	SAction::fromId("scrm_fold_unfoldall")->addToMenu(menu_fold);
	text_menu->AppendSeparator();
	SAction::fromId("scrm_wrap")->addToMenu(text_menu);
	menu->Append(text_menu, "&Text");

	// View menu
	auto view_menu = new wxMenu();
	SAction::fromId("scrm_showscripts")->addToMenu(view_menu);
	SAction::fromId("scrm_showconsole")->addToMenu(view_menu);
	menu->Append(view_menu, "&View");

	// Help menu
	auto help_menu = new wxMenu();
	SAction::fromId("scrm_showdocs")->addToMenu(help_menu);
	menu->Append(help_menu, "&Help");

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
			.Name("toolbar"));
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
			sm_maximized      = IsMaximized();
			const wxSize size = GetSize() * GetContentScaleFactor();
			if (!IsMaximized())
				misc::setWindowInfo(
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
	tree_scripts_->EnableSystemTheme(true);
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
		tree_scripts_->AppendItem(editor_script_nodes_[type], script->name, 0, 0, new ScriptTreeItemData(script.get()));
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
	auto root = tree_scripts_->AddRoot("Scripts");

	// Editor scripts (general)
	auto editor_scripts = tree_scripts_->AppendItem(root, "SLADE Editor Scripts", 1);
	tree_scripts_->AppendItem(editor_scripts, "Scratch Box", 0, 0, new ScriptTreeItemData(&script_scratchbox_));
	for (auto& script : scriptmanager::editorScripts())
		tree_scripts_->AppendItem(
			getOrCreateNode(tree_scripts_, editor_scripts, script->path),
			script->name,
			0,
			0,
			new ScriptTreeItemData(script.get()));

	// Editor scripts
	addEditorScriptsNode(editor_scripts, ScriptType::Custom, "Custom Scripts");
	addEditorScriptsNode(editor_scripts, ScriptType::Archive, "Archive Scripts");
	addEditorScriptsNode(editor_scripts, ScriptType::Entry, "Entry Scripts");
	addEditorScriptsNode(editor_scripts, ScriptType::Map, "Map Editor Scripts");

	// Expand editor scripts node initially
	tree_scripts_->Expand(editor_scripts);
}

// -----------------------------------------------------------------------------
// Returns the currently open/focused ScriptPanel
// -----------------------------------------------------------------------------
ScriptPanel* ScriptManagerWindow::currentPage() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page && page->GetName() == "script")
		return dynamic_cast<ScriptPanel*>(page);

	return nullptr;
}

// -----------------------------------------------------------------------------
// Closes the tab for [script] if it is currently open
// -----------------------------------------------------------------------------
void ScriptManagerWindow::closeScriptTab(const scriptmanager::Script* script) const
{
	// Find existing tab
	for (unsigned a = 0; a < tabs_scripts_->GetPageCount(); a++)
	{
		auto page = tabs_scripts_->GetPage(a);
		if (page->GetName() == "script")
			if (dynamic_cast<ScriptPanel*>(page)->script() == script)
			{
				tabs_scripts_->RemovePage(a);
				return;
			}
	}
}

// -----------------------------------------------------------------------------
// Opens the scripting documentation in the default browser.
// If [url] is specified, navigates to <scripting docs url>/[url]
// -----------------------------------------------------------------------------
void ScriptManagerWindow::showDocs(const wxString& url)
{
	wxLaunchDefaultBrowser(docs_url + "/" + url);
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
		if (page->GetName() == "script")
			if (dynamic_cast<ScriptPanel*>(page)->script() == script)
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
		icons::getIcon(icons::Entry, "code"));
	tabs_scripts_->Layout();
}

// -----------------------------------------------------------------------------
// Returns the currently open/focused script, or nullptr if none are open
// -----------------------------------------------------------------------------
scriptmanager::Script* ScriptManagerWindow::currentScript() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page->GetName() == "script")
		return dynamic_cast<ScriptPanel*>(page)->script();

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the currently open/focused script text
// -----------------------------------------------------------------------------
string ScriptManagerWindow::currentScriptText() const
{
	auto page = tabs_scripts_->GetCurrentPage();
	if (page->GetName() == "script")
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

	// File->New->Editor Script
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
				auto script = scriptmanager::createEditorScript(wxutil::strToView(name), type);
				populateEditorScriptsTree(type);
				openScriptTab(script);

				if (type == ScriptType::Map)
					mapeditor::window()->reloadScriptsMenu();
			}
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
		if (auto script = script_clicked_ ? script_clicked_ : currentScript())
		{
			auto name = wxGetTextFromUser("Enter a new name for the script", "Rename Script", script->name);

			if (!name.empty())
			{
				scriptmanager::renameScript(script, wxutil::strToView(name));
				populateEditorScriptsTree(script->type);
			}
		}

		script_clicked_ = nullptr;

		return true;
	}

	// Script->Delete
	if (id == "scrm_delete")
	{
		if (auto script = script_clicked_ ? script_clicked_ : currentScript())
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
		auto& p_inf = m_mgr->GetPane("scripts_area");
		p_inf.Show(!p_inf.IsShown());
		m_mgr->Update();
		return true;
	}

	// View->Console
	if (id == "scrm_showconsole")
	{
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("console");
		p_inf.Show(!p_inf.IsShown());
		p_inf.MinSize(200, 128);
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
