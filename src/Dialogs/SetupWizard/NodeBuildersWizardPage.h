
#ifndef __NODE_BUILDERS_WIZARD_PAGE_H__
#define __NODE_BUILDERS_WIZARD_PAGE_H__

#include "WizardPageBase.h"

class NodesPrefsPanel;
class NodeBuildersWizardPage : public WizardPageBase
{
private:
	NodesPrefsPanel*	panel_nodes;

public:
	NodeBuildersWizardPage(wxWindow* parent);
	~NodeBuildersWizardPage();

	bool	canGoNext();
	void	applyChanges();
	string	getTitle() { return "Node Builders"; }
	string	getDescription();
};

#endif//__NODE_BUILDERS_WIZARD_PAGE_H__
