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
	string getTitle() { return "Node Builders"; }
	string getDescription();

private:
	NodesPrefsPanel* panel_nodes_;
};
