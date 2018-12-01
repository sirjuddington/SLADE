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
	string	title() { return "Base Resource Archives"; }
	string	description();

private:
	BaseResourceArchivesPanel* bra_panel_;
};
