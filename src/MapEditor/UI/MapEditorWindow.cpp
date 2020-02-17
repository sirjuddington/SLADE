
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2020 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapEditorWindow.cpp
// Description: MapEditorWindow class, it's a map editor window.
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
#include "MapEditorWindow.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/WadArchive.h"
#include "Dialogs/MapEditorConfigDialog.h"
#include "Dialogs/Preferences/BaseResourceArchivesPanel.h"
#include "Dialogs/Preferences/PreferencesDialog.h"
#include "Dialogs/RunDialog.h"
#include "Game/Configuration.h"
#include "General/Misc.h"
#include "General/UI.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/MapBackupManager.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/NodeBuilders.h"
#include "MapEditor/UI/MapCanvas.h"
#include "MapEditor/UI/MapChecksPanel.h"
#include "MapEditor/UI/ObjectEditPanel.h"
#include "MapEditor/UI/PropsPanel/MapObjectPropsPanel.h"
#include "MapEditor/UI/ScriptEditorPanel.h"
#include "MapEditor/UI/ShapeDrawPanel.h"
#include "SLADEWxApp.h"
#include "Scripting/ScriptManager.h"
#include "UI/Controls/ConsolePanel.h"
#include "UI/Controls/UndoManagerHistoryPanel.h"
#include "UI/SAuiTabArt.h"
#include "UI/SToolBar/SToolBar.h"
#include "UI/WxUtils.h"
#include "Utility/SFileDialog.h"
#include "Utility/Tokenizer.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
bool nb_warned = false;
}
CVAR(Bool, mew_maximized, true, CVar::Flag::Save);
CVAR(String, nodebuilder_id, "zdbsp", CVar::Flag::Save);
CVAR(String, nodebuilder_options, "", CVar::Flag::Save);
CVAR(Bool, save_archive_with_map, true, CVar::Flag::Save);


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Int, flat_drawtype);


// -----------------------------------------------------------------------------
//
// MapEditorWindow Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapEditorWindow class constructor
// -----------------------------------------------------------------------------
MapEditorWindow::MapEditorWindow() : STopWindow{ "SLADE", "map" }
{
	if (mew_maximized)
		wxTopLevelWindow::Maximize();
	setupLayout();
	wxTopLevelWindow::Show(false);
	custom_menus_begin_ = 2;

	// Set icon
	auto icon_filename = App::path(App::iconFile(), App::Dir::Temp);
	App::archiveManager().programResourceArchive()->entry(App::iconFile())->exportFile(icon_filename);
	SetIcon(wxIcon(icon_filename, wxBITMAP_TYPE_ICO));
	wxRemoveFile(icon_filename);

	// Bind events
	Bind(wxEVT_CLOSE_WINDOW, &MapEditorWindow::onClose, this);
	Bind(wxEVT_SIZE, &MapEditorWindow::onSize, this);
}

// -----------------------------------------------------------------------------
// MapEditorWindow class destructor
// -----------------------------------------------------------------------------
MapEditorWindow::~MapEditorWindow()
{
	wxAuiManager::GetManager(this)->UnInit();
}

// -----------------------------------------------------------------------------
// Loads the previously saved layout file for the window
// -----------------------------------------------------------------------------
void MapEditorWindow::loadLayout()
{
	// Open layout file
	Tokenizer tz;
	if (!tz.openFile(App::path("mapwindow.layout", App::Dir::User)))
		return;

	// Parse layout
	auto m_mgr = wxAuiManager::GetManager(this);
	while (true)
	{
		// Read component+layout pair
		wxString component = tz.getToken();
		wxString layout    = tz.getToken();

		// Load layout to component
		if (!component.IsEmpty() && !layout.IsEmpty())
			m_mgr->LoadPaneInfo(layout, m_mgr->GetPane(component));

		// Check if we're done
		if (tz.peekToken().empty())
			break;
	}
}

