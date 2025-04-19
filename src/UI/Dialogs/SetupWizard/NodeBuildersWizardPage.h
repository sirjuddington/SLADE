#pragma once

#include "WizardPageBase.h"

namespace slade
{
class NodesPrefsPanel;
class NodeBuildersWizardPage : public WizardPageBase
{
public:
	NodeBuildersWizardPage(wxWindow* parent);
	~NodeBuildersWizardPage() = default;

	bool   canGoNext() override { return true; }
	void   applyChanges() override {}
	string title() override { return "Node Builders"; }
	string description() override;

private:
	NodesPrefsPanel* panel_nodes_ = nullptr;
};
} // namespace slade
