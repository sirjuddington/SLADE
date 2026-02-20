
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MainWindow.cpp
// Description: MainWindow class, ie the main SLADE window
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
#include "MainWindow.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "ArchiveManagerPanel.h"
#include "ArchivePanel.h"
#include "General/SAction.h"
#include "Graphics/Icons.h"
#include "MapEditor/MapEditor.h"
#include "SLADEWxApp.h"
#include "Scripting/ScriptManager.h"
#include "StartPanel.h"
#include "UI/Canvas/GL/GLCanvas.h"
#include "UI/Controls/BaseResourceChooser.h"
#include "UI/Controls/ConsolePanel.h"
#include "UI/Controls/PaletteChooser.h"
#include "UI/Controls/STabCtrl.h"
#include "UI/Controls/UndoManagerHistoryPanel.h"
#include "UI/Dialogs/SettingsDialog.h"
#include "UI/SAuiTabArt.h"
#include "UI/SAuiToolBar.h"
#include "UI/State.h"
#include "UI/UI.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, show_start_page, true, CVar::Flag::Save);
CVAR(String, global_palette, "", CVar::Flag::Save);
CVAR(Bool, confirm_exit, true, CVar::Flag::Save);


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, tabs_condensed)


// -----------------------------------------------------------------------------
// MainWindowDropTarget Class
//
// Handles drag'n'drop of files on to the SLADE window
// -----------------------------------------------------------------------------
class MainWindowDropTarget : public wxFileDropTarget
{
public:
	MainWindowDropTarget()           = default;
	~MainWindowDropTarget() override = default;

	bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override
	{
		for (const auto& filename : filenames)
			app::archiveManager().openArchive(filename.utf8_string());

		return true;
	}
};


// -----------------------------------------------------------------------------
//
// MainWindow Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MainWindow class constructor
// -----------------------------------------------------------------------------
MainWindow::MainWindow() : STopWindow("SLADE", "main")
{
	custom_menus_begin_ = 2;

	if (ui::getStateBool(ui::MAINWINDOW_MAXIMIZED))
		MainWindow::Maximize();

	setupLayout();

	wxWindow::SetDropTarget(new MainWindowDropTarget());
}

// -----------------------------------------------------------------------------
// MainWindow class destructor
// -----------------------------------------------------------------------------
MainWindow::~MainWindow()
{
	aui_mgr_->UnInit();
}

// -----------------------------------------------------------------------------
// Loads the previously saved layout file for the window
// -----------------------------------------------------------------------------
void MainWindow::loadLayout() const
{
	auto layout = ui::getWindowLayout(id_.c_str());

	for (const auto& component : layout)
		if (!component.first.empty() && !component.second.empty())
			aui_mgr_->LoadPaneInfo(
				wxString::FromUTF8(component.second), aui_mgr_->GetPane(wxString::FromUTF8(component.first)));
}

// -----------------------------------------------------------------------------
// Saves the current window layout to a file
// -----------------------------------------------------------------------------
void MainWindow::saveLayout() const
{
	vector<StringPair> layout;

	layout.emplace_back("console", aui_mgr_->SavePaneInfo(aui_mgr_->GetPane(wxS("console"))).utf8_string());
	layout.emplace_back(
		"archive_manager", aui_mgr_->SavePaneInfo(aui_mgr_->GetPane(wxS("archive_manager"))).utf8_string());
	layout.emplace_back("undo_history", aui_mgr_->SavePaneInfo(aui_mgr_->GetPane(wxS("undo_history"))).utf8_string());

	ui::setWindowLayout(id_.c_str(), layout);
}

