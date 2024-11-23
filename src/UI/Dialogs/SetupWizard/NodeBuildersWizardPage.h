#pragma once

#include "WizardPageBase.h"

namespace slade
{
namespace ui
{
	class NodeBuildersSettingsPanel;
}

class NodeBuildersWizardPage : public WizardPageBase
{
public:
	NodeBuildersWizardPage(wxWindow* parent);
	~NodeBuildersWizardPage() override = default;

	bool     canGoNext() override { return true; }
	void     applyChanges() override {}
	wxString title() override { return "Node Builders"; }
	wxString description() override;

private:
	ui::NodeBuildersSettingsPanel* panel_nodes_ = nullptr;
};
} // namespace slade
