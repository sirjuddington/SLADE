
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2012 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapEditorWindow.cpp
 * Description: MapEditorWindow class, it's a map editor window.
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
#include "WxStuff.h"
#include "MapEditorWindow.h"
#include "MainApp.h"
#include "ConsolePanel.h"
#include "BaseResourceArchivesPanel.h"
#include "PreferencesDialog.h"
#include "ArchiveManager.h"
#include "MapObjectPropsPanel.h"
#include "MainWindow.h"
#include "SToolBar.h"
#include "WadArchive.h"
#include "SFileDialog.h"
#include "NodeBuilders.h"
#include "ShapeDrawPanel.h"
#include "ScriptEditorPanel.h"
#include <wx/aui/aui.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
MapEditorWindow* MapEditorWindow::instance = NULL;
CVAR(Int, mew_width, 1024, CVAR_SAVE);
CVAR(Int, mew_height, 768, CVAR_SAVE);
CVAR(Int, mew_left, -1, CVAR_SAVE);
CVAR(Int, mew_top, -1, CVAR_SAVE);
CVAR(Bool, mew_maximized, true, CVAR_SAVE);
CVAR(String, nodebuilder_id, "zdbsp", CVAR_SAVE);
CVAR(String, nodebuilder_options, "", CVAR_SAVE);


/*******************************************************************
 * EXTERNAL VARIABLES
 *******************************************************************/
EXTERN_CVAR(Int, flat_drawtype);


/*******************************************************************
 * MAPEDITORWINDOW CLASS FUNCTIONS
 *******************************************************************/

/* MapEditorWindow::MapEditorWindow
 * MapEditorWindow class constructor
 *******************************************************************/
MapEditorWindow::MapEditorWindow()
: STopWindow("SLADE", mew_left, mew_top, mew_width, mew_height) {
	if (mew_maximized) Maximize();
	setupLayout();
	Show(false);
	custom_menus_begin = 2;

	// Set icon
	string icon_filename = appPath("slade.ico", DIR_TEMP);
	theArchiveManager->programResourceArchive()->getEntry("slade.ico")->exportFile(icon_filename);
	SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
	wxRemoveFile(icon_filename);

	// Bind events
	Bind(wxEVT_CLOSE_WINDOW, &MapEditorWindow::onClose, this);
	Bind(wxEVT_MOVE, &MapEditorWindow::onMove, this);
	Bind(wxEVT_SIZE, &MapEditorWindow::onSize, this);
}

/* MapEditorWindow::~MapEditorWindow
 * MapEditorWindow class destructor
 *******************************************************************/
MapEditorWindow::~MapEditorWindow() {
}

/* MapEditorWindow::loadLayout
 * Loads the previously saved layout file for the window
 *******************************************************************/
