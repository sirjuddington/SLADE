#pragma once

namespace slade
{
class WizardPageBase : public wxPanel
{
public:
	WizardPageBase(wxWindow* parent) : wxPanel(parent, -1) {}
	~WizardPageBase() {}

	virtual bool     canGoNext() { return true; }
	virtual void     applyChanges() {}
	virtual wxString title() { return "Page Title"; }
	virtual wxString description() { return ""; }
};
} // namespace slade
