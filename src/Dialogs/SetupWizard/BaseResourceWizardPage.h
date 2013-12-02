
#ifndef __BASE_RESOURCE_WIZARD_PAGE_H__
#define __BASE_RESOURCE_WIZARD_PAGE_H__

#include "WizardPageBase.h"

class BaseResourceArchivesPanel;
class BaseResourceWizardPage : public WizardPageBase
{
private:
	BaseResourceArchivesPanel*	bra_panel;

public:
	BaseResourceWizardPage(wxWindow* parent);
	~BaseResourceWizardPage();

	bool	canGoNext();
	void	applyChanges();
	string	getTitle() { return "Base Resource Archives"; }
	string	getDescription();
};

#endif//__BASE_RESOURCE_WIZARD_PAGE_H__
