
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    MapEditorConfigDialog.cpp
 * Description: Dialog that shows options for launching the map
 *              editor - game, port, and resource archives. Also
 *              can show a map list and preview
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
#include "MapEditorConfigDialog.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/WadArchive.h"
#include "Graphics/Icons.h"
#include "Game/Configuration.h"
#include "UI/BaseResourceChooser.h"
#include "UI/Canvas/MapPreviewCanvas.h"
#include "UI/ResourceArchiveChooser.h"


/*******************************************************************
 * NEWMAPDIALOG CLASS
 *******************************************************************
 * Dialog for creating a new map - select map format and name
 */
class NewMapDialog : public wxDialog
{
private:
	wxComboBox*	cbo_mapname;
	wxChoice*	choice_mapformat;
	wxButton*	btn_ok;
	wxButton*	btn_cancel;

public:
	NewMapDialog(
		wxWindow* parent,
		const string& game,
		const string& port,
		vector<Archive::mapdesc_t>& maps,
		Archive* archive
	) : wxDialog(parent, -1, "New Map")
	{
		// Setup dialog
		wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		wxGridBagSizer* sizer = new wxGridBagSizer(4, 4);
		msizer->Add(sizer, 1, wxEXPAND|wxALL, 10);

		// Open selected game configuration if no map names are currently loaded
		Game::configuration().openConfig(game, port);

		// Check if the game configuration allows any map name
		int flags = 0;
		if (!Game::configuration().featureSupported(Game::Feature::AnyMapName))
			flags = wxCB_READONLY;

		// Create map name combo box
		cbo_mapname = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, flags);
		sizer->Add(new wxStaticText(this, -1, "Map Name:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		sizer->Add(cbo_mapname, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

		// Limit map name length if necessary
		if (Game::configuration().featureSupported(Game::Feature::AnyMapName) &&
			(!Game::configuration().featureSupported(Game::Feature::LongNames) ||
			(archive && archive->getType() != ARCHIVE_ZIP &&
				archive->getType() != ARCHIVE_7Z &&
				archive->getType() != ARCHIVE_FOLDER)))
			cbo_mapname->SetMaxLength(8);

		// Add possible map names to the combo box
		for (unsigned a = 0; a < Game::configuration().nMapNames(); a++)
		{
			// Check if map already exists
			string mapname = Game::configuration().mapName(a);
			bool exists = false;
			for (unsigned m = 0; m < maps.size(); m++)
			{
				if (S_CMPNOCASE(maps[m].name, mapname))
				{
					exists = true;
					break;
				}
			}

			if (!exists)
				cbo_mapname->Append(mapname);
		}

		// Set inital map name selection
		if (Game::configuration().nMapNames() > 0)
			cbo_mapname->SetSelection(0);

		// Create map format combo box
		choice_mapformat = new wxChoice(this, -1);
		sizer->Add(new wxStaticText(this, -1, "Map Format:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		sizer->Add(choice_mapformat, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

		// Add possible map formats to the combo box
		uint8_t default_format = MAP_UNKNOWN;
		if (! maps.empty())
			default_format = maps[0].format;
		for (uint8_t map_type = 0; map_type < MAP_UNKNOWN; map_type++)
		{
			if (Game::mapFormatSupported(map_type, game, port))
			{
				choice_mapformat->Append(MAP_TYPE_NAMES[map_type]);
				if (map_type == default_format)
					choice_mapformat->SetSelection(choice_mapformat->GetCount() - 1);
			}
		}
		// Default to the "best" supported format, the last one in the list
		if (choice_mapformat->GetSelection() == wxNOT_FOUND)
			choice_mapformat->SetSelection(choice_mapformat->GetCount() - 1);

		// Add dialog buttons
		wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
		msizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
		hbox->AddStretchSpacer();
		btn_ok = new wxButton(this, -1, "OK");
		hbox->Add(btn_ok, 0, wxEXPAND | wxRIGHT, 4);
		btn_cancel = new wxButton(this, -1, "Cancel");
		hbox->Add(btn_cancel, 0, wxEXPAND);
		sizer->AddGrowableCol(1);

		// Bind events
		btn_ok->Bind(wxEVT_BUTTON, &NewMapDialog::onBtnOk, this);
		btn_cancel->Bind(wxEVT_BUTTON, &NewMapDialog::onBtnCancel, this);

		Layout();
		msizer->Fit(this);
		CenterOnParent();
	}

	string getMapName()
	{
		return cbo_mapname->GetValue();
	}

	string getMapFormat()
	{
		return choice_mapformat->GetStringSelection();
	}

	void onBtnOk(wxCommandEvent& e)
	{
		EndModal(wxID_OK);
	}

	void onBtnCancel(wxCommandEvent& e)
	{
		EndModal(wxID_CANCEL);
	}
};


/*******************************************************************
 * MAPEDITORCONFIGDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* MapEditorConfigDialog::MapEditorConfigDialog
 * MapEditorConfigDialog class constructor
 *******************************************************************/
MapEditorConfigDialog::MapEditorConfigDialog(wxWindow* parent, Archive* archive, bool show_maplist, bool creating)
	: SDialog(parent, "Launch Map Editor", "")
{
	// Init variables
	this->archive = archive;
	canvas_preview = nullptr;
	game_current = Game::configuration().currentGame();
	port_current = Game::configuration().currentPort();
	this->creating = creating;

	// Setup main sizer
	wxBoxSizer* mainsizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(mainsizer);

	// Left side sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	mainsizer->Add(sizer, 0, wxEXPAND|wxALL, 6);

	// Game configuration dropdown
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "Game:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	choice_game_config = new wxChoice(this, -1);
	hbox->Add(choice_game_config, 1, wxEXPAND|wxRIGHT, 4);

	// Port configuration dropdown
	hbox->Add(new wxStaticText(this, -1, "Port:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	choice_port_config = new wxChoice(this, -1);
	hbox->Add(choice_port_config, 1, wxEXPAND|wxRIGHT, 4);

	// Populate game/port lists
	populateGameList();
	populatePortList();

	wxStaticBox* frame;
	wxStaticBoxSizer* framesizer;

	// Setup image list
	img_list = new wxImageList(16, 16);
	img_list->Add(Icons::getIcon(Icons::GENERAL, "tick"));
	img_list->Add(Icons::getIcon(Icons::GENERAL, "close"));

	// Map section
	if (show_maplist)
	{
		frame = new wxStaticBox(this, -1, "Maps");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		sizer->Add(framesizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

		// Map list
		list_maps = new ListView(this, -1, wxLC_SINGLE_SEL|wxLC_LIST);
		list_maps->SetImageList(img_list, wxIMAGE_LIST_SMALL);
		framesizer->Add(list_maps, 1, wxEXPAND|wxALL, 4);

		// New map button
		btn_new_map = new wxButton(this, -1, "New Map");
		framesizer->Add(btn_new_map, 0, wxLEFT|wxRIGHT|wxBOTTOM, 4);
	}
	else
	{
		list_maps = nullptr;
		btn_new_map = nullptr;
	}

	// Resources section
	frame = new wxStaticBox(this, -1, "Resources");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Base resource dropdown
	hbox = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, 0, wxEXPAND|wxALL, 4);
	hbox->Add(new wxStaticText(this, -1, "Base Resource:"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4);
	choice_base_resource = new BaseResourceChooser(this);
	hbox->Add(choice_base_resource, 1, wxEXPAND, 0);

	rac_resources = new ResourceArchiveChooser(this, archive);
	framesizer->Add(rac_resources, 1, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);


	// Right side (map preview)
	if (show_maplist)
	{
		frame = new wxStaticBox(this, -1, "Preview");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		mainsizer->Add(framesizer, 1, wxEXPAND|wxALL, 10);

		// Add map preview
		canvas_preview = new MapPreviewCanvas(this);
		framesizer->Add(canvas_preview->toPanel(this), 1, wxEXPAND|wxALL, 4);
		canvas_preview->SetInitialSize(wxSize(400, 400));
	}


	// Dialog buttons
	sizer->AddSpacer(4);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND|wxBOTTOM, 6);
	hbox->AddStretchSpacer();
	btn_ok = new wxButton(this, -1, "OK");
	hbox->Add(btn_ok, 0, wxEXPAND|wxRIGHT, 4);
	btn_cancel = new wxButton(this, wxID_CANCEL, "Cancel");
	hbox->Add(btn_cancel, 0, wxEXPAND|wxRIGHT, 4);

	// Populate map list
	populateMapList();

	// Bind events
	choice_game_config->Bind(wxEVT_CHOICE, &MapEditorConfigDialog::onChoiceGameConfigChanged, this);
	choice_port_config->Bind(wxEVT_CHOICE, &MapEditorConfigDialog::onChoicePortConfigChanged, this);
	if (show_maplist)
	{
		list_maps->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MapEditorConfigDialog::onMapActivated, this);
		list_maps->Bind(wxEVT_LIST_ITEM_SELECTED, &MapEditorConfigDialog::onMapSelected, this);
		btn_new_map->Bind(wxEVT_BUTTON, &MapEditorConfigDialog::onBtnNewMap, this);
	}
	btn_ok->Bind(wxEVT_BUTTON, &MapEditorConfigDialog::onBtnOK, this);
	btn_cancel->Bind(wxEVT_BUTTON, &MapEditorConfigDialog::onBtnCancel, this);

	Layout();
	SetMinClientSize(mainsizer->GetMinSize());
	//mainsizer->Fit(this);
	CenterOnParent();

	// Select first map
	if (show_maplist)
		list_maps->selectItem(0);
}

/* MapEditorConfigDialog::~MapEditorConfigDialog
 * MapEditorConfigDialog class destructor
 *******************************************************************/
MapEditorConfigDialog::~MapEditorConfigDialog()
{
	delete img_list;
}

/* MapEditorConfigDialog::populateGameList
 * Adds all configured games to the game dropdown
 *******************************************************************/
void MapEditorConfigDialog::populateGameList()
{
	// Clear current list
	choice_game_config->Clear();
	games_list.clear();

	// Populate list
	int selection = 0;
	for (auto& i : Game::gameDefs())
	{
		games_list.push_back(i.second.name);
		choice_game_config->Append(i.second.title);
		if (game_current == i.second.name)
			selection = games_list.size() - 1;
	}

	// Select current game (if any)
	choice_game_config->SetSelection(selection);
}

/* MapEditorConfigDialog::populatePortList
 * Adds all configured ports to the port dropdown (that support the
 * currently selected game)
 *******************************************************************/
void MapEditorConfigDialog::populatePortList()
{
	// Clear current list
	choice_port_config->Clear();
	ports_list.clear();

	// Get currently selected game
	auto& game = Game::gameDef(games_list[choice_game_config->GetSelection()]);

	// Populate list
	int selection = 0;
	choice_port_config->Append("None");
	for (auto& i : Game::portDefs())
	{
		if (i.second.supportsGame(game.name))
		{
			ports_list.push_back(i.second.name);
			choice_port_config->Append(i.second.title);
			if (port_current == i.second.name)
				selection = ports_list.size();
		}
	}

	// Select current port (if any)
	choice_port_config->SetSelection(selection);
}

/* MapEditorConfigDialog::populateMapList
 * Adds all maps in the current archive to the map list
 *******************************************************************/
void MapEditorConfigDialog::populateMapList()
{
	// Do nothing if map list isn't active
	if (!list_maps)
		return;

	// Get current selection (if any)
	int selection = -1;
	if (list_maps->GetSelectedItemCount() > 0)
		selection = list_maps->selectedItems()[0];

	// Clear list
	list_maps->ClearAll();
	maps.clear();

	// Check if an archive is open
	if (!archive)
		return;

	// Get all archive maps
	maps = archive->detectMaps();

	// Get currently selected game/port
	string game = games_list[choice_game_config->GetSelection()];
	string port = "";
	if (choice_port_config->GetSelection() > 0)
		port = ports_list[choice_port_config->GetSelection() - 1];

	// Add maps matching the current game configuration
	int index = 0;
	for (unsigned a = 0; a < maps.size(); a++)
	{
		// Setup format string
		string fmt = "D";
		if (maps[a].format == MAP_DOOM64)		fmt = "64";
		else if (maps[a].format == MAP_HEXEN)	fmt = "H";
		else if (maps[a].format == MAP_UDMF)	fmt = "U";
		else if (maps[a].format == MAP_UNKNOWN)	fmt = "?";

		// Create list item
		wxListItem li;
		li.SetId(index);
		li.SetText(S_FMT("(%s) %s", fmt, maps[a].name));
		if (Game::mapFormatSupported(maps[a].format, game, port))
			li.SetImage(0);
		else
			li.SetImage(1);

		// Add to list
		list_maps->InsertItem(li);
		index++;
	}

	// Restore selection
	if (selection > -1)
		list_maps->selectItem(selection);
}

/* MapEditorConfigDialog::selectedMap
 * Returns info on the currently selected map
 *******************************************************************/
Archive::mapdesc_t MapEditorConfigDialog::selectedMap()
{
	if (creating)
	{
		// Get selected game/port index
		int index = choice_port_config->GetSelection() - 1;
		string sel_port = "";
		if (index >= 0)
			sel_port = ports_list[index];
		string sel_game = games_list[choice_game_config->GetSelection()];

		// Show new map dialog
		vector<Archive::mapdesc_t> temp;
		NewMapDialog dlg(this, sel_game, sel_port, temp, archive);
		dlg.SetInitialSize(wxSize(250, -1));
		if (dlg.ShowModal() == wxID_OK)
		{
			// Get selected map name
			Archive::mapdesc_t mdesc;
			mdesc.name = dlg.getMapName();

			// Get selected map format
			int map_format = MAP_DOOM;
			for (uint8_t map_type = 0; map_type < MAP_UNKNOWN; map_type++)
				if (dlg.getMapFormat() == MAP_TYPE_NAMES[map_type])
				{
					map_format = map_type;
					break;
				}
			mdesc.format = map_format;

			return mdesc;
		}

		return Archive::mapdesc_t();
	}

	// Get selected map
	int selection = -1;
	wxArrayInt sel = list_maps->selectedItems();
	if (sel.size() > 0)
		selection = sel[0];

	// Return it if valid
	if ((unsigned)selection >= maps.size())
		return Archive::mapdesc_t();
	else
		return maps[selection];
}

/* MapEditorConfigDialog::configMatchesMap
 * Returns true if the currently selected game/port supports the
 * format of [map]
 *******************************************************************/
bool MapEditorConfigDialog::configMatchesMap(Archive::mapdesc_t map)
{
	// Get currently selected game/port
	string game = games_list[choice_game_config->GetSelection()];
	string port = "";
	if (choice_port_config->GetSelection() > 0)
		port = ports_list[choice_port_config->GetSelection() - 1];

	return Game::mapFormatSupported(map.format, game, port);
}

/* MapEditorConfigDialog::selectedGame
 * Returns the id of the currently selected game configuration
 *******************************************************************/
string MapEditorConfigDialog::selectedGame()
{
	if (choice_game_config->GetCount() == 0)
		return "";

	return games_list[choice_game_config->GetSelection()];
}

/* MapEditorConfigDialog::selectedPort
 * Returns the id of the currently selected port configuration
 *******************************************************************/
string MapEditorConfigDialog::selectedPort()
{
	if (choice_port_config->GetSelection() == 0 || choice_port_config->GetCount() == 0)
		return "";

	return ports_list[choice_port_config->GetSelection()-1];
}


/*******************************************************************
 * MAPEDITORCONFIGDIALOG CLASS EVENTS
 *******************************************************************/

/* MapEditorConfigDialog::onChoiceGameConfigChanged
 * Called when the game dropdown selection is changed
 *******************************************************************/
void MapEditorConfigDialog::onChoiceGameConfigChanged(wxCommandEvent& e)
{
	// Refresh ports combo box
	populatePortList();

	// Refresh map list
	populateMapList();

	game_current = selectedGame();
	port_current = selectedPort();
}

/* MapEditorConfigDialog::onChoicePortConfigChanged
 * Called when the port dropdown selection is changed
 *******************************************************************/
void MapEditorConfigDialog::onChoicePortConfigChanged(wxCommandEvent& e)
{
	// Refresh map list
	populateMapList();

	game_current = selectedGame();
	port_current = selectedPort();
}

/* MapEditorConfigDialog::onMapActivated
 * Called when a map in the maps list is activated
 *******************************************************************/
void MapEditorConfigDialog::onMapActivated(wxListEvent& e)
{
	if (configMatchesMap(selectedMap()))
		EndModal(wxID_OK);
}

/* MapEditorConfigDialog::onBtnNewMap
 * Called when the 'New Map' button is clicked
 *******************************************************************/
void MapEditorConfigDialog::onBtnNewMap(wxCommandEvent& e)
{
	// Get selected game/port index
	int index = choice_port_config->GetSelection() - 1;
	string sel_port = "";
	if (index >= 0)
		sel_port = ports_list[index];
	string sel_game = games_list[choice_game_config->GetSelection()];

	// Create new map dialog
	NewMapDialog dlg(this, sel_game, sel_port, maps, archive);

	// Show it
	dlg.SetInitialSize(wxSize(250, -1));
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		string mapname = dlg.getMapName();
		if (mapname.IsEmpty())
			return;

		// Check the map name isn't already taken
		for (unsigned a = 0; a < maps.size(); a++)
		{
			if (S_CMPNOCASE(maps[a].name, mapname))
			{
				wxMessageBox("Map " + mapname + " already exists", "Error");
				return;
			}
		}

		// Get selected map format
		int map_format = MAP_DOOM;
		for (uint8_t map_type = 0; map_type < MAP_UNKNOWN; map_type++)
			if (dlg.getMapFormat() == MAP_TYPE_NAMES[map_type])
			{
				map_format = map_type;
				break;
			}

		// Check archive type
		if (archive->getType() == ARCHIVE_WAD)
		{
			// Create new (empty) map at the end of the wad
			ArchiveEntry* head = archive->addNewEntry(mapname);
			ArchiveEntry* end = nullptr;

			if (map_format == MAP_UDMF)
			{
				// UDMF
				archive->addNewEntry("TEXTMAP");
				end = archive->addNewEntry("ENDMAP");
			}
			else
			{
				// Doom(64) / Hexen
				archive->addNewEntry("THINGS");
				archive->addNewEntry("LINEDEFS");
				archive->addNewEntry("SIDEDEFS");
				archive->addNewEntry("VERTEXES");
				end = archive->addNewEntry("SECTORS");

				// Hexen
				if (map_format == MAP_HEXEN)
					end = archive->addNewEntry("BEHAVIOR");
			}

			// Refresh map list
			populateMapList();
			list_maps->selectItem(list_maps->GetItemCount()-1);
		}
		else if (archive->getType() == ARCHIVE_ZIP
			|| archive->getType() == ARCHIVE_FOLDER)
		{
			// Create new wad archive for the map
			Archive* wad = new WadArchive();

			// Create new (empty) map at the end of the wad
			ArchiveEntry* head = wad->addNewEntry(mapname);
			ArchiveEntry* end = nullptr;

			if (map_format == MAP_UDMF)
			{
				// UDMF
				wad->addNewEntry("TEXTMAP");
				end = wad->addNewEntry("ENDMAP");
			}
			else
			{
				// Doom(64) / Hexen
				wad->addNewEntry("THINGS");
				wad->addNewEntry("LINEDEFS");
				wad->addNewEntry("SIDEDEFS");
				wad->addNewEntry("VERTEXES");
				end = wad->addNewEntry("SECTORS");

				// Hexen
				if (map_format == MAP_HEXEN)
					end = wad->addNewEntry("BEHAVIOR");
				// Doom 64
				else if (map_format == MAP_DOOM64)
				{
					wad->addNewEntry("LIGHTS");
					end = wad->addNewEntry("MACROS");
				}
			}

			// Add new map entry to the maps dir
			//ArchiveEntry* mapentry = archive->addNewEntry(mapname+".wad", 0xFFFFFFFF, archive->createDir("maps"));
			ArchiveEntry* mapentry = archive->addNewEntry(mapname + ".wad", "maps");
			MemChunk mc;
			wad->write(mc);
			mapentry->importMemChunk(mc);

			// Clean up
			delete wad;

			// Refresh map list
			populateMapList();
			list_maps->selectItem(list_maps->GetItemCount()-1);
		}
	}
}

/* MapEditorConfigDialog::onMapSelected
 * Called when the selection on the map list is changed
 *******************************************************************/
void MapEditorConfigDialog::onMapSelected(wxListEvent& e)
{
	if (!canvas_preview)
		return;

	Archive::mapdesc_t map = selectedMap();
	canvas_preview->clearMap();
	canvas_preview->openMap(map);
	btn_ok->Enable(configMatchesMap(map));
}

/* MapEditorConfigDialog::onBtnOK
 * Called when the 'OK' button is clicked
 *******************************************************************/
void MapEditorConfigDialog::onBtnOK(wxCommandEvent& e)
{
	EndModal(wxID_OK);
}

/* MapEditorConfigDialog::onBtnCancel
 * Called when the 'Cancel' button is clicked
 *******************************************************************/
void MapEditorConfigDialog::onBtnCancel(wxCommandEvent& e)
{
	EndModal(wxID_CANCEL);
}