// -----------------------------------------------------------------------------
// Sets up the wxWidgets window layout
// -----------------------------------------------------------------------------
void MainWindow::setupLayout()
{
	// Create the wxAUI manager & related things
	aui_mgr_ = new wxAuiManager(this);
	aui_mgr_->SetArtProvider(new SAuiDockArt(this));
	wxAuiPaneInfo p_inf;

	// Set icon
	auto icon_filename = app::path(app::iconFile(), app::Dir::Temp);
	app::archiveManager().programResourceArchive()->entry(app::iconFile())->exportFile(icon_filename);
	SetIcon(wxIcon(wxString::FromUTF8(icon_filename), wxBITMAP_TYPE_ICO));
	fileutil::removeFile(icon_filename);


	// -- Editor Area --
	stc_tabs_ = new STabCtrl(this, true, true, tabs_condensed ? 27 : 31, true, true);

	// Setup panel info & add panel
	aui_mgr_->AddPane(stc_tabs_, wxAuiPaneInfo().CenterPane().Name(wxS("editor_area")).PaneBorder(false));

	// Create Start Page
	if (show_start_page)
		stc_tabs_->AddPage(
			new ui::StartPanel(stc_tabs_), wxS("Start Page"), true, icons::getIcon(icons::General, "logo"));

	// -- Console Panel --
	auto panel_console = new ConsolePanel(this, -1);

	// Setup panel info & add panel
	aui_mgr_->AddPane(
		panel_console,
		wxAuiPaneInfo()
			.DefaultPane()
			.Bottom()
			.FloatingSize(FromDIP(wxSize(600, 400)))
			.FloatingPosition(FromDIP(wxPoint(100, 100)))
			.MinSize(FromDIP(wxSize(-1, 192)))
			.Show(false)
			.Caption(wxS("Console"))
			.Name(wxS("console")));


	// -- Archive Manager Panel --
	panel_archivemanager_ = new ArchiveManagerPanel(this, stc_tabs_);

	// Setup panel info & add panel
	aui_mgr_->AddPane(
		panel_archivemanager_,
		wxAuiPaneInfo()
			.DefaultPane()
			.Right()
			.Row(0)
			.Position(0)
			.BestSize(FromDIP(wxSize(240, 480)))
			.Caption(wxS("Archive Manager"))
			.Name(wxS("archive_manager"))
			.Show(true)
			.Dock());

	// -- Undo History Panel --
	panel_undo_history_ = new UndoManagerHistoryPanel(this, nullptr);

	// Setup panel info & add panel
	aui_mgr_->AddPane(
		panel_undo_history_,
		wxAuiPaneInfo()
			.DefaultPane()
			.Right()
			.Row(0)
			.Position(1)
			.BestSize(FromDIP(wxSize(128, 480)))
			.Caption(wxS("Undo History"))
			.Name(wxS("undo_history"))
			.Show(false)
			.Dock());


	// -- Menu bar --
	auto menu = new wxMenuBar();
	menu->SetThemeEnabled(false);

	// File menu
	auto file_new_menu = new wxMenu();
	SAction::fromId("aman_newarchive")->addToMenu(file_new_menu, true, "&Archive");
	SAction::fromId("aman_newmap")->addToMenu(file_new_menu, true, "&Map");
	auto file_menu = new wxMenu();
	file_menu->AppendSubMenu(file_new_menu, wxS("&New"));
	SAction::fromId("aman_open")->addToMenu(file_menu);
	SAction::fromId("aman_opendir")->addToMenu(file_menu);
	file_menu->AppendSubMenu(panel_archivemanager_->recentFilesMenu(), wxS("&Recent Files"));
	file_menu->AppendSeparator();
	SAction::fromId("aman_save")->addToMenu(file_menu);
	SAction::fromId("aman_saveas")->addToMenu(file_menu);
	SAction::fromId("aman_saveall")->addToMenu(file_menu);
	file_menu->AppendSeparator();
	SAction::fromId("aman_close")->addToMenu(file_menu);
	SAction::fromId("aman_closeall")->addToMenu(file_menu);
	file_menu->AppendSeparator();
	SAction::fromId("main_exit")->addToMenu(file_menu);
	menu->Append(file_menu, wxS("&File"));

	// Edit menu
	auto editor_menu = new wxMenu();
	SAction::fromId("main_undo")->addToMenu(editor_menu);
	SAction::fromId("main_redo")->addToMenu(editor_menu);
	editor_menu->AppendSeparator();
	SAction::fromId("main_setbra")->addToMenu(editor_menu);
	SAction::fromId("main_preferences")->addToMenu(editor_menu);
	menu->Append(editor_menu, wxS("E&dit"));

	// View menu
	auto view_menu = new wxMenu();
	SAction::fromId("main_showam")->addToMenu(view_menu);
	SAction::fromId("main_showconsole")->addToMenu(view_menu);
	SAction::fromId("main_showundohistory")->addToMenu(view_menu);
	SAction::fromId("main_showstartpage")->addToMenu(view_menu);
	menu->Append(view_menu, wxS("&View"));

	// Tools menu
	auto tools_menu = new wxMenu();
	SAction::fromId("main_runscript")->addToMenu(tools_menu);
	menu->Append(tools_menu, wxS("&Tools"));

	// Help menu
	auto help_menu = new wxMenu();
	SAction::fromId("main_onlinedocs")->addToMenu(help_menu);
	SAction::fromId("main_homepage")->addToMenu(help_menu);
	SAction::fromId("main_github")->addToMenu(help_menu);
#ifdef __WXMSW__
	SAction::fromId("main_updatecheck")->addToMenu(help_menu);
#endif
	help_menu->AppendSeparator();
	SAction::fromId("main_about")->addToMenu(help_menu);
	menu->Append(help_menu, wxS("&Help"));

	// Set the menu
	SetMenuBar(menu);


	// -- Toolbar --
	toolbar_ = new SAuiToolBar(this, false, true, aui_mgr_);

	// Register custom controls and dropdown menus for toolbar
	toolbar_->registerCustomControl("base_resource_chooser", new BaseResourceChooser(toolbar_));
	palette_chooser_ = new PaletteChooser(toolbar_, -1);
	toolbar_->registerCustomControl("palette_chooser", palette_chooser_);
	toolbar_->registerDropdownMenu("maintenance", ArchivePanel::createMaintenanceMenu());
	toolbar_->registerDropdownMenu("bookmarks", panel_archivemanager_->bookmarksMenu());

	// Create toolbar from JSON definition
	auto toolbar_entry = app::programResource()->entryAtPath("toolbars/main_window.json");
	toolbar_->loadLayout(toolbar_entry->data().asString());

	// Archive toolbar is initially disabled
	toolbar_->enableGroup("Archive", false);

	// Add toolbar to aui
	aui_mgr_->AddPane(
		toolbar_,
		wxAuiPaneInfo()
			.ToolbarPane()
			.Top()
			.CaptionVisible(false)
			.MinSize(-1, toolbar_->GetMinSize().y)
			.Resizable(false)
			.PaneBorder(false)
			.Movable(false)
			.Floatable(false)
			.Name(wxS("toolbar")));


	// -- Status Bar --
	CreateStatusBar(3);


	// Load previously saved perspective string
	loadLayout();

	// Finalize
	aui_mgr_->Update();
	Layout();

	// Bind events
	Bind(wxEVT_SIZE, &MainWindow::onSize, this);
	Bind(wxEVT_CLOSE_WINDOW, &MainWindow::onClose, this);
	Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &MainWindow::onTabChanged, this);
	Bind(wxEVT_ACTIVATE, &MainWindow::onActivate, this);
	Bind(
		wxEVT_AUINOTEBOOK_PAGE_CLOSED,
		[&](wxAuiNotebookEvent& e)
		{
			// Clear undo history panel if all tabs closed
			if (stc_tabs_->GetSelection() == wxNOT_FOUND)
				panel_undo_history_->setManager(nullptr);
		});

	// Initial focus to toolbar
	toolbar_->SetFocus();
}

