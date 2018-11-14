#pragma once

#include "Archive/Archive.h"
#include "UI/Lists/ListView.h"
#include "UI/SDialog.h"

class MapPreviewCanvas;
class BaseResourceChooser;
class ResourceArchiveChooser;
class wxImageList;
class MapEditorConfigDialog : public SDialog
{
public:
	MapEditorConfigDialog(wxWindow* parent, Archive* archive = nullptr, bool show_maps = true, bool creating = false);
	~MapEditorConfigDialog();

	void             populateGameList();
	void             populatePortList();
	void             populateMapList();
	Archive::MapDesc selectedMap();
	bool             configMatchesMap(Archive::MapDesc map);
	string           selectedGame();
	string           selectedPort();

private:
	wxChoice*               choice_game_config_;
	wxChoice*               choice_port_config_;
	BaseResourceChooser*    choice_base_resource_;
	ListView*               list_maps_;
	ResourceArchiveChooser* rac_resources_;
	wxButton*               btn_new_map_;
	MapPreviewCanvas*       canvas_preview_;
	wxImageList*            img_list_;
	wxButton*               btn_ok_;
	wxButton*               btn_cancel_;
	string                  game_current_;
	string                  port_current_;
	int                     map_format_;
	bool                    creating_;

	Archive*                 archive_;
	vector<Archive::MapDesc> maps_;
	vector<string>           games_list_;
	vector<string>           ports_list_;

	// Events
	void onChoiceGameConfigChanged(wxCommandEvent& e);
	void onChoicePortConfigChanged(wxCommandEvent& e);
	void onMapActivated(wxListEvent& e);
	void onBtnNewMap(wxCommandEvent& e);
	void onMapSelected(wxListEvent& e);
	void onBtnOK(wxCommandEvent& e);
	void onBtnCancel(wxCommandEvent& e);
};
