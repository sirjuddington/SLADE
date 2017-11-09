#pragma once

#include "common.h"
#include "PrefsPanelBase.h"

class wxListBox;
class wxButton;
class FileLocationPanel;

class BaseResourceArchivesPanel : public PrefsPanelBase
{
public:
	BaseResourceArchivesPanel(wxWindow* parent);
	~BaseResourceArchivesPanel();

	int		selectedPathIndex();
	void	autodetect();

	void	init() override;
	void	applyPreferences() override;
	string	pageTitle() override { return "Base Resource Archive"; }
	//string	pageDescription() override;

private:
	wxListBox*			list_base_archive_paths_;
	wxButton*			btn_add_;
	wxButton*			btn_remove_;
	wxButton*			btn_detect_;
	FileLocationPanel*	flp_zdoom_pk3_;

	void	setupLayout();

	// Events
	void	onBtnAdd(wxCommandEvent& e);
	void	onBtnRemove(wxCommandEvent& e);
	void	onBtnDetect(wxCommandEvent& e);
	void	onBtnBrowseZDoomPk3(wxCommandEvent& e);
};
