
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MainWindow.cpp
 * Description: MainWindow class, ie the main SLADE window
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "MainWindow.h"
#include "UI/ConsolePanel.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Archive.h"
#include "Graphics/Icons.h"
#include "Dialogs/Preferences/BaseResourceArchivesPanel.h"
#include "UI/BaseResourceChooser.h"
#include "Dialogs/Preferences/PreferencesDialog.h"
#include "Utility/Tokenizer.h"
#include "UI/SplashWindow.h"
#include "MapEditor/MapEditorWindow.h"
#include "Dialogs/MapEditorConfigDialog.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/UndoManagerHistoryPanel.h"
#include "UI/ArchivePanel.h"
#include "General/Misc.h"
#include "UI/SAuiTabArt.h"
#include "UI/STabCtrl.h"
#include "UI/TextureXEditor/TextureXEditor.h"
#include "UI/PaletteChooser.h"

#ifdef USE_WEBVIEW_STARTPAGE
#include "UI/DocsPage.h"
#endif

/*******************************************************************
 * VARIABLES
 *******************************************************************/
string main_window_layout = "";
MainWindow* MainWindow::instance = NULL;
CVAR(Bool, show_start_page, true, CVAR_SAVE);
CVAR(String, global_palette, "", CVAR_SAVE);
CVAR(Bool, mw_maximized, true, CVAR_SAVE);
CVAR(Bool, confirm_exit, true, CVAR_SAVE);


/*******************************************************************
 * MAINWINDOWDROPTARGET CLASS
 *******************************************************************
 Handles drag'n'drop of files on to the SLADE window
*/
class MainWindowDropTarget : public wxFileDropTarget
{
public:
	MainWindowDropTarget() {}
	~MainWindowDropTarget() {}

	bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
	{
		for (unsigned a = 0; a < filenames.size(); a++)
			theArchiveManager->openArchive(filenames[a]);

		return true;
	}
};


/*******************************************************************
 * MAINWINDOW CLASS FUNCTIONS
 *******************************************************************/

/* MainWindow::MainWindow
 * MainWindow class constructor
 *******************************************************************/
MainWindow::MainWindow()
	: STopWindow("SLADE", "main")
{
	lasttipindex = 0;
	custom_menus_begin = 2;
	if (mw_maximized) Maximize();
	setupLayout();
	SetDropTarget(new MainWindowDropTarget());
#ifdef USE_WEBVIEW_STARTPAGE
	docs_page = NULL;
#endif
}

/* MainWindow::~MainWindow
 * MainWindow class destructor
 *******************************************************************/
MainWindow::~MainWindow()
{
	m_mgr->UnInit();
}

/* MainWindow::loadLayout
 * Loads the previously saved layout file for the window
 *******************************************************************/
