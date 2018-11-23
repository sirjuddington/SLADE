
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "App.h"
#include "SLADEWxApp.h"
#include "MainWindow.h"
#include "UI/Controls/ConsolePanel.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Archive.h"
#include "Graphics/Icons.h"
#include "Dialogs/Preferences/BaseResourceArchivesPanel.h"
#include "UI/Controls/BaseResourceChooser.h"
#include "Dialogs/Preferences/PreferencesDialog.h"
#include "Utility/Tokenizer.h"
#include "MapEditor/MapEditor.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/Controls/UndoManagerHistoryPanel.h"
#include "ArchivePanel.h"
#include "General/Misc.h"
#include "UI/SAuiTabArt.h"
#include "UI/Controls/STabCtrl.h"
#include "TextureXEditor/TextureXEditor.h"
#include "UI/Controls/PaletteChooser.h"
#include "ArchiveManagerPanel.h"
#include "StartPage.h"
#include "Scripting/ScriptManager.h"
#include "UI/WxUtils.h"
#ifdef USE_WEBVIEW_STARTPAGE
#include "DocsPage.h"
#endif


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
namespace
{
	string main_window_layout = "";
}
CVAR(Bool, show_start_page, true, CVAR_SAVE);
CVAR(String, global_palette, "", CVAR_SAVE);
CVAR(Bool, mw_maximized, true, CVAR_SAVE);
CVAR(Bool, confirm_exit, true, CVAR_SAVE);


// ----------------------------------------------------------------------------
//
// External Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(Bool, tabs_condensed)


// ----------------------------------------------------------------------------
// MainWindowDropTarget Class
//
// Handles drag'n'drop of files on to the SLADE window
// ----------------------------------------------------------------------------
class MainWindowDropTarget : public wxFileDropTarget
{
public:
	MainWindowDropTarget() {}
	~MainWindowDropTarget() {}

	bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override
	{
		for (unsigned a = 0; a < filenames.size(); a++)
			App::archiveManager().openArchive(filenames[a]);

		return true;
	}
};


// ----------------------------------------------------------------------------
//
// MainWindow Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// MainWindow::MainWindow
//
// MainWindow class constructor
// ----------------------------------------------------------------------------
MainWindow::MainWindow()
	: STopWindow("SLADE", "main")
{
	lasttipindex = 0;
	custom_menus_begin_ = 2;
	if (mw_maximized) Maximize();
	setupLayout();
	SetDropTarget(new MainWindowDropTarget());
#ifdef USE_WEBVIEW_STARTPAGE
	docs_page = nullptr;
#endif
}

// ----------------------------------------------------------------------------
// MainWindow::~MainWindow
//
// MainWindow class destructor
// ----------------------------------------------------------------------------
MainWindow::~MainWindow()
{
	m_mgr->UnInit();
}

