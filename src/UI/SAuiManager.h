#pragma once

#include <wx/aui/framemanager.h>

namespace slade
{
class STopWindow;
}

namespace slade::ui
{
class SAuiManager : public wxAuiManager
{
public:
	SAuiManager(STopWindow* parent);
	~SAuiManager() override = default;

	void showPane(string_view id, bool show = true, bool focus = false);
	bool togglePane(string_view id, bool focus_on_show = false);

	void loadLayout();
	void saveLayout();

private:
	STopWindow* parent_ = nullptr;

	void showPane(wxAuiPaneInfo& p_inf, bool show, bool focus);
};
} // namespace slade::ui
