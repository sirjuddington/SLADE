#pragma once

#include "WizardPageBase.h"

class NodesPrefsPanel;
class NodeBuildersWizardPage : public WizardPageBase
{
public:
	NodeBuildersWizardPage(wxWindow* parent);
	~NodeBuildersWizardPage();

	bool   canGoNext();
	void   applyChanges();
	string title() { return "Node Builders"; }
	string description();

private:
	NodesPrefsPanel* panel_nodes_;
};
