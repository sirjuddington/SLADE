#pragma once

#include "PrefsPanelBase.h"

class wxCheckListBox;
class NodesPrefsPanel : public PrefsPanelBase
{
public:
	NodesPrefsPanel(wxWindow* parent, bool frame = true);
	~NodesPrefsPanel();

	void init() override;
	void populateOptions(string options);
	void applyPreferences() override;

	string pageTitle() override { return "Node Builders"; }

private:
	wxChoice*       choice_nodebuilder_;
	wxButton*       btn_browse_path_;
	wxTextCtrl*     text_path_;
	wxCheckListBox* clb_options_;

	// Events
	void onChoiceBuilderChanged(wxCommandEvent& e);
	void onBtnBrowse(wxCommandEvent& e);
};
