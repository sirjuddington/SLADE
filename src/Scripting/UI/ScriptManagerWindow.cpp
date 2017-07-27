
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
#include "ScriptManagerWindow.h"
#include "App.h"
#include "Utility/Tokenizer.h"
#include "UI/SAuiTabArt.h"
#include "Archive/ArchiveManager.h"
#include "UI/ConsolePanel.h"
#include "UI/TextEditor/TextEditor.h"
#include "Scripting/Lua.h"
#include "General/Misc.h"
#include "General/SAction.h"
#include "UI/SToolBar/SToolBar.h"
#include "Dialogs/ExtMessageDialog.h"


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
CVAR(Bool, sm_maximized, false, CVAR_SAVE)


// ----------------------------------------------------------------------------
// ScriptTreeItemData Class
//
// Just used to store ArchiveEntry pointers with wxTreeCtrl items
// ----------------------------------------------------------------------------
class ScriptTreeItemData : public wxTreeItemData
{
public:
	ScriptTreeItemData(ArchiveEntry* entry) : entry{ entry } {}
	ArchiveEntry* entry;
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
ScriptManagerWindow::ScriptManagerWindow() : STopWindow("SLADE Script Manager", "scriptmanager")
{
	setupLayout();
}

// ----------------------------------------------------------------------------
// ScriptManagerWindow::loadLayout
//
// Loads the previously saved layout file for the window
// ----------------------------------------------------------------------------
void ScriptManagerWindow::loadLayout()
{
	// Open layout file
	Tokenizer tz;
	if (!tz.openFile(App::path("scriptmanager.layout", App::Dir::User)))
		return;

	// Parse layout
	auto m_mgr = wxAuiManager::GetManager(this);
	while (true)
	{
		// Read component+layout pair
		string component = tz.getToken();
		string layout = tz.getToken();

		// Load layout to component
		if (!component.IsEmpty() && !layout.IsEmpty())
			m_mgr->LoadPaneInfo(layout, m_mgr->GetPane(component));

		// Check if we're done
		if (tz.peekToken().IsEmpty())
			break;
	}
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
	auto m_mgr = wxAuiManager::GetManager(this);

	// Console pane
	file.Write("\"console\" ");
	auto pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("console"));
	file.Write(S_FMT("\"%s\"\n", pinf));

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

	// Scripts Tree
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND | wxALL, 10);
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
	populateScriptsTree();
	hbox->Add(tree_scripts_, 0, wxEXPAND | wxRIGHT, 10);

	// Text Editor
	text_editor_ = new TextEditor(panel, -1);
	text_editor_->setLanguage(TextLanguage::getLanguage("sladescript"));
	hbox->Add(text_editor_, 1, wxEXPAND);

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

	// Script menu
	auto scriptMenu = new wxMenu("");
	SAction::fromId("scrm_run")->addToMenu(scriptMenu);
	menu->Append(scriptMenu, "&Script");

	// View menu
	auto viewMenu = new wxMenu("");
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
		if (data && data->entry)
			text_editor_->loadEntry(data->entry);
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
// ScriptManagerWindow::populateScriptsTree
//
// Loads scripts from slade.pk3 into the scripts tree control
// ----------------------------------------------------------------------------
void ScriptManagerWindow::populateScriptsTree()
{
	// Clear tree
	tree_scripts_->DeleteAllItems();

	// Get 'scripts' dir of slade.pk3
	auto scripts_dir = App::archiveManager().programResourceArchive()->getDir("scripts");
	if (!scripts_dir)
		return;

	// Recursive function to populate the tree
	std::function<void(wxTreeCtrl*, wxTreeItemId, ArchiveTreeNode*)> addToTree =
		[&](wxTreeCtrl* tree, wxTreeItemId node, ArchiveTreeNode* dir)
	{
		// Add subdirs
		for (unsigned a = 0; a < dir->nChildren(); a++)
		{
			auto subdir = (ArchiveTreeNode*)dir->getChild(a);
			auto subnode = tree->AppendItem(node, subdir->getName());
			addToTree(tree, subnode, subdir);
		}

		// Add script files
		for (unsigned a = 0; a < dir->numEntries(); a++)
		{
			auto entry = dir->entryAt(a);
			if (entry->getName(true) != "init")
				tree->AppendItem(node, entry->getName(true), -1, -1, new ScriptTreeItemData(entry));
		}
	};

	// Populate from root
	auto root = tree_scripts_->AddRoot("Scripts");
	addToTree(tree_scripts_, root, scripts_dir);
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

	// Script->Run
	if (id == "scrm_run")
	{
		Lua::setCurrentWindow(this);
		auto start_time = wxDateTime::Now().GetTicks();
		if (!Lua::run(text_editor_->GetText()))
		{
			auto log = Log::since(start_time, Log::MessageType::Script);
			string output;
			for (auto msg : log)
				output += msg->formattedMessageLine() + "\n";

			ExtMessageDialog dlg(this, "Script Error");
			dlg.setMessage("An error occurred running the script, see details below");
			dlg.setExt(S_FMT(
				"%s Error\nLine %d: %s\n\nScript Output:\n%s",
				CHR(Lua::error().type),
				Lua::error().line_no,
				CHR(Lua::error().message),
				CHR(output)
			));
			dlg.CenterOnParent();
			dlg.ShowModal();

			auto m_mgr = wxAuiManager::GetManager(this);
			auto& p_inf = m_mgr->GetPane("console");
			p_inf.Show(true);
			p_inf.MinSize(200, 128);
			m_mgr->Update();
		}

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
