
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
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
#include "MapEditorWindow.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/WadArchive.h"
#include "Dialogs/MapEditorConfigDialog.h"
#include "Dialogs/Preferences/BaseResourceArchivesPanel.h"
#include "Dialogs/Preferences/PreferencesDialog.h"
#include "Dialogs/RunDialog.h"
#include "General/Misc.h"
#include "MainEditor/MainWindow.h"
#include "MapBackupManager.h"
#include "NodeBuilders.h"
#include "UI/ConsolePanel.h"
#include "UI/MapCanvas.h"
#include "UI/MapChecksPanel.h"
#include "UI/ObjectEditPanel.h"
#include "UI/PropsPanel/MapObjectPropsPanel.h"
#include "UI/SAuiTabArt.h"
#include "UI/ScriptEditorPanel.h"
#include "UI/ShapeDrawPanel.h"
#include "UI/SplashWindow.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/UndoManagerHistoryPanel.h"
#include "Utility/SFileDialog.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
MapEditorWindow* MapEditorWindow::instance = NULL;
CVAR(Bool, mew_maximized, true, CVAR_SAVE);
CVAR(String, nodebuilder_id, "zdbsp", CVAR_SAVE);
CVAR(String, nodebuilder_options, "", CVAR_SAVE);
CVAR(Bool, save_archive_with_map, true, CVAR_SAVE);


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
	: STopWindow("SLADE", "map")
{
	if (mew_maximized) Maximize();
	setupLayout();
	Show(false);
	custom_menus_begin = 2;
	backup_manager = new MapBackupManager();

	// Set icon
	string icon_filename = appPath("slade.ico", DIR_TEMP);
	theArchiveManager->programResourceArchive()->getEntry("slade.ico")->exportFile(icon_filename);
	SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
	wxRemoveFile(icon_filename);

	// Bind events
	Bind(wxEVT_CLOSE_WINDOW, &MapEditorWindow::onClose, this);
	Bind(wxEVT_SIZE, &MapEditorWindow::onSize, this);
}

/* MapEditorWindow::~MapEditorWindow
 * MapEditorWindow class destructor
 *******************************************************************/
MapEditorWindow::~MapEditorWindow()
{
	wxAuiManager::GetManager(this)->UnInit();
	delete backup_manager;
}

/* MapEditorWindow::loadLayout
 * Loads the previously saved layout file for the window
 *******************************************************************/
