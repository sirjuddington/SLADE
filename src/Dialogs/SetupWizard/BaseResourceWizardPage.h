#pragma once

#include "WizardPageBase.h"

class BaseResourceArchivesPanel;
class BaseResourceWizardPage : public WizardPageBase
{
public:
	BaseResourceWizardPage(wxWindow* parent);
	~BaseResourceWizardPage();

	bool	canGoNext();
	void	applyChanges();
	string	getTitle() { return "Base Resource Archives"; }
	string	getDescription();

private:
	BaseResourceArchivesPanel* bra_panel_;
};