void MainWindow::loadLayout()
{
	// Open layout file
	Tokenizer tz;
	if (!tz.openFile(appPath("mainwindow.layout", DIR_USER)))
		return;

	// Parse layout
	wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
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

/* MainWindow::saveLayout
 * Saves the current window layout to a file
 *******************************************************************/
void MainWindow::saveLayout()
{
	// Open layout file
	wxFile file(appPath("mainwindow.layout", DIR_USER), wxFile::write);

	// Write component layout
	wxAuiManager* m_mgr = wxAuiManager::GetManager(this);

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

/* MainWindow::setupLayout
 * Sets up the wxWidgets window layout
 *******************************************************************/
void MainWindow::setupLayout()
{
	// Create the wxAUI manager & related things
	m_mgr = new wxAuiManager(this);
	m_mgr->SetArtProvider(new SAuiDockArt());
	wxAuiPaneInfo p_inf;

	// Set icon
	string icon_filename = appPath("slade.ico", DIR_TEMP);
	theArchiveManager->programResourceArchive()->getEntry("slade.ico")->exportFile(icon_filename);
	SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
	wxRemoveFile(icon_filename);


	// -- Editor Area --
	stc_tabs = new STabCtrl(this, true, true, 27, true);

	// Setup panel info & add panel
	p_inf.CenterPane();
	p_inf.Name("editor_area");
	p_inf.PaneBorder(false);
	m_mgr->AddPane(stc_tabs, p_inf);

	// Create Start Page
#ifdef USE_WEBVIEW_STARTPAGE
	html_startpage = wxWebView::New(stc_tabs, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxWebViewBackendDefault, wxBORDER_NONE);
	html_startpage->SetName("startpage");
#ifdef __WXMAC__
	html_startpage->SetZoomType(wxWEBVIEW_ZOOM_TYPE_TEXT);
#else // !__WXMAC__
	html_startpage->SetZoomType(wxWEBVIEW_ZOOM_TYPE_LAYOUT);
#endif // __WXMAC__
	if (show_start_page)
	{
		stc_tabs->AddPage(html_startpage,"Start Page");
		stc_tabs->SetPageBitmap(0, Icons::getIcon(Icons::GENERAL, "logo"));
		createStartPage();
	}
	else
		html_startpage->Show(false);
#else
	html_startpage = new wxHtmlWindow(stc_tabs, -1, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_NEVER, "startpage");
	html_startpage->SetName("startpage");
	if (show_start_page)
	{
		stc_tabs->AddPage(html_startpage, "Start Page");
		stc_tabs->SetPageBitmap(0, Icons::getIcon(Icons::GENERAL, "logo"));
		createStartPage();
	}
	else
		html_startpage->Show(false);
#endif


	// -- Console Panel --
	ConsolePanel* panel_console = new ConsolePanel(this, -1);

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


	// -- Archive Manager Panel --
	panel_archivemanager = new ArchiveManagerPanel(this, stc_tabs);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Left();
	p_inf.BestSize(192, 480);
	p_inf.Caption("Archive Manager");
	p_inf.Name("archive_manager");
	p_inf.Show(true);
	p_inf.Dock();
	m_mgr->AddPane(panel_archivemanager, p_inf);


	// -- Undo History Panel --
	panel_undo_history = new UndoManagerHistoryPanel(this, NULL);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Right();
	p_inf.BestSize(128, 480);
	p_inf.Caption("Undo History");
	p_inf.Name("undo_history");
	p_inf.Show(false);
	p_inf.Dock();
	m_mgr->AddPane(panel_undo_history, p_inf);


	// -- Menu bar --
	wxMenuBar* menu = new wxMenuBar();
	menu->SetThemeEnabled(false);

	// File menu
	wxMenu* fileNewMenu = new wxMenu("");
	theApp->getAction("aman_newwad")->addToMenu(fileNewMenu, "&Wad Archive");
	theApp->getAction("aman_newzip")->addToMenu(fileNewMenu, "&Zip Archive");
	theApp->getAction("aman_newmap")->addToMenu(fileNewMenu, "&Map");
	wxMenu* fileMenu = new wxMenu("");
	fileMenu->AppendSubMenu(fileNewMenu, "&New", "Create a new Archive");
	theApp->getAction("aman_open")->addToMenu(fileMenu);
	theApp->getAction("aman_opendir")->addToMenu(fileMenu);
	fileMenu->AppendSeparator();
	theApp->getAction("aman_save")->addToMenu(fileMenu);
	theApp->getAction("aman_saveas")->addToMenu(fileMenu);
	theApp->getAction("aman_saveall")->addToMenu(fileMenu);
	fileMenu->AppendSubMenu(panel_archivemanager->getRecentMenu(), "&Recent Files");
	fileMenu->AppendSeparator();
	theApp->getAction("aman_close")->addToMenu(fileMenu);
	theApp->getAction("aman_closeall")->addToMenu(fileMenu);
	fileMenu->AppendSeparator();
	theApp->getAction("main_exit")->addToMenu(fileMenu);
	menu->Append(fileMenu, "&File");

	// Edit menu
	wxMenu* editorMenu = new wxMenu("");
	theApp->getAction("main_undo")->addToMenu(editorMenu);
	theApp->getAction("main_redo")->addToMenu(editorMenu);
	editorMenu->AppendSeparator();
	theApp->getAction("main_setbra")->addToMenu(editorMenu);
	theApp->getAction("main_preferences")->addToMenu(editorMenu);
	menu->Append(editorMenu, "E&dit");

	// View menu
	wxMenu* viewMenu = new wxMenu("");
	theApp->getAction("main_showam")->addToMenu(viewMenu);
	theApp->getAction("main_showconsole")->addToMenu(viewMenu);
	theApp->getAction("main_showundohistory")->addToMenu(viewMenu);
	menu->Append(viewMenu, "&View");

	// Help menu
	wxMenu* helpMenu = new wxMenu("");
	theApp->getAction("main_onlinedocs")->addToMenu(helpMenu);
	theApp->getAction("main_about")->addToMenu(helpMenu);
#ifdef __WXMSW__
	theApp->getAction("main_updatecheck")->addToMenu(helpMenu);
#endif
	menu->Append(helpMenu, "&Help");

	// Set the menu
	SetMenuBar(menu);



	// -- Toolbars --
	toolbar = new SToolBar(this, true);

	// Create File toolbar
	SToolBarGroup* tbg_file = new SToolBarGroup(toolbar, "_File");
	tbg_file->addActionButton("aman_newwad");
	tbg_file->addActionButton("aman_newzip");
	tbg_file->addActionButton("aman_open");
	tbg_file->addActionButton("aman_opendir");
	tbg_file->addActionButton("aman_save");
	tbg_file->addActionButton("aman_saveas");
	tbg_file->addActionButton("aman_saveall");
	tbg_file->addActionButton("aman_close");
	tbg_file->addActionButton("aman_closeall");
	toolbar->addGroup(tbg_file);

	// Create Archive toolbar
	SToolBarGroup* tbg_archive = new SToolBarGroup(toolbar, "_Archive");
	tbg_archive->addActionButton("arch_newentry");
	tbg_archive->addActionButton("arch_newdir");
	tbg_archive->addActionButton("arch_importfiles");
	tbg_archive->addActionButton("arch_texeditor");
	tbg_archive->addActionButton("arch_mapeditor");
	tbg_archive->addActionButton("arch_run");
	toolbar->addGroup(tbg_archive);

	// Create Entry toolbar
	SToolBarGroup* tbg_entry = new SToolBarGroup(toolbar, "_Entry");
	tbg_entry->addActionButton("arch_entry_rename");
	tbg_entry->addActionButton("arch_entry_delete");
	tbg_entry->addActionButton("arch_entry_import");
	tbg_entry->addActionButton("arch_entry_export");
	tbg_entry->addActionButton("arch_entry_moveup");
	tbg_entry->addActionButton("arch_entry_movedown");
	toolbar->addGroup(tbg_entry);

	// Create Base Resource Archive toolbar
	SToolBarGroup* tbg_bra = new SToolBarGroup(toolbar, "_Base Resource", true);
	BaseResourceChooser* brc = new BaseResourceChooser(tbg_bra);
	tbg_bra->addCustomControl(brc);
	tbg_bra->addActionButton("main_setbra", "settings");
	toolbar->addGroup(tbg_bra);

	// Create Palette Chooser toolbar
	SToolBarGroup* tbg_palette = new SToolBarGroup(toolbar, "_Palette", true);
	palette_chooser = new PaletteChooser(tbg_palette, -1);
	palette_chooser->selectPalette(global_palette);
	tbg_palette->addCustomControl(palette_chooser);
	toolbar->addGroup(tbg_palette);

	// Archive and Entry toolbars are initially disabled
	toolbar->enableGroup("_archive", false);
	toolbar->enableGroup("_entry", false);

	// Add toolbar
	m_mgr->AddPane(toolbar, wxAuiPaneInfo().Top().CaptionVisible(false).MinSize(-1, SToolBar::getBarHeight()).Resizable(false).PaneBorder(false).Name("toolbar"));


	// -- Status Bar --
	CreateStatusBar(3);


	// Load previously saved perspective string
	loadLayout();

	// Finalize
	m_mgr->Update();
	Layout();

	// Bind events
#ifdef USE_WEBVIEW_STARTPAGE
	html_startpage->Bind(wxEVT_WEBVIEW_NAVIGATING, &MainWindow::onHTMLLinkClicked, this);
#else
	html_startpage->Bind(wxEVT_COMMAND_HTML_LINK_CLICKED, &MainWindow::onHTMLLinkClicked, this);
#endif
	Bind(wxEVT_SIZE, &MainWindow::onSize, this);
	Bind(wxEVT_CLOSE_WINDOW, &MainWindow::onClose, this);
	Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &MainWindow::onTabChanged, this);
	Bind(wxEVT_STOOLBAR_LAYOUT_UPDATED, &MainWindow::onToolBarLayoutChanged, this, toolbar->GetId());
	Bind(wxEVT_ACTIVATE, &MainWindow::onActivate, this);

	// Initial focus to toolbar
	toolbar->SetFocus();
}

#ifdef USE_WEBVIEW_STARTPAGE
/* MainWindow::createStartPage
 * Builds the HTML start page and loads it into the html viewer
 * (start page tab)
 *******************************************************************/
void MainWindow::createStartPage(bool newtip)
{
	// Get relevant resource entries
	Archive* res_archive = theArchiveManager->programResourceArchive();
	if (!res_archive)
		return;

	// Get entries to export
	vector<ArchiveEntry*> export_entries;
	ArchiveEntry* entry_html = res_archive->entryAtPath("html/startpage.htm");
	ArchiveEntry* entry_tips = res_archive->entryAtPath("tips.txt");
	export_entries.push_back(res_archive->entryAtPath("logo.png"));
	export_entries.push_back(res_archive->entryAtPath("html/box-title-back.png"));

	// Can't do anything without html entry
	if (!entry_html)
	{
		LOG_MESSAGE(1, "No start page resource found");
		html_startpage->SetPage("<html><head><title>SLADE</title></head><body><center><h1>Something is wrong with slade.pk3 :(</h1><center></body></html>", wxEmptyString);
		return;
	}

	// Get html as string
	string html = wxString::FromAscii((const char*)(entry_html->getData()), entry_html->getSize());

	// Generate tip of the day string
	string tip = "It seems tips.txt is missing from your slade.pk3";
	if (entry_tips)
	{
		Tokenizer tz;
		tz.openMem((const char*)entry_tips->getData(), entry_tips->getSize(), entry_tips->getName());
		srand(wxGetLocalTime());
		int numtips = tz.getInteger();
		if (numtips < 2) // Needs at least two choices or it's kinda pointless.
			tip = "Did you know? Something is wrong with the tips.txt file in your slade.pk3.";
		else
		{
			int tipindex = 0;
			if (newtip || lasttipindex == 0)
			{
				// Don't show same tip twice in a row
				do { tipindex = 1 + (rand() % numtips); } while (tipindex == lasttipindex);
			}
			else
				tipindex = lasttipindex;
			
			lasttipindex = tipindex;
			for (int a = 0; a < tipindex; a++)
				tip = tz.getToken();
		}
	}

	// Generate recent files string
	string recent;
	recent += "<table class=\"box\">";
	if (theArchiveManager->numRecentFiles() > 0)
	{
		for (unsigned a = 0; a < 12; a++)
		{
			if (a >= theArchiveManager->numRecentFiles())
				break;	// No more recent files

			recent += "<tr><td valign=\"middle\" class=\"box\">";

			// Determine icon
			string fn = theArchiveManager->recentFile(a);
			string icon = "archive";
			if (fn.EndsWith(".wad"))
				icon = "wad";
			else if (fn.EndsWith(".zip") || fn.EndsWith(".pk3") || fn.EndsWith(".pke"))
				icon = "zip";
			else if (wxDirExists(fn))
				icon = "folder";

			// Add recent file link
			recent += S_FMT("<img src=\"%s.png\"></td><td valign=\"top\" class=\"box\">", icon);
			recent += S_FMT("<a href=\"recent://%d\">%s</a></td></tr>", a, fn);
		}
	}
	else
		recent += "<tr><td valign=\"top\" class=\"box\">No recently opened files</td></tr>";
	recent += "</table>";

	// Insert tip and recent files into html
	html.Replace("#recent#", recent);
	html.Replace("#totd#", tip);

	// Write html and images to temp folder
	for (unsigned a = 0; a < export_entries.size(); a++)
		export_entries[a]->exportFile(appPath(export_entries[a]->getName(), DIR_TEMP));
	Icons::exportIconPNG(Icons::ENTRY, "archive", appPath("archive.png", DIR_TEMP));
	Icons::exportIconPNG(Icons::ENTRY, "wad", appPath("wad.png", DIR_TEMP));
	Icons::exportIconPNG(Icons::ENTRY, "zip", appPath("zip.png", DIR_TEMP));
	Icons::exportIconPNG(Icons::ENTRY, "folder", appPath("folder.png", DIR_TEMP));
	string html_file = appPath("startpage.htm", DIR_TEMP);
	wxFile outfile(html_file, wxFile::write);
	outfile.Write(html);
	outfile.Close();

#ifdef __WXGTK__
	html_file = "file://" + html_file;
#endif

	// Load page
	html_startpage->ClearHistory();
	html_startpage->LoadURL(html_file);

#ifdef __WXMSW__
	html_startpage->Reload();
#endif
}

#else

/* MainWindow::createStartPage
 * Builds the HTML start page and loads it into the html viewer
 * (start page tab)
 *******************************************************************/
void MainWindow::createStartPage(bool newtip)
{
	// Get relevant resource entries
	Archive* res_archive = theArchiveManager->programResourceArchive();
	if (!res_archive)
		return;
	ArchiveEntry* entry_html = res_archive->entryAtPath("html/startpage_basic.htm");
	ArchiveEntry* entry_logo = res_archive->entryAtPath("logo.png");
	ArchiveEntry* entry_tips = res_archive->entryAtPath("tips.txt");

	// Can't do anything without html entry
	if (!entry_html)
	{
		html_startpage->SetPage("<html><head><title>SLADE</title></head><body><center><h1>Something is wrong with slade.pk3 :(</h1><center></body></html>");
		return;
	}

	// Get html as string
	string html = wxString::FromAscii((const char*)(entry_html->getData()), entry_html->getSize());

	// Generate tip of the day string
	string tip = "It seems tips.txt is missing from your slade.pk3";
	if (entry_tips)
	{
		Tokenizer tz;
		tz.openMem((const char*)entry_tips->getData(), entry_tips->getSize(), entry_tips->getName());
		srand(wxGetLocalTime());
		int numtips = tz.getInteger();
		if (numtips < 2) // Needs at least two choices or it's kinda pointless.
			tip = "Did you know? Something is wrong with the tips.txt file in your slade.pk3.";
		else
		{
			int tipindex = 0;
			// Don't show same tip twice in a row
			do { tipindex = 1 + (rand() % numtips); } while (tipindex == lasttipindex);
			lasttipindex = tipindex;
			for (int a = 0; a < tipindex; a++)
				tip = tz.getToken();
		}
	}

	// Generate recent files string
	string recent;
	for (unsigned a = 0; a < 12; a++)
	{
		if (a >= theArchiveManager->numRecentFiles())
			break;	// No more recent files

		// Add line break if needed
		if (a > 0) recent += "<br/>\n";

		// Add recent file link
		recent += S_FMT("<a href=\"recent://%d\">%s</a>", a, theArchiveManager->recentFile(a));
	}

	// Insert tip and recent files into html
	html.Replace("#recent#", recent);
	html.Replace("#totd#", tip);

	// Write html and images to temp folder
	if (entry_logo) entry_logo->exportFile(appPath("logo.png", DIR_TEMP));
	string html_file = appPath("startpage_basic.htm", DIR_TEMP);
	wxFile outfile(html_file, wxFile::write);
	outfile.Write(html);
	outfile.Close();

	// Load page
	html_startpage->LoadPage(html_file);

	// Clean up
	wxRemoveFile(html_file);
	wxRemoveFile(appPath("logo.png", DIR_TEMP));
}
#endif

/* MainWindow::exitProgram
 * Attempts to exit the program. Only fails if an unsaved archive is
 * found and the user cancels the exit
 *******************************************************************/
bool MainWindow::exitProgram()
{
	// Confirm exit
	if (confirm_exit && !panel_archivemanager->askedSaveUnchanged())
	{
		if (wxMessageBox("Are you sure you want to exit SLADE?", "SLADE", wxICON_QUESTION|wxYES_NO, this) != wxYES)
			return false;
	}

	// Check if we can close the map editor
	if (theMapEditor->IsShown())
		if (!theMapEditor->Close())
			return false;

	// Close all archives
	if (!panel_archivemanager->closeAll())
		return false;

	// Save current layout
	//main_window_layout = m_mgr->SavePerspective();
	saveLayout();
	mw_maximized = IsMaximized();
	if (!IsMaximized())
		Misc::setWindowInfo(id, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);

	// Save selected palette
	global_palette = palette_chooser->GetStringSelection();

	// Exit application
	wxTheApp->Exit();

	return true;
}

/* MainWindow::getCurrentArchive
 * Returns the currently open archive (ie the current tab's archive,
 * if any)
 *******************************************************************/
Archive* MainWindow::getCurrentArchive()
{
	return panel_archivemanager->currentArchive();
}

/* MainWindow::getCurrentEntry
 * Returns the currently open entry (current tab -> current entry
 * panel)
 *******************************************************************/
ArchiveEntry* MainWindow::getCurrentEntry()
{
	return panel_archivemanager->currentEntry();
}

/* MainWindow::getCurrentEntrySelection
 * Returns a list of all currently selected entries, in the current
 * archive panel
 *******************************************************************/
vector<ArchiveEntry*> MainWindow::getCurrentEntrySelection()
{
	return panel_archivemanager->currentEntrySelection();
}

/* MainWindow::openTextureEditor
 * Opens the texture editor for the current archive tab
 *******************************************************************/
void MainWindow::openTextureEditor(Archive* archive, ArchiveEntry* entry)
{
	panel_archivemanager->openTextureTab(theArchiveManager->archiveIndex(archive), entry);
}

/* MainWindow::openMapEditor
 * Opens the map editor for the current archive tab
 *******************************************************************/
void MainWindow::openMapEditor(Archive* archive)
{
	theMapEditor->chooseMap(archive);
}

/* MainWindow::openEntry
 * Opens [entry] in its own tab
 *******************************************************************/
void MainWindow::openEntry(ArchiveEntry* entry)
{
	panel_archivemanager->openEntryTab(entry);
}

/* MainWindow::openDocs
 * Opens [entry] in its own tab
 *******************************************************************/
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

/* MainWindow::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
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
		wxDialog dialog_ebr(this, -1, "Edit Base Resource Archives", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
		BaseResourceArchivesPanel brap(&dialog_ebr);

		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(&brap, 1, wxEXPAND|wxALL, 4);

		sizer->Add(dialog_ebr.CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxDOWN, 4);

		dialog_ebr.SetSizer(sizer);
		dialog_ebr.Layout();
		dialog_ebr.SetInitialSize(wxSize(500, 300));
		dialog_ebr.CenterOnParent();
		if (dialog_ebr.ShowModal() == wxID_OK)
			theArchiveManager->openBaseResource(brap.getSelectedPath());

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
		p_inf.MinSize(200, 128);
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
		string icon_filename = appPath("slade.ico", DIR_TEMP);
		theArchiveManager->programResourceArchive()->getEntry("slade.ico")->exportFile(icon_filename);
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
		theApp->checkForUpdates(true);
		return true;
	}

	// Unknown action
	return false;
}


/*******************************************************************
 * MAINWINDOW EVENTS
 *******************************************************************/

#ifdef USE_WEBVIEW_STARTPAGE

/* MainWindow::onHTMLLinkClicked
 * Called when a link is clicked on the HTML Window, so that
 * external (http) links are opened in the default browser
 *******************************************************************/
void MainWindow::onHTMLLinkClicked(wxEvent& e)
{
	wxWebViewEvent& ev = (wxWebViewEvent&)e;
	string href = ev.GetURL();

#ifdef __WXGTK__
	if (!href.EndsWith("startpage.htm"))
		href.Replace("file://", "");
#endif

	//LOG_MESSAGE(2, "URL %s", href);

	if (href.EndsWith("/"))
		href.RemoveLast(1);

	if (href.StartsWith("http://"))
	{
		wxLaunchDefaultBrowser(ev.GetURL());
		ev.Veto();
	}
	else if (href.StartsWith("recent://"))
	{
		// Recent file
		string rs = href.Mid(9);
		unsigned long index = 0;
		rs.ToULong(&index);
		theApp->doAction("aman_recent", index);
		createStartPage();
		html_startpage->Reload();
	}
	else if (href.StartsWith("action://"))
	{
		// Action
		if (href.EndsWith("open"))
			theApp->doAction("aman_open");
		else if (href.EndsWith("newwad"))
			theApp->doAction("aman_newwad");
		else if (href.EndsWith("newzip"))
			theApp->doAction("aman_newzip");
		else if (href.EndsWith("newmap"))
		{
			theApp->doAction("aman_newmap");
			return;
		}
		else if (href.EndsWith("reloadstartpage"))
			createStartPage();
		html_startpage->Reload();
	}
	else if (wxFileExists(href))
	{
		// Navigating to file, open it
		string page = appPath("startpage.htm", DIR_TEMP);
		if (wxFileName(href).GetLongPath() != wxFileName(page).GetLongPath())
			theArchiveManager->openArchive(href);
		ev.Veto();
	}
	else if (wxDirExists(href))
	{
		// Navigating to folder, open it
		theArchiveManager->openDirArchive(href);
		ev.Veto();
	}
}

#else

/* MainWindow::onHTMLLinkClicked
 * Called when a link is clicked on the HTML Window, so that
 * external (http) links are opened in the default browser
 *******************************************************************/
void MainWindow::onHTMLLinkClicked(wxEvent& e)
{
	wxHtmlLinkEvent& ev = (wxHtmlLinkEvent&)e;
	string href = ev.GetLinkInfo().GetHref();

	if (href.StartsWith("http://"))
		wxLaunchDefaultBrowser(ev.GetLinkInfo().GetHref());
	else if (href.StartsWith("recent://"))
	{
		// Recent file
		string rs = href.Mid(9);
		unsigned long index = 0;
		rs.ToULong(&index);
		theApp->doAction("aman_recent", index);
		createStartPage();
	}
	else if (href.StartsWith("action://"))
	{
		// Action
		if (href.EndsWith("open"))
			theApp->doAction("aman_open");
		else if (href.EndsWith("newwad"))
			theApp->doAction("aman_newwad");
		else if (href.EndsWith("newzip"))
			theApp->doAction("aman_newzip");
		else if (href.EndsWith("newmap"))
			theApp->doAction("aman_newmap");
		else if (href.EndsWith("reloadstartpage"))
			createStartPage();
	}
	else
		html_startpage->OnLinkClicked(ev.GetLinkInfo());
}

#endif

/* MainWindow::onClose
 * Called when the window is closed
 *******************************************************************/
void MainWindow::onClose(wxCloseEvent& e)
{
	if (!exitProgram())
		e.Veto();
}

/* MainWindow::onTabChanged
 * Called when the current tab is changed
 *******************************************************************/
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
		panel_undo_history->setManager(((ArchivePanel*)page)->getUndoManager());

	// Continue
	e.Skip();
}

/* MainWindow::onSize
 * Called when the window is resized
 *******************************************************************/
void MainWindow::onSize(wxSizeEvent& e)
{
	// Update toolbar layout (if needed)
	toolbar->updateLayout();
#ifndef __WXMSW__
	m_mgr->GetPane(toolbar).MinSize(-1, toolbar->minHeight());
	m_mgr->Update();
#endif

	// Update maximized cvar
	mw_maximized = IsMaximized();

	e.Skip();
}

/* MainWindow::onToolBarLayoutChanged
 * Called when the toolbar layout is changed
 *******************************************************************/
void MainWindow::onToolBarLayoutChanged(wxEvent& e)
{
	// Update toolbar size
	m_mgr->GetPane(toolbar).MinSize(-1, toolbar->minHeight());
	m_mgr->Update();
}

/* MainWindow::onActivate
 * Called when the window is activated
 *******************************************************************/
void MainWindow::onActivate(wxActivateEvent& e)
{
	if (!e.GetActive() || this->IsBeingDeleted())
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
