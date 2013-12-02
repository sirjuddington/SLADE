
#include "Main.h"
#include "WxStuff.h"
#include "BaseResourceWizardPage.h"
#include "BaseResourceArchivesPanel.h"

BaseResourceWizardPage::BaseResourceWizardPage(wxWindow* parent) : WizardPageBase(parent)
{
	// Setup sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Add Base Resource Archive panel
	bra_panel = new BaseResourceArchivesPanel(this);
	sizer->Add(bra_panel, 1, wxEXPAND);
}

BaseResourceWizardPage::~BaseResourceWizardPage()
{
}

bool BaseResourceWizardPage::canGoNext()
{
	return true;
}

void BaseResourceWizardPage::applyChanges()
{
}

string BaseResourceWizardPage::getDescription()
{
	return "Add 'Base Resource' archives to the list. These can be selected from the dropdown in the toolbar, and will be used as a base (eg. IWAD) for editing. Usually these will be game IWADs: doom2.wad, heretic.wad, etc. If no base resource archive is selected, certain features will not work correctly.";
}
