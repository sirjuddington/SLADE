#pragma once

#include "WizardPageBase.h"

class BaseResourceArchivesPanel;
class BaseResourceWizardPage : public WizardPageBase
{
public:
	BaseResourceWizardPage(wxWindow* parent);
	~BaseResourceWizardPage() = default;

	bool   canGoNext() override { return true; }
	void   applyChanges() override;
	string title() override { return "Base Resource Archives"; }
	string description() override;

private:
	BaseResourceArchivesPanel* bra_panel_ = nullptr;
};
