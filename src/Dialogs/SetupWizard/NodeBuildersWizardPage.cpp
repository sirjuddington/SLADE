
#include "Main.h"
#include "WxStuff.h"
#include "NodeBuildersWizardPage.h"
#include "NodesPrefsPanel.h"


NodeBuildersWizardPage::NodeBuildersWizardPage(wxWindow* parent) : WizardPageBase(parent)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add Base Resource Archive panel
	panel_nodes = new NodesPrefsPanel(this, false);
	sizer->Add(panel_nodes, 1, wxEXPAND);
}

NodeBuildersWizardPage::~NodeBuildersWizardPage()
{
}

bool NodeBuildersWizardPage::canGoNext()
{
	return true;
}

void NodeBuildersWizardPage::applyChanges()
{

}

string NodeBuildersWizardPage::getDescription()
{
	return "If you plan to do any map editing, set up the paths to any node builders you have. You can also set up the build options for each node builder.";
}
