#pragma once

#include "SettingsPanel.h"

class wxCheckListBox;

namespace slade::ui
{
class NodeBuildersSettingsPanel : public SettingsPanel
{
public:
	NodeBuildersSettingsPanel(wxWindow* parent);
	~NodeBuildersSettingsPanel() override = default;

	string title() const override { return "Node Builders"; }

	void loadSettings() override;
	void applySettings() override;

private:
	wxChoice*       choice_nodebuilder_ = nullptr;
	wxButton*       btn_browse_path_    = nullptr;
	wxTextCtrl*     text_path_          = nullptr;
	wxCheckListBox* clb_options_        = nullptr;

	void populateOptions(const string& options) const;

	// Events
	void onBtnBrowse(wxCommandEvent& e);
};
} // namespace slade::ui
