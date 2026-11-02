#pragma once

#include "SettingsPanel.h"

namespace slade
{
class FileLocationPanel;
}
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
	wxChoice*          choice_nodebuilder_   = nullptr;
	FileLocationPanel* flp_nodebuilder_path_ = nullptr;
	wxCheckListBox*    clb_options_          = nullptr;

	void populateOptions(const string& options) const;
};
} // namespace slade::ui