void MapEditorWindow::loadLayout() {
	// Open layout file
	Tokenizer tz;
	if (!tz.openFile(appPath("mapwindow.layout", DIR_USER)))
		return;

	// Parse layout
	wxAuiManager *m_mgr = wxAuiManager::GetManager(this);
	while (true) {
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

/* MapEditorWindow::saveLayout
 * Saves the current window layout to a file
 *******************************************************************/
void MapEditorWindow::saveLayout() {
	// Open layout file
	wxFile file(appPath("mapwindow.layout", DIR_USER), wxFile::write);

	// Write component layout
	wxAuiManager *m_mgr = wxAuiManager::GetManager(this);

	// Console pane
	file.Write("\"console\" ");
	string pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("console"));
	file.Write(S_FMT("\"%s\"\n", CHR(pinf)));

	// Item info pane
	file.Write("\"item_props\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("item_props"));
	file.Write(S_FMT("\"%s\"\n", CHR(pinf)));

	// Shape drawing pane
	file.Write("\"shape_draw\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("shape_draw"));
	file.Write(S_FMT("\"%s\"\n", CHR(pinf)));

	// Script editor pane
	file.Write("\"script_editor\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("script_editor"));
	file.Write(S_FMT("\"%s\"\n", CHR(pinf)));

	// Close file
	file.Close();
}

/* MapEditorWindow::setupLayout
 * Sets up the basic map editor window layout
 *******************************************************************/
void MapEditorWindow::setupLayout() {
	// Create the wxAUI manager & related things
	wxAuiManager* m_mgr = new wxAuiManager(this);
	wxAuiPaneInfo p_inf;

	// Map canvas
	map_canvas = new MapCanvas(this, -1, &editor);
	p_inf.CenterPane();
	m_mgr->AddPane(map_canvas, p_inf);

	// --- Menus ---
	wxMenuBar *menu = new wxMenuBar();

	// Map menu
	wxMenu* menu_map = new wxMenu("");
	theApp->getAction("mapw_save")->addToMenu(menu_map);
	theApp->getAction("mapw_saveas")->addToMenu(menu_map);
	theApp->getAction("mapw_rename")->addToMenu(menu_map);
	menu->Append(menu_map, "&Map");

	// Edit menu
	wxMenu* menu_editor = new wxMenu("");
	theApp->getAction("mapw_undo")->addToMenu(menu_editor);
	theApp->getAction("mapw_redo")->addToMenu(menu_editor);
	menu_editor->AppendSeparator();
	theApp->getAction("mapw_preferences")->addToMenu(menu_editor);
	theApp->getAction("mapw_setbra")->addToMenu(menu_editor);
	menu->Append(menu_editor, "&Edit");

	// View menu
	wxMenu* menu_view = new wxMenu("");
	theApp->getAction("mapw_showproperties")->addToMenu(menu_view);
	theApp->getAction("mapw_showconsole")->addToMenu(menu_view);
	theApp->getAction("mapw_showdrawoptions")->addToMenu(menu_view);
	theApp->getAction("mapw_showscripteditor")->addToMenu(menu_view);
	menu->Append(menu_view, "View");

	SetMenuBar(menu);


	// --- Toolbars ---
	toolbar = new SToolBar(this);

	// Map toolbar
	SToolBarGroup* tbg_map = new SToolBarGroup(toolbar, "_Map");
	tbg_map->addActionButton("mapw_save");
	tbg_map->addActionButton("mapw_saveas");
	tbg_map->addActionButton("mapw_rename");
	toolbar->addGroup(tbg_map);

	// Edit toolbar
	/*
	SToolBarGroup* tbg_edit = new SToolBarGroup(toolbar, "_Edit");
	tbg_edit->addActionButton("mapw_undo");
	tbg_edit->addActionButton("mapw_redo");
	toolbar->addGroup(tbg_edit);
	*/

	// Mode toolbar
	SToolBarGroup* tbg_mode = new SToolBarGroup(toolbar, "_Mode");
	tbg_mode->addActionButton("mapw_mode_vertices");
	tbg_mode->addActionButton("mapw_mode_lines");
	tbg_mode->addActionButton("mapw_mode_sectors");
	tbg_mode->addActionButton("mapw_mode_things");
	theApp->toggleAction("mapw_mode_lines");	// Lines mode by default
	toolbar->addGroup(tbg_mode);

	// Flat type toolbar
	SToolBarGroup* tbg_flats = new SToolBarGroup(toolbar, "_Flats Type");
	tbg_flats->addActionButton("mapw_flat_none");
	tbg_flats->addActionButton("mapw_flat_untextured");
	tbg_flats->addActionButton("mapw_flat_textured");
	toolbar->addGroup(tbg_flats);

	// Toggle current flat type
	if (flat_drawtype == 0) theApp->toggleAction("mapw_flat_none");
	else if (flat_drawtype == 1) theApp->toggleAction("mapw_flat_untextured");
	else theApp->toggleAction("mapw_flat_textured");

	// Add toolbar
	m_mgr->AddPane(toolbar, wxAuiPaneInfo().Top().CaptionVisible(false).MinSize(-1, 30).Resizable(false).PaneBorder(false).Name("toolbar"));


	// Status bar
	CreateStatusBar();

	// -- Console Panel --
	ConsolePanel *panel_console = new ConsolePanel(this, -1);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Bottom();
	p_inf.Dock();
	p_inf.BestSize(480, 192);
	p_inf.FloatingSize(600, 400);
	p_inf.FloatingPosition(100, 100);
	p_inf.MinSize(-1, 192);
	p_inf.Show(false);
	p_inf.Caption("Console");
	p_inf.Name("console");
	m_mgr->AddPane(panel_console, p_inf);


	// -- Map Object Properties Panel --
	panel_obj_props = new MapObjectPropsPanel(this);

	// Setup panel info & add panel
	p_inf.Right();
	p_inf.BestSize(256, 256);
	p_inf.FloatingSize(400, 600);
	p_inf.FloatingPosition(120, 120);
	p_inf.MinSize(256, 256);
	p_inf.Show(true);
	p_inf.Caption("Item Properties");
	p_inf.Name("item_props");
	m_mgr->AddPane(panel_obj_props, p_inf);


	// --- Shape Draw Options Panel ---
	ShapeDrawPanel* panel_shapedraw = new ShapeDrawPanel(this);

	// Setup panel info & add panel
	wxSize msize = panel_shapedraw->GetMinSize();
	p_inf.Float();
	p_inf.BestSize(msize.x, msize.y);
	p_inf.FloatingSize(msize.x, msize.y);
	p_inf.FloatingPosition(140, 140);
	p_inf.MinSize(msize.x, msize.y);
	p_inf.Show(false);
	p_inf.Caption("Shape Drawing");
	p_inf.Name("shape_draw");
	m_mgr->AddPane(panel_shapedraw, p_inf);


	// --- Script Editor Panel ---
	panel_script_editor = new ScriptEditorPanel(this);

	// Setup panel info & add panel
	p_inf.Float();
	p_inf.BestSize(300, 300);
	p_inf.FloatingSize(500, 400);
	p_inf.FloatingPosition(150, 150);
	p_inf.MinSize(300, 300);
	p_inf.Show(true);
	p_inf.Caption("Script Editor");
	p_inf.Name("script_editor");
	m_mgr->AddPane(panel_script_editor, p_inf);


	// Load previously saved window layout
	loadLayout();

	m_mgr->Update();
	Layout();
}

void MapEditorWindow::lockMapEntries(bool lock) {
	// Don't bother if no map is open
	if (!mdesc_current.head)
		return;

	// Just lock/unlock the 'head' entry if it's a pk3 map
	if (mdesc_current.archive) {
		if (lock)
			mdesc_current.head->lock();
		else
			mdesc_current.head->unlock();

		return;
	}
}

bool MapEditorWindow::openMap(Archive::mapdesc_t map) {
	// Get map parent archive
	Archive* archive = map.head->getParent();

	// Set texture manager archive
	tex_man.setArchive(archive);

	// Clear current map
	closeMap();

	// Attempt to open map
	bool ok = editor.openMap(map);

	// Show window if opened ok
	if (ok) {
		mdesc_current = map;
		
		// Read DECORATE definitions if any
		theGameConfiguration->parseDecorateDefs(archive);

		// Load scripts if any
		loadMapScripts(map);

		// Lock map entries
		lockMapEntries();

		this->Show(true);
		map_canvas->viewFitToMap();
		map_canvas->Refresh();

		// Set window title
		SetTitle(S_FMT("SLADE - %s of %s", CHR(map.name), CHR(archive->getFilename(false))));
	}

	return ok;
}

void MapEditorWindow::loadMapScripts(Archive::mapdesc_t map) {
	// Don't bother if no scripting language specified
	if (theGameConfiguration->scriptLanguage().IsEmpty())
		return;

	// Check for pk3 map
	if (map.archive) {
		WadArchive* wad = new WadArchive();
		wad->open(map.head->getMCData());
		vector<Archive::mapdesc_t> maps = wad->detectMaps();
		if (!maps.empty()) {
			loadMapScripts(maps[0]);
			wad->close();
			delete wad;
			return;
		}
	}

	// Go through map entries
	ArchiveEntry* entry = map.head->nextEntry();
	ArchiveEntry* scripts = NULL;
	ArchiveEntry* compiled = NULL;
	while (entry && entry != map.end->nextEntry()) {
		// Check for SCRIPTS/BEHAVIOR
		if (theGameConfiguration->scriptLanguage() == "acs_hexen" ||
			theGameConfiguration->scriptLanguage() == "acs_zdoom") {
			if (S_CMPNOCASE(entry->getName(), "SCRIPTS"))
				scripts = entry;
			if (S_CMPNOCASE(entry->getName(), "BEHAVIOR"))
				compiled = entry;
		}

		// Next entry
		entry = entry->nextEntry();
	}

	// Open scripts/compiled if found
	panel_script_editor->openScripts(scripts, compiled);
}

bool nb_warned = false;
void MapEditorWindow::buildNodes(Archive* wad) {
	NodeBuilders::builder_t builder;
	string command;
	string options;

	// Save wad to disk
	string filename = appPath("sladetemp.wad", DIR_TEMP);
	wad->save(filename);

	// Get current nodebuilder
	builder = NodeBuilders::getBuilder(nodebuilder_id);
	command = builder.command;
	options = nodebuilder_options;

	// Switch to ZDBSP if UDMF
	if (mdesc_current.format == MAP_UDMF && nodebuilder_id != "zdbsp") {
		wxMessageBox("Nodebuilder switched to ZDBSP for UDMF format", "Save Map", wxICON_INFORMATION);
		builder = NodeBuilders::getBuilder("zdbsp");
		command = builder.command;
	}

	// Check for undefined path
	if (!wxFileExists(builder.path) && !nb_warned) {
		// Open nodebuilder preferences
		PreferencesDialog::openPreferences(this, "Node Builders");

		// Get new builder if one was selected
		builder = NodeBuilders::getBuilder(nodebuilder_id);
		string command = builder.command;

		// Check again
		if (!wxFileExists(builder.path)) {
			wxMessageBox("No valid Node Builder is currently configured, nodes will not be built!", "Warning", wxICON_WARNING);
			nb_warned = true;
		}
	}

	// Build command line
	command.Replace("$f", S_FMT("\"%s\"", CHR(filename)));
	command.Replace("$o", CHR(wxString(options)));

	// Run nodebuilder
	if (wxFileExists(builder.path)) {
		wxArrayString out;
		wxLogMessage("execute \"%s\"", CHR(command));
		wxExecute(S_FMT("\"%s\" %s", CHR(builder.path), CHR(command)), out, wxEXEC_HIDE_CONSOLE);
		Raise();
		wxLogMessage("Nodebuilder output:");
		for (unsigned a = 0; a < out.size(); a++)
			wxLogMessage(out[a]);

		// Re-load wad
		wad->close();
		wad->open(filename);
	}
	else if (nb_warned)
		wxLogMessage("Nodebuilder path not set up, no nodes were built");
}

bool MapEditorWindow::saveMap() {
	// Get map data entries
	vector<ArchiveEntry*> map_data;
	if (mdesc_current.format == MAP_DOOM)
		editor.getMap().writeDoomMap(map_data);
	else if (mdesc_current.format == MAP_HEXEN)
		editor.getMap().writeHexenMap(map_data);
	else if (mdesc_current.format == MAP_UDMF) {
		ArchiveEntry* udmf = new ArchiveEntry("TEXTMAP");
		editor.getMap().writeUDMFMap(udmf);
		map_data.push_back(udmf);
	}
	else // TODO: doom64
		return false;

	// Check script language
	bool acs = false;
	if (theGameConfiguration->scriptLanguage() == "acs_hexen" ||
		theGameConfiguration->scriptLanguage() == "acs_zdoom")
		acs = true;

	// Add map data to temporary wad
	WadArchive* wad = new WadArchive();
	wad->addNewEntry("MAP01");
	for (unsigned a = 0; a < map_data.size(); a++)
		wad->addEntry(map_data[a]);
	if (acs) // BEHAVIOR
		wad->addEntry(panel_script_editor->compiledEntry(), "", true);
	if (acs && panel_script_editor->scriptEntry()->getSize() > 0) // SCRIPTS (if any)
		wad->addEntry(panel_script_editor->scriptEntry(), "", true);
	if (mdesc_current.format == MAP_UDMF)
		wad->addNewEntry("ENDMAP");

	// Check for map archive
	Archive* tempwad = NULL;
	Archive::mapdesc_t map = mdesc_current;
	if (mdesc_current.archive && mdesc_current.head) {
		tempwad = new WadArchive();
		tempwad->open(mdesc_current.head);
		vector<Archive::mapdesc_t> amaps = tempwad->detectMaps();
		if (amaps.size() > 0)
			map = amaps[0];
		else
			return false;
	}

	// Build nodes
	buildNodes(wad);

	// Unlock current map entries
	lockMapEntries(false);

	// Delete current map entries
	ArchiveEntry* entry = map.end;
	Archive* archive = map.head->getParent();
	while (entry && entry != map.head) {
		ArchiveEntry* prev = entry->prevEntry();
		archive->removeEntry(entry);
		entry = prev;
	}

	// Add new map entries
	for (unsigned a = 1; a < wad->numEntries(); a++)
		entry = archive->addEntry(wad->getEntry(a), archive->entryIndex(map.head) + a, NULL, true);

	// Clean up
	delete wad;
	if (tempwad) {
		tempwad->save();
		delete tempwad;
	}
	else {
		// Update map description
		mdesc_current.end = entry;
	}

	// Finish
	lockMapEntries();
	editor.getMap().setOpenedTime();

	return true;
}

bool MapEditorWindow::saveMapAs() {
	// Show dialog
	SFileDialog::fd_info_t info;
	if (!SFileDialog::saveFile(info, "Save Map As", "Wad Archives (*.wad)|*.wad", this))
		return false;

	// Create new, empty wad
	WadArchive wad;
	ArchiveEntry* head = wad.addNewEntry(mdesc_current.name);
	ArchiveEntry* end = NULL;
	if (mdesc_current.format == MAP_UDMF) {
		wad.addNewEntry("TEXTMAP");
		end = wad.addNewEntry("ENDMAP");
	}
	else {
		wad.addNewEntry("THINGS");
		wad.addNewEntry("LINEDEFS");
		wad.addNewEntry("SIDEDEFS");
		wad.addNewEntry("VERTEXES");
		end = wad.addNewEntry("SECTORS");
	}

	// Update current map description
	mdesc_current.head = head;
	mdesc_current.archive = false;
	mdesc_current.end = end;
	//mdesc_current.format = theGameConfiguration->getMapFormat();

	// Save map data
	saveMap();

	// Write wad to file
	wad.save(info.filenames[0]);
	theArchiveManager->openArchive(info.filenames[0], true, true);

	// Set window title
	SetTitle(S_FMT("SLADE - %s of %s", CHR(mdesc_current.name), CHR(wad.getFilename(false))));

	return true;
}

void MapEditorWindow::closeMap() {
	// Close map in editor
	editor.clearMap();

	// Unlock current map entries
	lockMapEntries(false);

	// Clear map info
	mdesc_current.head = NULL;
}

void MapEditorWindow::forceRefresh(bool renderer) {
	if (renderer) map_canvas->forceRefreshRenderer();
	map_canvas->Refresh();
}

void MapEditorWindow::refreshToolBar() {
	toolbar->Refresh();
}

/* MapEditorWindow::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool MapEditorWindow::handleAction(string id) {
	// Don't handle actions if hidden
	if (!IsShown())
		return false;

	// File->Save
	if (id == "mapw_save") {
		// Save map
		saveMap();

		// Save archive
		Archive* a = currentMapDesc().head->getParent();
		if (a) a->save();

		return true;
	}

	// File->Save As
	if (id == "mapw_saveas") {
		saveMapAs();
		return true;
	}

	// Edit->Undo
	if (id == "mapw_undo") {
		editor.doUndo();
		return true;
	}

	// Edit->Redo
	if (id == "mapw_redo") {
		editor.doRedo();
		return true;
	}

	// Editor->Set Base Resource Archive
	if (id == "mapw_setbra") {
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

	// Editor->Preferences
	if (id == "mapw_preferences") {
		PreferencesDialog::openPreferences(this);

		return true;
	}

	// View->Item Properties
	if (id == "mapw_showproperties") {
		wxAuiManager *m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("item_props");

		// Toggle window and focus
		p_inf.Show(!p_inf.IsShown());
		map_canvas->SetFocus();

		m_mgr->Update();
		return true;
	}

	// View->Console
	else if (id == "mapw_showconsole") {
		wxAuiManager *m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("console");

		// Toggle window and focus
		if (p_inf.IsShown()) {
			p_inf.Show(false);
			map_canvas->SetFocus();
		}
		else {
			p_inf.Show(true);
			p_inf.window->SetFocus();
		}

		p_inf.MinSize(200, 128);
		m_mgr->Update();
		return true;
	}

	// View->Shape Draw Options
	else if (id == "mapw_showdrawoptions") {
		wxAuiManager *m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("shape_draw");

		// Toggle window and focus
		p_inf.Show(!p_inf.IsShown());
		map_canvas->SetFocus();

		m_mgr->Update();
		return true;
	}

	// View->Script Editor
	else if (id == "mapw_showscripteditor") {
		wxAuiManager *m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("script_editor");

		// Toggle window and focus
		if (p_inf.IsShown()) {
			p_inf.Show(false);
			map_canvas->SetFocus();
		}
		else {
			p_inf.Show(true);
			p_inf.window->SetFocus();
		}

		p_inf.MinSize(200, 128);
		m_mgr->Update();
		return true;
	}

	return false;
}

/* MapEditorWindow::onClose
 * Called when the window is closed
 *******************************************************************/
void MapEditorWindow::onClose(wxCloseEvent& e) {
	if (editor.getMap().isModified()) {
		wxMessageDialog md(this, S_FMT("Save changes to %s", CHR(currentMapDesc().name)), "Unsaved Changes", wxYES_NO|wxCANCEL);
		int answer = md.ShowModal();
		if (answer == wxID_YES)
			saveMap();
		else if (answer == wxID_CANCEL) {
			e.Veto();
			return;
		}
	}

	// Save current layout
	saveLayout();

	this->Show(false);
	closeMap();
}

/* MapEditorWindow::onSize
 * Called when the window is resized
 *******************************************************************/
void MapEditorWindow::onSize(wxSizeEvent& e) {
	// Update window size settings, but only if not maximized
	if (!IsMaximized()) {
		mew_width = GetSize().x;
		mew_height = GetSize().y;
	}

	// Update maximized cvar
	mew_maximized = IsMaximized();

	e.Skip();
}

/* MapEditorWindow::onMove
 * Called when the window moves
 *******************************************************************/
void MapEditorWindow::onMove(wxMoveEvent& e) {
	// Update window position settings, but only if not maximized
	if (!IsMaximized()) {
		mew_left = GetPosition().x;
		mew_top = GetPosition().y;
	}

	e.Skip();
}
