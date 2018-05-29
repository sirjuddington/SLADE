#pragma once

#include "PrefsPanelBase.h"

class wxListBox;
class FileLocationPanel;

class ACSPrefsPanel : public PrefsPanelBase
{
public:
	ACSPrefsPanel(wxWindow* parent);
	~ACSPrefsPanel();

	void	init() override;
	void	applyPreferences() override;

	string pageTitle() override { return "ACS Compiler Settings"; }

private:
	FileLocationPanel*	flp_acc_path_;
	wxButton*			btn_incpath_add_;
	wxButton*			btn_incpath_remove_;
	wxListBox*			list_inc_paths_;
	wxCheckBox*			cb_always_show_output_;

	void	setupLayout();

	// Events
	void	onBtnAddIncPath(wxCommandEvent& e);
	void	onBtnRemoveIncPath(wxCommandEvent& e);
};