// -----------------------------------------------------------------------------
// Attempts to exit the program. Only fails if an unsaved archive is found and
// the user cancels the exit
// -----------------------------------------------------------------------------
bool MainWindow::exitProgram()
{
	// Confirm exit
	if (!wxGetApp().isSessionEnding() && confirm_exit && !panel_archivemanager_->askedSaveUnchanged())
	{
		if (wxMessageBox(wxS("Are you sure you want to exit SLADE?"), wxS("SLADE"), wxICON_QUESTION | wxYES_NO, this)
			!= wxYES)
			return false;
	}

	// Check if we can close the map editor
	if (mapeditor::windowCreated() && mapeditor::windowWx()->IsShown())
		if (!mapeditor::windowWx()->Close())
			return false;

	// Close all archives
	if (!panel_archivemanager_->closeAll())
		return false;

	// Save current layout
	// main_window_layout = aui_mgr_->SavePerspective();
	saveLayout();
	ui::saveStateBool(ui::MAINWINDOW_MAXIMIZED, IsMaximized());
	const wxSize size = GetSize();
	if (!IsMaximized())
		ui::setWindowInfo(this, id_, size.x, size.y, GetPosition().x, GetPosition().y);

	// Save selected palette
	global_palette = palette_chooser_->GetStringSelection().utf8_string();

	// Exit application
	app::exit(true);

	return true;
}

