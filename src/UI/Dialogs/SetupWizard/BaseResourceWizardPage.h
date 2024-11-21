#pragma once

#include "WizardPageBase.h"

namespace slade
{
namespace ui
{
	class BaseResourceArchiveSettingsPanel;
}

class BaseResourceWizardPage : public WizardPageBase
{
public:
	BaseResourceWizardPage(wxWindow* parent);
	~BaseResourceWizardPage() override = default;

	bool     canGoNext() override { return true; }
	void     applyChanges() override;
	wxString title() override { return "Base Resource Archives"; }
	wxString description() override;

private:
	ui::BaseResourceArchiveSettingsPanel* bra_panel_ = nullptr;
};
} // namespace slade
