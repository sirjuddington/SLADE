#pragma once

#include "WizardPageBase.h"

namespace slade
{
class BaseResourceArchivesPanel;
class BaseResourceWizardPage : public WizardPageBase
{
public:
	BaseResourceWizardPage(wxWindow* parent);
	~BaseResourceWizardPage() = default;

	bool     canGoNext() override { return true; }
	void     applyChanges() override;
	wxString title() override { return "Base Resource Archives"; }
	wxString description() override;

private:
	BaseResourceArchivesPanel* bra_panel_ = nullptr;
};
} // namespace slade