// -----------------------------------------------------------------------------
// Returns true if the Start Page tab is currently open
// -----------------------------------------------------------------------------
bool MainWindow::startPageTabOpen() const
{
	for (unsigned a = 0; a < stc_tabs_->GetPageCount(); a++)
	{
		if (stc_tabs_->GetPage(a)->GetName() == wxS("startpage"))
			return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Switches to the Start Page tab, or (re)creates it if it has been closed
// -----------------------------------------------------------------------------
void MainWindow::openStartPageTab() const
{
	// Find existing tab
	for (unsigned a = 0; a < stc_tabs_->GetPageCount(); a++)
	{
		if (stc_tabs_->GetPage(a)->GetName() == wxS("startpage"))
		{
			stc_tabs_->SetSelection(a);
			return;
		}
	}

	// Not found, create start page tab
	stc_tabs_->AddPage(new ui::StartPanel(stc_tabs_), wxS("Start Page"), true, icons::getIcon(icons::General, "logo"));
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool MainWindow::handleAction(string_view id)
{
	// We're only interested in "main_" actions
	if (!strutil::startsWith(id, "main_"))
		return false;

	// File->Exit
	if (id == "main_exit")
	{
		Close();
		return true;
	}

	// Edit->Undo
	if (id == "main_undo")
	{
		panel_archivemanager_->undo();
		return true;
	}

	// Edit->Redo
	if (id == "main_redo")
	{
		panel_archivemanager_->redo();
		return true;
	}

	// Edit->Set Base Resource Archive
	if (id == "main_setbra")
	{
		ui::SettingsDialog::popupSettingsPage(this, ui::SettingsPage::BaseResource);
		return true;
	}

	// Edit->Preferences
	if (id == "main_preferences")
	{
		ui::SettingsDialog settings(this);
		settings.ShowModal();

		return true;
	}

	// View->Archive Manager
	if (id == "main_showam")
	{
		auto  m_mgr = wxAuiManager::GetManager(panel_archivemanager_);
		auto& p_inf = m_mgr->GetPane(wxS("archive_manager"));
		p_inf.Show(!p_inf.IsShown());
		m_mgr->Update();
		return true;
	}

	// View->Console
	if (id == "main_showconsole")
	{
		auto  m_mgr = wxAuiManager::GetManager(panel_archivemanager_);
		auto& p_inf = m_mgr->GetPane(wxS("console"));
		p_inf.Show(!p_inf.IsShown());
		p_inf.MinSize(FromDIP(wxSize(200, 128)));
		dynamic_cast<ConsolePanel*>(p_inf.window)->focusInput();
		m_mgr->Update();
		return true;
	}

	// View->Undo History
	if (id == "main_showundohistory")
	{
		auto  m_mgr = wxAuiManager::GetManager(panel_archivemanager_);
		auto& p_inf = m_mgr->GetPane(wxS("undo_history"));
		p_inf.Show(!p_inf.IsShown());
		m_mgr->Update();
		return true;
	}

	// View->Show Start Page
	if (id == "main_showstartpage")
		openStartPageTab();

#ifndef NO_LUA
	// Tools->Run Script
	if (id == "main_runscript")
	{
		scriptmanager::open();
		return true;
	}
#endif

	// Help->About
	if (id == "main_about")
	{
		wxAboutDialogInfo info;
		info.SetName(wxS("SLADE"));
		string version = "v" + app::version().toString();
		if (!global::sc_rev.empty())
			version = version + " (Git Rev " + global::sc_rev + ")";
		info.SetVersion(wxString::FromUTF8(version));
		info.SetWebSite(wxS("https://slade.mancubus.net"));
		info.SetDescription(wxS("It's a Doom Editor"));
		// Set icon
		auto icon_filename = app::path(app::iconFile(), app::Dir::Temp);
		app::archiveManager().programResourceArchive()->entry(app::iconFile())->exportFile(icon_filename);
		info.SetIcon(wxIcon(wxString::FromUTF8(icon_filename), wxBITMAP_TYPE_ICO));
		fileutil::removeFile(icon_filename);

		auto year = wxNow().Right(4);
		info.SetCopyright(WX_FMT("\xC2\xA9 2008-{} Simon Judd <sirjuddington@gmail.com>", year.utf8_string()));

		wxAboutBox(info);

		return true;
	}

	// Help->Online Documentation
	if (id == "main_onlinedocs")
	{
		wxLaunchDefaultBrowser(wxS("https://slade.mancubus.net/wiki"));
		return true;
	}

	// Help->SLADE Homepage
	if (id == "main_homepage")
	{
		wxLaunchDefaultBrowser(wxS("https://slade.mancubus.net"));
		return true;
	}

	// Help->SLADE on GitHub
	if (id == "main_github")
	{
		wxLaunchDefaultBrowser(wxS("https://github.com/sirjuddington/SLADE"));
		return true;
	}

	// Help->Check For Updates
	if (id == "main_updatecheck")
	{
		wxGetApp().checkForUpdates(true);
		return true;
	}

	// Unknown action
	return false;
}


// -----------------------------------------------------------------------------
//
// MainWindow Class Events
//
// -----------------------------------------------------------------------------

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConstPtrOrRef

// -----------------------------------------------------------------------------
// Called when the window is closed
// -----------------------------------------------------------------------------
void MainWindow::onClose(wxCloseEvent& e)
{
	if (!exitProgram())
		e.Veto();
}

// -----------------------------------------------------------------------------
// Called when the current tab is changed
// -----------------------------------------------------------------------------
void MainWindow::onTabChanged(wxAuiNotebookEvent& e)
{
	// Get current page
	auto page = stc_tabs_->GetPage(stc_tabs_->GetSelection());

	// Archive tab, update undo history panel
	if (page->GetName() == wxS("archive"))
		panel_undo_history_->setManager(static_cast<ArchivePanel*>(page)->undoManager());

	// Continue
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the window is resized
// -----------------------------------------------------------------------------
void MainWindow::onSize(wxSizeEvent& e)
{
	// Update maximized state
	ui::saveStateBool(ui::MAINWINDOW_MAXIMIZED, IsMaximized());

	// Test creation of OpenGL context
	if (!opengl_test_done && e.GetSize().x > 20 && e.GetSize().y > 20)
	{
		auto mf = new wxMiniFrame(this, -1, wxS("OpenGL Test"), wxDefaultPosition, { 32, 32 });
		mf->SetSizer(new wxBoxSizer(wxVERTICAL));

		auto test_canvas = new GLCanvas(mf);
		mf->GetSizer()->Add(test_canvas, wxSizerFlags(1).Expand());

		mf->Show();
		test_canvas->activateContext();
		mf->Close(true);

		opengl_test_done = true;
	}

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the window is activated
// -----------------------------------------------------------------------------
void MainWindow::onActivate(wxActivateEvent& e)
{
	if (!e.GetActive() || this->IsBeingDeleted() || app::isExiting())
	{
		e.Skip();
		return;
	}

	e.Skip();
}
