
#ifndef __MAP_EDITOR_CONFIG_DIALOG_H__
#define __MAP_EDITOR_CONFIG_DIALOG_H__

#include "ListView.h"
#include "Archive.h"
#include "MapPreviewCanvas.h"
#include <wx/imaglist.h>

class BaseResourceChooser;
class ResourceArchiveChooser;
class MapEditorConfigDialog : public wxDialog
{
private:
	wxChoice*					choice_game_config;
	wxChoice*					choice_port_config;
	BaseResourceChooser*		choice_base_resource;
	ListView*					list_maps;
	ResourceArchiveChooser*		rac_resources;
	//wxCheckListBox*				list_resources;
	//wxButton*					btn_open_resource;
	//wxButton*					btn_recent;
	wxButton*					btn_new_map;
	MapPreviewCanvas*			canvas_preview;
	wxImageList*				img_list;
	string						game_current;
	string						port_current;
	int							map_format;
	bool						creating;

	Archive*					archive;
	vector<Archive::mapdesc_t>	maps;
	vector<int>					ports_list;

public:
	MapEditorConfigDialog(wxWindow* parent, Archive* archive = NULL, bool show_maps = true, bool creating = false);
	~MapEditorConfigDialog();

	void				populateGameList();
	void				populatePortList();
	void				populateMapList();
	Archive::mapdesc_t	selectedMap();
	bool				configMatchesMap(Archive::mapdesc_t map);
	string				selectedGame();
	string				selectedPort();

	// Events
	void	onChoiceGameConfigChanged(wxCommandEvent& e);
	void	onChoicePortConfigChanged(wxCommandEvent& e);
	void	onMapActivated(wxListEvent& e);
	void	onBtnNewMap(wxCommandEvent& e);
	//void	onBtnOpenResource(wxCommandEvent& e);
	//void	onBtnRecent(wxCommandEvent& e);
	void	onMapSelected(wxListEvent& e);
};

#endif//__MAP_EDITOR_CONFIG_DIALOG_H__
