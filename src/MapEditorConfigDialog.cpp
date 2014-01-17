
#include "Main.h"
#include "WxStuff.h"
#include "MapEditorConfigDialog.h"
#include "BaseResourceChooser.h"
#include "GameConfiguration.h"
#include "ArchiveManager.h"
#include "WadArchive.h"
#include "SFileDialog.h"
#include "SplashWindow.h"
#include "ColourConfiguration.h"
#include "Icons.h"
#include "ResourceArchiveChooser.h"
#include <wx/statline.h>
#include <wx/gbsizer.h>


class NewMapDialog : public wxDialog
{
private:
	wxComboBox*	cbo_mapname;
	wxChoice*	choice_mapformat;

public:
	NewMapDialog(wxWindow* parent, int game, int port, vector<Archive::mapdesc_t>& maps) : wxDialog(parent, -1, "New Map")
	{
		// Setup dialog
		wxBoxSizer* msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		wxGridBagSizer* sizer = new wxGridBagSizer(4, 4);
		msizer->Add(sizer, 1, wxEXPAND|wxALL, 10);

		// Open selected game configuration if no map names are currently loaded
		if (theGameConfiguration->nMapNames() == 0)
		{
			string gname = theGameConfiguration->gameConfig(game).name;
			string pname = theGameConfiguration->portConfig(port).name;
			theGameConfiguration->openConfig(gname, pname);
		}

		// Check if the game configuration allows any map name
		int flags = 0;
		if (!theGameConfiguration->anyMapName())
			flags = wxCB_READONLY;

		// Create map name combo box
		cbo_mapname = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, flags);
		cbo_mapname->SetMaxLength(8);
		sizer->Add(new wxStaticText(this, -1, "Map Name:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		sizer->Add(cbo_mapname, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

		// Add possible map names to the combo box
		for (unsigned a = 0; a < theGameConfiguration->nMapNames(); a++)
		{
			// Check if map already exists
			string mapname = theGameConfiguration->mapName(a);
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
		if (theGameConfiguration->nMapNames() > 0)
			cbo_mapname->SetSelection(0);

		// Create map format combo box
		choice_mapformat = new wxChoice(this, -1);
		sizer->Add(new wxStaticText(this, -1, "Map Format:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		sizer->Add(choice_mapformat, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

		// Add possible map formats to the combo box
		if (theGameConfiguration->mapFormatSupported(MAP_DOOM, game, port))
			choice_mapformat->Append("Doom");
		if (theGameConfiguration->mapFormatSupported(MAP_HEXEN, game, port))
			choice_mapformat->Append("Hexen");
		if (theGameConfiguration->mapFormatSupported(MAP_UDMF, game, port))
			choice_mapformat->Append("UDMF");
		if (theGameConfiguration->mapFormatSupported(MAP_DOOM64, game, port))
			choice_mapformat->Append("Doom64");
		choice_mapformat->SetSelection(0);

		// Add dialog buttons
		sizer->Add(CreateButtonSizer(wxOK|wxCANCEL), wxGBPosition(2, 0), wxGBSpan(1, 2), wxEXPAND);
		sizer->AddGrowableCol(1, 1);

		Layout();
		sizer->Fit(this);
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
};

MapEditorConfigDialog::MapEditorConfigDialog(wxWindow* parent, Archive* archive, bool show_maplist, bool creating) : wxDialog(parent, -1, "Launch Map Editor")
{
	// Init variables
	this->archive = archive;
	canvas_preview = NULL;
	game_current = theGameConfiguration->currentGame();
	port_current = theGameConfiguration->currentPort();
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
	img_list->Add(getIcon("i_tick"));
	img_list->Add(getIcon("t_close"));

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
		list_maps = NULL;
		btn_new_map = NULL;
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
		mainsizer->Add(framesizer, 0, wxEXPAND|wxALL, 10);

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
	btn_cancel = new wxButton(this, -1, "Cancel");
	hbox->Add(btn_cancel, 0, wxEXPAND|wxRIGHT, 4);

	// Populate map list
	populateMapList();

	// Bind events
	choice_game_config->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &MapEditorConfigDialog::onChoiceGameConfigChanged, this);
	choice_port_config->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &MapEditorConfigDialog::onChoicePortConfigChanged, this);
	if (show_maplist)
	{
		list_maps->Bind(wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &MapEditorConfigDialog::onMapActivated, this);
		list_maps->Bind(wxEVT_COMMAND_LIST_ITEM_SELECTED, &MapEditorConfigDialog::onMapSelected, this);
		btn_new_map->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MapEditorConfigDialog::onBtnNewMap, this);
	}
	btn_ok->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MapEditorConfigDialog::onBtnOK, this);
	btn_cancel->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MapEditorConfigDialog::onBtnCancel, this);

	Layout();
	mainsizer->Fit(this);
	CenterOnParent();

	// Select first map
	if (show_maplist)
		list_maps->selectItem(0);
}

MapEditorConfigDialog::~MapEditorConfigDialog()
{
	delete img_list;
}

void MapEditorConfigDialog::populateGameList()
{
	// Clear current list
	choice_game_config->Clear();

	// Populate list
	int selection = 0;
	for (unsigned a = 0; a < theGameConfiguration->nGameConfigs(); a++)
	{
		choice_game_config->Append(theGameConfiguration->gameConfig(a).title);
		if (game_current == theGameConfiguration->gameConfig(a).name)
			selection = a;
	}

	// Select current game (if any)
	choice_game_config->SetSelection(selection);
}