// ----------------------------------------------------------------------------
// MainWindow::loadLayout
//
// Loads the previously saved layout file for the window
// ----------------------------------------------------------------------------
void MainWindow::loadLayout()
{
	// Open layout file
	Tokenizer tz;
	if (!tz.openFile(App::path("mainwindow.layout", App::Dir::User)))
		return;

	// Parse layout
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
// MainWindow::saveLayout
//
// Saves the current window layout to a file
// ----------------------------------------------------------------------------
void MainWindow::saveLayout()
{
	// Open layout file
	wxFile file(App::path("mainwindow.layout", App::Dir::User), wxFile::write);

	// Write component layout

	// Console pane
	file.Write("\"console\" ");
	string pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("console"));
	file.Write(S_FMT("\"%s\"\n", pinf));

	// Archive Manager pane
	file.Write("\"archive_manager\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("archive_manager"));
	file.Write(S_FMT("\"%s\"\n", pinf));

	// Undo History pane
	file.Write("\"undo_history\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("undo_history"));
	file.Write(S_FMT("\"%s\"\n", pinf));

	// Close file
	file.Close();
}

// ----------------------------------------------------------------------------
// MainWindow::setupLayout
//
// Sets up the wxWidgets window layout
// ----------------------------------------------------------------------------
void MainWindow::setupLayout()
{
	// Create the wxAUI manager & related things
	m_mgr = new wxAuiManager(this);
	m_mgr->SetArtProvider(new SAuiDockArt());
	wxAuiPaneInfo p_inf;

	// Set icon
	string icon_filename = App::path(App::getIcon(), App::Dir::Temp);
	App::archiveManager().programResourceArchive()->getEntry(App::getIcon())->exportFile(icon_filename);
	SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
	wxRemoveFile(icon_filename);


	// -- Editor Area --
	stc_tabs = new STabCtrl(this, true, true, tabs_condensed ? 27 : 31, true, true);

	// Setup panel info & add panel
	p_inf.CenterPane();
	p_inf.Name("editor_area");
	p_inf.PaneBorder(false);
	m_mgr->AddPane(stc_tabs, p_inf);

	// Create Start Page
	start_page = new SStartPage(stc_tabs);
	if (show_start_page)
	{
		stc_tabs->AddPage(start_page, "Start Page");
		stc_tabs->SetPageBitmap(0, Icons::getIcon(Icons::GENERAL, "logo"));
		start_page->init();
		createStartPage();
	}
	else
		start_page->Show(false);

	// -- Console Panel --
	ConsolePanel* panel_console = new ConsolePanel(this, -1);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Float();
	p_inf.FloatingSize(WxUtils::scaledSize(600, 400));
	p_inf.FloatingPosition(WxUtils::scaledPoint(100, 100));
	p_inf.MinSize(WxUtils::scaledSize(-1, 192));
	p_inf.Show(false);
	p_inf.Caption("Console");
	p_inf.Name("console");
	m_mgr->AddPane(panel_console, p_inf);


	// -- Archive Manager Panel --
	panel_archivemanager = new ArchiveManagerPanel(this, stc_tabs);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Left();
	p_inf.BestSize(WxUtils::scaledSize(192, 480));
	p_inf.Caption("Archive Manager");
	p_inf.Name("archive_manager");
	p_inf.Show(true);
	p_inf.Dock();
	m_mgr->AddPane(panel_archivemanager, p_inf);


	// -- Undo History Panel --
	panel_undo_history = new UndoManagerHistoryPanel(this, nullptr);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Right();
	p_inf.BestSize(WxUtils::scaledSize(128, 480));
	p_inf.Caption("Undo History");
	p_inf.Name("undo_history");
	p_inf.Show(false);
	p_inf.Dock();
	m_mgr->AddPane(panel_undo_history, p_inf);


	// -- Menu bar --
	wxMenuBar* menu = new wxMenuBar();
	menu->SetThemeEnabled(false);

	// File menu
	wxMenu* file_new_menu = new wxMenu("");
	SAction::fromId("aman_newwad")->addToMenu(file_new_menu, "&Wad Archive");
	SAction::fromId("aman_newzip")->addToMenu(file_new_menu, "&Zip Archive");
	SAction::fromId("aman_newmap")->addToMenu(file_new_menu, "&Map");
	wxMenu* file_menu = new wxMenu("");
	file_menu->AppendSubMenu(file_new_menu, "&New", "Create a new Archive");
	SAction::fromId("aman_open")->addToMenu(file_menu);
	SAction::fromId("aman_opendir")->addToMenu(file_menu);
	file_menu->AppendSeparator();
	SAction::fromId("aman_save")->addToMenu(file_menu);
	SAction::fromId("aman_saveas")->addToMenu(file_menu);
	SAction::fromId("aman_saveall")->addToMenu(file_menu);
	file_menu->AppendSubMenu(panel_archivemanager->getRecentMenu(), "&Recent Files");
	file_menu->AppendSeparator();
	SAction::fromId("aman_close")->addToMenu(file_menu);
	SAction::fromId("aman_closeall")->addToMenu(file_menu);
	file_menu->AppendSeparator();
	SAction::fromId("main_exit")->addToMenu(file_menu);
	menu->Append(file_menu, "&File");

	// Edit menu
	wxMenu* editor_menu = new wxMenu("");
	SAction::fromId("main_undo")->addToMenu(editor_menu);
	SAction::fromId("main_redo")->addToMenu(editor_menu);
	editor_menu->AppendSeparator();
	SAction::fromId("main_setbra")->addToMenu(editor_menu);
	SAction::fromId("main_preferences")->addToMenu(editor_menu);
	menu->Append(editor_menu, "E&dit");

	// View menu
	wxMenu* view_menu = new wxMenu("");
	SAction::fromId("main_showam")->addToMenu(view_menu);
	SAction::fromId("main_showconsole")->addToMenu(view_menu);
	SAction::fromId("main_showundohistory")->addToMenu(view_menu);
	SAction::fromId("main_showstartpage")->addToMenu(view_menu);
	toolbar_menu_ = new wxMenu();
	view_menu->AppendSubMenu(toolbar_menu_, "Toolbars");
	menu->Append(view_menu, "&View");

	// Tools menu
	wxMenu* tools_menu = new wxMenu("");
	SAction::fromId("main_runscript")->addToMenu(tools_menu);
	menu->Append(tools_menu, "&Tools");

	// Help menu
	wxMenu* help_menu = new wxMenu("");
	SAction::fromId("main_onlinedocs")->addToMenu(help_menu);
	SAction::fromId("main_about")->addToMenu(help_menu);
#ifdef __WXMSW__
	SAction::fromId("main_updatecheck")->addToMenu(help_menu);
#endif
	menu->Append(help_menu, "&Help");

	// Set the menu
	SetMenuBar(menu);



	// -- Toolbars --
	toolbar_ = new SToolBar(this, true);

	// Create File toolbar
	SToolBarGroup* tbg_file = new SToolBarGroup(toolbar_, "_File");
	tbg_file->addActionButton("aman_newwad");
	tbg_file->addActionButton("aman_newzip");
	tbg_file->addActionButton("aman_open");
	tbg_file->addActionButton("aman_opendir");
	tbg_file->addActionButton("aman_save");
	tbg_file->addActionButton("aman_saveas");
	tbg_file->addActionButton("aman_saveall");
	tbg_file->addActionButton("aman_close");
	tbg_file->addActionButton("aman_closeall");
	toolbar_->addGroup(tbg_file);

	// Create Archive toolbar
	SToolBarGroup* tbg_archive = new SToolBarGroup(toolbar_, "_Archive");
	tbg_archive->addActionButton("arch_newentry");
	tbg_archive->addActionButton("arch_newdir");
	tbg_archive->addActionButton("arch_importfiles");
	tbg_archive->addActionButton("arch_texeditor");
	tbg_archive->addActionButton("arch_mapeditor");
	tbg_archive->addActionButton("arch_run");
	toolbar_->addGroup(tbg_archive);

	// Create Entry toolbar
	SToolBarGroup* tbg_entry = new SToolBarGroup(toolbar_, "_Entry");
	tbg_entry->addActionButton("arch_entry_rename");
	tbg_entry->addActionButton("arch_entry_delete");
	tbg_entry->addActionButton("arch_entry_import");
	tbg_entry->addActionButton("arch_entry_export");
	tbg_entry->addActionButton("arch_entry_moveup");
	tbg_entry->addActionButton("arch_entry_movedown");
	toolbar_->addGroup(tbg_entry);

	// Create Base Resource Archive toolbar
	SToolBarGroup* tbg_bra = new SToolBarGroup(toolbar_, "_Base Resource", true);
	BaseResourceChooser* brc = new BaseResourceChooser(tbg_bra);
	tbg_bra->addCustomControl(brc);
	tbg_bra->addActionButton("main_setbra", "settings");
	toolbar_->addGroup(tbg_bra);

	// Create Palette Chooser toolbar
	SToolBarGroup* tbg_palette = new SToolBarGroup(toolbar_, "_Palette", true);
	palette_chooser = new PaletteChooser(tbg_palette, -1);
	palette_chooser->selectPalette(global_palette);
	tbg_palette->addCustomControl(palette_chooser);
	toolbar_->addGroup(tbg_palette);

	// Archive and Entry toolbars are initially disabled
	toolbar_->enableGroup("_archive", false);
	toolbar_->enableGroup("_entry", false);

	// Add toolbar
	m_mgr->AddPane(
		toolbar_,
		wxAuiPaneInfo()
			.Top()
			.CaptionVisible(false)
			.MinSize(-1, SToolBar::getBarHeight())
			.Resizable(false)
			.PaneBorder(false)
			.Name("toolbar")
	);

	// Populate the 'View->Toolbars' menu
	populateToolbarsMenu();
	toolbar_->enableContextMenu();

	// -- Status Bar --
	CreateStatusBar(3);


	// Load previously saved perspective string
	loadLayout();

	// Finalize
	m_mgr->Update();
	Layout();

	// Bind events
	Bind(wxEVT_SIZE, &MainWindow::onSize, this);
	Bind(wxEVT_CLOSE_WINDOW, &MainWindow::onClose, this);
	Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &MainWindow::onTabChanged, this);
	Bind(wxEVT_STOOLBAR_LAYOUT_UPDATED, &MainWindow::onToolBarLayoutChanged, this, toolbar_->GetId());
	Bind(wxEVT_ACTIVATE, &MainWindow::onActivate, this);
	Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, [&](wxAuiNotebookEvent& e)
	{
		// Null start_page pointer if start page tab is closed
		auto page = stc_tabs->GetPage(stc_tabs->GetSelection());
		if (page->GetName() == "startpage")
			start_page = nullptr;
	});

	// Initial focus to toolbar
	toolbar_->SetFocus();
}

