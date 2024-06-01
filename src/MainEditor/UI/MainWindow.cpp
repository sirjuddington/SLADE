
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
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
#include "General/Misc.h"
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
#include "UI/Dialogs/Preferences/PreferencesDialog.h"
#include "UI/SAuiTabArt.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/SToolBar/SToolBarButton.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"

using namespace slade;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Bool, show_start_page, true, CVar::Flag::Save);
CVAR(String, global_palette, "", CVar::Flag::Save);
CVAR(Bool, mw_maximized, true, CVar::Flag::Save);
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
			app::archiveManager().openArchive(filename.ToStdString());

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

	if (mw_maximized)
		wxTopLevelWindow::Maximize();

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
	// Open layout file
	Tokenizer tz;
	if (!tz.openFile(app::path("mainwindow.layout", app::Dir::User)))
		return;

	// Parse layout
	while (true)
	{
		// Read component+layout pair
		wxString component = tz.getToken();
		wxString layout    = tz.getToken();

		// Load layout to component
		if (!component.IsEmpty() && !layout.IsEmpty())
			aui_mgr_->LoadPaneInfo(layout, aui_mgr_->GetPane(component));

		// Check if we're done
		if (tz.peekToken().empty())
			break;
	}
}

// -----------------------------------------------------------------------------
// Saves the current window layout to a file
// -----------------------------------------------------------------------------
void MainWindow::saveLayout() const
{
	// Open layout file
	wxFile file(app::path("mainwindow.layout", app::Dir::User), wxFile::write);

	// Write component layout

	// Console pane
	file.Write("\"console\" ");
	wxString pinf = aui_mgr_->SavePaneInfo(aui_mgr_->GetPane("console"));
	file.Write(wxString::Format("\"%s\"\n", pinf));

	// Archive Manager pane
	file.Write("\"archive_manager\" ");
	pinf = aui_mgr_->SavePaneInfo(aui_mgr_->GetPane("archive_manager"));
	file.Write(wxString::Format("\"%s\"\n", pinf));

	// Undo History pane
	file.Write("\"undo_history\" ");
	pinf = aui_mgr_->SavePaneInfo(aui_mgr_->GetPane("undo_history"));
	file.Write(wxString::Format("\"%s\"\n", pinf));

	// Close file
	file.Close();
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
	SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
	wxRemoveFile(icon_filename);


	// -- Editor Area --
	stc_tabs_ = new STabCtrl(this, true, true, tabs_condensed ? 27 : 31, true, true);

	// Setup panel info & add panel
	p_inf.CenterPane();
	p_inf.Name("editor_area");
	p_inf.PaneBorder(false);
	aui_mgr_->AddPane(stc_tabs_, p_inf);

	// Create Start Page
	if (show_start_page)
		stc_tabs_->AddPage(new ui::StartPanel(stc_tabs_), "Start Page", true, icons::getIcon(icons::General, "logo"));

	// -- Console Panel --
	auto panel_console = new ConsolePanel(this, -1);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Float();
	p_inf.FloatingSize(FromDIP(wxSize(600, 400)));
	p_inf.FloatingPosition(FromDIP(wxPoint(100, 100)));
	p_inf.MinSize(FromDIP(wxSize(-1, 192)));
	p_inf.Show(false);
	p_inf.Caption("Console");
	p_inf.Name("console");
	aui_mgr_->AddPane(panel_console, p_inf);


	// -- Archive Manager Panel --
	panel_archivemanager_ = new ArchiveManagerPanel(this, stc_tabs_);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Left();
	p_inf.BestSize(FromDIP(wxSize(192, 480)));
	p_inf.Caption("Archive Manager");
	p_inf.Name("archive_manager");
	p_inf.Show(true);
	p_inf.Dock();
	aui_mgr_->AddPane(panel_archivemanager_, p_inf);


	// -- Undo History Panel --
	panel_undo_history_ = new UndoManagerHistoryPanel(this, nullptr);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Right();
	p_inf.BestSize(FromDIP(wxSize(128, 480)));
	p_inf.Caption("Undo History");
	p_inf.Name("undo_history");
	p_inf.Show(false);
	p_inf.Dock();
	aui_mgr_->AddPane(panel_undo_history_, p_inf);


	// -- Menu bar --
	auto menu = new wxMenuBar();
	menu->SetThemeEnabled(false);

	// File menu
	auto file_new_menu = new wxMenu("");
	SAction::fromId("aman_newarchive")->addToMenu(file_new_menu, true, "&Archive");
	SAction::fromId("aman_newmap")->addToMenu(file_new_menu, true, "&Map");
	auto file_menu = new wxMenu("");
	file_menu->AppendSubMenu(file_new_menu, "&New");
	SAction::fromId("aman_open")->addToMenu(file_menu);
	SAction::fromId("aman_opendir")->addToMenu(file_menu);
	file_menu->AppendSubMenu(panel_archivemanager_->recentFilesMenu(), "&Recent Files");
	file_menu->AppendSeparator();
	SAction::fromId("aman_save")->addToMenu(file_menu);
	SAction::fromId("aman_saveas")->addToMenu(file_menu);
	SAction::fromId("aman_saveall")->addToMenu(file_menu);
	file_menu->AppendSeparator();
	SAction::fromId("aman_close")->addToMenu(file_menu);
	SAction::fromId("aman_closeall")->addToMenu(file_menu);
	file_menu->AppendSeparator();
	SAction::fromId("main_exit")->addToMenu(file_menu);
	menu->Append(file_menu, "&File");

	// Edit menu
	auto editor_menu = new wxMenu("");
	SAction::fromId("main_undo")->addToMenu(editor_menu);
	SAction::fromId("main_redo")->addToMenu(editor_menu);
	editor_menu->AppendSeparator();
	SAction::fromId("main_setbra")->addToMenu(editor_menu);
	SAction::fromId("main_preferences")->addToMenu(editor_menu);
	menu->Append(editor_menu, "E&dit");

	// View menu
	auto view_menu = new wxMenu("");
	SAction::fromId("main_showam")->addToMenu(view_menu);
	SAction::fromId("main_showconsole")->addToMenu(view_menu);
	SAction::fromId("main_showundohistory")->addToMenu(view_menu);
	SAction::fromId("main_showstartpage")->addToMenu(view_menu);
	toolbar_menu_ = new wxMenu();
	view_menu->AppendSubMenu(toolbar_menu_, "Toolbars");
	menu->Append(view_menu, "&View");

	// Tools menu
	auto tools_menu = new wxMenu("");
	SAction::fromId("main_runscript")->addToMenu(tools_menu);
	menu->Append(tools_menu, "&Tools");

	// Help menu
	auto help_menu = new wxMenu("");
	SAction::fromId("main_onlinedocs")->addToMenu(help_menu);
	SAction::fromId("main_homepage")->addToMenu(help_menu);
	SAction::fromId("main_github")->addToMenu(help_menu);
#ifdef __WXMSW__
	SAction::fromId("main_updatecheck")->addToMenu(help_menu);
#endif
	help_menu->AppendSeparator();
	SAction::fromId("main_about")->addToMenu(help_menu);
	menu->Append(help_menu, "&Help");

	// Set the menu
	SetMenuBar(menu);



	// -- Toolbars --
	toolbar_ = new SToolBar(this, true);

	// Create File toolbar
	auto tbg_file = new SToolBarGroup(toolbar_, "_File");
	tbg_file->addActionButton("aman_newarchive");
	tbg_file->addActionButton("aman_open");
	tbg_file->addActionButton("aman_opendir");
	tbg_file->addActionButton("aman_save");
	tbg_file->addActionButton("aman_saveas");
	tbg_file->addActionButton("aman_saveall");
	tbg_file->addActionButton("aman_close");
	tbg_file->addActionButton("aman_closeall");
	toolbar_->addGroup(tbg_file);

	// Create Archive toolbar
	auto tbg_archive = new SToolBarGroup(toolbar_, "_Archive");
	tbg_archive->addActionButton("arch_texeditor");
	tbg_archive->addActionButton("arch_mapeditor");
	tbg_archive->addActionButton("arch_run");
	auto* b_maint = tbg_archive->addActionButton(
		"arch_maintenance", "Maintenance", "wrench", "Archive maintenance/cleanup tools");
	b_maint->setMenu(ArchivePanel::createMaintenanceMenu());
	toolbar_->addGroup(tbg_archive);

	// Create Boomkarks toolbar
	auto* tbg_bookmarks = new SToolBarGroup(toolbar_, "_Bookmarks");
	auto* b_bookmarks   = tbg_bookmarks->addActionButton(
        "bookmarks", "Bookmarks", "bookmark", "Go to a bookmarked entry");
	b_bookmarks->setMenu(panel_archivemanager_->bookmarksMenu());
	toolbar_->addGroup(tbg_bookmarks);

	// Create Base Resource Archive toolbar
	auto tbg_bra = new SToolBarGroup(toolbar_, "_Base Resource", true);
	auto brc     = new BaseResourceChooser(tbg_bra);
	tbg_bra->addCustomControl(brc);
	tbg_bra->addActionButton("main_setbra", "settings");
	toolbar_->addGroup(tbg_bra, true);

	// Create Palette Chooser toolbar
	auto tbg_palette = new SToolBarGroup(toolbar_, "_Palette", true);
	palette_chooser_ = new PaletteChooser(tbg_palette, -1);
	palette_chooser_->selectPalette(global_palette);
	tbg_palette->addCustomControl(palette_chooser_);
	toolbar_->addGroup(tbg_palette, true);

	// Archive and Entry toolbars are initially disabled
	toolbar_->enableGroup("_archive", false);
	toolbar_->enableGroup("_entry", false);

	// Add toolbar
	aui_mgr_->AddPane(
		toolbar_,
		wxAuiPaneInfo()
			.Top()
			.CaptionVisible(false)
			.MinSize(-1, SToolBar::getBarHeight(this))
			.Resizable(false)
			.PaneBorder(false)
			.Name("toolbar"));

	// Populate the 'View->Toolbars' menu
	populateToolbarsMenu();
	toolbar_->enableContextMenu();

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
	Bind(wxEVT_STOOLBAR_LAYOUT_UPDATED, &MainWindow::onToolBarLayoutChanged, this, toolbar_->GetId());
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
		if (wxMessageBox("Are you sure you want to exit SLADE?", "SLADE", wxICON_QUESTION | wxYES_NO, this) != wxYES)
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
	mw_maximized      = IsMaximized();
	const wxSize size = GetSize() * GetContentScaleFactor();
	if (!IsMaximized())
		misc::setWindowInfo(
			id_, size.x, size.y, GetPosition().x * GetContentScaleFactor(), GetPosition().y * GetContentScaleFactor());

	// Save selected palette
	global_palette = wxutil::strToView(palette_chooser_->GetStringSelection());

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
		if (stc_tabs_->GetPage(a)->GetName() == "startpage")
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
		if (stc_tabs_->GetPage(a)->GetName() == "startpage")
		{
			stc_tabs_->SetSelection(a);
			return;
		}
	}

	// Not found, create start page tab
	stc_tabs_->AddPage(new ui::StartPanel(stc_tabs_), "Start Page", true, icons::getIcon(icons::General, "logo"));
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
		PreferencesDialog::openPreferences(this, "Base Resource Archive");

		return true;
	}

	// Edit->Preferences
	if (id == "main_preferences")
	{
		PreferencesDialog::openPreferences(this);

		return true;
	}

	// View->Archive Manager
	if (id == "main_showam")
	{
		auto  m_mgr = wxAuiManager::GetManager(panel_archivemanager_);
		auto& p_inf = m_mgr->GetPane("archive_manager");
		p_inf.Show(!p_inf.IsShown());
		m_mgr->Update();
		return true;
	}

	// View->Console
	if (id == "main_showconsole")
	{
		auto  m_mgr = wxAuiManager::GetManager(panel_archivemanager_);
		auto& p_inf = m_mgr->GetPane("console");
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
		auto& p_inf = m_mgr->GetPane("undo_history");
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
		info.SetName("SLADE");
		wxString version = "v" + app::version().toString();
		if (!global::sc_rev.empty())
			version = version + " (Git Rev " + global::sc_rev + ")";
		info.SetVersion(version);
		info.SetWebSite("https://slade.mancubus.net");
		info.SetDescription("It's a Doom Editor");

		// Set icon
		auto icon_filename = app::path(app::iconFile(), app::Dir::Temp);
		app::archiveManager().programResourceArchive()->entry(app::iconFile())->exportFile(icon_filename);
		info.SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
		wxRemoveFile(icon_filename);

		wxString year = wxNow().Right(4);
		info.SetCopyright(wxString::Format("(C) 2008-%s Simon Judd <sirjuddington@gmail.com>", year));

		wxAboutBox(info);

		return true;
	}

	// Help->Online Documentation
	if (id == "main_onlinedocs")
	{
		wxLaunchDefaultBrowser("https://slade.mancubus.net/wiki");
		return true;
	}

	// Help->SLADE Homepage
	if (id == "main_homepage")
	{
		wxLaunchDefaultBrowser("https://slade.mancubus.net");
		return true;
	}

	// Help->SLADE on GitHub
	if (id == "main_github")
	{
		wxLaunchDefaultBrowser("https://github.com/sirjuddington/SLADE");
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
	if (page->GetName() == "archive")
		panel_undo_history_->setManager(static_cast<ArchivePanel*>(page)->undoManager());

	// Continue
	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the window is resized
// -----------------------------------------------------------------------------
void MainWindow::onSize(wxSizeEvent& e)
{
	// Update toolbar layout (if needed)
	toolbar_->updateLayout();
#ifndef __WXMSW__
	aui_mgr_->GetPane(toolbar_).MinSize(-1, toolbar_->getBarHeight(this));
	aui_mgr_->Update();
#endif

	// Update maximized cvar
	mw_maximized = IsMaximized();

	// Test creation of OpenGL context
	if (!opengl_test_done && e.GetSize().x > 20 && e.GetSize().y > 20)
	{
		auto mf = new wxMiniFrame(this, -1, "OpenGL Test", wxDefaultPosition, { 32, 32 });
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
// Called when the toolbar layout is changed
// -----------------------------------------------------------------------------
void MainWindow::onToolBarLayoutChanged(wxEvent& e)
{
	// Update toolbar size
	aui_mgr_->GetPane(toolbar_).MinSize(-1, SToolBar::getBarHeight(this));
	aui_mgr_->Update();
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
