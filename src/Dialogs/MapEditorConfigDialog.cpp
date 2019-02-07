
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapEditorConfigDialog.cpp
// Description: Dialog that shows options for launching the map editor - game,
//              port, and resource archives. Also can show a map list and
//              preview
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
#include "MapEditorConfigDialog.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/WadArchive.h"
#include "Game/Configuration.h"
#include "Graphics/Icons.h"
#include "UI/Canvas/MapPreviewCanvas.h"
#include "UI/Controls/BaseResourceChooser.h"
#include "UI/Controls/ResourceArchiveChooser.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
struct MapFormatDef
{
	MapFormat format;
	wxString  name;
	wxString  abbreviation;
};
MapFormatDef map_formats[] = { { MapFormat::Doom, "Doom", "D" },
							   { MapFormat::Hexen, "Hexen", "H" },
							   { MapFormat::Doom64, "Doom64", "64" },
							   { MapFormat::UDMF, "UDMF", "U" } };
} // namespace


// -----------------------------------------------------------------------------
// NewMapDialog Class
//
// Dialog for creating a new map - select map format and name
// -----------------------------------------------------------------------------
class NewMapDialog : public wxDialog
{
public:
	NewMapDialog(
		wxWindow*                 parent,
		const wxString&           game,
		const wxString&           port,
		vector<Archive::MapDesc>& maps,
		Archive*                  archive) :
		wxDialog(parent, -1, "New Map")
	{
		// Setup dialog
		auto msizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(msizer);
		auto sizer = new wxGridBagSizer(UI::pad(), UI::pad());
		msizer->Add(sizer, 1, wxEXPAND | wxALL, UI::padLarge());

		// Open selected game configuration if no map names are currently loaded
		Game::configuration().openConfig(game, port);

		// Check if the game configuration allows any map name
		int flags = 0;
		if (!Game::configuration().featureSupported(Game::Feature::AnyMapName))
			flags = wxCB_READONLY;

		// Create map name combo box
		cbo_mapname_ = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, flags);
		sizer->Add(new wxStaticText(this, -1, "Map Name:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		sizer->Add(cbo_mapname_, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);

		// Limit map name length if necessary
		if (Game::configuration().featureSupported(Game::Feature::AnyMapName)
			&& (!Game::configuration().featureSupported(Game::Feature::LongNames)
				|| (archive && archive->formatId() != "zip" && archive->formatId() != "7z"
					&& archive->formatId() != "folder")))
			cbo_mapname_->SetMaxLength(8);

		// Add possible map names to the combo box
		for (unsigned a = 0; a < Game::configuration().nMapNames(); a++)
		{
			// Check if map already exists
			wxString mapname = Game::configuration().mapName(a);
			bool     exists  = false;
			for (auto& map : maps)
			{
				if (S_CMPNOCASE(map.name, mapname))
				{
					exists = true;
					break;
				}
			}

			if (!exists)
				cbo_mapname_->Append(mapname);
		}

		// Set inital map name selection
		if (Game::configuration().nMapNames() > 0)
			cbo_mapname_->SetSelection(0);

		// Create map format combo box
		choice_mapformat_ = new wxChoice(this, -1);
		sizer->Add(
			new wxStaticText(this, -1, "Map Format:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		sizer->Add(choice_mapformat_, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);

		// Add possible map formats to the combo box
		auto default_format = MapFormat::Unknown;
		if (!maps.empty())
			default_format = maps[0].format;
		for (auto mf : map_formats)
		{
			if (Game::mapFormatSupported(mf.format, game, port))
			{
				choice_mapformat_->Append(mf.name);
				if (mf.format == default_format)
					choice_mapformat_->SetSelection(choice_mapformat_->GetCount() - 1);
			}
		}
		// Default to the "best" supported format, the last one in the list
		if (choice_mapformat_->GetSelection() == wxNOT_FOUND)
			choice_mapformat_->SetSelection(choice_mapformat_->GetCount() - 1);

		// Add dialog buttons
		auto hbox = new wxBoxSizer(wxHORIZONTAL);
		msizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::padLarge());
		hbox->AddStretchSpacer();
		btn_ok_ = new wxButton(this, -1, "OK");
		hbox->Add(btn_ok_, 0, wxEXPAND | wxRIGHT, UI::pad());
		btn_cancel_ = new wxButton(this, -1, "Cancel");
		hbox->Add(btn_cancel_, 0, wxEXPAND);
		sizer->AddGrowableCol(1);

		// Bind events
		btn_ok_->Bind(wxEVT_BUTTON, &NewMapDialog::onBtnOk, this);
		btn_cancel_->Bind(wxEVT_BUTTON, &NewMapDialog::onBtnCancel, this);

		wxWindowBase::Layout();
		msizer->Fit(this);
		CenterOnParent();
	}

	wxString getMapName() const { return cbo_mapname_->GetValue(); }
	wxString getMapFormat() const { return choice_mapformat_->GetStringSelection(); }

	void onBtnOk(wxCommandEvent& e) { EndModal(wxID_OK); }
	void onBtnCancel(wxCommandEvent& e) { EndModal(wxID_CANCEL); }

private:
	wxComboBox* cbo_mapname_;
	wxChoice*   choice_mapformat_;
	wxButton*   btn_ok_;
	wxButton*   btn_cancel_;
};


// -----------------------------------------------------------------------------
//
// MapEditorConfigDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapEditorConfigDialog class constructor
// -----------------------------------------------------------------------------
MapEditorConfigDialog::MapEditorConfigDialog(wxWindow* parent, Archive* archive, bool show_maplist, bool creating) :
	SDialog(parent, "Launch Map Editor", ""),
	game_current_{ Game::configuration().currentGame() },
	port_current_{ Game::configuration().currentPort() },
	creating_{ creating },
	archive_{ archive }
{
	// Setup main sizer
	auto mainsizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(mainsizer);

	// Left side sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	mainsizer->Add(sizer, 0, wxEXPAND | wxALL, UI::padLarge());

	// Game configuration dropdown
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxBOTTOM, UI::pad());
	hbox->Add(new wxStaticText(this, -1, "Game:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	choice_game_config_ = new wxChoice(this, -1);
	hbox->Add(choice_game_config_, 1, wxEXPAND | wxRIGHT, UI::padLarge());

	// Port configuration dropdown
	hbox->Add(new wxStaticText(this, -1, "Port:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	choice_port_config_ = new wxChoice(this, -1);
	hbox->Add(choice_port_config_, 1, wxEXPAND);

	// Populate game/port lists
	populateGameList();
	populatePortList();

	wxStaticBox*      frame;
	wxStaticBoxSizer* framesizer;

	// Setup image list
	img_list_ = WxUtils::createSmallImageList();
	img_list_->Add(Icons::getIcon(Icons::General, "tick"));
	img_list_->Add(Icons::getIcon(Icons::General, "close"));

	// Map section
	if (show_maplist)
	{
		frame      = new wxStaticBox(this, -1, "Maps");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		sizer->Add(framesizer, 1, wxEXPAND | wxBOTTOM, UI::pad());

		// Map list
		list_maps_ = new ListView(this, -1, wxLC_SINGLE_SEL | wxLC_LIST);
		list_maps_->SetImageList(img_list_, wxIMAGE_LIST_SMALL);
		framesizer->Add(list_maps_, 1, wxEXPAND | wxALL, UI::pad());

		// New map button
		btn_new_map_ = new wxButton(this, -1, "New Map");
		framesizer->Add(btn_new_map_, 0, wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());
	}
	else
	{
		list_maps_   = nullptr;
		btn_new_map_ = nullptr;
	}

	// Resources section
	frame      = new wxStaticBox(this, -1, "Resources");
	framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
	sizer->Add(framesizer, 1, wxEXPAND | wxBOTTOM, UI::pad());

	// Base resource dropdown
	hbox = new wxBoxSizer(wxHORIZONTAL);
	framesizer->Add(hbox, 0, wxEXPAND | wxALL, UI::pad());
	hbox->Add(new wxStaticText(this, -1, "Base Resource:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	choice_base_resource_ = new BaseResourceChooser(this);
	hbox->Add(choice_base_resource_, 1, wxEXPAND, 0);

	rac_resources_ = new ResourceArchiveChooser(this, archive);
	framesizer->Add(rac_resources_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::pad());


	// Right side (map preview)
	if (show_maplist)
	{
		frame      = new wxStaticBox(this, -1, "Preview");
		framesizer = new wxStaticBoxSizer(frame, wxVERTICAL);
		mainsizer->Add(framesizer, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, UI::padLarge());

		// Add map preview
		canvas_preview_ = new MapPreviewCanvas(this);
		framesizer->Add(canvas_preview_->toPanel(this), 1, wxEXPAND | wxALL, UI::pad());
		int size = UI::scalePx(400);
		canvas_preview_->SetInitialSize(wxSize(size, size));
	}


	// Dialog buttons
	sizer->AddSpacer(UI::pad());
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND);
	hbox->AddStretchSpacer();
	btn_ok_ = new wxButton(this, -1, "OK");
	hbox->Add(btn_ok_, 0, wxEXPAND | wxRIGHT, UI::pad());
	btn_cancel_ = new wxButton(this, wxID_CANCEL, "Cancel");
	hbox->Add(btn_cancel_, 0, wxEXPAND);

	// Populate map list
	populateMapList();

	// Bind events
	choice_game_config_->Bind(wxEVT_CHOICE, &MapEditorConfigDialog::onChoiceGameConfigChanged, this);
	choice_port_config_->Bind(wxEVT_CHOICE, &MapEditorConfigDialog::onChoicePortConfigChanged, this);
	if (show_maplist)
	{
		list_maps_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MapEditorConfigDialog::onMapActivated, this);
		list_maps_->Bind(wxEVT_LIST_ITEM_SELECTED, &MapEditorConfigDialog::onMapSelected, this);
		btn_new_map_->Bind(wxEVT_BUTTON, &MapEditorConfigDialog::onBtnNewMap, this);
	}
	btn_ok_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { EndModal(wxID_OK); });
	btn_cancel_->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { EndModal(wxID_CANCEL); });

	wxWindowBase::Layout();
	wxWindowBase::SetMinClientSize(mainsizer->GetMinSize());
	CenterOnParent();

	// Select first map
	if (show_maplist)
		list_maps_->selectItem(0);
}

// -----------------------------------------------------------------------------
// MapEditorConfigDialog class destructor
// -----------------------------------------------------------------------------
MapEditorConfigDialog::~MapEditorConfigDialog()
{
	delete img_list_;
}

// -----------------------------------------------------------------------------
// Adds all configured games to the game dropdown
// -----------------------------------------------------------------------------
void MapEditorConfigDialog::populateGameList()
{
	// Clear current list
	choice_game_config_->Clear();
	games_list_.clear();

	// Populate list
	int selection = 0;
	for (auto& i : Game::gameDefs())
	{
		games_list_.push_back(i.second.name);
		choice_game_config_->Append(i.second.title);
		if (game_current_ == i.second.name)
			selection = games_list_.size() - 1;
	}

	// Select current game (if any)
	choice_game_config_->SetSelection(selection);
}

// -----------------------------------------------------------------------------
// Adds all configured ports to the port dropdown (that support the currently
// selected game)
// -----------------------------------------------------------------------------
void MapEditorConfigDialog::populatePortList()
{
	// Clear current list
	choice_port_config_->Clear();
	ports_list_.clear();

	// Get currently selected game
	auto& game = Game::gameDef(games_list_[choice_game_config_->GetSelection()]);

	// Populate list
	int selection = 0;
	choice_port_config_->Append("None");
	for (auto& i : Game::portDefs())
	{
		if (i.second.supportsGame(game.name))
		{
			ports_list_.push_back(i.second.name);
			choice_port_config_->Append(i.second.title);
			if (port_current_ == i.second.name)
				selection = ports_list_.size();
		}
	}

	// Select current port (if any)
	choice_port_config_->SetSelection(selection);
}

// -----------------------------------------------------------------------------
// Adds all maps in the current archive to the map list
// -----------------------------------------------------------------------------
void MapEditorConfigDialog::populateMapList()
{
	// Do nothing if map list isn't active
	if (!list_maps_)
		return;

	// Get current selection (if any)
	int selection = -1;
	if (list_maps_->GetSelectedItemCount() > 0)
		selection = list_maps_->selectedItems()[0];

	// Clear list
	list_maps_->ClearAll();
	maps_.clear();

	// Check if an archive is open
	if (!archive_)
		return;

	// Get all archive maps
	maps_ = archive_->detectMaps();

	// Get currently selected game/port
	wxString game = games_list_[choice_game_config_->GetSelection()];
	wxString port = "";
	if (choice_port_config_->GetSelection() > 0)
		port = ports_list_[choice_port_config_->GetSelection() - 1];

	// Add maps matching the current game configuration
	int index = 0;
	for (auto& map : maps_)
	{
		// Setup format string
		wxString fmt = "?";
		for (auto mf : map_formats)
			if (mf.format == map.format)
				fmt = mf.abbreviation;

		// Create list item
		wxListItem li;
		li.SetId(index);
		li.SetText(wxString::Format("(%s) %s", fmt, map.name));
		if (Game::mapFormatSupported(map.format, game, port))
			li.SetImage(0);
		else
			li.SetImage(1);

		// Add to list
		list_maps_->InsertItem(li);
		index++;
	}

	// Restore selection
	if (selection > -1)
		list_maps_->selectItem(selection);
}

// -----------------------------------------------------------------------------
// Returns info on the currently selected map
// -----------------------------------------------------------------------------
Archive::MapDesc MapEditorConfigDialog::selectedMap()
{
	if (creating_)
	{
		// Get selected game/port index
		int      index    = choice_port_config_->GetSelection() - 1;
		wxString sel_port = "";
		if (index >= 0)
			sel_port = ports_list_[index];
		wxString sel_game = games_list_[choice_game_config_->GetSelection()];

		// Show new map dialog
		vector<Archive::MapDesc> temp;
		NewMapDialog             dlg(this, sel_game, sel_port, temp, archive_);
		dlg.SetInitialSize(wxSize(250, -1));
		if (dlg.ShowModal() == wxID_OK)
		{
			// Get selected map name
			Archive::MapDesc mdesc;
			mdesc.name = dlg.getMapName();

			// Get selected map format
			auto map_format = MapFormat::Doom;
			for (auto mf : map_formats)
				if (dlg.getMapFormat() == mf.name)
				{
					map_format = mf.format;
					break;
				}
			mdesc.format = map_format;

			return mdesc;
		}

		return Archive::MapDesc();
	}

	// Get selected map
	int  selection = -1;
	auto sel       = list_maps_->selectedItems();
	if (!sel.empty())
		selection = sel[0];

	// Return it if valid
	if ((unsigned)selection >= maps_.size())
		return Archive::MapDesc();
	else
		return maps_[selection];
}

// -----------------------------------------------------------------------------
// Returns true if the currently selected game/port supports the format of [map]
// -----------------------------------------------------------------------------
bool MapEditorConfigDialog::configMatchesMap(const Archive::MapDesc& map)
{
	// Get currently selected game/port
	wxString game = games_list_[choice_game_config_->GetSelection()];
	wxString port = "";
	if (choice_port_config_->GetSelection() > 0)
		port = ports_list_[choice_port_config_->GetSelection() - 1];

	return Game::mapFormatSupported(map.format, game, port);
}

// -----------------------------------------------------------------------------
// Returns the id of the currently selected game configuration
// -----------------------------------------------------------------------------
wxString MapEditorConfigDialog::selectedGame()
{
	if (choice_game_config_->GetCount() == 0)
		return "";

	return games_list_[choice_game_config_->GetSelection()];
}

// -----------------------------------------------------------------------------
// Returns the id of the currently selected port configuration
// -----------------------------------------------------------------------------
wxString MapEditorConfigDialog::selectedPort()
{
	if (choice_port_config_->GetSelection() == 0 || choice_port_config_->GetCount() == 0)
		return "";

	return ports_list_[choice_port_config_->GetSelection() - 1];
}


// -----------------------------------------------------------------------------
//
// MapEditorConfigDialog Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the game dropdown selection is changed
// -----------------------------------------------------------------------------
void MapEditorConfigDialog::onChoiceGameConfigChanged(wxCommandEvent& e)
{
	// Refresh ports combo box
	populatePortList();

	// Refresh map list
	populateMapList();

	game_current_ = selectedGame();
	port_current_ = selectedPort();
}

// -----------------------------------------------------------------------------
// Called when the port dropdown selection is changed
// -----------------------------------------------------------------------------
void MapEditorConfigDialog::onChoicePortConfigChanged(wxCommandEvent& e)
{
	// Refresh map list
	populateMapList();

	game_current_ = selectedGame();
	port_current_ = selectedPort();
}

// -----------------------------------------------------------------------------
// Called when a map in the maps list is activated
// -----------------------------------------------------------------------------
void MapEditorConfigDialog::onMapActivated(wxListEvent& e)
{
	if (configMatchesMap(selectedMap()))
		EndModal(wxID_OK);
}

// -----------------------------------------------------------------------------
// Called when the 'New Map' button is clicked
// -----------------------------------------------------------------------------
void MapEditorConfigDialog::onBtnNewMap(wxCommandEvent& e)
{
	// Get selected game/port index
	int      index    = choice_port_config_->GetSelection() - 1;
	wxString sel_port = "";
	if (index >= 0)
		sel_port = ports_list_[index];
	wxString sel_game = games_list_[choice_game_config_->GetSelection()];

	// Create new map dialog
	NewMapDialog dlg(this, sel_game, sel_port, maps_, archive_);

	// Show it
	dlg.SetInitialSize(wxSize(250, -1));
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		wxString mapname = dlg.getMapName();
		if (mapname.IsEmpty())
			return;

		// Check the map name isn't already taken
		for (auto& map : maps_)
		{
			if (S_CMPNOCASE(map.name, mapname))
			{
				wxMessageBox("Map " + mapname + " already exists", "Error");
				return;
			}
		}

		// Get selected map format
		auto map_format = MapFormat::Doom;
		for (auto& mf : map_formats)
			if (dlg.getMapFormat() == mf.name)
			{
				map_format = mf.format;
				break;
			}

		// Check archive type
		if (archive_->formatId() == "wad")
		{
			// Create new (empty) map at the end of the wad
			ArchiveEntry* head = archive_->addNewEntry(mapname);
			ArchiveEntry* end  = nullptr;

			if (map_format == MapFormat::UDMF)
			{
				// UDMF
				archive_->addNewEntry("TEXTMAP");
				end = archive_->addNewEntry("ENDMAP");
			}
			else
			{
				// Doom(64) / Hexen
				archive_->addNewEntry("THINGS");
				archive_->addNewEntry("LINEDEFS");
				archive_->addNewEntry("SIDEDEFS");
				archive_->addNewEntry("VERTEXES");
				end = archive_->addNewEntry("SECTORS");

				// Hexen
				if (map_format == MapFormat::Hexen)
					end = archive_->addNewEntry("BEHAVIOR");

				// Doom64
				if (map_format == MapFormat::Doom64)
				{
					archive_->addNewEntry("LEAFS");
					archive_->addNewEntry("LIGHTS");
					end = archive_->addNewEntry("MACROS");
				}
			}

			// Refresh map list
			populateMapList();
			list_maps_->selectItem(list_maps_->GetItemCount() - 1);
		}
		else if (archive_->formatId() == "zip" || archive_->formatId() == "folder")
		{
			// Create new wad archive for the map
			Archive* wad = new WadArchive();

			// Create new (empty) map at the end of the wad
			ArchiveEntry* head = wad->addNewEntry(mapname);
			ArchiveEntry* end  = nullptr;

			if (map_format == MapFormat::UDMF)
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
				if (map_format == MapFormat::Hexen)
					end = wad->addNewEntry("BEHAVIOR");
				// Doom 64
				else if (map_format == MapFormat::Doom64)
				{
					wad->addNewEntry("LIGHTS");
					end = wad->addNewEntry("MACROS");
				}
			}

			// Add new map entry to the maps dir
			ArchiveEntry* mapentry = archive_->addNewEntry(mapname + ".wad", "maps");
			MemChunk      mc;
			wad->write(mc);
			mapentry->importMemChunk(mc);

			// Clean up
			delete wad;

			// Refresh map list
			populateMapList();
			list_maps_->selectItem(list_maps_->GetItemCount() - 1);
		}
	}
}

// -----------------------------------------------------------------------------
// Called when the selection on the map list is changed
// -----------------------------------------------------------------------------
void MapEditorConfigDialog::onMapSelected(wxListEvent& e)
{
	if (!canvas_preview_)
		return;

	auto map = selectedMap();
	canvas_preview_->clearMap();
	canvas_preview_->openMap(map);
	btn_ok_->Enable(configMatchesMap(map));
}