// ----------------------------------------------------------------------------
// MainWindow::createStartPage
//
// (Re-)Creates the start page
// ----------------------------------------------------------------------------
void MainWindow::createStartPage(bool newtip) const
{
	if (start_page)
		start_page->load(newtip);
}

// ----------------------------------------------------------------------------
// MainWindow::exitProgram
//
// Attempts to exit the program. Only fails if an unsaved archive is found and
// the user cancels the exit
// ----------------------------------------------------------------------------
bool MainWindow::exitProgram()
{
	// Confirm exit
	if (confirm_exit && !panel_archivemanager->askedSaveUnchanged())
	{
		if (wxMessageBox("Are you sure you want to exit SLADE?", "SLADE", wxICON_QUESTION|wxYES_NO, this) != wxYES)
			return false;
	}

	// Check if we can close the map editor
	if (MapEditor::windowWx()->IsShown())
		if (!MapEditor::windowWx()->Close())
			return false;

	// Close all archives
	if (!panel_archivemanager->closeAll())
		return false;

	// Save current layout
	//main_window_layout = m_mgr->SavePerspective();
	saveLayout();
	mw_maximized = IsMaximized();
	if (!IsMaximized())
		Misc::setWindowInfo(id_, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);

	// Save selected palette
	global_palette = palette_chooser->GetStringSelection();

	// Exit application
	App::exit(true);

	return true;
}