// -----------------------------------------------------------------------------
// Saves the current window layout to a file
// -----------------------------------------------------------------------------
void MapEditorWindow::saveLayout()
{
	// Open layout file
	wxFile file(App::path("mapwindow.layout", App::Dir::User), wxFile::write);

	// Write component layout
	auto m_mgr = wxAuiManager::GetManager(this);

	// Console pane
	file.Write("\"console\" ");
	wxString pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("console"));
	file.Write(wxString::Format("\"%s\"\n", pinf));

	// Item info pane
	file.Write("\"item_props\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("item_props"));
	file.Write(wxString::Format("\"%s\"\n", pinf));

	// Script editor pane
	file.Write("\"script_editor\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("script_editor"));
	file.Write(wxString::Format("\"%s\"\n", pinf));

	// Map checks pane
	file.Write("\"map_checks\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("map_checks"));
	file.Write(wxString::Format("\"%s\"\n", pinf));

	// Undo history pane
	file.Write("\"undo_history\" ");
	pinf = m_mgr->SavePaneInfo(m_mgr->GetPane("undo_history"));
	file.Write(wxString::Format("\"%s\"\n", pinf));

	// Close file
	file.Close();
}

// -----------------------------------------------------------------------------
// Sets up the basic map editor window menu bar
// -----------------------------------------------------------------------------
void MapEditorWindow::setupMenu()
{
	// Get menu bar
	auto menu = GetMenuBar();
	if (menu)
	{
		// Clear existing menu bar
		unsigned n_menus = menu->GetMenuCount();
		for (unsigned a = 0; a < n_menus; a++)
		{
			auto sm = menu->Remove(0);
			delete sm;
		}
	}
	else // Create new menu bar
		menu = new wxMenuBar();

	// Map menu
	auto menu_map = new wxMenu("");
	SAction::fromId("mapw_save")->addToMenu(menu_map);
	SAction::fromId("mapw_saveas")->addToMenu(menu_map);
	SAction::fromId("mapw_rename")->addToMenu(menu_map);
	SAction::fromId("mapw_backup")->addToMenu(menu_map);
	menu_map->AppendSeparator();
	SAction::fromId("mapw_run_map")->addToMenu(menu_map);
	menu->Append(menu_map, "&Map");

	// Edit menu
	auto menu_editor = new wxMenu("");
	SAction::fromId("mapw_undo")->addToMenu(menu_editor);
	SAction::fromId("mapw_redo")->addToMenu(menu_editor);
	menu_editor->AppendSeparator();
	SAction::fromId("mapw_clear_selection")->addToMenu(menu_editor);
	menu_editor->AppendSeparator();
	SAction::fromId("mapw_draw_lines")->addToMenu(menu_editor);
	SAction::fromId("mapw_draw_shape")->addToMenu(menu_editor);
	SAction::fromId("mapw_edit_objects")->addToMenu(menu_editor);
	SAction::fromId("mapw_mirror_x")->addToMenu(menu_editor);
	SAction::fromId("mapw_mirror_y")->addToMenu(menu_editor);
	menu_editor->AppendSeparator();
	SAction::fromId("mapw_preferences")->addToMenu(menu_editor);
	SAction::fromId("mapw_setbra")->addToMenu(menu_editor);
	menu->Append(menu_editor, "&Edit");

	// View menu
	auto menu_view   = new wxMenu("");
	auto menu_window = new wxMenu();
	menu_view->AppendSubMenu(menu_window, "Windows");
	SAction::fromId("mapw_showproperties")->addToMenu(menu_window);
	SAction::fromId("mapw_showconsole")->addToMenu(menu_window);
	SAction::fromId("mapw_showundohistory")->addToMenu(menu_window);
	SAction::fromId("mapw_showchecks")->addToMenu(menu_window);
	SAction::fromId("mapw_showscripteditor")->addToMenu(menu_window);
	toolbar_menu_ = new wxMenu();
	menu_view->AppendSubMenu(toolbar_menu_, "Toolbars");
	menu_view->AppendSeparator();
	SAction::fromId("mapw_show_fullmap")->addToMenu(menu_view);
	SAction::fromId("mapw_show_item")->addToMenu(menu_view);
	menu_view->AppendSeparator();
	auto menu_grid = new wxMenu();
	menu_view->AppendSubMenu(menu_grid, "Grid");
	SAction::fromId("mapw_grid_increment")->addToMenu(menu_grid);
	SAction::fromId("mapw_grid_decrement")->addToMenu(menu_grid);
	SAction::fromId("mapw_grid_snap")->addToMenu(menu_grid);
	menu->Append(menu_view, "View");

	// Tools menu
	auto menu_tools = new wxMenu("");
	menu_scripts_   = new wxMenu();
	ScriptManager::populateEditorScriptMenu(menu_scripts_, ScriptManager::ScriptType::Map, "mapw_script");
	menu_tools->AppendSubMenu(menu_scripts_, "Run Script");
	SAction::fromId("mapw_runscript")->addToMenu(menu_tools);
	menu->Append(menu_tools, "&Tools");

	SetMenuBar(menu);
}

// -----------------------------------------------------------------------------
// Sets up the basic map editor window layout
// -----------------------------------------------------------------------------
void MapEditorWindow::setupLayout()
{
	// Create the wxAUI manager & related things
	auto m_mgr = new wxAuiManager(this);
	m_mgr->SetArtProvider(new SAuiDockArt());
	wxAuiPaneInfo p_inf;

	// Map canvas
	map_canvas_ = new MapCanvas(this, -1, &MapEditor::editContext());
	p_inf.CenterPane();
	m_mgr->AddPane(map_canvas_, p_inf);

	// --- Menus ---
	setupMenu();


	// --- Toolbars ---
	toolbar_ = new SToolBar(this, true);

	// Map toolbar
	auto tbg_map = new SToolBarGroup(toolbar_, "_Map");
	tbg_map->addActionButton("mapw_save");
	tbg_map->addActionButton("mapw_saveas");
	tbg_map->addActionButton("mapw_rename");
	toolbar_->addGroup(tbg_map);

	// Mode toolbar
	auto tbg_mode = new SToolBarGroup(toolbar_, "_Mode");
	tbg_mode->addActionButton("mapw_mode_vertices");
	tbg_mode->addActionButton("mapw_mode_lines");
	tbg_mode->addActionButton("mapw_mode_sectors");
	tbg_mode->addActionButton("mapw_mode_things");
	tbg_mode->addActionButton("mapw_mode_3d");
	SAction::fromId("mapw_mode_lines")->setChecked(); // Lines mode by default
	toolbar_->addGroup(tbg_mode);

	// Flat type toolbar
	auto tbg_flats = new SToolBarGroup(toolbar_, "_Flats Type");
	tbg_flats->addActionButton("mapw_flat_none");
	tbg_flats->addActionButton("mapw_flat_untextured");
	tbg_flats->addActionButton("mapw_flat_textured");
	toolbar_->addGroup(tbg_flats);

	// Toggle current flat type
	if (flat_drawtype == 0)
		SAction::fromId("mapw_flat_none")->setChecked();
	else if (flat_drawtype == 1)
		SAction::fromId("mapw_flat_untextured")->setChecked();
	else
		SAction::fromId("mapw_flat_textured")->setChecked();

	// Edit toolbar
	auto tbg_edit = new SToolBarGroup(toolbar_, "_Edit");
	tbg_edit->addActionButton("mapw_draw_lines");
	tbg_edit->addActionButton("mapw_draw_shape");
	tbg_edit->addActionButton("mapw_edit_objects");
	tbg_edit->addActionButton("mapw_mirror_x");
	tbg_edit->addActionButton("mapw_mirror_y");
	toolbar_->addGroup(tbg_edit);

	// Extra toolbar
	auto tbg_misc = new SToolBarGroup(toolbar_, "_Misc");
	tbg_misc->addActionButton("mapw_run_map");
	toolbar_->addGroup(tbg_misc);

	// Add toolbar
	m_mgr->AddPane(
		toolbar_,
		wxAuiPaneInfo()
			.Top()
			.CaptionVisible(false)
			.MinSize(-1, SToolBar::getBarHeight())
			.Resizable(false)
			.PaneBorder(false)
			.Name("toolbar"));

	// Populate the 'View->Toolbars' menu
	populateToolbarsMenu();
	toolbar_->enableContextMenu();


	// Status bar
	CreateStatusBar(4);
	int status_widths[4] = { -1, UI::scalePx(240), UI::scalePx(240), UI::scalePx(240) };
	SetStatusWidths(4, status_widths);

	// -- Console Panel --
	auto panel_console = new ConsolePanel(this, -1);

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Bottom();
	p_inf.Dock();
	p_inf.BestSize(WxUtils::scaledSize(480, 192));
	p_inf.FloatingSize(WxUtils::scaledSize(600, 400));
	p_inf.FloatingPosition(100, 100);
	p_inf.MinSize(WxUtils::scaledSize(-1, 192));
	p_inf.Show(false);
	p_inf.Caption("Console");
	p_inf.Name("console");
	m_mgr->AddPane(panel_console, p_inf);


	// -- Map Object Properties Panel --
	panel_obj_props_ = new MapObjectPropsPanel(this);

	// Setup panel info & add panel
	p_inf.Right();
	p_inf.BestSize(WxUtils::scaledSize(256, 256));
	p_inf.FloatingSize(WxUtils::scaledSize(400, 600));
	p_inf.FloatingPosition(120, 120);
	p_inf.MinSize(WxUtils::scaledSize(256, 256));
	p_inf.Show(true);
	p_inf.Caption("Item Properties");
	p_inf.Name("item_props");
	m_mgr->AddPane(panel_obj_props_, p_inf);


	// --- Script Editor Panel ---
	panel_script_editor_ = new ScriptEditorPanel(this);

	// Setup panel info & add panel
	p_inf.Float();
	p_inf.BestSize(WxUtils::scaledSize(300, 300));
	p_inf.FloatingSize(WxUtils::scaledSize(500, 400));
	p_inf.FloatingPosition(150, 150);
	p_inf.MinSize(WxUtils::scaledSize(300, 300));
	p_inf.Show(false);
	p_inf.Caption("Script Editor");
	p_inf.Name("script_editor");
	m_mgr->AddPane(panel_script_editor_, p_inf);


	// --- Shape Draw Options Panel ---
	auto panel_shapedraw = new ShapeDrawPanel(this);

	// Setup panel info & add panel
	auto msize = panel_shapedraw->GetMinSize();
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
	panel_obj_edit_ = new ObjectEditPanel(this);

	// Setup panel info & add panel
	msize = panel_obj_edit_->GetBestSize();
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
	m_mgr->AddPane(panel_obj_edit_, p_inf);


	// --- Map Checks Panel ---
	panel_checks_ = new MapChecksPanel(this, &(MapEditor::editContext().map()));

	// Setup panel info & add panel
	msize = panel_checks_->GetBestSize();
	p_inf.DefaultPane();
	p_inf.Left();
	p_inf.Dock();
	p_inf.BestSize(msize.x, msize.y);
	p_inf.FloatingSize(msize.x, msize.y);
	p_inf.FloatingPosition(160, 160);
	p_inf.MinSize(msize.x, msize.y);
	p_inf.Show(false);
	p_inf.Caption("Map Checks");
	p_inf.Name("map_checks");
	p_inf.Layer(0);
	m_mgr->AddPane(panel_checks_, p_inf);


	// -- Undo History Panel --
	panel_undo_history_ = new UndoManagerHistoryPanel(this, nullptr);
	panel_undo_history_->setManager(MapEditor::editContext().undoManager());

	// Setup panel info & add panel
	p_inf.DefaultPane();
	p_inf.Right();
	p_inf.BestSize(WxUtils::scaledSize(128, 480));
	p_inf.Caption("Undo History");
	p_inf.Name("undo_history");
	p_inf.Show(false);
	p_inf.Dock();
	m_mgr->AddPane(panel_undo_history_, p_inf);


	// Load previously saved window layout
	loadLayout();

	m_mgr->Update();
	Layout();

	// Initial focus on the canvas, so shortcuts work
	map_canvas_->SetFocus();
}

// -----------------------------------------------------------------------------
// Locks/unlocks the entries for the current map
// -----------------------------------------------------------------------------
void MapEditorWindow::lockMapEntries(bool lock) const
{
	// Don't bother if no map is open
	auto& map_desc = MapEditor::editContext().mapDesc();
	auto  head     = map_desc.head.lock();
	if (!head)
		return;

	// Just lock/unlock the 'head' entry if it's a pk3 map
	if (map_desc.archive)
	{
		if (lock)
			head->lock();
		else if (!App::archiveManager().getArchive(head.get()))
			head->unlock();
	}
}

// -----------------------------------------------------------------------------
// Opens the map editor launcher dialog to create or open a map
// -----------------------------------------------------------------------------
bool MapEditorWindow::chooseMap(Archive* archive)
{
	MapEditorConfigDialog dlg(MainEditor::windowWx(), archive, (bool)archive, !(bool)archive);

	if (dlg.ShowModal() == wxID_OK)
	{
		auto md = dlg.selectedMap();

		if (md.name.empty() || (archive && !md.head.lock()))
			return false;

		// Attempt to load selected game configuration
		if (!Game::configuration().openConfig(
				dlg.selectedGame().ToStdString(), dlg.selectedPort().ToStdString(), md.format))
		{
			wxMessageBox(
				"An error occurred loading the game configuration, see the console log for details",
				"Error",
				wxICON_ERROR);
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
			wxMessageBox(
				wxString::Format("Unable to open md %s: %s", md.name, Global::error), "Invalid md error", wxICON_ERROR);
			return false;
		}
		else
			return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
// Opens [map] in the editor
// -----------------------------------------------------------------------------
bool MapEditorWindow::openMap(Archive::MapDesc map)
{
	// If a map is currently open and modified, prompt to save changes
	if (MapEditor::editContext().map().isModified())
	{
		wxMessageDialog md{ this,
							wxString::Format("Save changes to map %s?", MapEditor::editContext().mapDesc().name),
							"Unsaved Changes",
							wxYES_NO | wxCANCEL };

		int answer = md.ShowModal();
		if (answer == wxID_YES)
			saveMap();
		else if (answer == wxID_CANCEL)
			return true;
	}

	// Show blank map
	Show(true);
	map_canvas_->Refresh();
	Layout();
	Update();
	Refresh();

	// Clear current map data
	map_data_.clear();

	// Get map parent archive
	Archive* archive = nullptr;
	if (auto head = map.head.lock())
	{
		archive = head->parent();

		// Load map data
		if (map.archive)
		{
			WadArchive temp;
			temp.open(head->data());
			for (unsigned a = 0; a < temp.numEntries(); a++)
				map_data_.emplace_back(new ArchiveEntry(*(temp.entryAt(a))));
		}
		else
		{
			auto entries = map.entries(*archive, true);
			for (auto entry : entries)
				map_data_.emplace_back(new ArchiveEntry(*entry));
		}
	}

	// Set texture manager archive
	MapEditor::textureManager().setArchive(App::archiveManager().shareArchive(archive));

	// Clear current map
	closeMap();

	// Attempt to open map
	UI::showSplash("Loading Map", true, this);
	bool ok = MapEditor::editContext().openMap(map);
	UI::hideSplash();

	// Show window if opened ok
	if (ok)
	{
		MapEditor::editContext().mapDesc() = map;

		// Update DECORATE and *MAPINFO definitions
		Game::updateCustomDefinitions();

		// Load scripts if any
		loadMapScripts(map);

		// Lock map entries
		lockMapEntries();

		// Reset map checks panel
		panel_checks_->reset();

		MapEditor::editContext().renderer().viewFitToMap(true);
		map_canvas_->Refresh();

		// Set window title
		if (archive)
			SetTitle(wxString::Format("SLADE - %s of %s", map.name, archive->filename(false)));
		else
			SetTitle(wxString::Format("SLADE - %s (UNSAVED)", map.name));

		// Create backup
		auto head = map.head.lock();
		if (head
			&& !MapEditor::backupManager().writeBackup(
				   map_data_, head->topParent()->filename(false), head->nameNoExt()))
			Log::warning("Failed to backup map data");
	}

	return ok;
}

// -----------------------------------------------------------------------------
// Loads any scripts from [map] into the script editor
// -----------------------------------------------------------------------------
void MapEditorWindow::loadMapScripts(Archive::MapDesc map)
{
	// Don't bother if no scripting language specified
	if (Game::configuration().scriptLanguage().empty())
	{
		// Hide script editor
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("script_editor");
		p_inf.Show(false);
		m_mgr->Update();
		return;
	}

	// Don't bother if new map
	auto head = map.head.lock();
	if (!head)
	{
		panel_script_editor_->openScripts(nullptr, nullptr);
		return;
	}

	// Check for pk3 map
	if (map.archive)
	{
		auto wad = new WadArchive();
		wad->open(head->data());
		auto maps = wad->detectMaps();
		if (!maps.empty())
		{
			loadMapScripts(maps[0]);
			wad->close();
			return;
		}

		delete wad;
	}

	// Go through map entries
	ArchiveEntry* scripts  = nullptr;
	ArchiveEntry* compiled = nullptr;
	auto          entries  = map.entries(*head->parent());
	for (auto entry : entries)
	{
		// Check for SCRIPTS/BEHAVIOR
		if (Game::configuration().scriptLanguage() == "acs_hexen"
			|| Game::configuration().scriptLanguage() == "acs_zdoom")
		{
			if (entry->upperName() == "SCRIPTS")
				scripts = entry;
			if (entry->upperName() == "BEHAVIOR")
				compiled = entry;
		}
	}

	// Open scripts/compiled if found
	panel_script_editor_->openScripts(scripts, compiled);
}

// -----------------------------------------------------------------------------
// Builds nodes for the maps in [wad]
// -----------------------------------------------------------------------------
void MapEditorWindow::buildNodes(Archive* wad)
{
	// Save wad to disk
	auto filename = App::path("sladetemp.wad", App::Dir::Temp);
	wad->save(filename);

	// Get current nodebuilder
	auto     builder = NodeBuilders::builder(nodebuilder_id);
	wxString command = builder.command;
	wxString options = nodebuilder_options;

	// Don't build if none selected
	if (builder.id == "none")
		return;

	// Switch to ZDBSP if UDMF
	if (MapEditor::editContext().mapDesc().format == MapFormat::UDMF && nodebuilder_id != "zdbsp")
	{
		wxMessageBox("Nodebuilder switched to ZDBSP for UDMF format", "Save Map", wxICON_INFORMATION);
		builder = NodeBuilders::builder("zdbsp");
		command = builder.command;
	}

	// Check for undefined path
	if (!wxFileExists(builder.path) && !nb_warned)
	{
		// Open nodebuilder preferences
		PreferencesDialog::openPreferences(this, "Node Builders");

		// Get new builder if one was selected
		builder = NodeBuilders::builder(nodebuilder_id);
		command = builder.command;

		// Check again
		if (!wxFileExists(builder.path))
		{
			wxMessageBox(
				"No valid Node Builder is currently configured, nodes will not be built!", "Warning", wxICON_WARNING);
			nb_warned = true;
		}
	}

	// Build command line
	command.Replace("$f", wxString::Format("\"%s\"", filename));
	command.Replace("$o", wxString(options));

	// Run nodebuilder
	if (wxFileExists(builder.path))
	{
		wxArrayString out;
		Log::info(wxString::Format("execute \"%s %s\"", builder.path, command));
		wxGetApp().SetTopWindow(this);
		auto focus = wxWindow::FindFocus();
		wxExecute(wxString::Format("\"%s\" %s", builder.path, command), out, wxEXEC_HIDE_CONSOLE);
		wxGetApp().SetTopWindow(MainEditor::windowWx());
		if (focus)
			focus->SetFocusFromKbd();
		Log::info(1, "Nodebuilder output:");
		for (const auto& line : out)
			Log::info(line);

		// Re-load wad
		wad->close();
		wad->open(filename);
	}
	else if (nb_warned)
		Log::info(1, "Nodebuilder path not set up, no nodes were built");
}

// -----------------------------------------------------------------------------
// Writes the current map as [name] to a wad archive and returns it
// -----------------------------------------------------------------------------
bool MapEditorWindow::writeMap(WadArchive& wad, const wxString& name, bool nodes)
{
	auto& mdesc_current = MapEditor::editContext().mapDesc();
	auto& map           = MapEditor::editContext().map();

	// Get map data entries
	vector<ArchiveEntry*> new_map_data;
	if (!map.writeMap(new_map_data))
		return false;

	// Check script language
	bool acs = false;
	if (Game::configuration().scriptLanguage() == "acs_hexen" || Game::configuration().scriptLanguage() == "acs_zdoom")
		acs = true;
	// Force ACS on for Hexen map format, and off for Doom map format
	if (mdesc_current.format == MapFormat::Doom)
		acs = false;
	if (mdesc_current.format == MapFormat::Hexen)
		acs = true;
	bool dialogue = false;
	if (Game::configuration().scriptLanguage() == "usdf" || Game::configuration().scriptLanguage() == "zsdf")
		dialogue = true;

	// Add map data to temporary wad
	wad.addNewEntry(name.ToStdString());
	// Handle fragglescript and similar content in the map header
	auto head = mdesc_current.head.lock();
	if (head && head->size() && !mdesc_current.archive)
		wad.entry(name.ToStdString())->importMemChunk(head->data());
	for (auto& entry : new_map_data)
		wad.addEntry(shared_ptr<ArchiveEntry>(entry));
	if (acs) // BEHAVIOR
		wad.addEntry(std::make_shared<ArchiveEntry>(*panel_script_editor_->compiledEntry()), "");
	if (acs && panel_script_editor_->scriptEntry()->size() > 0) // SCRIPTS (if any)
		wad.addEntry(std::make_shared<ArchiveEntry>(*panel_script_editor_->scriptEntry()), "");
	if (mdesc_current.format == MapFormat::UDMF)
	{
		// Add extra UDMF entries
		for (auto& entry : map.udmfExtraEntries())
			wad.addEntry(std::make_shared<ArchiveEntry>(*entry), -1, nullptr);

		wad.addNewEntry("ENDMAP");
	}

	// Build nodes
	if (nodes)
		buildNodes(&wad);

	// Clear current map data
	map_data_.clear();

	// Update map data
	for (unsigned a = 0; a < wad.numEntries(); a++)
		map_data_.emplace_back(new ArchiveEntry(*(wad.entryAt(a))));

	return true;
}

// -----------------------------------------------------------------------------
// Saves the current map to its archive, or opens the 'save as' dialog if it
// doesn't currently belong to one
// -----------------------------------------------------------------------------
bool MapEditorWindow::saveMap()
{
	auto& mdesc_current = MapEditor::editContext().mapDesc();

	// Check for newly created map
	auto current_head = mdesc_current.head.lock();
	if (!current_head)
		return saveMapAs();

	// Write map to temp wad
	WadArchive wad;
	if (!writeMap(wad))
		return false;

	// Check for map archive
	unique_ptr<Archive> tempwad;
	auto                map = mdesc_current;
	if (mdesc_current.archive && current_head)
	{
		tempwad = std::make_unique<WadArchive>();
		tempwad->open(current_head.get());
		auto amaps = tempwad->detectMaps();
		if (!amaps.empty())
			map = amaps[0];
		else
			return false;
	}

	// Unlock current map entries
	lockMapEntries(false);

	// Delete current map entries
	auto m_head  = map.head.lock();
	auto archive = m_head->parent();
	auto entries = map.entries(*archive);
	for (auto entry : entries)
		archive->removeEntry(entry);

	// Create backup
	if (!MapEditor::backupManager().writeBackup(map_data_, m_head->topParent()->filename(false), m_head->nameNoExt()))
		Log::warning(1, "Warning: Failed to backup map data");

	// Add new map entries
	auto entry_end = map.head;
	for (unsigned a = 1; a < wad.numEntries(); a++)
	{
		auto copy = std::make_shared<ArchiveEntry>(*wad.entryAt(a));
		archive->addEntry(copy, archive->entryIndex(m_head.get()) + a, nullptr);
		entry_end = copy;
	}

	// Clean up
	if (tempwad)
		tempwad->save();
	else
		mdesc_current.end = entry_end; // Update map description

	// Finish
	lockMapEntries();
	MapEditor::editContext().map().setOpenedTime();

	return true;
}

// -----------------------------------------------------------------------------
// Saves the current map to a new archive
// -----------------------------------------------------------------------------
bool MapEditorWindow::saveMapAs()
{
	auto& mdesc_current = MapEditor::editContext().mapDesc();

	// Show dialog
	SFileDialog::FDInfo info;
	if (!SFileDialog::saveFile(info, "Save Map As", "Wad Archives (*.wad)|*.wad", this))
		return false;

	// Create new, empty wad
	WadArchive               wad;
	auto                     head = wad.addNewEntry(mdesc_current.name);
	shared_ptr<ArchiveEntry> end;
	if (mdesc_current.format == MapFormat::UDMF)
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
	mdesc_current.head    = head;
	mdesc_current.archive = false;
	mdesc_current.end     = end;
	saveMap();

	// Write wad to file
	wad.save(info.filenames[0]);
	auto archive = App::archiveManager().openArchive(info.filenames[0], true, true);
	App::archiveManager().addRecentFile(info.filenames[0]);

	// Update current map description
	auto maps = archive->detectMaps();
	if (!maps.empty())
	{
		mdesc_current.head    = maps[0].head;
		mdesc_current.archive = false;
		mdesc_current.end     = maps[0].end;
	}

	// Set window title
	SetTitle(wxString::Format("SLADE - %s of %s", mdesc_current.name, wad.filename(false)));

	return true;
}

// -----------------------------------------------------------------------------
// Closes/clears the current map
// -----------------------------------------------------------------------------
void MapEditorWindow::closeMap() const
{
	// Close map in editor
	MapEditor::editContext().clearMap();

	// Unlock current map entries
	lockMapEntries(false);

	// Clear map info
	MapEditor::editContext().mapDesc().head.reset();
}

// -----------------------------------------------------------------------------
// Forces a refresh of the map canvas, and the renderer if [renderer] is true
// -----------------------------------------------------------------------------
void MapEditorWindow::forceRefresh(bool renderer) const
{
	if (!IsShown())
		return;

	if (renderer)
		MapEditor::editContext().forceRefreshRenderer();
	map_canvas_->Refresh();
}

// -----------------------------------------------------------------------------
// Refreshes the toolbar
// -----------------------------------------------------------------------------
void MapEditorWindow::refreshToolBar() const
{
	toolbar_->Refresh();
}

// -----------------------------------------------------------------------------
// Checks if the currently open map is modified and prompts to save.
// If 'Cancel' is clicked then this will return false (ie. we don't want to
// close the window)
// -----------------------------------------------------------------------------
bool MapEditorWindow::tryClose()
{
	if (MapEditor::editContext().map().isModified())
	{
		wxMessageDialog md{ this,
							wxString::Format("Save changes to map %s?", MapEditor::editContext().mapDesc().name),
							"Unsaved Changes",
							wxYES_NO | wxCANCEL };
		int             answer = md.ShowModal();
		if (answer == wxID_YES)
			return saveMap();
		else if (answer == wxID_CANCEL)
			return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns true if the currently open map is from [archive]
// -----------------------------------------------------------------------------
bool MapEditorWindow::hasMapOpen(Archive* archive) const
{
	auto& mdesc = MapEditor::editContext().mapDesc();
	if (auto head = mdesc.head.lock())
		return head->parent() == archive;

	return false;
}

// -----------------------------------------------------------------------------
// Reloads the map editor scripts menu
// -----------------------------------------------------------------------------
void MapEditorWindow::reloadScriptsMenu() const
{
	while (menu_scripts_->FindItemByPosition(0))
		menu_scripts_->Delete(menu_scripts_->FindItemByPosition(0));

	ScriptManager::populateEditorScriptMenu(menu_scripts_, ScriptManager::ScriptType::Map, "mapw_script");
}

// -----------------------------------------------------------------------------
// Sets the undo manager to show in the undo history panel
// -----------------------------------------------------------------------------
void MapEditorWindow::setUndoManager(UndoManager* manager) const
{
	panel_undo_history_->setManager(manager);
}

// -----------------------------------------------------------------------------
// Shows/hides the object edit panel (opens [group] if shown)
// -----------------------------------------------------------------------------
void MapEditorWindow::showObjectEditPanel(bool show, ObjectEditGroup* group)
{
	// Get panel
	auto  m_mgr = wxAuiManager::GetManager(this);
	auto& p_inf = m_mgr->GetPane("object_edit");

	// Save current y offset
	double top = MapEditor::editContext().renderer().view().mapY(0);

	// Enable/disable panel
	if (show)
		panel_obj_edit_->init(group);
	p_inf.Show(show);

	// Update layout
	map_canvas_->Enable(false);
	m_mgr->Update();

	// Restore y offset
	MapEditor::editContext().renderer().setTopY(top);
	map_canvas_->Enable(true);
	map_canvas_->SetFocus();
}

// -----------------------------------------------------------------------------
// Shows/hides the shape drawing panel
// -----------------------------------------------------------------------------
void MapEditorWindow::showShapeDrawPanel(bool show)
{
	// Get panel
	auto  m_mgr = wxAuiManager::GetManager(this);
	auto& p_inf = m_mgr->GetPane("shape_draw");

	// Save current y offset
	double top = MapEditor::editContext().renderer().view().mapY(0);

	// Enable/disable panel
	p_inf.Show(show);

	// Update layout
	map_canvas_->Enable(false);
	m_mgr->Update();

	// Restore y offset
	MapEditor::editContext().renderer().setTopY(top);
	map_canvas_->Enable(true);
	map_canvas_->SetFocus();
}

// -----------------------------------------------------------------------------
// Handles the action [id].
// Returns true if the action was handled, false otherwise
// -----------------------------------------------------------------------------
bool MapEditorWindow::handleAction(string_view id)
{
	auto& mdesc_current = MapEditor::editContext().mapDesc();

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
			if (auto head = mdesc_current.head.lock())
			{
				auto a = head->parent();
				if (a && save_archive_with_map)
					a->save();
			}
		}
		MapEditor::editContext().renderer().forceUpdate();
		return true;
	}

	// Map->Save As
	if (id == "mapw_saveas")
	{
		saveMapAs();
		MapEditor::editContext().renderer().forceUpdate();
		return true;
	}

	// Map->Restore Backup
	if (id == "mapw_backup")
	{
		if (auto head = mdesc_current.head.lock())
		{
			auto data = MapEditor::backupManager().openBackup(head->topParent()->filename(false), mdesc_current.name);

			if (data)
			{
				auto maps = data->detectMaps();
				if (!maps.empty())
				{
					MapEditor::editContext().clearMap();
					MapEditor::editContext().openMap(maps[0]);
					loadMapScripts(maps[0]);
				}
			}
		}

		return true;
	}

	// Edit->Undo
	if (id == "mapw_undo")
	{
		MapEditor::editContext().doUndo();
		return true;
	}

	// Edit->Redo
	if (id == "mapw_redo")
	{
		MapEditor::editContext().doRedo();
		return true;
	}

	// Editor->Set Base Resource Archive
	if (id == "mapw_setbra")
	{
		PreferencesDialog::openPreferences(this, "Base Resource Archive");

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
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("item_props");

		// Toggle window and focus
		p_inf.Show(!p_inf.IsShown());
		map_canvas_->SetFocus();

		p_inf.MinSize(WxUtils::scaledSize(256, 256));
		m_mgr->Update();
		return true;
	}

	// View->Console
	else if (id == "mapw_showconsole")
	{
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("console");

		// Toggle window and focus
		if (p_inf.IsShown())
		{
			p_inf.Show(false);
			map_canvas_->SetFocus();
		}
		else
		{
			p_inf.Show(true);
			p_inf.window->SetFocus();
		}

		p_inf.MinSize(WxUtils::scaledSize(200, 128));
		m_mgr->Update();
		return true;
	}

	// View->Script Editor
	else if (id == "mapw_showscripteditor")
	{
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("script_editor");

		// Toggle window and focus
		if (p_inf.IsShown())
		{
			p_inf.Show(false);
			map_canvas_->SetFocus();
		}
		else if (!Game::configuration().scriptLanguage().empty())
		{
			p_inf.Show(true);
			p_inf.window->SetFocus();
			dynamic_cast<ScriptEditorPanel*>(p_inf.window)->updateUI();
		}

		p_inf.MinSize(WxUtils::scaledSize(200, 128));
		m_mgr->Update();
		return true;
	}

	// View->Map Checks
	else if (id == "mapw_showchecks")
	{
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("map_checks");

		// Toggle window and focus
		if (p_inf.IsShown())
		{
			p_inf.Show(false);
			map_canvas_->SetFocus();
		}
		else
		{
			p_inf.Show(true);
			p_inf.window->SetFocus();
		}

		p_inf.MinSize(panel_checks_->GetBestSize());
		m_mgr->Update();
		return true;
	}

	// View->Undo History
	else if (id == "mapw_showundohistory")
	{
		auto  m_mgr = wxAuiManager::GetManager(this);
		auto& p_inf = m_mgr->GetPane("undo_history");

		// Toggle window
		p_inf.Show(!p_inf.IsShown());

		m_mgr->Update();
		return true;
	}

	// Run Map
	else if (id == "mapw_run_map" || id == "mapw_run_map_here")
	{
		Archive* archive = nullptr;
		if (auto head = mdesc_current.head.lock())
			archive = head->parent();
		RunDialog dlg(this, archive, id == "mapw_run_map");
		if (dlg.ShowModal() == wxID_OK)
		{
			auto& edit_context = MapEditor::editContext();
			// Move player 1 start if needed
			if (id == "mapw_run_map_here")
				edit_context.swapPlayerStart2d(edit_context.input().mouseDownPosMap());
			else if (dlg.start3dModeChecked())
				edit_context.swapPlayerStart3d();

			// Write temp wad
			WadArchive wad;
			if (writeMap(wad, mdesc_current.name))
				wad.save(App::path("sladetemp_run.wad", App::Dir::Temp));

			// Reset player 1 start if moved
			if (dlg.start3dModeChecked() || id == "mapw_run_map_here")
				MapEditor::editContext().resetPlayerStart();

			wxString command = dlg.selectedCommandLine(archive, mdesc_current.name, wad.filename());
			if (!command.IsEmpty())
			{
				// Set working directory
				wxString wd = wxGetCwd();
				wxSetWorkingDirectory(dlg.selectedExeDir());

				// Run
				wxExecute(command, wxEXEC_ASYNC);

				// Restore working directory
				wxSetWorkingDirectory(wd);
			}
		}

		return true;
	}

	// Tools->Run Script
	else if (id == "mapw_script")
	{
		ScriptManager::runMapScript(&MapEditor::editContext().map(), wx_id_offset_, this);
		return true;
	}

	// Tools->Script Manager
	else if (id == "mapw_runscript")
	{
		ScriptManager::open();
		return true;
	}

	return false;
}


// -----------------------------------------------------------------------------
//
// MapEditorWindow Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the window is closed
// -----------------------------------------------------------------------------
void MapEditorWindow::onClose(wxCloseEvent& e)
{
	// Unlock mouse cursor
	bool locked = MapEditor::editContext().mouseLocked();
	MapEditor::editContext().lockMouse(false);

	if (!tryClose())
	{
		// Restore mouse cursor lock
		MapEditor::editContext().lockMouse(locked);

		e.Veto();
		return;
	}

	// Save current layout
	saveLayout();
	if (!IsMaximized())
		Misc::setWindowInfo(id_, GetSize().x, GetSize().y, GetPosition().x, GetPosition().y);

	Show(false);
	closeMap();
}

// -----------------------------------------------------------------------------
// Called when the window is resized
// -----------------------------------------------------------------------------
void MapEditorWindow::onSize(wxSizeEvent& e)
{
	// Update maximized cvar
	mew_maximized = IsMaximized();

	e.Skip();
}
