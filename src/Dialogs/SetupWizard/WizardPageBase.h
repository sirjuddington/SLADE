
#ifndef __WIZARD_PAGE_BASE_H__
#define __WIZARD_PAGE_BASE_H__

#include "common.h"

class WizardPageBase : public wxPanel
{
private:

public:
	WizardPageBase(wxWindow* parent) : wxPanel(parent, -1) {}
	~WizardPageBase() {}

	virtual bool	canGoNext() { return true; }
	virtual void	applyChanges() {}
	virtual string	getTitle() { return "Page Title"; }
	virtual string	getDescription() { return ""; }
};

#endif//__WIZARD_PAGE_BASE_H__
