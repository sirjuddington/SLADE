#pragma once

#include "PrefsPanelBase.h"

class wxCheckListBox;

namespace slade
{
class NodesPrefsPanel : public PrefsPanelBase
{
public:
	NodesPrefsPanel(wxWindow* parent, bool frame = true);
	~NodesPrefsPanel() = default;

	void init() override;
	void populateOptions(const string& options) const;
	void applyPreferences() override;

	string pageTitle() override { return "Node Builders"; }

private:
	wxChoice*       choice_nodebuilder_ = nullptr;
	wxButton*       btn_browse_path_    = nullptr;
	wxTextCtrl*     text_path_          = nullptr;
	wxCheckListBox* clb_options_        = nullptr;

	// Events
	void onBtnBrowse(wxCommandEvent& e);
};
} // namespace slade
