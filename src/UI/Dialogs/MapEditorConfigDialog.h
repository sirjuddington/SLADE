#pragma once

#include "Archive/Archive.h"
#include "UI/Lists/ListView.h"
#include "UI/SDialog.h"

class wxImageList;

namespace slade
{
class MapPreviewCanvas;
class BaseResourceChooser;
class ResourceArchiveChooser;

class MapEditorConfigDialog : public SDialog
{
public:
	MapEditorConfigDialog(wxWindow* parent, Archive* archive = nullptr, bool show_maps = true, bool creating = false);
	~MapEditorConfigDialog() override;

	void             populateGameList();
	void             populatePortList();
	void             populateMapList();
	Archive::MapDesc selectedMap();
	bool             configMatchesMap(const Archive::MapDesc& map) const;
	string           selectedGame();
	string           selectedPort();

private:
	wxChoice*               choice_game_config_   = nullptr;
	wxChoice*               choice_port_config_   = nullptr;
	BaseResourceChooser*    choice_base_resource_ = nullptr;
	ListView*               list_maps_            = nullptr;
	ResourceArchiveChooser* rac_resources_        = nullptr;
	wxButton*               btn_new_map_          = nullptr;
	MapPreviewCanvas*       canvas_preview_       = nullptr;
	wxImageList*            img_list_             = nullptr;
	wxButton*               btn_ok_               = nullptr;
	wxButton*               btn_cancel_           = nullptr;
	string                  game_current_;
	string                  port_current_;
	bool                    creating_ = false;

	Archive*                 archive_ = nullptr;
	vector<Archive::MapDesc> maps_;
	vector<string>           games_list_;
	vector<string>           ports_list_;

	// Events
	void onChoiceGameConfigChanged(wxCommandEvent& e);
	void onChoicePortConfigChanged(wxCommandEvent& e);
	void onMapActivated(wxListEvent& e);
	void onBtnNewMap(wxCommandEvent& e);
	void onMapSelected(wxListEvent& e);
};
} // namespace slade