void MapEditorConfigDialog::populatePortList()
{
	// Clear current list
	choice_port_config->Clear();
	ports_list.clear();

	// Get currently selected game
	string game = theGameConfiguration->gameConfig(choice_game_config->GetSelection()).name;

	// Populate list
	int selection = 0;
	int index = 1;
	choice_port_config->Append("None");
	for (unsigned a = 0; a < theGameConfiguration->nPortConfigs(); a++)
	{
		if (theGameConfiguration->portSupportsGame(a, game))
		{
			ports_list.push_back(a);
			choice_port_config->Append(theGameConfiguration->portConfig(a).title);
			if (port_current == theGameConfiguration->portConfig(a).name)
				selection = index;
			index++;
		}
	}

	// Select current port (if any)
	choice_port_config->SetSelection(selection);
}

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
	int game = choice_game_config->GetSelection();
	int port = -1;
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
		if (theGameConfiguration->mapFormatSupported(maps[a].format, game, port))
			li.SetImage(0);
		else
			li.SetImage(1);

		// Add to list
		list_maps->InsertItem(li);
		index++;
	}

	// Restore selection
	list_maps->selectItem(selection);
}

Archive::mapdesc_t MapEditorConfigDialog::selectedMap()
{
	if (creating)
	{
		// Get selected game/port index
		int sel_port = choice_port_config->GetSelection() - 1;
		if (sel_port >= 0)
			sel_port = ports_list[sel_port];
		int sel_game = choice_game_config->GetSelection();

		// Show new map dialog
		vector<Archive::mapdesc_t> temp;
		NewMapDialog dlg(this, sel_game, sel_port, temp);
		dlg.SetInitialSize(wxSize(250, -1));
		if (dlg.ShowModal() == wxID_OK)
		{
			// Get selected map name
			Archive::mapdesc_t mdesc;
			mdesc.name = dlg.getMapName();

			// Get selected map format
			int map_format = MAP_DOOM;
			if (dlg.getMapFormat() == "Hexen")
				map_format = MAP_HEXEN;
			else if (dlg.getMapFormat() == "UDMF")
				map_format = MAP_UDMF;
			else if (dlg.getMapFormat() == "Doom64")
				map_format = MAP_DOOM64;
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

bool MapEditorConfigDialog::configMatchesMap(Archive::mapdesc_t map)
{
	// Get currently selected game/port
	int game = choice_game_config->GetSelection();
	int port = -1;
	if (choice_port_config->GetSelection() > 0)
		port = ports_list[choice_port_config->GetSelection() - 1];

	return theGameConfiguration->mapFormatSupported(map.format, game, port);
}

string MapEditorConfigDialog::selectedGame()
{
	if (choice_game_config->GetCount() == 0)
		return "";

	return theGameConfiguration->gameConfig(choice_game_config->GetSelection()).name;
}

string MapEditorConfigDialog::selectedPort()
{
	if (choice_port_config->GetSelection() == 0 || choice_port_config->GetCount() == 0)
		return "";

	return theGameConfiguration->portConfig(ports_list[choice_port_config->GetSelection()-1]).name;
}



void MapEditorConfigDialog::onChoiceGameConfigChanged(wxCommandEvent& e)
{
	// Refresh ports combo box
	populatePortList();

	// Refresh map list
	populateMapList();

	game_current = selectedGame();
	port_current = selectedPort();
}

void MapEditorConfigDialog::onChoicePortConfigChanged(wxCommandEvent& e)
{
	// Refresh map list
	populateMapList();

	game_current = selectedGame();
	port_current = selectedPort();
}

void MapEditorConfigDialog::onMapActivated(wxListEvent& e)
{
	if (configMatchesMap(selectedMap()))
		EndModal(wxID_OK);
}

void MapEditorConfigDialog::onBtnNewMap(wxCommandEvent& e)
{
	// Get selected game/port index
	int sel_port = choice_port_config->GetSelection() - 1;
	if (sel_port >= 0)
		sel_port = ports_list[sel_port];
	int sel_game = choice_game_config->GetSelection();

	// Create new map dialog
	NewMapDialog dlg(this, sel_game, sel_port, maps);

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
		if (dlg.getMapFormat() == "Hexen")
			map_format = MAP_HEXEN;
		else if (dlg.getMapFormat() == "UDMF")
			map_format = MAP_UDMF;
		else if (dlg.getMapFormat() == "Doom64")
			map_format = MAP_DOOM64;

		// Check archive type
		if (archive->getType() == ARCHIVE_WAD)
		{
			// Create new (empty) map at the end of the wad
			ArchiveEntry* head = archive->addNewEntry(mapname);
			ArchiveEntry* end = NULL;

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
		else if (archive->getType() == ARCHIVE_ZIP)
		{
			// Create new wad archive for the map
			Archive* wad = new WadArchive();

			// Create new (empty) map at the end of the wad
			ArchiveEntry* head = wad->addNewEntry(mapname);
			ArchiveEntry* end = NULL;

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
			ArchiveEntry* mapentry = archive->addNewEntry(mapname+".wad", 0xFFFFFFFF, archive->getDir("maps"));
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

void MapEditorConfigDialog::onMapSelected(wxListEvent& e)
{
	if (!canvas_preview)
		return;

	Archive::mapdesc_t map = selectedMap();
	canvas_preview->clearMap();
	canvas_preview->openMap(map);
	btn_ok->Enable(configMatchesMap(map));
}

void MapEditorConfigDialog::onBtnOK(wxCommandEvent& e)
{
	EndModal(wxID_OK);
}

void MapEditorConfigDialog::onBtnCancel(wxCommandEvent& e)
{
	EndModal(wxID_CANCEL);
}