// ----------------------------------------------------------------------------
// MainWindow::startPageTabOpen
//
// Returns true if the Start Page tab is currently open
// ----------------------------------------------------------------------------
bool MainWindow::startPageTabOpen() const
{
	for (unsigned a = 0; a < stc_tabs->GetPageCount(); a++)
	{
		if (stc_tabs->GetPage(a)->GetName() == "startpage")
			return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
// MainWindow::openStartPageTab
//
// Switches to the Start Page tab, or (re)creates it if it has been closed
// ----------------------------------------------------------------------------
void MainWindow::openStartPageTab()
{
	// Find existing tab
	for (unsigned a = 0; a < stc_tabs->GetPageCount(); a++)
	{
		if (stc_tabs->GetPage(a)->GetName() == "startpage")
		{
			stc_tabs->SetSelection(a);
			return;
		}
	}

	// Not found, create start page tab
	start_page = new SStartPage(stc_tabs);
	start_page->init();
	stc_tabs->AddPage(start_page, "Start Page");
	stc_tabs->SetPageBitmap(0, Icons::getIcon(Icons::GENERAL, "logo"));
	createStartPage();
}

// ----------------------------------------------------------------------------
// MainWindow::openDocs
//
// Opens [entry] in its own tab
// ----------------------------------------------------------------------------
#ifdef USE_WEBVIEW_STARTPAGE
void MainWindow::openDocs(string page_name)
{
	// Check if docs tab is already open
	bool found = false;
	for (unsigned a = 0; a < stc_tabs->GetPageCount(); a++)
	{
		if (stc_tabs->GetPage(a)->GetName() == "docs")
		{
			stc_tabs->SetSelection(a);
			found = true;
			break;
		}
	}

	// Open new docs tab if not already open
	if (!found)
	{
		// Create docs page
		docs_page = new DocsPage(this);
		docs_page->SetName("docs");

		// Add tab
		stc_tabs->AddPage(docs_page, "Documentation", true, -1);
		stc_tabs->SetPageBitmap(stc_tabs->GetPageCount() - 1, Icons::getIcon(Icons::GENERAL, "wiki"));
	}

	// Load specified page, if any
	if (page_name != "")
		docs_page->openPage(page_name);

	// Refresh page
	docs_page->Layout();
	docs_page->Update();
}
#endif

// ----------------------------------------------------------------------------
// MainWindow::handleAction
//
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// ----------------------------------------------------------------------------
bool MainWindow::handleAction(string id)
{
	// We're only interested in "main_" actions
	if (!id.StartsWith("main_"))
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
		panel_archivemanager->undo();
		return true;
	}

	// Edit->Redo
	if (id == "main_redo")
	{
		panel_archivemanager->redo();
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
		wxAuiManager* m_mgr = wxAuiManager::GetManager(panel_archivemanager);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("archive_manager");
		p_inf.Show(!p_inf.IsShown());
		m_mgr->Update();
		return true;
	}

	// View->Console
	if (id == "main_showconsole")
	{
		wxAuiManager* m_mgr = wxAuiManager::GetManager(panel_archivemanager);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("console");
		p_inf.Show(!p_inf.IsShown());
		p_inf.MinSize(WxUtils::scaledSize(200, 128));
		m_mgr->Update();
		return true;
	}

	// View->Undo History
	if (id == "main_showundohistory")
	{
		wxAuiManager* m_mgr = wxAuiManager::GetManager(panel_archivemanager);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("undo_history");
		p_inf.Show(!p_inf.IsShown());
		m_mgr->Update();
		return true;
	}

	// View->Show Start Page
	if (id == "main_showstartpage")
		openStartPageTab();

	// Tools->Run Script
	if (id == "main_runscript")
	{
		ScriptManager::open();
		return true;
	}

	// Help->About
	if (id == "main_about")
	{
		wxAboutDialogInfo info;
		info.SetName("SLADE");
		string version = "v" + Global::version;
		if (Global::sc_rev != "")
			version = version + " (Git Rev " + Global::sc_rev + ")";
		info.SetVersion(version);
		info.SetWebSite("http://slade.mancubus.net");
		info.SetDescription("It's a Doom Editor");

		// Set icon
		string icon_filename = App::path(App::getIcon(), App::Dir::Temp);
		App::archiveManager().programResourceArchive()->getEntry(App::getIcon())->exportFile(icon_filename);
		info.SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
		wxRemoveFile(icon_filename);

		string year = wxNow().Right(4);
		info.SetCopyright(S_FMT("(C) 2008-%s Simon Judd <sirjuddington@gmail.com>", year));

		wxAboutBox(info);

		return true;
	}

	// Help->Online Documentation
	if (id == "main_onlinedocs")
	{
#ifdef USE_WEBVIEW_STARTPAGE
		openDocs();
#else
		wxLaunchDefaultBrowser("http://slade.mancubus.net/wiki");
#endif
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


// ----------------------------------------------------------------------------
//
// MainWindow Class Events
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// MainWindow::onClose
//
// Called when the window is closed
// ----------------------------------------------------------------------------
void MainWindow::onClose(wxCloseEvent& e)
{
	if (!exitProgram())
		e.Veto();
}

// ----------------------------------------------------------------------------
// MainWindow::onTabChanged
//
// Called when the current tab is changed
// ----------------------------------------------------------------------------
void MainWindow::onTabChanged(wxAuiNotebookEvent& e)
{
	// Get current page
	wxWindow* page = stc_tabs->GetPage(stc_tabs->GetSelection());

	// If start page is selected, refresh it
	if (page->GetName() == "startpage")
	{
		createStartPage();
		SetStatusText("", 1);
		SetStatusText("", 2);
	}

	// Archive tab, update undo history panel
	else if (page->GetName() == "archive")
		panel_undo_history->setManager(((ArchivePanel*)page)->undoManager());

	// Continue
	e.Skip();
}

// ----------------------------------------------------------------------------
// MainWindow::onSize
//
// Called when the window is resized
// ----------------------------------------------------------------------------
void MainWindow::onSize(wxSizeEvent& e)
{
	// Update toolbar layout (if needed)
	toolbar_->updateLayout();
#ifndef __WXMSW__
	m_mgr->GetPane(toolbar_).MinSize(-1, toolbar_->minHeight());
	m_mgr->Update();
#endif

	// Update maximized cvar
	mw_maximized = IsMaximized();

	e.Skip();
}

// ----------------------------------------------------------------------------
// MainWindow::onToolBarLayoutChanged
//
// Called when the toolbar layout is changed
// ----------------------------------------------------------------------------
void MainWindow::onToolBarLayoutChanged(wxEvent& e)
{
	// Update toolbar size
	m_mgr->GetPane(toolbar_).MinSize(-1, toolbar_->minHeight());
	m_mgr->Update();
}

// ----------------------------------------------------------------------------
// MainWindow::onActivate
//
// Called when the window is activated
// ----------------------------------------------------------------------------
void MainWindow::onActivate(wxActivateEvent& e)
{
	if (!e.GetActive() || this->IsBeingDeleted() || App::isExiting())
	{
		e.Skip();
		return;
	}

	// Get current tab
	if (stc_tabs->GetPageCount())
	{
		wxWindow* page = stc_tabs->GetPage(stc_tabs->GetSelection());

		// If start page is selected, refresh it
		if (page && page->GetName() == "startpage")
		{
			createStartPage(false);
			SetStatusText("", 1);
			SetStatusText("", 2);
		}
	}

	e.Skip();
}