void MapEditorWindow::loadLayout()
{
	// Open layout file
	Tokenizer tz;
	if (!tz.openFile(appPath("mapwindow.layout", DIR_USER)))
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

/* MapEditorWindow::saveLayout
 * Saves the current window layout to a file
 *******************************************************************/
void MapEditorWindow::saveLayout()
{
	// Open layout file
	wxFile file(appPath("mapwindow.layout", DIR_USER), wxFile::write);

	// Write component layout
	wxAuiManager* m_mgr = wxAuiManager::GetManager(this);

	// Console pane
	file.Write("\"console\" ");
	string pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("console"));
	file.Write(S_FMT("\"%s\"\n", pinf));

	// Item info pane
	file.Write("\"item_props\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("item_props"));
	file.Write(S_FMT("\"%s\"\n", pinf));

	// Script editor pane
	file.Write("\"script_editor\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("script_editor"));
	file.Write(S_FMT("\"%s\"\n", pinf));

	// Map checks pane
	file.Write("\"map_checks\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("map_checks"));
	file.Write(S_FMT("\"%s\"\n", pinf));

	// Undo history pane
	file.Write("\"undo_history\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("undo_history"));
	file.Write(S_FMT("\"%s\"\n", pinf));

	// Close file
	file.Close();
}

/* MapEditorWindow::setupLayout
 * Sets up the basic map editor window menu bar
 *******************************************************************/
void MapEditorWindow::setupMenu()
{
	// Get menu bar
	wxMenuBar* menu = GetMenuBar();
	if (menu)
	{
		// Clear existing menu bar
		unsigned n_menus = menu->GetMenuCount();
		for (unsigned a = 0; a < n_menus; a++)
		{
			wxMenu* sm = menu->Remove(0);
			delete sm;
		}
	}
	else	// Create new menu bar
		menu = new wxMenuBar();

	// Map menu
	wxMenu* menu_map = new wxMenu("");
	theApp->getAction("mapw_save")->addToMenu(menu_map);
	theApp->getAction("mapw_saveas")->addToMenu(menu_map);
	theApp->getAction("mapw_rename")->addToMenu(menu_map);
	theApp->getAction("mapw_backup")->addToMenu(menu_map);
	menu_map->AppendSeparator();
	theApp->getAction("mapw_run_map")->addToMenu(menu_map);
	menu->Append(menu_map, "&Map");

	// Edit menu
	wxMenu* menu_editor = new wxMenu("");
	theApp->getAction("mapw_undo")->addToMenu(menu_editor);
	theApp->getAction("mapw_redo")->addToMenu(menu_editor);
	menu_editor->AppendSeparator();
	theApp->getAction("mapw_draw_lines")->addToMenu(menu_editor);
	theApp->getAction("mapw_draw_shape")->addToMenu(menu_editor);
	theApp->getAction("mapw_edit_objects")->addToMenu(menu_editor);
	theApp->getAction("mapw_mirror_x")->addToMenu(menu_editor);
	theApp->getAction("mapw_mirror_y")->addToMenu(menu_editor);
	menu_editor->AppendSeparator();
	theApp->getAction("mapw_preferences")->addToMenu(menu_editor);
	theApp->getAction("mapw_setbra")->addToMenu(menu_editor);
	menu->Append(menu_editor, "&Edit");

	// View menu
	wxMenu* menu_view = new wxMenu("");
	theApp->getAction("mapw_showproperties")->addToMenu(menu_view);
	theApp->getAction("mapw_showconsole")->addToMenu(menu_view);
	theApp->getAction("mapw_showundohistory")->addToMenu(menu_view);
	theApp->getAction("mapw_showchecks")->addToMenu(menu_view);
	theApp->getAction("mapw_showscripteditor")->addToMenu(menu_view);
	menu_view->AppendSeparator();
	theApp->getAction("mapw_show_fullmap")->addToMenu(menu_view);
	theApp->getAction("mapw_show_item")->addToMenu(menu_view);
	menu->Append(menu_view, "View");

	SetMenuBar(menu);
}

/* MapEditorWindow::setupLayout
 * Sets up the basic map editor window layout
 *******************************************************************/
void MapEditorWindow::setupLayout()
{
	// Create the wxAUI manager & related things
	wxAuiManager* m_mgr = new wxAuiManager(this);
	m_mgr->SetArtProvider(new SAuiDockArt());
	wxAuiPaneInfo p_inf;

	// Map canvas
	map_canvas = new MapCanvas(this, -1, &editor);
	p_inf.CenterPane();
	m_mgr->AddPane(map_canvas, p_inf);

	// --- Menus ---
	setupMenu();


	// --- Toolbars ---
	toolbar = new SToolBar(this, true);

	// Map toolbar
	SToolBarGroup* tbg_map = new SToolBarGroup(toolbar, "_Map");
	tbg_map->addActionButton("mapw_save");
	tbg_map->addActionButton("mapw_saveas");
	tbg_map->addActionButton("mapw_rename");
	toolbar->addGroup(tbg_map);

	// Mode toolbar
	SToolBarGroup* tbg_mode = new SToolBarGroup(toolbar, "_Mode");
	tbg_mode->addActionButton("mapw_mode_vertices");
	tbg_mode->addActionButton("mapw_mode_lines");
	tbg_mode->addActionButton("mapw_mode_sectors");
	tbg_mode->addActionButton("mapw_mode_things");
	tbg_mode->addActionButton("mapw_mode_3d");
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

	// Edit toolbar
	SToolBarGroup* tbg_edit = new SToolBarGroup(toolbar, "_Edit");
	tbg_edit->addActionButton("mapw_draw_lines");
	tbg_edit->addActionButton("mapw_draw_shape");
	tbg_edit->addActionButton("mapw_edit_objects");
	tbg_edit->addActionButton("mapw_mirror_x");
	tbg_edit->addActionButton("mapw_mirror_y");
	toolbar->addGroup(tbg_edit);

	// Extra toolbar
	SToolBarGroup* tbg_misc = new SToolBarGroup(toolbar, "_Misc");
	tbg_misc->addActionButton("mapw_run_map");
	toolbar->addGroup(tbg_misc);

	// Add toolbar
	m_mgr->AddPane(toolbar, wxAuiPaneInfo().Top().CaptionVisible(false).MinSize(-1, SToolBar::getBarHeight()).Resizable(false).PaneBorder(false).Name("toolbar"));


	// Status bar
	CreateStatusBar(4);
	int status_widths[4] = { -1, 240, 200, 160 };
	SetStatusWidths(4, status_widths);

	// -- Console Panel --
	ConsolePanel* panel_console = new ConsolePanel(this, -1);

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


	// --- Script Editor Panel ---
	panel_script_editor = new ScriptEditorPanel(this);

	// Setup panel info & add panel
	p_inf.Float();
	p_inf.BestSize(300, 300);
	p_inf.FloatingSize(500, 400);
	p_inf.FloatingPosition(150, 150);
	p_inf.MinSize(300, 300);
	p_inf.Show(false);
	p_inf.Caption("Script Editor");
	p_inf.Name("script_editor");
	m_mgr->AddPane(panel_script_editor, p_inf);


	// --- Shape Draw Options Panel ---
	ShapeDrawPanel* panel_shapedraw = new ShapeDrawPanel(this);

	// Setup panel info & add panel
	wxSize msize = panel_shapedraw->GetMinSize();
	p_inf.DefaultPane();
	p_inf.Bottom();
	p_inf.Dock();
	p_inf.CloseButton(false);
	p_inf.CaptionVisible(false);
	p_inf.Resizable(false);
	p_inf.Layer(2);
	p_inf.BestSize(msize.x, msize.y);
	p_inf.FloatingSize(msize.x, msize.y);
	p_inf.FloatingPosition(140, 140);
	p_inf.MinSize(msize.x, msize.y);
	p_inf.Show(false);
	p_inf.Caption("Shape Drawing");
	p_inf.Name("shape_draw");
	m_mgr->AddPane(panel_shapedraw, p_inf);


	// --- Object Edit Panel ---
	panel_obj_edit = new ObjectEditPanel(this);

	// Setup panel info & add panel
	msize = panel_obj_edit->GetBestSize();

	p_inf.Bottom();
	p_inf.Dock();
	p_inf.CloseButton(false);
	p_inf.CaptionVisible(false);
	p_inf.Resizable(false);
	p_inf.Layer(2);
	p_inf.BestSize(msize.x, msize.y);
	p_inf.FloatingSize(msize.x, msize.y);
	p_inf.FloatingPosition(140, 140);
	p_inf.MinSize(msize.x, msize.y);
	p_inf.Show(false);
	p_inf.Caption("Object Edit");
	p_inf.Name("object_edit");
	m_mgr->AddPane(panel_obj_edit, p_inf);


	// --- Map Checks Panel ---
	panel_checks = new MapChecksPanel(this, &(editor.getMap()));

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Left();
	p_inf.Dock();
	p_inf.BestSize(400, 300);
	p_inf.FloatingSize(500, 400);
	p_inf.FloatingPosition(160, 160);
	p_inf.MinSize(300, 300);
	p_inf.Show(false);
	p_inf.Caption("Map Checks");
	p_inf.Name("map_checks");
	p_inf.Layer(0);
	m_mgr->AddPane(panel_checks, p_inf);


	// -- Undo History Panel --
	panel_undo_history = new UndoManagerHistoryPanel(this, NULL);
	panel_undo_history->setManager(editor.undoManager());

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Right();
	p_inf.BestSize(128, 480);
	p_inf.Caption("Undo History");
	p_inf.Name("undo_history");
	p_inf.Show(false);
	p_inf.Dock();
	m_mgr->AddPane(panel_undo_history, p_inf);


	// Load previously saved window layout
	loadLayout();

	m_mgr->Update();
	Layout();

	// Initial focus on the canvas, so shortcuts work
	map_canvas->SetFocus();
}

/* MapEditorWindow::lockMapEntries
 * Locks/unlocks the entries for the current map
 *******************************************************************/
void MapEditorWindow::lockMapEntries(bool lock)
{
	// Don't bother if no map is open
	if (!mdesc_current.head)
		return;

	// Just lock/unlock the 'head' entry if it's a pk3 map
	if (mdesc_current.archive)
	{
		if (lock)
			mdesc_current.head->lock();
		else
			mdesc_current.head->unlock();

		return;
	}
}

/* MapEditorWindow::chooseMap
 * Opens the map editor launcher dialog to create or open a map
 *******************************************************************/
bool MapEditorWindow::chooseMap(Archive* archive)
{
	MapEditorConfigDialog dlg(theMainWindow, archive, (bool)archive, !(bool)archive);

	if (dlg.ShowModal() == wxID_OK)
	{
		Archive::mapdesc_t md = dlg.selectedMap();

		if (md.name.IsEmpty() || (archive && !md.head))
			return false;

		// Attempt to load selected game configuration
		if (!theGameConfiguration->openConfig(dlg.selectedGame(), dlg.selectedPort(), md.format))
		{
			wxMessageBox("An error occurred loading the game configuration, see the console log for details", "Error", wxICON_ERROR);
			return false;
		}

		// Show md editor window
		if (IsIconized())
			Restore();
		Raise();

		// Attempt to open md
		if (!openMap(md))
		{
			Hide();
			wxMessageBox(S_FMT("Unable to open md %s: %s", md.name, Global::error), "Invalid md error", wxICON_ERROR);
			return false;
		}
		else
			return true;
	}
	return false;
}

/* MapEditorWindow::openMap
 * Opens [map] in the editor
 *******************************************************************/
bool MapEditorWindow::openMap(Archive::mapdesc_t map)
{
	// If a map is currently open and modified, prompt to save changes
	if (editor.getMap().isModified())
	{
		wxMessageDialog md(this, S_FMT("Save changes to map %s?", currentMapDesc().name), "Unsaved Changes", wxYES_NO | wxCANCEL);
		int answer = md.ShowModal();
		if (answer == wxID_YES)
			saveMap();
		else if (answer == wxID_CANCEL)
			return true;
	}

	// Show blank map
	this->Show(true);
	map_canvas->Refresh();
	Layout();
	Update();
	Refresh();

	// Clear current map data
	for (unsigned a = 0; a < map_data.size(); a++)
		delete map_data[a];
	map_data.clear();

	// Get map parent archive
	Archive* archive = NULL;
	if (map.head)
	{
		archive = map.head->getParent();

		// Load map data
		if (map.archive)
		{
			WadArchive temp;
			temp.open(map.head->getMCData());
			for (unsigned a = 0; a < temp.numEntries(); a++)
				map_data.push_back(new ArchiveEntry(*(temp.getEntry(a))));
		}
		else
		{
			ArchiveEntry* entry = map.head;
			while (entry)
			{
				bool end = (entry == map.end);
				map_data.push_back(new ArchiveEntry(*entry));
				entry = entry->nextEntry();
				if (end)
					break;
			}
		}
	}

	// Set texture manager archive
	tex_man.setArchive(archive);

	// Clear current map
	closeMap();

	// Attempt to open map
	theSplashWindow->show("Loading Map", true, this);
	bool ok = editor.openMap(map);
	theSplashWindow->hide();

	// Show window if opened ok
	if (ok)
	{
		mdesc_current = map;

		// Read DECORATE definitions if any
		theGameConfiguration->clearDecorateDefs();
		theGameConfiguration->parseDecorateDefs(theArchiveManager->baseResourceArchive());
		for (int i = 0; i < theArchiveManager->numArchives(); ++i)
			theGameConfiguration->parseDecorateDefs(theArchiveManager->getArchive(i));

		// Load scripts if any
		loadMapScripts(map);

		// Lock map entries
		lockMapEntries();

		// Reset map checks panel
		panel_checks->reset();

		map_canvas->viewFitToMap(true);
		map_canvas->Refresh();

		// Set window title
		if (archive)
			SetTitle(S_FMT("SLADE - %s of %s", map.name, archive->getFilename(false)));
		else
			SetTitle(S_FMT("SLADE - %s (UNSAVED)", map.name));

		// Create backup
		if (map.head && !backup_manager->writeBackup(map_data, map.head->getTopParent()->getFilename(false), map.head->getName(true)))
			LOG_MESSAGE(1, "Warning: Failed to backup map data");
	}

	return ok;
}

/* MapEditorWindow::loadMapScripts
 * Loads any scripts from [map] into the script editor
 *******************************************************************/
void MapEditorWindow::loadMapScripts(Archive::mapdesc_t map)
{
	// Don't bother if no scripting language specified
	if (theGameConfiguration->scriptLanguage().IsEmpty())
	{
		// Hide script editor
		wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("script_editor");
		p_inf.Show(false);
		m_mgr->Update();
		return;
	}

	// Don't bother if new map
	if (!map.head)
	{
		panel_script_editor->openScripts(NULL, NULL);
		return;
	}

	// Check for pk3 map
	if (map.archive)
	{
		WadArchive* wad = new WadArchive();
		wad->open(map.head->getMCData());
		vector<Archive::mapdesc_t> maps = wad->detectMaps();
		if (!maps.empty())
		{
			loadMapScripts(maps[0]);
			wad->close();
			return;
		}

		delete wad;
	}

	// Go through map entries
	ArchiveEntry* entry = map.head->nextEntry();
	ArchiveEntry* scripts = NULL;
	ArchiveEntry* compiled = NULL;
	while (entry && entry != map.end->nextEntry())
	{
		// Check for SCRIPTS/BEHAVIOR
		if (theGameConfiguration->scriptLanguage() == "acs_hexen" ||
		        theGameConfiguration->scriptLanguage() == "acs_zdoom")
		{
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

/* MapEditorWindow::buildNodes
 * Builds nodes for the maps in [wad]
 *******************************************************************/
bool nb_warned = false;
void MapEditorWindow::buildNodes(Archive* wad)
{
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

	// Don't build if none selected
	if (builder.id == "none")
		return;

	// Switch to ZDBSP if UDMF
	if (mdesc_current.format == MAP_UDMF && nodebuilder_id != "zdbsp")
	{
		wxMessageBox("Nodebuilder switched to ZDBSP for UDMF format", "Save Map", wxICON_INFORMATION);
		builder = NodeBuilders::getBuilder("zdbsp");
		command = builder.command;
	}

	// Check for undefined path
	if (!wxFileExists(builder.path) && !nb_warned)
	{
		// Open nodebuilder preferences
		PreferencesDialog::openPreferences(this, "Node Builders");

		// Get new builder if one was selected
		builder = NodeBuilders::getBuilder(nodebuilder_id);
		command = builder.command;

		// Check again
		if (!wxFileExists(builder.path))
		{
			wxMessageBox("No valid Node Builder is currently configured, nodes will not be built!", "Warning", wxICON_WARNING);
			nb_warned = true;
		}
	}

	// Build command line
	command.Replace("$f", S_FMT("\"%s\"", filename));
	command.Replace("$o", wxString(options));

	// Run nodebuilder
	if (wxFileExists(builder.path))
	{
		wxArrayString out;
		wxLogMessage("execute \"%s %s\"", builder.path, command);
		theApp->SetTopWindow(this);
		wxWindow* focus = wxWindow::FindFocus();
		wxExecute(S_FMT("\"%s\" %s", builder.path, command), out, wxEXEC_HIDE_CONSOLE);
		theApp->SetTopWindow(theMainWindow);
		if (focus) focus->SetFocusFromKbd();
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

/* MapEditorWindow::writeMap
 * Writes the current map as [name] to a wad archive and returns it
 *******************************************************************/
WadArchive* MapEditorWindow::writeMap(string name, bool nodes)
{
	// Get map data entries
	vector<ArchiveEntry*> new_map_data;
	SLADEMap& map = editor.getMap();
	if (mdesc_current.format == MAP_DOOM)
		map.writeDoomMap(new_map_data);
	else if (mdesc_current.format == MAP_HEXEN)
		map.writeHexenMap(new_map_data);
	else if (mdesc_current.format == MAP_UDMF)
	{
		ArchiveEntry* udmf = new ArchiveEntry("TEXTMAP");
		map.writeUDMFMap(udmf);
		new_map_data.push_back(udmf);
	}
	else // TODO: doom64
		return NULL;

	// Check script language
	bool acs = false;
	if (theGameConfiguration->scriptLanguage() == "acs_hexen" ||
		theGameConfiguration->scriptLanguage() == "acs_zdoom")
		acs = true;
	// Force ACS on for Hexen map format, and off for Doom map format
	if (mdesc_current.format == MAP_DOOM) acs = false;
	if (mdesc_current.format == MAP_HEXEN) acs = true;
	bool dialogue = false;
	if (theGameConfiguration->scriptLanguage() == "usdf" ||
		theGameConfiguration->scriptLanguage() == "zsdf")
		dialogue = true;

	// Add map data to temporary wad
	WadArchive* wad = new WadArchive();
	wad->addNewEntry(name);
	// Handle fragglescript and similar content in the map header
	if (mdesc_current.head && mdesc_current.head->getSize() && !mdesc_current.archive)
	{
		wad->getEntry(name)->importMemChunk(mdesc_current.head->getMCData());
	}
	for (unsigned a = 0; a < new_map_data.size(); a++)
		wad->addEntry(new_map_data[a]);
	if (acs) // BEHAVIOR
		wad->addEntry(panel_script_editor->compiledEntry(), "", true);
	if (acs && panel_script_editor->scriptEntry()->getSize() > 0) // SCRIPTS (if any)
		wad->addEntry(panel_script_editor->scriptEntry(), "", true);
	if (mdesc_current.format == MAP_UDMF)
	{
		// Add extra UDMF entries
		for (unsigned a = 0; a < map.udmfExtraEntries().size(); a++)
			wad->addEntry(map.udmfExtraEntries()[a], -1, NULL, true);

		wad->addNewEntry("ENDMAP");
	}

	// Build nodes
	if (nodes)
		buildNodes(wad);

	// Clear current map data
	for (unsigned a = 0; a < map_data.size(); a++)
		delete map_data[a];
	map_data.clear();

	// Update map data
	for (unsigned a = 0; a < wad->numEntries(); a++)
		map_data.push_back(new ArchiveEntry(*(wad->getEntry(a))));

	return wad;
}

/* MapEditorWindow::saveMap
 * Saves the current map to its archive, or opens the 'save as'
 * dialog if it doesn't currently belong to one
 *******************************************************************/
bool MapEditorWindow::saveMap()
{
	// Check for newly created map
	if (!mdesc_current.head)
		return saveMapAs();

	// Write map to temp wad
	WadArchive* wad = writeMap();
	if (!wad)
		return false;

	// Check for map archive
	Archive* tempwad = NULL;
	Archive::mapdesc_t map = mdesc_current;
	if (mdesc_current.archive && mdesc_current.head)
	{
		tempwad = new WadArchive();
		tempwad->open(mdesc_current.head);
		vector<Archive::mapdesc_t> amaps = tempwad->detectMaps();
		if (amaps.size() > 0)
			map = amaps[0];
		else
		{
			delete tempwad;
			return false;
		}
	}

	// Unlock current map entries
	lockMapEntries(false);

	// Delete current map entries
	ArchiveEntry* entry = map.end;
	Archive* archive = map.head->getParent();
	while (entry && entry != map.head)
	{
		ArchiveEntry* prev = entry->prevEntry();
		archive->removeEntry(entry);
		entry = prev;
	}

	// Create backup
	if (!backup_manager->writeBackup(map_data, map.head->getTopParent()->getFilename(false), map.head->getName(true)))
		LOG_MESSAGE(1, "Warning: Failed to backup map data");

	// Add new map entries
	for (unsigned a = 1; a < wad->numEntries(); a++)
		entry = archive->addEntry(wad->getEntry(a), archive->entryIndex(map.head) + a, NULL, true);

	// Clean up
	delete wad;
	if (tempwad)
	{
		tempwad->save();
		delete tempwad;
	}
	else
	{
		// Update map description
		mdesc_current.end = entry;
	}

	// Finish
	lockMapEntries();
	editor.getMap().setOpenedTime();

	return true;
}

/* MapEditorWindow::saveMapAs
 * Saves the current map to a new archive
 *******************************************************************/
bool MapEditorWindow::saveMapAs()
{
	// Show dialog
	SFileDialog::fd_info_t info;
	if (!SFileDialog::saveFile(info, "Save Map As", "Wad Archives (*.wad)|*.wad", this))
		return false;

	// Create new, empty wad
	WadArchive wad;
	ArchiveEntry* head = wad.addNewEntry(mdesc_current.name);
	ArchiveEntry* end = NULL;
	if (mdesc_current.format == MAP_UDMF)
	{
		wad.addNewEntry("TEXTMAP");
		end = wad.addNewEntry("ENDMAP");
	}
	else
	{
		wad.addNewEntry("THINGS");
		wad.addNewEntry("LINEDEFS");
		wad.addNewEntry("SIDEDEFS");
		wad.addNewEntry("VERTEXES");
		end = wad.addNewEntry("SECTORS");
	}

	// Save map data
	mdesc_current.head = head;
	mdesc_current.archive = false;
	mdesc_current.end = end;
	saveMap();

	// Write wad to file
	wad.save(info.filenames[0]);
	Archive* archive = theArchiveManager->openArchive(info.filenames[0], true, true);
	theArchiveManager->addRecentFile(info.filenames[0]);

	// Update current map description
	vector<Archive::mapdesc_t> maps = archive->detectMaps();
	if (!maps.empty())
	{
		mdesc_current.head = maps[0].head;
		mdesc_current.archive = false;
		mdesc_current.end = maps[0].end;
	}

	// Set window title
	SetTitle(S_FMT("SLADE - %s of %s", mdesc_current.name, wad.getFilename(false)));

	return true;
}

/* MapEditorWindow::closeMap
 * Closes/clears the current map
 *******************************************************************/
void MapEditorWindow::closeMap()
{
	// Close map in editor
	editor.clearMap();

	// Unlock current map entries
	lockMapEntries(false);

	// Clear map info
	mdesc_current.head = NULL;
}

/* MapEditorWindow::forceRefresh
 * Forces a refresh of the map canvas, and the renderer if [renderer]
 * is true
 *******************************************************************/
void MapEditorWindow::forceRefresh(bool renderer)
{
	if (!IsShown())
		return;

	if (renderer) map_canvas->forceRefreshRenderer();
	map_canvas->Refresh();
}

/* MapEditorWindow::refreshToolbar
 * Refreshes the toolbar
 *******************************************************************/
void MapEditorWindow::refreshToolBar()
{
	toolbar->Refresh();
}

/* MapEditorWindow::tryClose
 * Checks if the currently open map is modified and prompts to save.
 * If 'Cancel' is clicked then this will return false (ie. we don't
 * want to close the window)
 *******************************************************************/
bool MapEditorWindow::tryClose()
{
	if (editor.getMap().isModified())
	{
		wxMessageDialog md(this, S_FMT("Save changes to map %s?", currentMapDesc().name), "Unsaved Changes", wxYES_NO | wxCANCEL);
		int answer = md.ShowModal();
		if (answer == wxID_YES)
			return saveMap();
		else if (answer == wxID_CANCEL)
			return false;
	}

	return true;
}

/* MapEditorWindow::hasMapOpen
 * Returns true if the currently open map is from [archive]
 *******************************************************************/
bool MapEditorWindow::hasMapOpen(Archive* archive)
{
	if (!mdesc_current.head)
		return false;
	return (mdesc_current.head->getParent() == archive);
}

/* MapEditorWindow::editObjectProperties
 * Opens the property editor for [objects]
 *******************************************************************/
void MapEditorWindow::editObjectProperties(vector<MapObject*>& objects)
{
	map_canvas->editObjectProperties(objects);
}

/* MapEditorWindow::setUndoManager
 * Sets the undo manager to show in the undo history panel
 *******************************************************************/
void MapEditorWindow::setUndoManager(UndoManager* manager)
{
	panel_undo_history->setManager(manager);
}

/* MapEditorWindow::showObjectEditPanel
 * Shows/hides the object edit panel (opens [group] if shown)
 *******************************************************************/
void MapEditorWindow::showObjectEditPanel(bool show, ObjectEditGroup* group)
{
	// Get panel
	wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
	wxAuiPaneInfo& p_inf = m_mgr->GetPane("object_edit");

	// Save current y offset
	double top = map_canvas->translateY(0);

	// Enable/disable panel
	if (show) panel_obj_edit->init(group);
	p_inf.Show(show);

	// Update layout
	map_canvas->Enable(false);
	m_mgr->Update();

	// Restore y offset
	map_canvas->setTopY(top);
	map_canvas->Enable(true);
	map_canvas->SetFocus();
}

/* MapEditorWindow::showShapeDrawPanel
 * Shows/hides the shape drawing panel
 *******************************************************************/
void MapEditorWindow::showShapeDrawPanel(bool show)
{
	// Get panel
	wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
	wxAuiPaneInfo& p_inf = m_mgr->GetPane("shape_draw");

	// Save current y offset
	double top = map_canvas->translateY(0);

	// Enable/disable panel
	p_inf.Show(show);

	// Update layout
	map_canvas->Enable(false);
	m_mgr->Update();

	// Restore y offset
	map_canvas->setTopY(top);
	map_canvas->Enable(true);
	map_canvas->SetFocus();
}

/* MapEditorWindow::handleAction
 * Handles the action [id]. Returns true if the action was handled,
 * false otherwise
 *******************************************************************/
bool MapEditorWindow::handleAction(string id)
{
	// Don't handle actions if hidden
	if (!IsShown())
		return false;

	// Map->Save
	if (id == "mapw_save")
	{
		// Save map
		if (saveMap())
		{
			// Save archive
			Archive* a = currentMapDesc().head->getParent();
			if (a && save_archive_with_map) a->save();
		}

		return true;
	}

	// Map->Save As
	if (id == "mapw_saveas")
	{
		saveMapAs();
		return true;
	}

	// Map->Restore Backup
	if (id == "mapw_backup")
	{
		if (mdesc_current.head)
		{
			Archive* data = backup_manager->openBackup(mdesc_current.head->getTopParent()->getFilename(false), mdesc_current.name);
			if (data)
			{
				vector<Archive::mapdesc_t> maps = data->detectMaps();
				if (!maps.empty())
				{
					editor.clearMap();
					editor.openMap(maps[0]);
					loadMapScripts(maps[0]);
				}
			}
		}

		return true;
	}

	// Edit->Undo
	if (id == "mapw_undo")
	{
		editor.doUndo();
		return true;
	}

	// Edit->Redo
	if (id == "mapw_redo")
	{
		editor.doRedo();
		return true;
	}

	// Editor->Set Base Resource Archive
	if (id == "mapw_setbra")
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

	// Editor->Preferences
	if (id == "mapw_preferences")
	{
		PreferencesDialog::openPreferences(this);

		return true;
	}

	// View->Item Properties
	if (id == "mapw_showproperties")
	{
		wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("item_props");

		// Toggle window and focus
		p_inf.Show(!p_inf.IsShown());
		map_canvas->SetFocus();

		m_mgr->Update();
		return true;
	}

	// View->Console
	else if (id == "mapw_showconsole")
	{
		wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("console");

		// Toggle window and focus
		if (p_inf.IsShown())
		{
			p_inf.Show(false);
			map_canvas->SetFocus();
		}
		else
		{
			p_inf.Show(true);
			p_inf.window->SetFocus();
		}

		p_inf.MinSize(200, 128);
		m_mgr->Update();
		return true;
	}

	// View->Script Editor
	else if (id == "mapw_showscripteditor")
	{
		wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("script_editor");

		// Toggle window and focus
		if (p_inf.IsShown())
		{
			p_inf.Show(false);
			map_canvas->SetFocus();
		}
		else if (!theGameConfiguration->scriptLanguage().IsEmpty())
		{
			p_inf.Show(true);
			p_inf.window->SetFocus();
			((ScriptEditorPanel*)p_inf.window)->updateUI();
		}

		p_inf.MinSize(200, 128);
		m_mgr->Update();
		return true;
	}

	// View->Map Checks
	else if (id == "mapw_showchecks")
	{
		wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("map_checks");

		// Toggle window and focus
		if (p_inf.IsShown())
		{
			p_inf.Show(false);
			map_canvas->SetFocus();
		}
		else
		{
			p_inf.Show(true);
			p_inf.window->SetFocus();
		}

		//p_inf.MinSize(200, 128);
		m_mgr->Update();
		return true;
	}

	// View->Undo History
	else if (id == "mapw_showundohistory")
	{
		wxAuiManager* m_mgr = wxAuiManager::GetManager(this);
		wxAuiPaneInfo& p_inf = m_mgr->GetPane("undo_history");

		// Toggle window
		p_inf.Show(!p_inf.IsShown());

		m_mgr->Update();
		return true;
	}

	// Run Map
	else if (id == "mapw_run_map" || id == "mapw_run_map_here")
	{
		Archive* archive = NULL;
		if (mdesc_current.head)
			archive = mdesc_current.head->getParent();
		RunDialog dlg(this, archive, id == "mapw_run_map");
		if (dlg.ShowModal() == wxID_OK)
		{
			// Move player 1 start if needed
			if (id == "mapw_run_map_here")
				editor.swapPlayerStart2d(map_canvas->mouseDownPosM());
			else if (dlg.start3dModeChecked())
				editor.swapPlayerStart3d();

			// Write temp wad
			WadArchive* wad = writeMap(mdesc_current.name);
			if (wad)
				wad->save(appPath("sladetemp_run.wad", DIR_TEMP));

			// Reset player 1 start if moved
			if (dlg.start3dModeChecked() || id == "mapw_run_map_here")
				editor.resetPlayerStart();

			string command = dlg.getSelectedCommandLine(archive, mdesc_current.name, wad->getFilename());
			if (!command.IsEmpty())
			{
				// Set working directory
				string wd = wxGetCwd();
				wxSetWorkingDirectory(dlg.getSelectedExeDir());

				// Run
				wxExecute(command, wxEXEC_ASYNC);

				// Restore working directory
				wxSetWorkingDirectory(wd);
			}
		}

		return true;
	}
	
	return false;
}


/*******************************************************************
 * MAPEDITORWINDOW CLASS EVENTS
 *******************************************************************/

/* MapEditorWindow::onClose
 * Called when the window is closed
 *******************************************************************/
void MapEditorWindow::onClose(wxCloseEvent& e)
{
	if (!tryClose())
	{
		e.Veto();
		return;
	}

	// Save current layout
	saveLayout();
	if (!IsMaximized())
		Misc::setWindowInfo(id, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);

	this->Show(false);
	closeMap();
}

/* MapEditorWindow::onSize
 * Called when the window is resized
 *******************************************************************/
void MapEditorWindow::onSize(wxSizeEvent& e)
{
	// Update maximized cvar
	mew_maximized = IsMaximized();

	e.Skip();
}
