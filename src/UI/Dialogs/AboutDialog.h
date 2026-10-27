#pragma once

#include "UI/SDialog.h"

namespace slade::ui
{
class AboutDialog : public SDialog
{
public:
	AboutDialog(wxWindow* parent);

private:
	wxPanel* createLogoPanel(wxWindow* parent);
	wxPanel* createTextTab(wxWindow* parent, const wxString& text, bool wrap);
	string   systemInfo() const;
	string   contributors() const;
};
} // namespace slade::ui
