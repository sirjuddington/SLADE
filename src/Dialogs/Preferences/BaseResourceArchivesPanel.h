#pragma once

#include "common.h"

class wxListBox;
class wxButton;
class BaseResourceArchivesPanel : public wxPanel
{
public:
	BaseResourceArchivesPanel(wxWindow* parent);
	~BaseResourceArchivesPanel();

	int		getSelectedPath();
	void	autodetect();

private:
	wxListBox*	list_base_archive_paths_;
	wxButton*	btn_add_;
	wxButton*	btn_remove_;
	wxButton*	btn_detect_;
	wxTextCtrl*	text_zdoom_pk3_path_;
	wxButton*	btn_browse_zdoom_pk3_;

	// Events
	void	onBtnAdd(wxCommandEvent& e);
	void	onBtnRemove(wxCommandEvent& e);
	void	onBtnDetect(wxCommandEvent& e);
	void	onBtnBrowseZDoomPk3(wxCommandEvent& e);
};
